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

#include "bluetooth_hal/bqr/bqr_link_quality_event_v7.h"

#include <string>

#include "bluetooth_hal/bqr/bqr_link_quality_event_v6.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {

using ::bluetooth_hal::hci::HalPacket;

BqrLinkQualityEventV7::BqrLinkQualityEventV7(const HalPacket& packet)
    : BqrLinkQualityEventV6(packet) {
  is_valid_ = BqrLinkQualityEventV6::IsValid();
}

bool BqrLinkQualityEventV7::IsValid() const { return is_valid_; }

std::string BqrLinkQualityEventV7::ToString() const {
  if (!is_valid_) {
    return "BqrLinkQualityEventV7(Invalid)";
  }
  return "BqrLinkQualityEventV7: " + ToBqrString();
}

std::string BqrLinkQualityEventV7::ToBqrString() const {
  return BqrLinkQualityEventV6::ToBqrString();
}

}  // namespace bqr
}  // namespace bluetooth_hal
