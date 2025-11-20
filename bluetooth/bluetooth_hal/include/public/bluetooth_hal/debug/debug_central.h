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

#pragma once

#include <signal.h>

#include <list>
#include <map>
#include <sstream>

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"

// Code Enter Point
enum class AnchorType : uint8_t {
  // For DURATION_TRACKER.
  BTHAL_INIT = 0,
  BTHAL_INIT_OUT,
  BTHAL_PERFORM_INIT,
  BTHAL_PERFORM_INIT_OUT,
  SEND_HCI_CMD,
  SEND_HCI_CMD_OUT,
  SEND_ACL_DAT,
  SEND_ACL_DAT_OUT,
  SEND_SCO_DAT,
  SEND_SCO_DAT_OUT,
  SEND_ISO_DAT,
  SEND_ISO_DAT_OUT,
  THREAD_SEND_DAT_UPLINK,
  THREAD_SEND_DAT_DOWNLINK,
  THREAD_START_DAEMON,
  THREAD_STOP_DAEMON,
  THREAD_ACCEPT_CLIENT,
  THREAD_DADEMON_CLOSED,
  THREAD_SOCKET_FILE_DELETED,
  THREAD_CLIENT_ERROR,
  THREAD_CLIENT_CONNECT,
  THREAD_HARDWARE_RESET,
  CALLBACK_HCI_EVT,
  CALLBACK_HCI_EVT_OUT,
  CALLBACK_HCI_ACL,
  CALLBACK_HCI_ACL_OUT,
  CALLBACK_HCI_SCO,
  CALLBACK_HCI_SCO_OUT,
  CALLBACK_HCI_ISO,
  CALLBACK_HCI_ISO_OUT,
  USERIAL_OPEN,
  USERIAL_OPEN_OUT,
  USERIAL_CLOSE,
  USERIAL_CLOSE_OUT,
  POWER_CTRL,
  POWER_CTRL_OUT,
  HCI_RESET,
  HCI_RESET_OUT,
  CHANGE_BAUDRATE,
  CHANGE_BAUDRATE_OUT,
  FW_DOWNLOAD,
  FW_DOWNLOAD_OUT,
  READ_LOCAL_NAME,
  READ_LOCAL_NAME_OUT,
  // For ONE_TIME_LOGGER.
  SERVICE_DIED = 46,
  BTHAL_THD_INIT,
  BTHAL_THD_REINIT,
  BTHAL_CLOSE,
  BTHAL_PERFORM_CLOSE,
  BIG_HAMMER,
  ACTIVITY_WATCHER_ERR,
  BT_REDARY,
  BTHAL_INIT_ERR,
  BT_PREPARE,
  BT_INIT,
  USERIAL_INFO,
  USERIAL_OPEN_ERR,
  USERIAL_TTY_OPEN,
  USERIAL_READY,
  HCI_SOCKET,
  POWER_STATE,
  BAUDRATE_ERR,
  LOCAL_NAME_ERR,
  FW_FAST_DNLD,
  FW_DNLD_ERR,
  FW_DNLD_SELECT,
  FW_DNLD_DONE,
  BT_CHIP_ID,
  BT_CMD_ERR,
  WAKELOCK_ERR,
  WAKELOCK_DUP,
  WAKELOCK_ACQUIRE,
  WAKELOCK_RELEASE,
  WAKELOCK_VOTE,
  WAKELOCK_UNVOTE,
  WATCHDOG,
  LPM_WAKEUP_ERR,
  LPM_WAKEUP,
  LPM_SUSPEND,
  LPM_WAKEUP_TIMEOUT,
  LPM_SETUP_ERR,
  LPM_ENABLE,
  LPM_DISABLE,
  LPM_CLOSE_ERR,
  BT_SHUTDOWN,
  BTHAL_USERIAL_TYPE_SELECT,
  H4_TX_ERR,
  H4_RX_ERR,
  H4_TX_CMD,
  H4_RX_EVT,
  BQR_ERR_MSG,
  HW_ERR_EVT,
  DEBUG_INFO,
  BTHAL_EXT_INJECT,
};

/*
 * ONE_TIME_LOGGER is used to record HAL log messages in any places in codes and
 * send to DebugCentral.
 * @deprecated
 */
#define ONE_TIME_LOGGER(type, fmt, ...)                \
  do {                                                 \
    char log[128] = {0};                               \
    std::snprintf(log, sizeof(log), fmt, __VA_ARGS__); \
    DebugCentral::Get()->UpdateRecord(type, log);      \
  } while (0)

/*
 * DURATION_TRACKER is used to log the Enter and Exit of a HAL function and
 * send to DebugCentral.
 */
