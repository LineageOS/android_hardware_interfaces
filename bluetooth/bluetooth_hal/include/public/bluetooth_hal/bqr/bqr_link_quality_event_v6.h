/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>
#include <string>

#include "bluetooth_hal/bqr/bqr_link_quality_event_v5.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {

enum class LinkQualityOffsetV6 : uint8_t {
  kRxUnreceivedPackets =
      static_cast<uint8_t>(LinkQualityOffsetV5::kEnd),  // 4 bytes
  kCoexInfoMask = 87,                                   // 2 bytes
  kEnd = 89,
};

class BqrLinkQualityEventV6 : public BqrLinkQualityEventV5 {
 public:
  explicit BqrLinkQualityEventV6(const ::bluetooth_hal::hci::HalPacket& packet);
  ~BqrLinkQualityEventV6() = default;

  // Checks if the BQR Link Quality Event V6 is valid.
  bool IsValid() const override;

  // Retrieves the number of unreceived packets, same as LE Read ISO Link
  // Quality command.
  uint32_t GetRxUnreceivedPackets() const;
  // Retrieves the coex activities information mask.
  uint16_t GetCoexInfoMask() const;

  // Returns a string representation of the event.
  std::string ToString() const;

 protected:
  void ParseData();
  std::string ToBqrString() const;

  uint32_t rx_unreceived_packets_;
  uint16_t coex_info_mask_;
};

}  // namespace bqr
}  // namespace bluetooth_hal
