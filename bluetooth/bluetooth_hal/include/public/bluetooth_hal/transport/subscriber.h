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
namespace transport {

class Subscriber {
 public:
  virtual ~Subscriber() = default;

  bool operator==(const Subscriber& other) const { return this == &other; }

  /**
   * @brief Notifies the subscriber about a change in the HAL state.
   *
   * This function provides a mechanism for the subscriber to receive
   * notifications about changes in the Bluetooth HAL state.
   *
   * @param hal_state The new state of the Bluetooth HAL.
   *
   */
  virtual void NotifyHalStateChange(::bluetooth_hal::HalState hal_state) = 0;
};

}  // namespace transport
}  // namespace bluetooth_hal
