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

#define LOG_TAG "bluetooth_hal.debug_central"

#include "bluetooth_hal/debug/debug_central.h"

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "android-base/logging.h"
#include "android-base/properties.h"
#include "bluetooth_hal/bqr/bqr_handler.h"
#include "bluetooth_hal/bqr/bqr_root_inflammation_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/debug_client.h"
#include "bluetooth_hal/debug/debug_monitor.h"
#include "bluetooth_hal/debug/debug_types.h"
#include "bluetooth_hal/debug/debug_util.h"
#include "bluetooth_hal/extensions/thread/thread_handler.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hci_router.h"
#include "bluetooth_hal/util/logging.h"
#include "bluetooth_hal/util/power/wakelock_watchdog.h"
#include "bluetooth_hal/util/timer_manager.h"

namespace bluetooth_hal {
namespace debug {
namespace {

using ::bluetooth_hal::bqr::BqrErrorCode;
using ::bluetooth_hal::bqr::BqrErrorToStringView;
using ::bluetooth_hal::bqr::BqrRootInflammationEvent;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciRouter;
using ::bluetooth_hal::thread::ThreadHandler;
using ::bluetooth_hal::util::Logger;
using ::bluetooth_hal::util::power::WakelockWatchdog;

constexpr int kDebugInfoPayloadOffset = 8;
constexpr int kDebugInfoLastBlockOffset = 5;

constexpr int kHandleDebugInfoCommandMs = 1000;
constexpr int kRestartHalTimeoutMs = 6000;
constexpr int kMaxCoredumpFiles = 3;
const std::string kCoredumpFilePrefix = kCoredumpFilePath + kCoredumpPrefix;
const std::string kSocdumpFilePrefix =
    kCoredumpFilePath + "coredump_bt_socdump_";

const std::string kDebugNodeBtLpm = "dev/logbuffer_btlpm";
constexpr char kDebugNodeBtUartPrefix[] = "/dev/logbuffer_tty";
constexpr char kHwStage[] = "ro.boot.hardware.revision";

}  // namespace

void LogFatal(BqrErrorCode error, std::string extra_info);

DurationTracker::DurationTracker(AnchorType type, const std::string& log)
    : log_(log), type_(type) {
  std::stringstream ss;
  ss << "[ IN] " << log_;
  DebugCentral::Get().AddLog(type_, ss.str());
}

DurationTracker::~DurationTracker() {
  if (log_.empty()) {
    return;
  }
  std::stringstream ss;
  ss << "[OUT] " << log_;
  DebugCentral::Get().AddLog(type_, ss.str());
}

class DebugCentralImpl : public DebugCentral {
 public:
  DebugCentralImpl();
  ~DebugCentralImpl() = default;

  bool RegisterDebugClient(DebugClient* debug_client) override;

  bool UnregisterDebugClient(DebugClient* debug_client) override;

  void Dump(int fd) override;

  void SetBtUartDebugPort(const std::string& uart_port) override;

  void AddLog(AnchorType type, const std::string& log) override;

  void ReportBqrError(::bluetooth_hal::bqr::BqrErrorCode error,
                      std::string extra_info) override;

  void HandleRootInflammationEvent(
      const ::bluetooth_hal::bqr::BqrRootInflammationEvent& event) override;

  void HandleDebugInfoEvent(
      const ::bluetooth_hal::hci::HalPacket& packet) override;

  void HandleDebugInfoCommand() override;

  void GenerateVendorDumpFile(const std::string& file_path,
                              const std::vector<uint8_t>& data,
                              uint8_t vendor_error_code = 0) override;

  void GenerateCoredump(CoredumpErrorCode error_code,
                        uint8_t sub_error_code = 0) override;

  void ResetCoredumpGenerator() override;

  bool IsCoredumpGenerated() override;

  std::string& GetCoredumpTimestampString() override;

