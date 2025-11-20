/*
 * Copyright 2023 The Android Open Source Project
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

#include "aidl/vendor/google/bluetooth_ext/BnBTChannelAvoidance.h"
#include "bluetooth_hal/hal_packet.h"
#include "legacy/hci_flow_control.h"

namespace vendor {
namespace google {
namespace bluetooth_ext {
namespace bt_channel_avoidance {
namespace aidl {
namespace implementation {

using ::aidl::vendor::google::bluetooth_ext::BnBTChannelAvoidance;
using ::bluetooth_hal::hci::HciEventWatcher;
using ::bluetooth_hal::hci::HciFlowControl;

struct BTChannelAvoidance : public BnBTChannelAvoidance,
                            public HciEventWatcher {
 public:
  BTChannelAvoidance();

  ndk::ScopedAStatus setBluetoothChannelStatus(
      const std::array<uint8_t, 10>& channel_map) override;
  bool OnEventReceive(const ::bluetooth_hal::hci::HalPacket& event) override;
  bool OnEventPost(const ::bluetooth_hal::hci::HalPacket& event) override;

  static void OnBluetoothEnabled(HciFlowControl* handle);
  static void OnBluetoothDisabled();

 private:
  static HciFlowControl* hci_handle_;
  static BTChannelAvoidance instance_;
  std::atomic_uint event_waiting_{0};
};

}  // namespace implementation
}  // namespace aidl
}  // namespace bt_channel_avoidance
}  // namespace bluetooth_ext
}  // namespace google
}  // namespace vendor
