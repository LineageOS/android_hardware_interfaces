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

#include <cstdint>
#include <span>

#include "bluetooth_hal/transport/vendor_packet_validator_interface.h"

namespace bluetooth_hal {
namespace transport {

class VendorPacketValidator
    : public bluetooth_hal::transport::VendorPacketValidatorInterface {
 public:
  VendorPacketValidator() = default;
  ~VendorPacketValidator() = default;

  /**
   * Validates a vendor-specific event.
   *
   * This is the default implementation which always returns false. Vendors
   * should override this with their own implementation for event validation.
   *
   * @param data The vendor-specific event to validate.
   * @return `true` if the event is valid, `false` otherwise. This default
   *         implementation always returns `false`.
   */
  bool IsValidVendorSpecificEvent(std::span<const uint8_t> data) const override;
};

}  // namespace transport
}  // namespace bluetooth_hal
