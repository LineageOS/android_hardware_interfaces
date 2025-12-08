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

#include <memory>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace hci {

// A mock implementation of the HciRouter class for testing purposes.
class MockHciRouter : public HciRouter {
 public:
  static void SetMockRouter(MockHciRouter* mock_hci_router);

  MOCK_METHOD(bool, Initialize,
              (const std::shared_ptr<HciRouterCallback>& callback), (override));

  MOCK_METHOD(void, Close, (), (override));

  MOCK_METHOD(void, Cleanup, (), (override));

  MOCK_METHOD(bool, Send, (const HalPacket& packet), (override));

  MOCK_METHOD(bool, SendCommand,
              (const HalPacket& packet, const HalPacketCallback& callback),
              (override));

  MOCK_METHOD(bool, SendCommandNoAck, (const HalPacket& packet), (override));

  MOCK_METHOD(::bluetooth_hal::HalState, GetHalState, (), (override));

  MOCK_METHOD(void, UpdateHalState, (::bluetooth_hal::HalState state),
              (override));

  MOCK_METHOD(void, SendPacketToStack, (const HalPacket& packet), (override));

  static inline MockHciRouter* mock_hci_router_{nullptr};
};

}  // namespace hci
}  // namespace bluetooth_hal
