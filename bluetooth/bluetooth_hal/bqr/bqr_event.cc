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

#include "bluetooth_hal/bqr/bqr_event.h"

#include <cstddef>
#include <string>

#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::GoogleEventSubCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;

enum class Offset : uint8_t {
  // The 3 bytes are for the HCI event header
  // H4 packet type(1) + event code(1) + length(1)
  kSubEvent = 3,
  kReportId,
};

// H4 packet type(1) + event code(1) + length(1) + sub event(1) + report id(1)
constexpr size_t kBqrEventHeaderLength = 5;
}  // namespace

BqrEvent::BqrEvent(const HalPacket& packet)
    : HalPacket(packet),
      is_valid_(size() >= kBqrEventHeaderLength &&
                GetType() == HciPacketType::kEvent && IsVendorEvent() &&
                At(Offset::kSubEvent) ==
                    static_cast<uint8_t>(GoogleEventSubCode::kBqrEvent)),
      report_id_(BqrReportId::kNone),
      bqr_event_type_(BqrEventType::kNone) {
  if (is_valid_) {
    ParseData();
  }
}

bool BqrEvent::IsValid() const { return is_valid_; }

BqrReportId BqrEvent::GetBqrReportId() const { return report_id_; }

BqrEventType BqrEvent::GetBqrEventType() const { return bqr_event_type_; }

void BqrEvent::ParseData() {
  report_id_ = static_cast<BqrReportId>(At(Offset::kReportId));
  bqr_event_type_ = GetBqrEventTypeFromReportId(report_id_);
}

std::string BqrEvent::ToString() const {
  if (!is_valid_) {
    return "BqrEvent(Invalid)";
  }
  return "BqrEvent: " + ToBqrString();
}

std::string BqrEvent::ToBqrString() const {
  return BqrReportIdToString(report_id_);
}

}  // namespace bqr
}  // namespace bluetooth_hal
