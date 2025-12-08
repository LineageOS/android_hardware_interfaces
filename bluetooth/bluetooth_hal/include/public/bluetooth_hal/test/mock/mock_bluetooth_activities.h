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

#include <cstddef>
#include <cstdint>

#include "bluetooth_hal/debug/bluetooth_activities.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace debug {

class MockBluetoothActivities : public BluetoothActivities {
 public:
  MOCK_METHOD(bool, HasConnectedDevice, (), (const));
  MOCK_METHOD(bool, IsConnected, (uint16_t connection_handle), (const));
  MOCK_METHOD(size_t, GetConnectionHandleCount, (), (const));
  MOCK_METHOD(void, OnMonitorPacketCallback,
              (::bluetooth_hal::hci::MonitorMode mode,
               const ::bluetooth_hal::hci::HalPacket& packet),
              ());
  MOCK_METHOD(void, OnBluetoothChipClosed, (), ());

  static void SetMockBluetoothActivities(MockBluetoothActivities* mock);

  static inline MockBluetoothActivities* mock_bluetooth_activities_{nullptr};
};

}  // namespace debug
}  // namespace bluetooth_hal
