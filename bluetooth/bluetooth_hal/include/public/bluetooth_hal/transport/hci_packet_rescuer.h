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
#include <memory>
#include <span>

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/vendor_packet_validator_interface.h"

namespace bluetooth_hal {
namespace transport {

class HciPacketRescuer {
 public:
  HciPacketRescuer();
  ~HciPacketRescuer() = default;

  /**
   * @brief Scans a byte stream to find the offset of the first valid HCI
   * packet.
   *
   * Iterates through the data from the beginning, using `IsValidHciPacket` to
   * check each potential starting byte.
   *
   * @param data A span representing the raw byte stream to be scanned.
   * @return The byte offset of the first valid packet start. Returns the size
   * of the input `data` span if no valid packet start is found.
   */
  size_t FindValidPacketOffset(std::span<const uint8_t> data);

 private:
  bool VerifyEventCodeAndItsParamLength(
      std::span<const uint8_t> data,
      ::bluetooth_hal::hci::EventCode event_code);
  bool IsProbablyValidAclPacket(std::span<const uint8_t> data);
  bool IsProbablyValidThreadPacket(std::span<const uint8_t> data);
  bool IsValidHciPacket(std::span<const uint8_t> data);

  std::unique_ptr<VendorPacketValidatorInterface> vendor_packet_validator_;
};

}  // namespace transport
}  // namespace bluetooth_hal
