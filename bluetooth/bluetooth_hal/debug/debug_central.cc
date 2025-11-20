/*
 * Copyright 2020 The Android Open Source Project
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

#define LOG_TAG "bthal.debug_central"

#include "bluetooth_hal/debug/debug_central.h"

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

#include "android-base/logging.h"
#include "android-base/properties.h"
#include "android-base/stringprintf.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/bluetooth_activity.h"
#include "bluetooth_hal/debug/bluetooth_bqr.h"
#include "bluetooth_hal/extensions/thread/thread_handler.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "bluetooth_hal/util/logging.h"

namespace {

using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::thread::ThreadHandler;
using ::bluetooth_hal::transport::TransportInterface;
using ::bluetooth_hal::transport::TransportType;
using ::bluetooth_hal::util::Logger;

static constexpr int kVseSubEventCodeOffset = 2;
static constexpr int kBqrReportIdOffset = 3;
static constexpr int kBqrInflamedErrorCode = 4;
static constexpr int kBqrInflamedVendorErrCode = 5;
static constexpr int kDebugInfoPayloadOffset = 8;
static constexpr int kChreDebugDumpLastBlockOffsetFisrtByte = 4;
static constexpr int kChreDebugDumpLastBlockOffsetSecondByte = 5;
static constexpr int kDebugInfoLastBlockOffset = 5;
static constexpr int kHwCodeOffset = 2;

static constexpr uint8_t kEventVendorSpecific =
    0xFF;  // Event code: Vendor specific event
static constexpr uint8_t kEventHardwareError =
    0x10;  // Event code: Hardware error
static constexpr uint8_t kVseSubEventDebugInfo =
    0x57;  // Vendor specific sub event: Debug info
static constexpr uint8_t kVseSubEventBqr =
    0x58;  // Vendor specific sub event: Bluetooth quality report
static constexpr uint8_t kVseSubEventDebug1 =
    0x6A;  // Vendor specific sub event:  Debug logging
static constexpr uint8_t kVseSubEventDebug2 =
    0x1B;  // Vendor specific sub event:  Debug logging

bool force_coredump_timer_created = false;
timer_t force_coredump_timer;
constexpr uint8_t kForceCoredumpTimerExpiredSec = 1;
constexpr uint32_t kForceCoredumpTimerExpiredNs = 0;
static const std::string kDumpReasonForceCollectCoredump =
    "Force Collect Coredump";
static const std::string kDumpReasonControllerHwError = "ControllerHwError";
static const std::string kDumpReasonControllerRootInflammed =
    "ControllerRootInflammed";
static const std::string kDumpReasonControllerDebugDumpWithoutRootInflammed =
    "ControllerDebugInfoDataDumpWithoutRootInflammed";
static const std::string kDumpReasonControllerDebugInfo = "Debug Info Event";

static const std::string kCrashInfoFilePath = "/data/vendor/ssrdump/";
static const std::string kSsrdumpFilePath = "/data/vendor/ssrdump/coredump/";
static const std::string kCrashInfoFilePrefix = "crashinfo_bt_";
static const std::string kSsrdumpFilePrefix = "coredump_bt_";
static const std::string kSsrdumpSocFilePrefix = "coredump_bt_socdump_";
static const std::string kSsrdumpChreFilePrefix = "coredump_bt_chredump_";
static const std::string kSocdumpFilePath =
    "/data/vendor/ssrdump/coredump/coredump_bt_socdump_";
static const std::string kChredumpFilePath =
    "/data/vendor/ssrdump/coredump/coredump_bt_chredump_";
static const std::string kVendorSnoopFilePath =
    "/data/vendor/bluetooth/btsnoop_hci_vnd.log";
static const std::string kBackupVendorSnoopFilePath =
    "/data/vendor/bluetooth/backup_btsnoop_hci_vnd.log";
static const std::string kVendorSnoopLastFilePath =
    "/data/vendor/bluetooth/btsnoop_hci_vnd.log.last";
static const std::string kBackupVendorSnoopLastFilePath =
    "/data/vendor/bluetooth/backup_btsnoop_hci_vnd.log.last";

static const std::string kDebugNodeBtLpm = "dev/logbuffer_btlpm";
constexpr char kDebugNodeBtUartPrefix[] = "/dev/logbuffer_tty";
constexpr char kHwStage[] = "ro.boot.hardware.revision";
constexpr uint8_t kReservedCoredumpFileCount = 2;

std::string crash_file_create_timestamp() {
  time_t rawtime;
  time(&rawtime);
  struct tm* timeinfo = localtime(&rawtime);

  std::stringstream ss;
  ss << std::to_string(timeinfo->tm_year + 1900) << "-" << std::setw(2)
     << std::setfill('0') << std::to_string(timeinfo->tm_mon + 1) << "-"
     << std::setw(2) << std::setfill('0') << std::to_string(timeinfo->tm_mday)
     << "_" << std::setw(2) << std::setfill('0')
     << std::to_string(timeinfo->tm_hour) << "-" << std::setw(2)
     << std::setfill('0') << std::to_string(timeinfo->tm_min) << "-"
     << std::setw(2) << std::setfill('0') << std::to_string(timeinfo->tm_sec);
  return ss.str();
}

void copy_file(const std::string& SrcDir, const std::string& DestDir) {
  std::ifstream src(SrcDir, std::ios::binary);
  std::ofstream dst(DestDir, std::ios::binary);

  if (!src.is_open()) {
    LOG(ERROR) << __func__ << ": Error opening source file.";
    return;
  }

  if (!dst.is_open()) {
    LOG(ERROR) << __func__ << ": Error opening destination file.";
    return;
  }

  dst << src.rdbuf();

  // Get source file permissions
  struct stat src_stat;
  if (stat(SrcDir.c_str(), &src_stat) != 0) {
    LOG(ERROR) << __func__ << ": Error getting source file permissions.";
    return;
  }

  // Apply permissions to destination file
  if (chmod(DestDir.c_str(), src_stat.st_mode) != 0) {
    LOG(ERROR) << __func__ << ": Error setting destination file permissions.";
    return;
  }
}

void backup_logging_files_before_crash(std::string crash_timestamp) {
  LOG(INFO) << __func__;
  if (crash_timestamp.empty()) {
    crash_timestamp = crash_file_create_timestamp();
  }
  copy_file(kVendorSnoopLastFilePath,
            kBackupVendorSnoopLastFilePath + "_" + crash_timestamp);
  copy_file(kVendorSnoopFilePath,
            kBackupVendorSnoopFilePath + "_" + crash_timestamp);
}

void dump_debugfs_to_fd(int fd, const std::string& debugfs) {
  std::stringstream ss;
  std::ifstream file;

  ss << "=============================================" << std::endl;
  ss << "Debugfs:" << debugfs << std::endl;
  ss << "=============================================" << std::endl;
  file.open(debugfs);
  if (file.is_open()) {
    ss << file.rdbuf() << std::endl;
  } else {
    ss << "Fail to read debugfs: " << debugfs << std::endl;
  }
  ss << std::endl;
  write(fd, ss.str().c_str(), ss.str().length());
}

void read_as_hex(std::ifstream& file, std::stringstream& content) {
  std::array<char, 64> memblock{};
  while (!file.eof()) {
    file.read(memblock.data(), memblock.size());
    size_t len = file.gcount();
    if (len == 0) continue;

    for (size_t i = 0; i < len; i++) {
      content << std::setbase(16) << std::setw(2) << std::setfill('0')
              << (memblock[i] & 0xff);
    }
    content << "\n";
  }
}

void collect_sscd_logs(int fd, const std::string& prefix,
                       const std::string& dir) {
  std::unique_ptr<DIR, decltype(&closedir)> dir_dump(opendir(dir.c_str()),
                                                     closedir);

  if (!dir_dump) {
    LOG(WARNING) << __func__ << ": Failed to open directory, skip " << prefix
                 << ".";
    return;
  }

  std::stringstream content;
  struct dirent* dp;
  while ((dp = readdir(dir_dump.get()))) {
    std::ifstream file;
    std::string file_name;
    std::stringstream path;

    if (dp->d_type != DT_REG) {
      continue;
    }
    file_name = dp->d_name;
    size_t pos = file_name.find(prefix);
    if (pos != 0) {
      continue;
    }

    path << dir << file_name;
    LOG(DEBUG) << __func__ << ": Dumping " << path.str() << ".";

    // for coredump_bt_socdump_[timestamp].bin
    std::size_t last_dot = file_name.rfind('.');
    bool is_bin = (last_dot != std::string::npos &&
                   file_name.substr(last_dot + 1) == "bin" &&
                   (file_name.find(kSsrdumpSocFilePrefix) == 0 ||
                    file_name.find(kSsrdumpChreFilePrefix) == 0));
    if (is_bin)
      file.open(path.str(), std::ifstream::binary);
    else
      file.open(path.str());
    content << "*********************************************\n\n";
    content << "BEGIN of LogFile: " << file_name << "\n\n";
    content << "*********************************************\n";
    if (file_name.length() != 0 && file.is_open()) {
      if (is_bin) {
        read_as_hex(file, content);
      } else {
        content << file.rdbuf() << std::endl;
      }
    } else {
      content << "File open failed: " << prefix << std::endl;
    }
    content << "*********************************************\n\n";
    content << "END of LogFile: " << file_name << "\n\n";
    content << "*********************************************\n\n";
  }
  write(fd, content.str().c_str(), content.str().length());
}

int open_firmware_dump_file(const std::string& crash_timestamp) {
  std::stringstream fname;
  fname << kSocdumpFilePath << crash_timestamp << ".bin";
  int socdump_fd;
  if ((socdump_fd =
           open(fname.str().c_str(), O_APPEND | O_CREAT | O_SYNC | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP)) < 0) {
    LOG(ERROR) << __func__ << ": Failed to open socdump file: " << fname.str()
               << ", failed: " << strerror(errno) << " (" << errno << ")";
  }
  // Change the file's permissions to OWNER Read/Write, GROUP Read, OTHER Read
  if (chmod(fname.str().c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
    LOG(ERROR) << __func__ << ": Unable to change file permissions "
               << fname.str() << ".";
  }
  return socdump_fd;
}

int open_lpp_chre_dump_file(const std::string& crash_timestamp) {
  std::stringstream fname;
  fname << kChredumpFilePath << crash_timestamp << ".bin";
  int chredump_fd;
  if ((chredump_fd =
           open(fname.str().c_str(), O_APPEND | O_CREAT | O_SYNC | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP)) < 0) {
    LOG(ERROR) << __func__ << ": Failed to open chredump file: " << fname.str()
               << ", failed: " << strerror(errno) << " (" << errno << ")";
  }
  // Change the file's permissions to OWNER Read/Write, GROUP Read, OTHER Read
  if (chmod(fname.str().c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
    LOG(ERROR) << __func__ << ": Unable to change file permissions "
               << fname.str() << ".";
  }
  return chredump_fd;
}

bool is_coredump_file(const std::string& filename) {
  return filename.find("coredump_bt_") == 0 &&
         filename.find(".bin") == filename.size() - 4;
}

struct CoredumpFile {
  std::string filename;
  time_t timestamp;
};

bool compare_file_create_time(const CoredumpFile& a, const CoredumpFile& b) {
  return a.timestamp < b.timestamp;
}

void delete_coredump_files(const std::string& dir) {
  std::vector<CoredumpFile> coredumpfiles;
  DIR* dir_ptr = opendir(dir.c_str());
  if (dir_ptr != nullptr) {
    struct dirent* entry;
    while ((entry = readdir(dir_ptr)) != nullptr) {
      if (is_coredump_file(entry->d_name)) {
        struct stat statbuf;
        stat((dir + "/" + entry->d_name).c_str(), &statbuf);
        coredumpfiles.push_back({entry->d_name, statbuf.st_mtime});
      }
    }
    closedir(dir_ptr);
  }

  sort(coredumpfiles.begin(), coredumpfiles.end(), compare_file_create_time);
  LOG(INFO) << __func__ << ": Coredump files count: " << coredumpfiles.size()
            << ".";
  if (coredumpfiles.size() > kReservedCoredumpFileCount) {
    for (int i = 0; i < coredumpfiles.size() - kReservedCoredumpFileCount;
         i++) {
      remove((dir + "/" + coredumpfiles[i].filename).c_str());
      LOG(INFO) << __func__
                << ": Delete file: " << (dir + "/" + coredumpfiles[i].filename)
                << ".";
    }
  }
}

}  // namespace

namespace bluetooth_hal {
namespace debug {

using ::android::base::StringPrintf;
using ::bluetooth_hal::debug::BqrQualityReportId;
using ::bluetooth_hal::debug::BtActivitiesLogger;
using ::bluetooth_hal::debug::BtBqrEnergyRecoder;
using ::bluetooth_hal::debug::ParseAdvanceRFStatsEvt;
using ::bluetooth_hal::debug::ParseLinkQualityRelatedEvt;
using ::bluetooth_hal::debug::ParseVendorSpecificQualityEvt;
using ::bluetooth_hal::debug::ParseVendorSpecificTraceEvt;
using ::bluetooth_hal::debug::updateControllerCapability;

void LogFatal(BqrErrorCode error, std::string extra_info);
const std::string get_error_code_string(BqrErrorCode error_code);

const std::map<BqrErrorCode, std::string> error_code_string = {
    // SOC FW Report Error Code
    {BqrErrorCode::UART_PARSING, "UART Parsing error (BtFw)"},
    {BqrErrorCode::UART_INCOMPLETE_PACKET, "UART Incomplete Packet (BtFw)"},
    {BqrErrorCode::FIRMWARE_CHECKSUM, "Patch Firmware checksum failure (BtFw)"},
    {BqrErrorCode::FIRMWARE_HARD_FAULT,
     "Firmware Crash due to Hard Fault (BtFw)"},
    {BqrErrorCode::FIRMWARE_MEM_MANAGE_FAULT,
     "Firmware Crash due to Mem manage Fault (BtFw)"},
    {BqrErrorCode::FIRMWARE_BUS_FAULT,
     "Firmware Crash due to Bus Fault (BtFw)"},
    {BqrErrorCode::FIRMWARE_USAGE_FAULT,
     "Firmware Crash due to Usage fault (BtFw)"},
    {BqrErrorCode::FIRMWARE_WATCHDOG_TIMEOUT,
     "Firmware Crash due to Watchdog timeout (BtFw)"},
    {BqrErrorCode::FIRMWARE_ASSERTION_FAILURE,
     "Firmware Crash due to Assertion failure (BtFw)"},
    {BqrErrorCode::FIRMWARE_MISCELLANEOUS,
     "Firmware Crash Miscallaneuous (BtFw)"},
    {BqrErrorCode::FIRMWARE_HOST_REQUEST_DUMP, "HCI Command Timeout (BtCmd)"},
    {BqrErrorCode::FIRMWARE_MISCELLANEOUS_MAJOR_FAULT,
     "Firmware Miscellaneous error - Major (BtFw)"},
    {BqrErrorCode::FIRMWARE_MISCELLANEOUS_CRITICAL_FAULT,
     "Firmware Miscellaneous error - Critical (BtFw)"},
    {BqrErrorCode::FIRMWARE_THREAD_GENERIC_ERROR,
     "Firmware crash due to 15.4 Thread error (ThreadFw)"},
    {BqrErrorCode::FIRMWARE_THREAD_INVALID_FRAME,
     "Firmware crash due to detecting malformed frame from host (ThreadFw)"},
    {BqrErrorCode::FIRMWARE_THREAD_INVALID_PARAM,
     "Firmware crash due to receiving invalid frame meta-data/parameters "
     "(ThreadFw)"},
    {BqrErrorCode::FIRMWARE_THREAD_UNSUPPORTED_FRAME,
     "Firmware crash due to receiving frames from host with unsupported "
     "command ID (ThreadFw)"},
    {BqrErrorCode::SOC_BIG_HAMMER_FAULT, "Soc Big Hammer Error (BtWifi)"},
    // BT HAL Report Error Code
    {BqrErrorCode::HOST_RX_THREAD_STUCK, "Host RX Thread Stuck (BtHal)"},
    {BqrErrorCode::HOST_HCI_COMMAND_TIMEOUT,
     "Host HCI Command Timeout (BtHal)"},
    {BqrErrorCode::HOST_INVALID_HCI_EVENT,
     "Invalid / un-reassembled HCI event (BtHal)"},
    {BqrErrorCode::HOST_UNIMPLEMENTED_PACKET_TYPE,
     "Host Received Unimplemented Packet Type (BtHal)"},
    {BqrErrorCode::HOST_HCI_H4_TX_ERROR, "Host HCI H4 TX Error (BtHal)"},
    {BqrErrorCode::HOST_OPEN_USERIAL, "Host Open Userial Error (BtHal)"},
    {BqrErrorCode::HOST_POWER_UP_CONTROLLER,
     "Host Can't Power Up Controller (BtHal)"},
    {BqrErrorCode::HOST_CHANGE_BAUDRATE, "Host Change Baudrate Error (BtHal)"},
    {BqrErrorCode::HOST_RESET_BEFORE_FW,
     "Host HCI Reset Error Before FW Download (BtHal)"},
    {BqrErrorCode::HOST_DOWNLOAD_FW, "Host Firmware Download Error (BtHal)"},
    {BqrErrorCode::HOST_RESET_AFTER_FW,
     "Host HCI Reset Error After FW Download (BtHal)"},
    {BqrErrorCode::HOST_BDADDR_FAULT,
     "Host Can't fetch the provisioning BDA (BtHal)"},
    {BqrErrorCode::HOST_ACCEL_BT_INIT_FAILED,
     "Host Accelerated Init Failed (BtHal)"},
    {BqrErrorCode::HOST_ACCEL_BT_SHUTDOWN_FAILED,
     "Host Accelerated ShutDown Failed (BtHal)"},
    {BqrErrorCode::CHRE_ARBITRATOR_UNIMPLEMENTED_PACKET,
     "Arbitrator Detected Unimplemented Packet Type Error (BtChre)"},
    {BqrErrorCode::CHRE_ARBITRATOR_INVALID_PACKET_SIZE,
     "Arbitrator Detected Invalid Packet Size (BtChre)"},
};

DebugAnchor::DebugAnchor(AnchorType type, const std::string& anchor,
                         DebugCentral& debugcentral)
    : debugcentral_(&debugcentral), anchor_(anchor), type_(type) {
  std::stringstream ss;
  ss << anchor << " [ IN]";
  debugcentral_->UpdateRecord(type_, ss.str());
}

DebugAnchor::~DebugAnchor() {
  if (anchor_.empty()) {
    return;
  }
  std::stringstream ss;
  ss << anchor_ << " [OUT]";
  debugcentral_->UpdateRecord(
      static_cast<AnchorType>(static_cast<uint8_t>(type_) + 1), ss.str());
  anchor_.clear();
}

// Keep this section for future refactory from HciFlowControl to
// HciRouterClient.
#if 0
namespace {

using WatcherCallback = std::function<bool(const HalPacket&)>;

class DebugEventWatcher : public hci::HciEventWatcher {
 public:
  DebugEventWatcher(const char* tag, hci::HciFlowControl* handle,
                    uint16_t event_code, uint16_t command_opcode,
                    WatcherCallback callback)
      : HciEventWatcher(tag, event_code, command_opcode, false, true),
        callback_(callback),
        hci_handle_(handle) {
    hci_handle_->RegisterEventWatcher(this);
  }

  ~DebugEventWatcher() { hci_handle_->UnregisterEventWatcher(this); }

  bool OnEventReceive(const hci::HalPacket& event) {
    bool hijack_event = false;
    if (callback_ != nullptr) {
      hijack_event = callback_(event);
    }
    return hijack_event;
  }
  bool OnEventPost(const hci::HalPacket& event) {
    (void)event;
    return true;
  }

 private:
  WatcherCallback callback_;
  hci::HciFlowControl* hci_handle_;
};

}  // namespace
#endif

DebugCentral DebugCentral::instance_;

DebugCentral* DebugCentral::Get() { return &instance_; }

void DebugCentral::Dump(int fd) {
  // Dump BtHal debug log
  dump_hal_log(fd);
  if (TransportInterface::GetTransportType() == TransportType::kUartH4) {
    // Dump Kernel driver debugfs log
    dump_debugfs_to_fd(fd, serial_debug_port_);
    dump_debugfs_to_fd(fd, kDebugNodeBtLpm);
  }
  // Dump all crashinfo_bt files in ssrdump folder
  collect_sscd_logs(fd, kCrashInfoFilePrefix, kCrashInfoFilePath);
  // Dump all coredump_bt files in coredump folder
  LOG(INFO) << __func__
            << ": Write bt coredump files to `IBluetoothHci_default.txt`.";
  collect_sscd_logs(fd, kSsrdumpFilePrefix, kSsrdumpFilePath);
  // Dump Controller BT Activities Statistics
  BtActivitiesLogger::GetInstacne()->ForceUpdating();
  BtActivitiesLogger::GetInstacne()->DumpBtActivitiesStatistics(fd);
  delete_coredump_files(kSsrdumpFilePath);
}

void DebugCentral::HasClientConnectWith(bool has_client) {
  has_client_ = has_client;
  LOG(INFO) << __func__ << ": has_client: " << has_client_ << ".";
}

void DebugCentral::SetBtUartDebugPort(const std::string& uart_port) {
  if (uart_port.empty()) {
    LOG(ERROR) << __func__ << ": UART port is empty!";
    return;
  }

  std::size_t const found = uart_port.find_first_of("0123456789");
  if (found != std::string::npos) {
    serial_debug_port_ = kDebugNodeBtUartPrefix + uart_port.substr(found);
    LOG(INFO) << __func__ << ": Serial debug port: " << serial_debug_port_
              << ".";
    return;
  }
  LOG(ERROR) << __func__ << ": Cannot found uart port!";
}

#if 0
void DebugCentral::StartMonitor(hci::HciFlowControl* handle) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (handle == nullptr) {
    LOG(ERROR) << __func__ << ": Invalid handle!";
    return;
  }

  event_watchers_.emplace_back(std::make_unique<DebugEventWatcher>(
      "debug_HwError", handle, kEventHardwareError,
      hci::HciEventWatcher::kCommandUnspecifiedOpcode,
      [](const HalPacket& packet) {
        LOG(ERROR) << __func__ << ": Received Hardware error event!";
        ONE_TIME_LOGGER(AnchorType::HW_ERR_EVT,
                        "%s: Received Hardware error event!", __func__);
        if (packet.size() > kHwCodeOffset) {
          LOG(ERROR) << __func__ << ": Error code=" << std::hex
                     << static_cast<int>(packet.data()[kHwCodeOffset]) << ".";
        }
        // TODO: b/373786258 - Need to start_crash_dump with first hci reset
        // flag for silent report.

        // Do not hijack event.
        return false;
      }));
  event_watchers_.emplace_back(std::make_unique<DebugEventWatcher>(
      "debug_VendorEvent", handle, kEventVendorSpecific,
      hci::HciEventWatcher::kCommandUnspecifiedOpcode,
      [this](const HalPacket& packet) {
        handle_vendor_specific_event(packet);
        if (hijack_event_) {
          hijack_event_ = false;
          // hijack this vse
          return true;
        }
        // do not hijack this vse
        return false;
      }));
  event_watchers_.emplace_back(std::make_unique<DebugEventWatcher>(
      "vendorCapabilityEvent", handle,
      hci::HciEventWatcher::kCommandCompleteEventCode,
      hci::HciEventWatcher::kCommandVendorCapabilityOpcode,
      [this](const HalPacket& packet) {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        updateControllerCapability(packet);
        return false;
      }));
  // TODO(b/263604600): debugging message will be removed after issue clarified
  event_watchers_.emplace_back(std::make_unique<DebugEventWatcher>(
      "LeSetExtentedScanEnableEvent", handle,
      hci::HciEventWatcher::kCommandCompleteEventCode,
      hci::HciEventWatcher::kCommandLeScanEnableOpcode,
      [this](const HalPacket& packet) {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        uint16_t opcode = packet[3] + ((packet[4] << 8u) & 0xFF00);
        LOG(INFO) << __func__
                  << ": Command Complete Event of LE_SET_EXTENDED_SCAN_ENABLE, "
                  << "opcode:" << std::hex << opcode << ".";
        return false;
      }));
}

void DebugCentral::StopMonitor() {
  LOG(INFO) << __func__;
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  event_watchers_.clear();
  crash_timestamp_.clear();
  // reset Soc MemDump Cache
  if (!socdump_.empty()) {
    std::queue<std::vector<uint8_t>> empty_queue;
    std::swap(socdump_, empty_queue);
  }
}
#endif

void DebugCentral::UpdateRecord(AnchorType type, const std::string& anchor) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  std::string anchor_timestamp = Logger::GetLogFormatTimestamp();
  std::pair log_entry =
      std::pair<std::string, std::string>(anchor, anchor_timestamp);
  if (history_record_.size() >= kMaxHistory) {
    history_record_.pop_front();
  }
  history_record_.push_back(log_entry);
  lasttime_record_[type] = log_entry;
}

void DebugCentral::ReportBqrError(BqrErrorCode error, std::string extra_info) {
  HalPacket bqr_event({0xff, 0x04, 0x58, 0x05, 0x00, (uint8_t)error});
  // collect debug dump and popup ssrdump notification UI
  ONE_TIME_LOGGER(AnchorType::BQR_ERR_MSG, "%s", extra_info.c_str());
  LOG(ERROR) << __func__ << ": Root inflamed event with error_code: ("
             << static_cast<uint8_t>(error) << "), error_info: " << extra_info
             << ".";
  // report bqr root inflamed event to Stack
  if (notify_cb_ != nullptr) {
    LOG(WARNING) << __func__ << ": notify_cb_(bqr_event).";
    notify_cb_(bqr_event);
  }

  if (report_ssr_crash(static_cast<uint8_t>(error))) {
    start_crash_dump(false,
                     kDumpReasonControllerRootInflammed + " (" +
                         StringPrintf("error_code: 0x%02hhX",
                                      static_cast<unsigned char>(error)) +
                         ")" + " - " + get_error_code_string(error));
    backup_logging_files_before_crash(crash_timestamp_);
    LogFatal(error, extra_info);
  } else {
    LOG(ERROR) << __func__ << ": Silent recover!";
    ThreadHandler::Cleanup();
    kill(getpid(), SIGKILL);
  }
}

void DebugCentral::ForceGetCoredumpTimeout(union sigval sig) {
  DebugCentral* debug_ctrl = static_cast<DebugCentral*>(sig.sival_ptr);
  // It is supported to generate coredump and record crash_timestamp_ when bthal
  // received root-inflamed event or any fw dump packet, if the controller
  // did not send any response packets, we force to trigger coredump here
  if (debug_ctrl->crash_timestamp_.empty()) {
    LOG(ERROR) << __func__
               << ": Force a coredump to be generated if it has not been "
                  "generated for 1 second.";
    debug_ctrl->start_crash_dump(true,
                                 kDumpReasonForceCollectCoredump + " (BtFw)");
  }

  if (force_coredump_timer_created == true) {
    itimerspec ts{};
    LOG(WARNING) << __func__ << ": Delete force_coredump_timer.";
    timer_settime(force_coredump_timer, 0, &ts, NULL);
    timer_delete(force_coredump_timer);
    force_coredump_timer_created = false;
  }
}

bool DebugCentral::IsControllerDebugDumpOpcode(const HalPacket& data) {
  if (data.size() < 2) {
    return false;
  }

  if (data[0] == 0x5b && data[1] == 0xfd) {
    LOG(WARNING) << __func__ << ": Sending Controller Debug Info Command.";
    ONE_TIME_LOGGER(AnchorType::DEBUG_INFO,
                    "%s: Sending Controller Debug Info command!", __func__);
    int ret;
    itimerspec dump_ts{};
    sigevent force_coredump_se{};
    force_coredump_se.sigev_notify = SIGEV_THREAD;
    force_coredump_se.sigev_value.sival_ptr = this;
    force_coredump_se.sigev_notify_function =
        (void (*)(sigval))ForceGetCoredumpTimeout;
    force_coredump_se.sigev_notify_attributes = NULL;
    ret = timer_create(CLOCK_MONOTONIC, &force_coredump_se,
                       &force_coredump_timer);
    if (ret < 0) {
      LOG(ERROR) << __func__ << ": Cannot create force_coredump_timer!";
    } else {
      force_coredump_timer_created = true;
    }

    // update force_coredump_timer
    dump_ts.it_value.tv_sec = kForceCoredumpTimerExpiredSec;
    dump_ts.it_value.tv_nsec = kForceCoredumpTimerExpiredNs;
    if (force_coredump_timer_created == true) {
      ret = timer_settime(force_coredump_timer, 0, &dump_ts, NULL);
      if (ret < 0) {
        LOG(ERROR) << __func__ << ": Cannot arm force_coredump_timer!";
      }
    }
    return true;
  }
  return false;
}

bool DebugCentral::is_hw_stage_supported() {
  std::string cur_hw_stage = ::android::base::GetProperty(kHwStage, "default");
  std::vector<std::string> not_supported_hw_stages =
      HalConfigLoader::GetLoader().GetFwUnsupportedHwStages();
  return std::find_if(not_supported_hw_stages.begin(),
                      not_supported_hw_stages.end(),
                      [&](std::string& not_supported_hw_stage) {
                        return cur_hw_stage == not_supported_hw_stage;
                      }) == not_supported_hw_stages.end();
}

bool DebugCentral::report_ssr_crash(uint8_t vendor_error_code) {
  // Report scenario:
  // 1) report ssr crash when bt is on
  // 2) report ssr crash when bt is off, thread is enabled, and supports
  // accelerated bt on

  bool is_major_fault = (static_cast<BqrErrorCode>(vendor_error_code) ==
                         BqrErrorCode::FIRMWARE_MISCELLANEOUS_MAJOR_FAULT);

  if (is_major_fault || !is_hw_stage_supported()) {
    return false;
  }

  bool is_thread_dispatcher_working =
      ThreadHandler::IsHandlerRunning() &&
      ThreadHandler::GetHandler().IsDaemonRunning();

  return is_thread_dispatcher_working || has_client_;
}

void DebugCentral::dump_hal_log(int fd) {
  std::stringstream ss;

  ss << "=============================================" << std::endl;
  ss << "Controller Firmware Information" << std::endl;
  ss << "=============================================" << std::endl;
  // TODO: b/373786258 - Print local name into stringstream.

  ss << "=============================================" << std::endl;
  ss << "Anchors' Last Appear" << std::endl;
  ss << "=============================================" << std::endl;
  for (auto it = lasttime_record_.begin(); it != lasttime_record_.end(); ++it) {
    std::string anchor = it->second.first;
    std::string anchor_timestamp = it->second.second;
    ss << "Timestamp of " << anchor << ": " << anchor_timestamp << std::endl;
  }

  ss << "=============================================" << std::endl;
  ss << "Anchors' History" << std::endl;
  ss << "=============================================" << std::endl;
  for (auto it = history_record_.begin(); it != history_record_.end(); ++it) {
    std::string anchor = it->first;
    std::string anchor_timestamp = it->second;
    ss << anchor_timestamp << ": " << anchor << std::endl;
  }
  write(fd, ss.str().c_str(), ss.str().length());
}

void DebugCentral::handle_vendor_specific_event(const HalPacket& packet) {
  if (packet.size() < kVseSubEventCodeOffset) {
    LOG(WARNING) << __func__ << ": Invalid length of VS event!";
    return;
  }
  uint8_t sub_event_code = packet[kVseSubEventCodeOffset];
  switch (sub_event_code) {
    case kVseSubEventDebugInfo:
      static int msg_counter = 0;
      if (++msg_counter <= 20) {
        LOG(WARNING) << __func__ << ": Received Debug Info event!";
      }
      handle_debug_info_event(packet);
      hijack_event_ = true;
      break;
    case kVseSubEventBqr:
      handle_bqr_event(packet);
      break;
    case kVseSubEventDebug1:
      [[fallthrough]];
    case kVseSubEventDebug2:
      hijack_event_ = true;
      break;
    default:
      break;
  }
}

void DebugCentral::handle_bqr_event(const HalPacket& packet) {
  if (packet.size() <= kBqrReportIdOffset) {
    LOG(WARNING) << __func__ << ": Invalid length of BQR event!";
    return;
  }
  auto quality_report_id =
      static_cast<BqrQualityReportId>(packet[kBqrReportIdOffset]);
  switch (quality_report_id) {
    case BqrQualityReportId::kMonitorMode:
      [[fallthrough]];
    case BqrQualityReportId::kApproachLsto:
      [[fallthrough]];
    case BqrQualityReportId::kA2dpAudioChoppy:
      [[fallthrough]];
    case BqrQualityReportId::kScoVoiceChoppy:
      [[fallthrough]];
    case BqrQualityReportId::kLeAudioChoppy:
      ParseLinkQualityRelatedEvt(packet);
      break;

    case BqrQualityReportId::kRootInflammation: {
      uint8_t error_code = packet[kBqrInflamedErrorCode];
      uint8_t vendor_error_code = packet[kBqrInflamedVendorErrCode];
      LOG(ERROR) << __func__ << ": Received Root Inflammation event! (0x"
                 << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(error_code) << std::setw(2)
                 << std::setfill('0') << static_cast<int>(vendor_error_code)
                 << ").";
      // for some vendor error code event that we do not report root inflamed
      // event
      if (report_ssr_crash(vendor_error_code)) {
        start_crash_dump(
            false,
            kDumpReasonControllerRootInflammed + " (" +
                StringPrintf("vendor_error: 0x%02hhX", vendor_error_code) +
                ")" + " - " +
                get_error_code_string(
                    static_cast<BqrErrorCode>(vendor_error_code)));
        backup_logging_files_before_crash(crash_timestamp_);
        hijack_event_ = false;
      } else {
        hijack_event_ = true;
      }
      break;
    }

    // Just logs to vnd snoop and skips reporting to stack
    case BqrQualityReportId::kEnergyMonitoring:
      BtBqrEnergyRecoder::GetInstacne()->ParseBqrEnergyMonitorEvt(packet);
      hijack_event_ = true;
      break;

    case BqrQualityReportId::kControllerDbgInfo: {
      handle_bqr_fw_debug_data_dump(packet);
      hijack_event_ = true;
      break;
    }

    case BqrQualityReportId::kAdvanceRfStats:
      [[fallthrough]];
    case BqrQualityReportId::kAdvanceRfStatsPeriodic:
      ParseAdvanceRFStatsEvt(packet);
      hijack_event_ = true;
      break;
    case BqrQualityReportId::kControllerHealthMonitor:
      [[fallthrough]];
    case BqrQualityReportId::kControllerHealthMonitorPeriodic:
      ParseControllerHealthMonitorEvt(packet);
      hijack_event_ = true;
      break;
    case BqrQualityReportId::kChreDbgInfo: {
      handle_bqr_chre_debug_data_dump(packet);
      hijack_event_ = true;
      break;
    }
    case BqrQualityReportId::kVendorSpecificTrace: {
      ParseVendorSpecificTraceEvt(packet);
      break;
    }
    case BqrQualityReportId::kVendorSpecificQuality: {
      ParseVendorSpecificQualityEvt(packet);
      break;
    }
    default: {
      break;
    }
  }
}

void DebugCentral::handle_debug_info_event(const HalPacket& packet) {
  bool last_soc_dump_packet = false;
  if (packet.size() <= kDebugInfoPayloadOffset) {
    LOG(INFO) << __func__ << ": Invalid length of debug info event!";
    return;
  }

  if (crash_timestamp_.empty()) {
    start_crash_dump(is_hw_stage_supported() ? false : true,
                     kDumpReasonControllerDebugInfo + " (BtFw)");
  }

  // the Last soc dump debug info packet has been received
  if (packet[kDebugInfoLastBlockOffset]) {
    LOG(INFO) << __func__ << ": Last soc dump fragment has been received.";
    backup_logging_files_before_crash(crash_timestamp_);
    last_soc_dump_packet = true;
  }

  int socdump_fd;
  if ((socdump_fd = open_firmware_dump_file(crash_timestamp_)) < 0) {
    return;
  }

  size_t ret = 0;
  if ((ret = TEMP_FAILURE_RETRY(
           write(socdump_fd, packet.data(), packet.size()))) < 0) {
    LOG(ERROR) << __func__ << ": Error writing to dest file: " << ret << " ("
               << strerror(errno) << ").";
  }

  close(socdump_fd);
  if (last_soc_dump_packet) {
    LOG(ERROR) << __func__ << ": Restart bthal service for recovery!";
    last_soc_dump_packet = false;
    ThreadHandler::Cleanup();
    kill(getpid(), SIGKILL);
  }
}

void DebugCentral::handle_bqr_fw_debug_data_dump(const HalPacket& packet) {
  if (packet.size() <= kBqrReportIdOffset) {
    LOG(WARNING) << __func__
                 << ": Invalid length of bqr debug firmware dump event!";
    return;
  }

  if (crash_timestamp_.empty()) {
    LOG(ERROR) << __func__
               << ": Did not receive Root Inflammation event before FW Dump!";
    start_crash_dump(
        is_hw_stage_supported() ? false : true,
        kDumpReasonControllerDebugDumpWithoutRootInflammed + " (BtFw)");
  }

  int socdump_fd;
  if ((socdump_fd = open_firmware_dump_file(crash_timestamp_)) < 0) {
    return;
  }

  size_t ret = 0;
  if ((ret = TEMP_FAILURE_RETRY(
           write(socdump_fd, packet.data(), packet.size()))) < 0) {
    LOG(ERROR) << __func__ << ": Error writing to dest file: " << ret << " ("
               << strerror(errno) << ").";
  }

  close(socdump_fd);
}

void DebugCentral::handle_bqr_chre_debug_data_dump(const HalPacket& packet) {
  if (packet.size() <= kBqrReportIdOffset) {
    LOG(WARNING) << __func__
                 << ": Invalid length of bqr chre debug dump event!";
    return;
  }

  if (crash_timestamp_.empty()) {
    LOG(ERROR) << __func__
               << ": Did not receive Root Inflammation event before CHRE Dump!";
    start_crash_dump(false, kDumpReasonControllerDebugDumpWithoutRootInflammed +
                                " (BtChre)");
  }

  std::vector<uint8_t> payload(packet.size());
  memcpy(payload.data(), packet.data(), packet.size());
  chredump_.push(payload);

  // Cache Chre dump in DEBUG_INFO event, dump to file if last fragment.
  uint16_t last_block;
  last_block =
      packet[kChreDebugDumpLastBlockOffsetSecondByte] +
      ((packet[kChreDebugDumpLastBlockOffsetFisrtByte] << 8u) & 0xFF00);

  if (last_block != 0x5A00) {
    return;
  }
  LOG(INFO) << __func__ << ": Last Chre dump packet has been received.";

  int chredump_fd;
  if ((chredump_fd = open_lpp_chre_dump_file(crash_timestamp_)) < 0) {
    return;
  }

  size_t bytes_written = 0;
  size_t ret = 0;
  while (!chredump_.empty()) {
    std::vector<uint8_t>& segment = chredump_.front();
    if ((ret = TEMP_FAILURE_RETRY(
             write(chredump_fd, segment.data(), segment.size()))) < 0) {
      LOG(ERROR) << __func__ << ": Error writing to dest file: " << ret << " ("
                 << strerror(errno) << ").";
      break;
    }
    if (ret != segment.size()) {
      LOG(ERROR) << __func__ << ": Actual write (" << ret
                 << " bytes) is not the same with expected write "
                 << "(" << segment.size() << " bytes) size.";
    }
    bytes_written += ret;
    chredump_.pop();
  }
  LOG(INFO) << __func__ << ": Total written " << bytes_written << " bytes.";

  if (!chredump_.empty()) {
    LOG(WARNING) << __func__ << ": There are " << chredump_.size()
                 << " segments not be saved.";
    chredump_ = {};
  }

  close(chredump_fd);
  hijack_event_ = true;
}

void DebugCentral::start_crash_dump(bool slient_report,
                                    const std::string& reason) {
  if (!crash_timestamp_.empty()) {
    // coredump has already been generated, avoid duplicated dump in one crash
    // cycle
    return;
  }

  LOG(ERROR) << __func__ << ": Reason: " << reason.c_str()
             << ", slient_report:" << slient_report << ".";
  crash_timestamp_ = crash_file_create_timestamp();
  std::stringstream coredump_fname;
  coredump_fname << kSsrdumpFilePath << kSsrdumpFilePrefix << crash_timestamp_
                 << ".bin";
  LOG(WARNING) << __func__ << ": Starting to generate Bluetooth ssrdump files: "
               << coredump_fname.str().c_str() << ".";

  int coredump_fd;
  if ((coredump_fd =
           open(coredump_fname.str().c_str(), O_CREAT | O_SYNC | O_RDWR,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
    LOG(ERROR) << __func__ << ": Failed to open coredump file: "
               << coredump_fname.str().c_str()
               << ", failed: " << strerror(errno) << " (" << errno << ").";
    return;
  }
  fchmod(coredump_fd, S_IRUSR | S_IRGRP | S_IROTH);

  std::stringstream ss;
  ss << "DUMP REASON: " << std::string(reason) << " - occurred at "
     << crash_timestamp_ << std::endl;
  write(coredump_fd, ss.str().c_str(), ss.str().length());

  dump_hal_log(coredump_fd);
  LOG(INFO) << __func__ << ": Request to get Transport Layer Debug Dump.";
  // TODO: b/373786258 - Need to dump debug info.
  close(coredump_fd);

  if (slient_report) {
    return;
  }
  // generate crashinfo file
  std::stringstream crashinfo_fname;
  crashinfo_fname << kCrashInfoFilePath << kCrashInfoFilePrefix
                  << crash_timestamp_ << ".txt";

  int crashinfo_fd;
  if ((crashinfo_fd =
           open(crashinfo_fname.str().c_str(), O_CREAT | O_SYNC | O_RDWR,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
    LOG(ERROR) << __func__
               << ": Failed to open crashinfo file: " << crashinfo_fname.str()
               << ", failed: " << strerror(errno) << " (" << errno << ").";
    return;
  }
  fchmod(crashinfo_fd, S_IRUSR | S_IRGRP | S_IROTH);

  std::stringstream crashinfo_ss;
  static int crash_count = 0;
  crashinfo_ss << "crash_reason: " << std::string(reason) << std::endl;
  crash_count++;
  crashinfo_ss << "crash_count: " << std::to_string(crash_count) << std::endl;
  crashinfo_ss << "timestamp: " << crash_timestamp_ << std::endl;
  write(crashinfo_fd, crashinfo_ss.str().c_str(), crashinfo_ss.str().length());
  close(crashinfo_fd);
}

const std::string get_error_code_string(BqrErrorCode error_code) {
  return error_code_string.find(error_code) != error_code_string.end()
             ? error_code_string.at(error_code)
             : "Undefined error code";
}

}  // namespace debug
}  // namespace bluetooth_hal