 private:
  static constexpr int kMaxHalLogLines = 400;
  std::string serial_debug_port_;
  std::string crash_timestamp_;
  std::recursive_mutex mutex_;
  std::list<std::pair<std::string, std::string>> hal_log_;
  std::map<AnchorType, std::pair<std::string, std::string>> anchor_log_;
  ::bluetooth_hal::util::Timer debug_info_command_timer_;
  DebugMonitor debug_monitor_;
  ::bluetooth_hal::bqr::BqrHandler bqr_handler_;
  std::unordered_set<DebugClient*> debug_clients_;
  bool is_coredump_generated_;

  std::string DumpBluetoothHalLog(const std::vector<Coredump>& client_dumps);
  bool OkToGenerateCrashDump(uint8_t error_code);
  bool IsHardwareStageSupported();
  std::string GetOrCreateCoredumpTimestampString();
  int OpenOrCreateCoredumpBin(const std::string& file_prefix);
  std::vector<Coredump> GetCoredumpFromDebugClients();
};

DebugCentral& DebugCentral::Get() {
  static DebugCentralImpl debug_central;
  return debug_central;
}

DebugCentralImpl::DebugCentralImpl() : is_coredump_generated_(false) {}

bool DebugCentralImpl::RegisterDebugClient(DebugClient* client) {
  if (!client) {
    return false;
  }

  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (debug_clients_.count(client) > 0) {
    LOG(WARNING) << "debug client already registered!";
    return false;
  }
  debug_clients_.insert(client);
  return true;
}

bool DebugCentralImpl::UnregisterDebugClient(DebugClient* client) {
  if (!client) {
    return false;
  }
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (debug_clients_.erase(client) == 0) {
    LOG(WARNING) << "debug client was not registered!";
    return false;
  }
  return true;
}

std::vector<Coredump> DebugCentralImpl::GetCoredumpFromDebugClients() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  std::vector<Coredump> client_dumps;
  for (auto client : debug_clients_) {
    if (client == nullptr) {
      LOG(WARNING) << __func__
                   << ": null router client in the registration list!";
      continue;
    }
    auto dump = client->Dump();
    client_dumps.insert(client_dumps.end(), dump.begin(), dump.end());
  }
  return client_dumps;
}

void DebugCentralImpl::Dump(int fd) {
  LOG(INFO) << __func__
            << ": Write bt coredump files to `IBluetoothHci_default.txt`.";

  const std::string dumpsys_header = "Bluetooth HAL DUMP";

  std::stringstream dump;
  auto client_dumps = GetCoredumpFromDebugClients();
  dump << GenerateHalLogStringFrame(dumpsys_header,
                                    DumpBluetoothHalLog(client_dumps), false);
  dump << CoredumpToStringLog(client_dumps, CoredumpPosition::kCustomDumpsys);
  write(fd, dump.str().c_str(), dump.str().length());

  FlushCoredumpToFd(fd);
}

void DebugCentralImpl::SetBtUartDebugPort(const std::string& uart_port) {
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

void DebugCentralImpl::AddLog(AnchorType type, const std::string& log) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  std::string timestamp_str = Logger::GetLogFormatTimestamp();
  std::pair log_with_timestamp =
      std::pair<std::string, std::string>(log, timestamp_str);
  if (hal_log_.size() >= kMaxHalLogLines) {
    hal_log_.pop_front();
  }
  hal_log_.push_back(log_with_timestamp);
  if (type != AnchorType::kNone) {
    anchor_log_[type] = log_with_timestamp;
  }
}

void DebugCentralImpl::ReportBqrError(BqrErrorCode error,
                                      std::string extra_info) {
  HalPacket bqr_event(
      {0xff, 0x04, 0x58, 0x05, 0x00, static_cast<uint8_t>(error)});

  HAL_LOG(ERROR) << extra_info;
  LOG(ERROR) << __func__ << ": Root inflamed event with error_code: ("
             << static_cast<uint8_t>(error) << "), error_info: " << extra_info
             << ".";
  // report bqr root inflamed event to Stack
  HciRouter::GetRouter().SendPacketToStack(bqr_event);

  if (OkToGenerateCrashDump(static_cast<uint8_t>(error))) {
    GenerateCoredump(CoredumpErrorCode::kControllerRootInflammed,
                     static_cast<uint8_t>(error));
    LogFatal(error, extra_info);
  } else {
    LOG(ERROR) << __func__ << ": Silent recover!";
    ThreadHandler::Cleanup();
    kill(getpid(), SIGKILL);
  }
}

void DebugCentralImpl::HandleDebugInfoCommand() {
  // It is supported to generate coredump and record the timestamp when bthal
  // received root-inflamed event or any fw dump packet, if the controller
  // did not send any response packets, we force to trigger coredump here
  debug_info_command_timer_.Schedule(
      [this]() {
        LOG(ERROR) << __func__
                   << ": Force a coredump to be generated if it has not been "
                      "generated for 1 second.";
        GenerateCoredump(CoredumpErrorCode::kForceCollectCoredump);
      },
      std::chrono::milliseconds(kHandleDebugInfoCommandMs));
}

void DebugCentralImpl::GenerateVendorDumpFile(const std::string& file_path,
                                              const std::vector<uint8_t>& data,
                                              uint8_t vendor_error_code) {
  if (file_path.empty()) {
    LOG(ERROR) << "File name is empty!";
    return;
  }
  GenerateCoredump(CoredumpErrorCode::kVendor, vendor_error_code);

  int fd = OpenOrCreateCoredumpBin(file_path);
  if (fd < 0) {
    LOG(ERROR) << "Failed to open vendor dump file: " << file_path;
    return;
  }

  ssize_t ret = 0;
  if ((ret = TEMP_FAILURE_RETRY(write(fd, data.data(), data.size()))) < 0) {
    LOG(ERROR) << "Error writing to dest file: " << ret << " ("
               << strerror(errno) << ")";
  }
  close(fd);
}

bool DebugCentralImpl::IsHardwareStageSupported() {
  std::string cur_hw_stage = ::android::base::GetProperty(kHwStage, "default");
  std::vector<std::string> not_supported_hw_stages =
      HalConfigLoader::GetLoader().GetUnsupportedHwStages();
  return std::find_if(not_supported_hw_stages.begin(),
                      not_supported_hw_stages.end(),
                      [&](std::string& not_supported_hw_stage) {
                        return cur_hw_stage == not_supported_hw_stage;
                      }) == not_supported_hw_stages.end();
}

bool DebugCentralImpl::OkToGenerateCrashDump(uint8_t error_code) {
  // 1) generate coredump when bt is on
  // 2) generate coredump when bt is off, thread is enabled, and supports
  // accelerated bt on

  bool is_major_fault = (static_cast<BqrErrorCode>(error_code) ==
                         BqrErrorCode::kFirmwareMiscellaneousMajorFault);

  if (is_major_fault || !IsHardwareStageSupported()) {
    return false;
  }

  bool is_thread_dispatcher_working =
      ThreadHandler::IsHandlerRunning() &&
      ThreadHandler::GetHandler().IsDaemonRunning();

  return is_thread_dispatcher_working || debug_monitor_.IsBluetoothEnabled();
}

std::string DebugCentralImpl::DumpBluetoothHalLog(
    const std::vector<Coredump>& client_dumps) {
  std::stringstream anchor_log;
  for (auto it = anchor_log_.begin(); it != anchor_log_.end(); ++it) {
    std::string log = it->second.first;
    std::string timestamp = it->second.second;
    anchor_log << timestamp << ": " << log << std::endl;
  }
  std::stringstream anchor_history;
  for (auto it = hal_log_.begin(); it != hal_log_.end(); ++it) {
    std::string log = it->first;
    std::string timestamp = it->second;
    anchor_history << timestamp << ": " << log << std::endl;
  }

  std::stringstream ss;
  ss << CoredumpToStringLog(client_dumps, CoredumpPosition::kBegin);
  ss << GenerateHalLogString("Anchor Log", anchor_log.str());
  ss << GenerateHalLogString("Bluetooth HAL Log", anchor_history.str());
  ss << CoredumpToStringLog(client_dumps, CoredumpPosition::kEnd);

  if (!serial_debug_port_.empty()) {
    // Dump Kernel driver debugfs log
    ss << DumpDebugfs(serial_debug_port_);
    ss << DumpDebugfs(kDebugNodeBtLpm);
  }

  return ss.str();
}

void DebugCentralImpl::HandleRootInflammationEvent(
    const BqrRootInflammationEvent& event) {
  if (!event.IsValid()) {
    LOG(ERROR) << __func__ << ": Invalid root inflammation event! "
               << event.ToString();
    return;
  }

  uint8_t error_code = event.GetErrorCode();
  uint8_t vendor_error_code = event.GetVendorErrorCode();
  LOG(ERROR) << __func__ << ": Received Root Inflammation event! (0x"
             << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(error_code) << std::setw(2)
             << std::setfill('0') << static_cast<int>(vendor_error_code)
             << ").";
  // For some vendor error codes that we do not generate a crash dump.
  if (OkToGenerateCrashDump(vendor_error_code)) {
    GenerateCoredump(CoredumpErrorCode::kControllerRootInflammed,
                     vendor_error_code);
  }
}

void DebugCentralImpl::HandleDebugInfoEvent(const HalPacket& packet) {
  bool last_soc_dump_packet = false;
  if (packet.size() <= kDebugInfoPayloadOffset) {
    LOG(INFO) << __func__ << ": Invalid length of debug info event!";
    return;
  }

  GenerateCoredump(CoredumpErrorCode::kControllerDebugInfo);

  // the Last soc dump debug info packet has been received
  if (packet[kDebugInfoLastBlockOffset]) {
    LOG(INFO) << __func__ << ": Last soc dump fragment has been received.";
    last_soc_dump_packet = true;
  }

  int socdump_fd;
  if ((socdump_fd = OpenOrCreateCoredumpBin(kSocdumpFilePrefix)) < 0) {
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
    LOG(ERROR) << __func__ << ": Restart Bluetooth HAL service for recovery!";
    last_soc_dump_packet = false;
    ThreadHandler::Cleanup();
  }
}

void DebugCentralImpl::GenerateCoredump(CoredumpErrorCode error_code,
                                        uint8_t sub_error_code) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (is_coredump_generated_) {
    // coredump has already been generated, avoid duplicated dump in one crash
    // cycle
    return;
  }