#ifdef UNIT_TEST
#define DURATION_TRACKER(type, anchor)
#else
#define DURATION_TRACKER(type, anchor)    \
  ::bluetooth_hal::debug::DebugAnchor a = \
      ::bluetooth_hal::debug::DebugCentral::Get()->SetAnchor(type, anchor);
#endif

/*
 * ANCHOR_LOG* is used to log a message with a specific severity level
 * and send it to DebugCentral. It takes an anchor type as input.
 */
#define ANCHOR_LOG(type)                            \
  ([](auto&& logger) -> auto&& { return logger; })( \
      ::bluetooth_hal::debug::LogHelper(type, ::android::base::VERBOSE))
#define ANCHOR_LOG_DEBUG(type)                      \
  ([](auto&& logger) -> auto&& { return logger; })( \
      ::bluetooth_hal::debug::LogHelper(type, ::android::base::DEBUG))
#define ANCHOR_LOG_INFO(type)                       \
  ([](auto&& logger) -> auto&& { return logger; })( \
      ::bluetooth_hal::debug::LogHelper(type, ::android::base::INFO))
#define ANCHOR_LOG_WARNING(type)                    \
  ([](auto&& logger) -> auto&& { return logger; })( \
      ::bluetooth_hal::debug::LogHelper(type, ::android::base::WARNING))
#define ANCHOR_LOG_ERROR(type)                      \
  ([](auto&& logger) -> auto&& { return logger; })( \
      ::bluetooth_hal::debug::LogHelper(type, ::android::base::ERROR))

namespace bluetooth_hal {
namespace debug {

class DebugCentral;

class DebugAnchor {
 public:
  DebugAnchor(AnchorType type, const std::string& anchor,
              DebugCentral& debugcentral);

  // Manually release the auto debug anchor.
  ~DebugAnchor();

 private:
  DebugCentral* debugcentral_;
  std::string anchor_;
  AnchorType type_;
};

// BQR root inflammation vendor error codes
enum class BqrErrorCode : uint8_t {
  UART_PARSING = 0x01,
  UART_INCOMPLETE_PACKET = 0x02,
  FIRMWARE_CHECKSUM = 0x03,
  FIRMWARE_HARD_FAULT = 0x10,
  FIRMWARE_MEM_MANAGE_FAULT = 0x11,
  FIRMWARE_BUS_FAULT = 0x12,
  FIRMWARE_USAGE_FAULT = 0x13,
  FIRMWARE_WATCHDOG_TIMEOUT = 0x14,
  FIRMWARE_ASSERTION_FAILURE = 0x15,
  FIRMWARE_MISCELLANEOUS = 0x16,
  FIRMWARE_HOST_REQUEST_DUMP = 0x17,
  FIRMWARE_MISCELLANEOUS_MAJOR_FAULT = 0x20,
  FIRMWARE_MISCELLANEOUS_CRITICAL_FAULT = 0x21,
  FIRMWARE_THREAD_GENERIC_ERROR = 0x40,
  FIRMWARE_THREAD_INVALID_FRAME = 0x41,
  FIRMWARE_THREAD_INVALID_PARAM = 0x42,
  FIRMWARE_THREAD_UNSUPPORTED_FRAME = 0x43,
  SOC_BIG_HAMMER_FAULT = 0x7F,
  HOST_RX_THREAD_STUCK = 0x80,
  HOST_HCI_COMMAND_TIMEOUT = 0x81,
  HOST_INVALID_HCI_EVENT = 0x82,
  HOST_UNIMPLEMENTED_PACKET_TYPE = 0x83,
  HOST_HCI_H4_TX_ERROR = 0x84,
  HOST_OPEN_USERIAL = 0x90,
  HOST_POWER_UP_CONTROLLER = 0x91,
  HOST_CHANGE_BAUDRATE = 0x92,
  HOST_RESET_BEFORE_FW = 0x93,
  HOST_DOWNLOAD_FW = 0x94,
  HOST_RESET_AFTER_FW = 0x95,
  HOST_BDADDR_FAULT = 0x96,
  HOST_OPEN_COEX_DEVICE_ERROR = 0x97,
  HOST_ACCEL_BT_INIT_FAILED = 0x98,
  HOST_ACCEL_BT_SHUTDOWN_FAILED = 0x99,
  CHRE_ARBITRATOR_ERR_BASE = 0xE0,
  CHRE_ARBITRATOR_UNIMPLEMENTED_PACKET = 0xE0,
  CHRE_ARBITRATOR_INVALID_PACKET_SIZE = 0xE1,
};

class DebugCentral {
 public:
  /*
   * Get a singleton static instance of the debug central.
   */
  static DebugCentral* Get();

