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

#include "bluetooth_hal/bqr/bqr_link_quality_event_v1_to_v3.h"

#include <string>

#include "bluetooth_hal/bqr/bqr_link_quality_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {

using ::bluetooth_hal::hci::HalPacket;

BqrLinkQualityEventV1ToV3::BqrLinkQualityEventV1ToV3(const HalPacket& packet)
    : BqrLinkQualityEventBase(packet) {
  if (is_valid_) {
    version_ = BqrVersion::kV1ToV3;
  }
}

bool BqrLinkQualityEventV1ToV3::IsValid() const { return is_valid_; }

std::string BqrLinkQualityEventV1ToV3::ToString() const {
  if (!is_valid_) {
    return "BqrLinkQualityEventV1ToV3(Invalid)";
  }
  return "BqrLinkQualityEventV1ToV3: " + ToBqrString();
}

std::string BqrLinkQualityEventV1ToV3::ToBqrString() const {
  return BqrLinkQualityEventBase::ToBqrString();
}

}  // namespace bqr
}  // namespace bluetooth_hal
