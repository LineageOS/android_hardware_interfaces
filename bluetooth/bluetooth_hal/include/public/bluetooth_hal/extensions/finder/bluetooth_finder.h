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

#include <vector>

#include "aidl/android/hardware/bluetooth/finder/BnBluetoothFinder.h"
#include "aidl/android/hardware/bluetooth/finder/Eid.h"
#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/finder/bluetooth_finder_handler.h"

namespace bluetooth_hal {
namespace extensions {
namespace finder {

class BluetoothFinder
    : public ::aidl::android::hardware::bluetooth::finder::BnBluetoothFinder {
 public:
  BluetoothFinder();

  ::ndk::ScopedAStatus sendEids(
      const std::vector<::aidl::android::hardware::bluetooth::finder::Eid>&
          eids) override;

  ::ndk::ScopedAStatus setPoweredOffFinderMode(bool enable) override;

  ::ndk::ScopedAStatus getPoweredOffFinderMode(bool* _aidl_return) override;

 private:
  BluetoothFinderHandler& handler_;
};

}  // namespace finder
}  // namespace extensions
}  // namespace bluetooth_hal
