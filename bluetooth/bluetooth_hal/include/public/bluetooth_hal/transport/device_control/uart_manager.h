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

#pragma once

#include "android-base/unique_fd.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace transport {

/**
 * @brief The UartManager provides a default implementation that users, such as
 * transport instances, can inherit. They can then use this default
 * implementation or override it with their own proprietary implementation.
 *
 */
class UartManager {
 public:
  virtual ~UartManager() = default;

  /**
   * @brief Opens the UART port.
   *
   * This function opens the UART port for communication.
   *
   * @return True if the port was opened successfully, false otherwise.
   *
   */
  virtual bool Open();

  /**
   * @brief Closes the UART port.
   *
   * This function closes the UART port.
   *
   */
  virtual void Close();

  /**
   * @brief Controls whether the UART should skip suspend.
   *
   * This function configures the UART to either skip or enter suspend mode.
   *
   * @param skip_suspend True to skip suspend, false to enter suspend.
   *
   * @return True if the operation was successful, false otherwise.
   *
   */
  virtual bool SetUartSkipSuspend(bool skip_suspend);

  /**
   * @brief Change the baud rate of the UART port.
   *
   *
   * @param rate The new baud rate for the UART port.
   *
   * @return True if the baud rate was successfully changed, false otherwise.
   *
   */
  virtual void UpdateBaudRate(::bluetooth_hal::uart::BaudRate rate) const;

  /**
   * @brief Gets the file descriptor associated with the UART port.
   *
   * This function returns the file descriptor that can be used for
   * low-level I/O operations on the UART port.
   *
   * @return The file descriptor.
   *
   */
  virtual int GetFd();

 protected:
  ::android::base::unique_fd uart_fd_{-1};
};

}  // namespace transport
}  // namespace bluetooth_hal
