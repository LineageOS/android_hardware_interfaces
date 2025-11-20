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

#include <cstdint>
#include <memory>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/subscriber.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace transport {

class MockTransportInterface : public TransportInterface {
 public:
  MOCK_METHOD(bool, UpdateTransportType, (TransportType requested_type), ());

  MOCK_METHOD(TransportType, GetTransportType, (), ());

  MOCK_METHOD(bool, RegisterVendorTransport,
              (std::unique_ptr<TransportInterface> transport), ());

  MOCK_METHOD(bool, UnregisterVendorTransport, (TransportType type), ());

  MOCK_METHOD(void, SetHciRouterBusy, (bool is_busy), ());

  MOCK_METHOD(void, NotifyHalStateChange, (::bluetooth_hal::HalState hal_state),
              ());

  MOCK_METHOD(void, Subscribe, (Subscriber & subscriber), ());

  MOCK_METHOD(void, Unsubscribe, (Subscriber & subscriber), ());

  MOCK_METHOD(bool, Initialize,
              (TransportInterfaceCallback * transport_interface_callback),
              (override));

  MOCK_METHOD(void, Cleanup, (), (override));

  MOCK_METHOD(bool, IsTransportActive, (), (const, override));

  MOCK_METHOD(bool, Send, (const ::bluetooth_hal::hci::HalPacket& packet),
              (override));

  MOCK_METHOD(TransportType, GetInstanceTransportType, (), (const, override));

  static void SetMockTransport(MockTransportInterface* transport);
};
}  // namespace transport
}  // namespace bluetooth_hal
