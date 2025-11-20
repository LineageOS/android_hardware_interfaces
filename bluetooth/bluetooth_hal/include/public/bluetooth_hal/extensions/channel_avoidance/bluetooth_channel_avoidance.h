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

#include "aidl/hardware/google/bluetooth/bt_channel_avoidance/BnBTChannelAvoidance.h"
#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/channel_avoidance/bluetooth_channel_avoidance_handler.h"

namespace bluetooth_hal {
namespace extensions {
namespace channel_avoidance {

class BluetoothChannelAvoidance
    : public ::aidl::hardware::google::bluetooth::bt_channel_avoidance::
          BnBTChannelAvoidance {
 public:
  ::ndk::ScopedAStatus setBluetoothChannelStatus(
      const std::array<uint8_t, 10>& channel_map) override;

 private:
  BluetoothChannelAvoidanceHandler handler_;
};

}  // namespace channel_avoidance
}  // namespace extensions
}  // namespace bluetooth_hal
