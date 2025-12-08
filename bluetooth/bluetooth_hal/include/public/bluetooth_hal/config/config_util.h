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

#include <string>

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/android_base_wrapper.h"

namespace bluetooth_hal {
namespace config {

inline void EnableTransportFallback() {
  // Disable Accelerate Bluetooth On to re-initialize the UART.
  ::bluetooth_hal::util::AndroidBaseWrapper::GetWrapper().SetProperty(
      ::bluetooth_hal::Property::kIsAcceleratedBtOnEnabled, "false");
  ::bluetooth_hal::util::AndroidBaseWrapper::GetWrapper().SetProperty(
      ::bluetooth_hal::Property::kTransportFallbackEnabled, "true");
}

}  // namespace config
}  // namespace bluetooth_hal
