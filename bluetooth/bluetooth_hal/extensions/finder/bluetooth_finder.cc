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

#include "bluetooth_hal/extensions/finder/bluetooth_finder.h"

#include <vector>

#include "aidl/android/hardware/bluetooth/finder/Eid.h"
#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/finder/bluetooth_finder_handler.h"

namespace bluetooth_hal {
namespace extensions {
namespace finder {

using ::aidl::android::hardware::bluetooth::finder::Eid;

using ::ndk::ScopedAStatus;

BluetoothFinder::BluetoothFinder()
    : handler_(BluetoothFinderHandler::GetHandler()) {}

ScopedAStatus BluetoothFinder::sendEids(const std::vector<Eid>& eids) {
  bool status = handler_.SendEids(eids);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothFinder::setPoweredOffFinderMode(bool enable) {
  bool status = handler_.SetPoweredOffFinderMode(enable);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothFinder::getPoweredOffFinderMode(bool* _aidl_return) {
  bool status = handler_.GetPoweredOffFinderMode(_aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

}  // namespace finder
}  // namespace extensions
}  // namespace bluetooth_hal