  // Pause the watchdog to prevent it from biting before coredump is
  // completed. The HAL will be restarted when the router state exits from
  // Running state.
  WakelockWatchdog::GetWatchdog().Pause();
  is_coredump_generated_ = true;

  HAL_LOG(ERROR) << __func__ << ": Reason: "
                 << CoredumpErrorCodeToString(error_code, sub_error_code);

  // Start a timer to automatically restart Bluetooth HAL after generating
  // coredump. Normally the host stack kills itself after an error and before
  // this thread timesout, which also terminates the Bluetooth HAL.
  std::thread restart_hal_thread([]() {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(kRestartHalTimeoutMs));
    LOG(ERROR) << __func__ << ": Restarting Bluetooth HAL!";
    kill(getpid(), SIGKILL);
  });
  restart_hal_thread.detach();

  // Inform vendor implementations that the dump has started.
  for (auto client : debug_clients_) {
    if (client == nullptr) {
      LOG(WARNING) << __func__
                   << ": null router client in the registration list!";
      continue;
    }
    client->OnGenerateCoredump(error_code, sub_error_code);
  }

  int coredump_fd = OpenOrCreateCoredumpBin(kCoredumpFilePrefix);

  if (coredump_fd < 0) {
    LOG(ERROR) << __func__ << ": Failed to open coredump file!";
    return;
  }

  std::stringstream ss;
  ss << "║\tDUMP REASON: "
     << CoredumpErrorCodeToString(error_code, sub_error_code)
     << " - occurred at " << GetCoredumpTimestampString() << std::endl;
  ss << "║" << std::endl;

  auto client_dumps = GetCoredumpFromDebugClients();
  ss << DumpBluetoothHalLog(client_dumps);

  write(coredump_fd, ss.str().c_str(), ss.str().length());
  close(coredump_fd);
}

