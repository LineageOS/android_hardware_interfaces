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

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "aidl/hardware/google/bluetooth/ccc/BnBluetoothCcc.h"
#include "aidl/hardware/google/bluetooth/ccc/IBluetoothCccCallback.h"
#include "android/binder_auto_utils.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {

class BluetoothCcc
    : public ::aidl::hardware::google::bluetooth::ccc::BnBluetoothCcc {
 public:
  BluetoothCcc() = default;

  ::ndk::ScopedAStatus registerForLmpEvents(
      const std::shared_ptr<
          ::aidl::hardware::google::bluetooth::ccc::IBluetoothCccCallback>&
          callback,
      const std::array<uint8_t, 6>& address,
      const std::vector<::aidl::hardware::google::bluetooth::ccc::LmpEventId>&
          lmpEventIds) override;

  ::ndk::ScopedAStatus unregisterLmpEvents(
      const std::array<uint8_t, 6>& address) override;
};

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
