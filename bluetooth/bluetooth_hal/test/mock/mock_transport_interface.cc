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

#include "bluetooth_hal/test/mock/mock_transport_interface.h"

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/transport_interface.h"

namespace bluetooth_hal {
namespace transport {

using ::bluetooth_hal::HalState;

static MockTransportInterface* mock_transport_interface = nullptr;

TransportInterface& TransportInterface::GetTransport() {
  return *mock_transport_interface;
}

bool TransportInterface::UpdateTransportType(TransportType requested_type) {
  return mock_transport_interface->UpdateTransportType(requested_type);
}

TransportType TransportInterface::GetTransportType() {
  return mock_transport_interface->GetTransportType();
}

bool TransportInterface::RegisterVendorTransport(
    std::unique_ptr<TransportInterface> transport) {
  return mock_transport_interface->RegisterVendorTransport(
      std::move(transport));
}

bool TransportInterface::UnregisterVendorTransport(TransportType type) {
  return mock_transport_interface->UnregisterVendorTransport(type);
}

void TransportInterface::SetHciRouterBusy(bool is_busy) {
  mock_transport_interface->SetHciRouterBusy(is_busy);
}

void TransportInterface::NotifyHalStateChange(HalState hal_state) {
  mock_transport_interface->NotifyHalStateChange(hal_state);
}

void TransportInterface::Subscribe(Subscriber& subscriber) {
  mock_transport_interface->Subscribe(subscriber);
}

void TransportInterface::Unsubscribe(Subscriber& subscriber) {
  mock_transport_interface->Unsubscribe(subscriber);
}

void MockTransportInterface::SetMockTransport(
    MockTransportInterface* transport) {
  mock_transport_interface = transport;
}

}  // namespace transport
}  // namespace bluetooth_hal