int DebugCentralImpl::OpenOrCreateCoredumpBin(
    const std::string& file_name_prefix) {
  std::string file_name =
      file_name_prefix + GetOrCreateCoredumpTimestampString() + ".bin";

  if (access(file_name.c_str(), F_OK) != 0) {
    // File does not exist, require to create a new one.
    HAL_LOG(WARNING) << "Creating coredump file: " << file_name;
  }

  int fd = open(file_name.c_str(), O_APPEND | O_CREAT | O_SYNC | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP);

  if (fd < 0) {
    LOG(ERROR) << __func__
               << ": Failed to open or create coredump file: " << file_name
               << ", error: " << strerror(errno) << " (" << errno << ")";
    return -1;
  }

  if (chmod(file_name.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
    LOG(ERROR) << __func__ << ": Unable to change file permissions for "
               << file_name << ", error: " << strerror(errno) << " (" << errno
               << ")";
  }

  // Delete old files and keep the latest ones.
  size_t last_slash_pos = file_name_prefix.find_last_of('/');
  if (last_slash_pos != std::string::npos) {
    auto file_path = file_name_prefix.substr(0, last_slash_pos + 1);
    auto prefix = file_name_prefix.substr(last_slash_pos + 1);
    DeleteOldestBinFiles(file_path, prefix, kMaxCoredumpFiles);
  }
  return fd;
}

std::string DebugCentralImpl::GetOrCreateCoredumpTimestampString() {
  if (crash_timestamp_.empty()) {
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
    crash_timestamp_ = ss.str();
  }
  return crash_timestamp_;
}

bool DebugCentralImpl::IsCoredumpGenerated() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  return is_coredump_generated_;
}

