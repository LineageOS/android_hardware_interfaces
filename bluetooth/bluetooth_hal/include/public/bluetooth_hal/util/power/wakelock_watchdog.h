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

#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace util {
namespace power {

class WakelockWatchdog {
 public:
  /**
   * @brief Start a watchdog timer for the WakeSource.
   *
   * @param source The source of the requester.
   *
   */
  virtual void Start(WakeSource source) = 0;

  /**
   * @brief Stop the watchdog timer for a WakeSource.
   *
   * @param source The source of the requester.
   *
   */
  virtual void Stop(WakeSource source) = 0;

  /**
   * @brief Pause all wakelock watchdog from barking or biting. This is used
   * when the HAL is handling error and do not want the watchdog interrupts the
   * process.
   */
  virtual void Pause() = 0;

  /**
   * @brief Resume the watchdog to bark or bite from a Pause.
   */
  virtual void Resume() = 0;

  static WakelockWatchdog& GetWatchdog();

 protected:
  virtual ~WakelockWatchdog() = default;
};

}  // namespace power
}  // namespace util
}  // namespace bluetooth_hal