  /*
   * Get the stack callback function
   */
  void Prepare(::bluetooth_hal::hci::HalPacketCallback notify_cb) {
    notify_cb_ = notify_cb;
  }

#if 0
  /*
   * Start to monitor error event
   */
  void StartMonitor(hci::HciFlowControl* handle);

  /*
   * Stop to monitor error event
   */
  void StopMonitor();
#endif

  /*
   * Invokes when bugreport is triggered, dump all information to the debug fd.
   */
  void Dump(int fd);

  /*
   * Upadte if there is a client connect bthal.
   */
  void HasClientConnectWith(bool has_client);

  /*
   * set bluetooth serial port information.
   */
  void SetBtUartDebugPort(const std::string& uart_port);

  /*
   * Write debug message to logger.
   */
  void UpdateRecord(AnchorType type, const std::string& anchor);

  /*
   * Notify BtHal have detected error, we will collect debug log first then and
   * report eror code to stack via BQR root inflammation event
   */
  void ReportBqrError(BqrErrorCode error, std::string extra_info);

  /*
   * Detect if current HCI command is host get controller debug dump opcode ?
   */
  bool IsControllerDebugDumpOpcode(const ::bluetooth_hal::hci::HalPacket& data);

  /*
   * Two kinds of debug anchor are supported to collect log messages.
   * is_lifetime : true - we handle the debug anchor's life cycle,
   * add [ Set] message in the constructor and [Free] message in the destructor
   * to record the enter and exit timestamp of an invoked function.
   * Usually, we put this kind debug anchor in begin of function, the purpose
   * of this debug anchor usage is record time-consuming of a function.
   * [Example]: initialize_impl [ Set]: 13:36:15:133
   *            initialize_impl [Free]: 13:36:17:072
   * is_lifetime : false - we record the timestamp that debug anchor appeared in
   * code. Usually, we put this kind debug anchor at the exception occurred
   * place or record informative messages in a function. [Example]: Received
   * Hardware error event: 19:56:49:036 [Example]: firmware_download Done:
   * 13:36:17:024
   */
  DebugAnchor SetAnchor(AnchorType type, const std::string& anchor) {
    return DebugAnchor(type, anchor, instance_);
  }

 private:
  static constexpr int kMaxHistory = 400;
  static DebugCentral instance_;
  // Determine if we should hijack the vendor debug event or not
  bool hijack_event_ = false;
  bool has_client_ = false;
  ::bluetooth_hal::hci::HalPacketCallback notify_cb_;
  std::string serial_debug_port_;
  std::string crash_timestamp_;
  std::recursive_mutex mutex_;
  // std::vector<std::unique_ptr<hci::HciEventWatcher>> event_watchers_;
  std::queue<std::vector<uint8_t>> socdump_;
  std::queue<std::vector<uint8_t>> chredump_;
  // BtHal Logger
  std::list<std::pair<std::string, std::string>> history_record_;
  std::map<AnchorType, std::pair<std::string, std::string>> lasttime_record_;

  static void ForceGetCoredumpTimeout(union sigval sig);
  bool report_ssr_crash(uint8_t vendor_error_code);
  bool is_hw_stage_supported();
  void dump_hal_log(int fd);
  void handle_vendor_specific_event(
      const ::bluetooth_hal::hci::HalPacket& packet);
  void handle_bqr_fw_debug_data_dump(
      const ::bluetooth_hal::hci::HalPacket& packet);
  void handle_bqr_chre_debug_data_dump(
      const ::bluetooth_hal::hci::HalPacket& packet);
  void handle_debug_info_event(const ::bluetooth_hal::hci::HalPacket& packet);
  void handle_bqr_event(const ::bluetooth_hal::hci::HalPacket& packet);
  void start_crash_dump(bool slient_report, const std::string& reason);
};

class LogHelper {
 public:
  LogHelper(AnchorType type, ::android::base::LogSeverity severity)
      : type_(type), severity_(severity) {}

  template <typename T>
  LogHelper& operator<<(const T& value) {
    oss_ << value;
    return *this;
  }

  ~LogHelper() {
    std::string log_message = oss_.str();
    if (!log_message.empty()) {
#ifdef UNIT_TEST
      (void)type_;
#else
      DebugCentral::Get()->UpdateRecord(type_, log_message);
#endif
      LOG(severity_) << log_message;
    }
  }

 private:
  AnchorType type_;
  ::android::base::LogSeverity severity_;
  std::ostringstream oss_;
};

}  // namespace debug
}  // namespace bluetooth_hal
