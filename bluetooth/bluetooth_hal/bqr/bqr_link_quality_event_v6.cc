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

#include "bluetooth_hal/bqr/bqr_link_quality_event_v6.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v5.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kLinkQualityEventV6MinSize =
    static_cast<size_t>(LinkQualityOffsetV6::kEnd);

}  // namespace

BqrLinkQualityEventV6::BqrLinkQualityEventV6(const HalPacket& packet)
    : BqrLinkQualityEventV5(packet),
      rx_unreceived_packets_(0),
      coex_info_mask_(0) {
  is_valid_ =
      BqrLinkQualityEventV5::IsValid() && size() >= kLinkQualityEventV6MinSize;
  ParseData();
}

void BqrLinkQualityEventV6::ParseData() {
  if (is_valid_) {
    version_ = BqrVersion::kV6;
    rx_unreceived_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV6::kRxUnreceivedPackets);
    coex_info_mask_ = AtUint16LittleEndian(LinkQualityOffsetV6::kCoexInfoMask);
  }
}

bool BqrLinkQualityEventV6::IsValid() const { return is_valid_; }

uint32_t BqrLinkQualityEventV6::GetRxUnreceivedPackets() const {
  return rx_unreceived_packets_;
}

uint16_t BqrLinkQualityEventV6::GetCoexInfoMask() const {
  return coex_info_mask_;
}

std::string BqrLinkQualityEventV6::ToString() const {
  if (!is_valid_) {
    return "BqrLinkQualityEventV6(Invalid)";
  }
  return "BqrLinkQualityEventV6: " + ToBqrString();
}

std::string BqrLinkQualityEventV6::ToBqrString() const {
  std::stringstream ss;
  ss << BqrLinkQualityEventV5::ToBqrString() << ", RxUnreceived: " << std::dec
     << rx_unreceived_packets_ << ", CoexInfoMask: 0x" << std::hex
     << std::setw(4) << std::setfill('0') << coex_info_mask_;
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
