/*
 * Copyright 2024 The Android Open Source Project
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

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_client_agent.h"
#include "bluetooth_hal/hci_router_client_callback.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace hci {

// A mock implementation of the HciRouterClientAgent class for testing purposes.
class MockHciRouterClientAgent : public HciRouterClientAgent {
 public:
  static void SetMockAgent(MockHciRouterClientAgent* mock_agent);

  MOCK_METHOD(bool, RegisterClient, (HciRouterClientCallback * callback),
              (override));
  MOCK_METHOD(bool, UnregisterClient, (HciRouterClientCallback * callback),
              (override));
  MOCK_METHOD(MonitorMode, DispatchPacketToClients, (const HalPacket& packet),
              (override));
  MOCK_METHOD(void, NotifyHalStateChange,
              (HalState new_state, HalState old_state), (override));
  MOCK_METHOD(bool, IsBluetoothEnabled, (), (override));
  MOCK_METHOD(bool, IsBluetoothChipReady, (), (override));

  static inline MockHciRouterClientAgent* mock_agent_{nullptr};
};

}  // namespace hci
}  // namespace bluetooth_hal
