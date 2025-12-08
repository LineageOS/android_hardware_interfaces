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

#include "bluetooth_hal/transport/vendor_packet_validator_interface.h"

#include <memory>

#include "bluetooth_hal/transport/vendor_packet_validator.h"

namespace bluetooth_hal {
namespace transport {

VendorPacketValidatorInterface::FactoryFn
    VendorPacketValidatorInterface::vendor_factory_ = nullptr;

std::unique_ptr<VendorPacketValidatorInterface>
VendorPacketValidatorInterface::Create() {
  if (vendor_factory_) {
    return vendor_factory_();
  }
  return std::make_unique<VendorPacketValidator>();
}

void VendorPacketValidatorInterface::RegisterVendorPacketValidator(
    FactoryFn factory) {
  vendor_factory_ = std::move(factory);
}

}  // namespace transport
}  // namespace bluetooth_hal