void DebugCentralImpl::ResetCoredumpGenerator() {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  crash_timestamp_.clear();
  if (is_coredump_generated_) {
    HAL_LOG(ERROR) << "Reset Bluetooth HAL after generating coredump!";
    kill(getpid(), SIGKILL);
  }
}

std::string& DebugCentralImpl::GetCoredumpTimestampString() {
  return crash_timestamp_;
}

std::string DebugCentral::CoredumpErrorCodeToString(
    CoredumpErrorCode error_code, uint8_t sub_error_code) {
  switch (error_code) {
    case CoredumpErrorCode::kForceCollectCoredump:
      return "Force Collect Coredump (BtFw)";
    case CoredumpErrorCode::kControllerHwError:
      return "Controller Hw Error (BtFw)";
    case CoredumpErrorCode::kControllerRootInflammed: {
      std::stringstream ss;
      ss << "Controller Root Inflammed (vendor_error: 0x" << std::hex
         << std::setw(2) << std::setfill('0')
         << static_cast<int>(sub_error_code) << ") - "
         << BqrErrorToStringView(static_cast<BqrErrorCode>(sub_error_code));
      return ss.str();
    }
    case CoredumpErrorCode::kControllerDebugDumpWithoutRootInflammed:
      return "Controller Debug Info Data Dump Without Root Inflammed (BtFw)";
    case CoredumpErrorCode::kControllerDebugInfo:
      return "Debug Info Event (BtFw)";
    case CoredumpErrorCode::kControllerUnimplementedPacketType:
      return "Controller Unimplemented Packet Type (BtFw)";
    case CoredumpErrorCode::kVendor:
      return "Vendor Error";
    default: {
      std::stringstream ss;
      ss << "Unknown Error Code <" << static_cast<int>(error_code) << ">";
      return ss.str();
    }
  }
}

}  // namespace debug
}  // namespace bluetooth_hal
