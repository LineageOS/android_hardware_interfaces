/*
 * Copyright 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "bluetooth_hal.vndsnoop"

#include "bluetooth_hal/debug/vnd_snoop_logger.h"

#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "android-base/logging.h"
#include "android-base/properties.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/files.h"
#include "bluetooth_hal/util/worker.h"

namespace bluetooth_hal {
namespace debug {
namespace {

using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::util::Worker;

struct PacketHeaderType {
  uint32_t length_original;
  uint32_t length_captured;
  uint32_t flags;
  uint32_t dropped_packets;
  uint64_t timestamp;
  uint8_t type;
} __attribute__((__packed__));

struct FileHeaderType {
  uint8_t identification_pattern[8];
  uint32_t version_number;
  uint32_t datalink_type;
} __attribute__((__packed__));

// Epoch in microseconds since 01/01/0000.
constexpr uint64_t kBtSnoopEpochDelta = 0x00dcddb30f2f8000ULL;

// A compile-time check for system endianness.
constexpr uint32_t kBytesToTest = 0x12345678;
constexpr uint8_t kFirstByte = static_cast<uint8_t>(kBytesToTest);
constexpr bool kIsLittleEndian = kFirstByte == 0x78;
constexpr bool kIsBigEndian = kFirstByte == 0x12;
static_assert(kIsLittleEndian ||
                  (kIsBigEndian && kIsLittleEndian != kIsBigEndian),
              "System must be either little-endian or big-endian");

constexpr uint32_t kBtSnoopVersionNumber = kIsLittleEndian ? 0x01000000 : 1;
constexpr uint32_t kBtSnoopDatalinkType =
    kIsLittleEndian ? 0xea030000
                    : 0x03ea;  // Datalink Type code for HCI UART (H4) is 1002.
constexpr FileHeaderType kBtSnoopFileHeader = {
    .identification_pattern = {'b', 't', 's', 'n', 'o', 'o', 'p', 0x00},
    .version_number = kBtSnoopVersionNumber,
    .datalink_type = kBtSnoopDatalinkType};

// Default number of packets per btsnoop file before rotation. Two snoop files
// are rotated, and the size can be dynamically configured via a system
// property. Changes take effect after toggling Bluetooth off and on.
constexpr size_t kDefaultBtSnoopMaxPacketsPerFile = 0xffff;

constexpr std::string_view kLogDirectory = "/data/vendor/bluetooth";
constexpr std::string_view kLogFilePrefix = "btsnoop_hci_vnd";
constexpr std::string_view kBtLogPathPrefix =
    "/data/vendor/bluetooth/btsnoop_hci_vnd";
constexpr int kMaxLogFileCount = 10;

constexpr std::string kBtLogModeFull = "full";
constexpr std::string kBtLogModeFiltered = "filtered";
constexpr std::string kBtLogModeDisabled = "disabled";
// Truncate to certain length for packet types that need to be filtered.
constexpr int kFilteredPacketLength = 32;

uint64_t Htonll(uint64_t ll) {
  if constexpr (kIsLittleEndian) {
    return static_cast<uint64_t>(htonl(ll & 0xffffffff)) << 32 |
           htonl(ll >> 32);
  } else {
    return ll;
  }
}

std::string GetLogPathWithTimeStamp(std::string_view prefix) {
  std::chrono::time_point now = std::chrono::system_clock::now();
  time_t in_time_t = std::chrono::system_clock::to_time_t(now);
  std::tm* time_info = std::localtime(&in_time_t);

  std::stringstream ss;
  ss << prefix << "-" << std::put_time(time_info, "%Y-%m-%d_%H-%M-%S.log");

  return ss.str();
}

std::string GetBtSnoopLogMode() {
  return android::base::GetProperty(Property::kBtSnoopLogMode,
                                    kBtLogModeDisabled);
}

bool IsBtVndSnoopLogEnabled() {
  return android::base::GetBoolProperty(Property::kBtVendorSnoopEnabledProperty,
                                        false);
}

std::string GetBtVndSnoopLogMode() {
  if (!IsBtVndSnoopLogEnabled()) {
    return kBtLogModeDisabled;
  }
  std::string bt_snoop_log_mode = GetBtSnoopLogMode();
  if (bt_snoop_log_mode == kBtLogModeDisabled) {
    return HalConfigLoader::GetLoader().IsUserDebugOrEngBuild()
               ? kBtLogModeFiltered
               : kBtLogModeDisabled;
  }
  return bt_snoop_log_mode;
}

size_t GetMaxPacketsPerFile() {
  const size_t max_packets_per_file = android::base::GetUintProperty<uint64_t>(
      Property::kBtSnoopMaxPacketsPerFileProperty,
      kDefaultBtSnoopMaxPacketsPerFile);
  LOG(INFO) << __func__
            << ": Vendor btsnoop max packets: " << max_packets_per_file << ".";
  return max_packets_per_file;
}

}  // namespace

class LoggerTask {
 public:
  enum class LoggerTaskType : int {
    kStartNewRecording = 1,
    kStopRecording = 2,
    kCapture = 3,
  };

  struct CaptureArgs {
    HalPacket packet;
    VndSnoopLogger::Direction direction;
    uint64_t timestamp_us;

    CaptureArgs(const HalPacket& packet, VndSnoopLogger::Direction direction,
                uint64_t timestamp_us)
        : packet(packet), direction(direction), timestamp_us(timestamp_us) {}
  };

  static LoggerTask StartNewRecordingTask() {
    return LoggerTask{LoggerTaskType::kStartNewRecording, {}};
  }

  static LoggerTask StopRecordingTask() {
    return LoggerTask{LoggerTaskType::kStopRecording, {}};
  }

  static LoggerTask CaptureTask(const HalPacket& packet,
                                VndSnoopLogger::Direction direction,
                                uint64_t timestamp_us) {
    return LoggerTask{LoggerTaskType::kCapture,
                      CaptureArgs{packet, direction, timestamp_us}};
  }
  LoggerTaskType type_;
  std::optional<CaptureArgs> args_;

 private:
  LoggerTask(LoggerTaskType type, std::optional<CaptureArgs> args)
      : type_(type), args_(std::move(args)) {}
};

class LoggerHandler {
 public:
  LoggerHandler() {
    logger_thread_ = std::make_unique<Worker<LoggerTask>>(
        std::bind_front(&LoggerHandler::TaskHandler, this));
  }

  ~LoggerHandler() {
    logger_thread_.reset();
    CloseCurrentLogFile();
  }

  static LoggerHandler& GetHandler() {
    static LoggerHandler handler;
    return handler;
  }

  void Post(LoggerTask task) { logger_thread_->Post(std::move(task)); }

 private:
  enum class State : int {
    kStoppedOrDisabled = 0,
    kRecording,
  };

  void TaskHandler(LoggerTask task) {
    switch (task.type_) {
      case LoggerTask::LoggerTaskType::kStartNewRecording:
        StartNewRecording();
        break;
      case LoggerTask::LoggerTaskType::kStopRecording:
        StopRecording();
        break;
      case LoggerTask::LoggerTaskType::kCapture:
        if (task.args_.has_value()) {
          Capture(task.args_->packet, task.args_->direction,
                  task.args_->timestamp_us);
        }
        break;
      default:
        LOG(ERROR) << "Unknown task type: " << static_cast<int>(task.type_);
        break;
    }
  }

  void StartNewRecording() {
    LOG(INFO) << __func__ << ": Start recording vendor btsnoop log.";

    std::string vnd_snoop_log_mode = GetBtVndSnoopLogMode();
    LOG(INFO) << __func__ << ": Vendor btsnoop log mode: " << vnd_snoop_log_mode
              << ".";

    max_packets_per_file_ = GetMaxPacketsPerFile();
    filtered = vnd_snoop_log_mode != kBtLogModeFull;
    if (vnd_snoop_log_mode != kBtLogModeDisabled) {
      PrepareNewLogFile();
      state_ = State::kRecording;
    } else {
      os::DeleteOldestFiles(kLogDirectory, kLogFilePrefix, 0);
      state_ = State::kStoppedOrDisabled;
    }
  }

  void StopRecording() {
    LOG(INFO) << __func__ << ": Stop recording vendor btsnoop log.";
    CloseCurrentLogFile();
    state_ = State::kStoppedOrDisabled;
  }

  void Capture(const HalPacket& packet, VndSnoopLogger::Direction direction,
               uint64_t timestamp_us) {
    if (state_ == State::kStoppedOrDisabled) {
      return;
    }

    const HciPacketType type = packet.GetType();
    const std::vector<uint8_t> payload = packet.GetBody();

    // Set btsnoop packet flags:
    // Bit 0: Direction (0 for Sent/Outgoing, 1 for Received/Incoming)
    // Bit 1: Type (0 for Data, 1 for Command/Event)
    uint32_t flags = 0;
    switch (type) {
      case HciPacketType::kCommand:
        flags |= (1 << 1);
        break;
      case HciPacketType::kEvent:
        flags |= (1 << 0);
        flags |= (1 << 1);
        break;
      case HciPacketType::kAclData:
      case HciPacketType::kIsoData:
      case HciPacketType::kScoData:
      case HciPacketType::kThreadData:
        if (direction == VndSnoopLogger::Direction::kIncoming) {
          flags |= (1 << 0);
        }
        break;
      default:
        break;
    }

    uint32_t captured_length = packet.size();
    if (filtered && captured_length > kFilteredPacketLength) {
      captured_length = kFilteredPacketLength;
    }
    PacketHeaderType header = {
        .length_original = htonl(packet.size()),
        .length_captured = htonl(captured_length),
        .flags = htonl(flags),
        .dropped_packets = 0,
        .timestamp = Htonll(timestamp_us + kBtSnoopEpochDelta),
        .type = static_cast<uint8_t>(type)};

    if (++packet_counter_ > max_packets_per_file_) {
      LOG(INFO) << __func__
                << ": Reach max packet per file, open new log file.";
      PrepareNewLogFile();
    }

    if (!log_ostream_.write(reinterpret_cast<const char*>(&header),
                            sizeof(PacketHeaderType))) {
      LOG(ERROR) << __func__
                 << ": Failed to write packet header for btsnoop, error: \""
                 << strerror(errno) << "\".";
      return;
    }

    // -1 for type byte.
    const char* packet_data = reinterpret_cast<const char*>(payload.data());
    if (!log_ostream_.write(packet_data, captured_length - 1)) {
      LOG(ERROR) << __func__
                 << ": Failed to write packet payload for btsnoop, error: \""
                 << strerror(errno) << "\".";
    }

    if (!log_ostream_.flush()) {
      LOG(ERROR) << __func__ << ": Failed to flush, error: \""
                 << strerror(errno) << "\".";
    }
  }

  void CloseCurrentLogFile() {
    LOG(INFO) << __func__ << ": Close btsnoop log file.";
    os::CloseLogFileStream(log_ostream_);
    packet_counter_ = 0;
  }

  void OpenNewLogFile() {
    const std::string log_file_path = GetLogPathWithTimeStamp(kBtLogPathPrefix);
    const mode_t previous_umask = umask(0);

    // Open file in binary write mode, without append, to overwrite existing
    // data.
    log_ostream_.open(log_file_path, std::ios::binary | std::ios::out);

    // Set file permissions to OWNER Read/Write, GROUP Read, OTHER Read.
    if (chmod(log_file_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) !=
        0) {
      LOG(ERROR) << __func__ << ": Unable to change file permissions for "
                 << log_file_path << ".";
    }

    if (!log_ostream_.is_open()) {
      LOG(ERROR) << __func__ << ": Unable to open snoop log at \""
                 << log_file_path << "\", error: \"" << strerror(errno)
                 << "\".";
    }

    umask(previous_umask);

    if (!log_ostream_.write(reinterpret_cast<const char*>(&kBtSnoopFileHeader),
                            sizeof(FileHeaderType))) {
      LOG(ERROR) << __func__ << ": Unable to write file header to \""
                 << log_file_path << "\", error: \"" << strerror(errno)
                 << "\".";
    }

    if (!log_ostream_.flush()) {
      LOG(ERROR) << __func__ << ": Failed to flush, error: \""
                 << strerror(errno) << "\".";
    }

    LOG(INFO) << __func__ << ": Open new btsnoop log file at " << log_file_path
              << ".";
  }

  void PrepareNewLogFile() {
    CloseCurrentLogFile();
    os::DeleteOldestFiles(kLogDirectory, kLogFilePrefix, kMaxLogFileCount - 1);
    OpenNewLogFile();
  }

  std::ofstream log_ostream_;
  State state_{State::kStoppedOrDisabled};
  size_t max_packets_per_file_{0};
  size_t packet_counter_{0};
  bool filtered = true;
  std::unique_ptr<Worker<LoggerTask>> logger_thread_;
};

class VndSnoopLoggerImpl : public VndSnoopLogger {
 public:
  ~VndSnoopLoggerImpl() override = default;
  void StartNewRecording() override;
  void StopRecording() override;
  void Capture(const ::bluetooth_hal::hci::HalPacket& packet,
               Direction direction) override;
};

VndSnoopLogger& VndSnoopLogger::GetLogger() {
  static VndSnoopLoggerImpl logger;
  return logger;
}

void VndSnoopLoggerImpl::Capture(const HalPacket& packet, Direction direction) {
  uint64_t timestamp_us =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  LoggerHandler::GetHandler().Post(
      LoggerTask::CaptureTask(packet, direction, timestamp_us));
}

void VndSnoopLoggerImpl::StartNewRecording() {
  LoggerHandler::GetHandler().Post(LoggerTask::StartNewRecordingTask());
}

void VndSnoopLoggerImpl::StopRecording() {
  LoggerHandler::GetHandler().Post(LoggerTask::StopRecordingTask());
}

}  // namespace debug
}  // namespace bluetooth_hal
