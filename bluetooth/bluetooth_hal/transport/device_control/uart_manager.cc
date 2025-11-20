/*
 * Copyright 2025 The Android Open Source Project
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

#define LOG_TAG "bthal.device_control"

#include "bluetooth_hal/transport/device_control/uart_manager.h"

#include <sys/types.h>
#include <termios.h>

#include <chrono>
#include <string>
#include <string_view>
#include <thread>

#include "android-base/logging.h"
#include "android-base/unique_fd.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/util/system_call_wrapper.h"

namespace bluetooth_hal {
namespace transport {
namespace {

using ::android::base::unique_fd;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::debug::BqrErrorCode;
using ::bluetooth_hal::debug::DebugCentral;
using ::bluetooth_hal::uart::BaudRate;
using ::bluetooth_hal::util::SystemCallWrapper;

// TODO: b/391226112 - Move to property config manager.
constexpr std::chrono::milliseconds kUartStartupSettlementMs{50};
constexpr std::string_view kUartCtrlNode =
    "/sys/devices/platform/155d0000.serial/uart_dbg";

bool ConfigureUartPort(int fd) {
  termios tty_attrs = {};
  if (tcgetattr(fd, &tty_attrs) != 0) {
    LOG(ERROR) << "Failed to get UART attributes: " << strerror(errno);
    return false;
  }

  cfmakeraw(&tty_attrs);
  // Enable RTS/CTS (hardware flow control).
  tty_attrs.c_cflag |= CRTSCTS;

  // Set baud rate to 115200.
  if (cfsetspeed(&tty_attrs, B115200) != 0) {
    LOG(ERROR) << "Failed to set baud rate: " << strerror(errno);
    return false;
  }

  if (tcsetattr(fd, TCSANOW, &tty_attrs) != 0) {
    LOG(ERROR) << "Failed to set UART attributes: " << strerror(errno);
    return false;
  }

  // Flush input and output queues.
  if (tcflush(fd, TCIOFLUSH) != 0) {
    LOG(ERROR) << "Failed to flush UART port: " << strerror(errno);
    return false;
  }

  return true;
}

}  // namespace

bool UartManager::Open() {
  DURATION_TRACKER(AnchorType::USERIAL_OPEN, __func__);

  const std::string bt_uart_port =
      HalConfigLoader::GetLoader().GetBtUartDevicePort();

#ifndef UNIT_TEST
  DebugCentral::Get()->SetBtUartDebugPort(bt_uart_port);
#endif

  ANCHOR_LOG(AnchorType::USERIAL_TTY_OPEN)
      << __func__ << ": open " << bt_uart_port;

  uart_fd_.reset(
      SystemCallWrapper::GetWrapper().Open(bt_uart_port.c_str(), O_RDWR));
  if (!uart_fd_.ok()) {
#ifndef UNIT_TEST
    DebugCentral::Get()->ReportBqrError(BqrErrorCode::HOST_OPEN_USERIAL,
                                        "Host Open Port Error");
#endif
    return false;
  }

  if (!ConfigureUartPort(uart_fd_.get())) {
    LOG(ERROR) << __func__
               << ": Failed to configure UART port: " << strerror(errno) << " ("
               << errno << ").";
  }

  // Wait for the device to power cycle and stabilize.
  std::this_thread::sleep_for(kUartStartupSettlementMs);

  return true;
}

void UartManager::Close() {
  DURATION_TRACKER(AnchorType::USERIAL_CLOSE, __func__);
  uart_fd_.reset();
}

bool UartManager::SetUartSkipSuspend(bool skip_suspend) {
  LOG(INFO) << __func__ << ": Open UartCtrl device node.";

  unique_fd ctrl_fd(
      SystemCallWrapper::GetWrapper().Open(kUartCtrlNode.data(), O_WRONLY));
  if (!ctrl_fd.ok()) {
    LOG(WARNING) << __func__ << ": Unable to open UartCtrl port ("
                 << kUartCtrlNode << "): " << strerror(errno) << " (" << errno
                 << ").";
    return false;
  }

  char skip_suspend_cmd = skip_suspend ? '8' : '9';
  const ssize_t length =
      TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Write(
          ctrl_fd.get(), &skip_suspend_cmd, sizeof(skip_suspend_cmd)));
  if (length < 1) {
    LOG(ERROR) << __func__ << ": Unable to set uart IOCTRL:" << strerror(errno)
               << " (" << errno << ")";
    return false;
  }

  LOG(INFO) << __func__ << ": Is enabled: " << skip_suspend;

  return true;
}

void UartManager::UpdateBaudRate(BaudRate rate) const {
  speed_t kernel_rate;
  switch (rate) {
    case BaudRate::kRate115200:
      kernel_rate = B115200;
      break;
    case BaudRate::kRate3000000:
      kernel_rate = B3000000;
      break;
    case BaudRate::kRate4000000:
      kernel_rate = B4000000;
      break;
    default:
      LOG(WARNING) << __func__ << ": Baud rate (" << static_cast<int>(rate)
                   << ") unsupported";
      return;
  };

  termios tty_attrs;
  if (tcgetattr(uart_fd_.get(), &tty_attrs) != 0) {
    LOG(ERROR) << __func__ << ": Failed to get terminal attributes: "
               << std::strerror(errno);
    return;
  }

  cfmakeraw(&tty_attrs);
  cfsetspeed(&tty_attrs, kernel_rate);
  if (tcsetattr(uart_fd_.get(), TCSANOW, &tty_attrs) != 0) {
    LOG(ERROR) << __func__ << ": Failed to set terminal attributes: "
               << std::strerror(errno);
    return;
  }

  tcflush(uart_fd_, TCIOFLUSH);
}

int UartManager::GetFd() { return uart_fd_.get(); }

}  // namespace transport
}  // namespace bluetooth_hal
