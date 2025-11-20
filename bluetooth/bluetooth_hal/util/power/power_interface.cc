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

#include "bluetooth_hal/util/power/power_interface.h"

#include "hardware_legacy/power.h"

namespace bluetooth_hal {
namespace util {
namespace power {

constexpr char kWakeLockName[] = "bthal_wakelock";

bool PowerInterface::AcquireWakelock() {
  return acquire_wake_lock(PARTIAL_WAKE_LOCK, kWakeLockName) == 0;
}

bool PowerInterface::ReleaseWakelock() {
  return release_wake_lock(kWakeLockName) == 0;
}

PowerInterface& PowerInterface::GetInterface() {
  static PowerInterface interface;
  return interface;
}

}  // namespace power
}  // namespace util
}  // namespace bluetooth_hal
