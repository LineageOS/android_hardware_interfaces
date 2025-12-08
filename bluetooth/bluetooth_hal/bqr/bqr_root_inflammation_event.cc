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

#include "bluetooth_hal/bqr/bqr_root_inflammation_event.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

enum class RootInflammationOffset : uint8_t {
  // After H4 type(1) + event code(1) + length(1) + sub event(1) + report id(1)
  kErrorCode = 5,        // 1 byte
  kVendorErrorCode = 6,  // 1 bytes
  kVendorParameter = 7,  // (optional) X bytes
  kEnd = kVendorParameter,
};

constexpr size_t kRootInflammationEventMinSize =
    static_cast<size_t>(RootInflammationOffset::kEnd);

}  // namespace

BqrRootInflammationEvent::BqrRootInflammationEvent(const HalPacket& packet)
    : BqrEvent(packet),
      is_valid_(BqrEvent::IsValid() &&
                GetBqrEventType() == BqrEventType::kRootInflammation &&
                size() >= kRootInflammationEventMinSize),
      error_code_(0),
      vendor_error_code_(0) {
  if (is_valid_) {
    error_code_ = At(RootInflammationOffset::kErrorCode);
    vendor_error_code_ = At(RootInflammationOffset::kVendorErrorCode);
  }
}

bool BqrRootInflammationEvent::IsValid() const { return is_valid_; }

uint8_t BqrRootInflammationEvent::GetErrorCode() const { return error_code_; }

uint8_t BqrRootInflammationEvent::GetVendorErrorCode() const {
  return vendor_error_code_;
}

std::vector<uint8_t> BqrRootInflammationEvent::GetVendorParameter() const {
  std::vector<uint8_t> vendor_parameter;
  if (size() > kRootInflammationEventMinSize) {
    vendor_parameter.assign(
        begin() + static_cast<size_t>(RootInflammationOffset::kVendorParameter),
        end());
  }
  return vendor_parameter;
}

std::string BqrRootInflammationEvent::ToString() const {
  if (!is_valid_) {
    return "BqrRootInflammationEvent(Invalid)";
  }
  return "BqrRootInflammationEvent: " + ToBqrString();
}

std::string BqrRootInflammationEvent::ToBqrString() const {
  std::stringstream ss;
  auto vendor_param_size = GetVendorParameter().size();

  ss << BqrEvent::ToBqrString() << ", Error Code:0x" << std::hex << std::setw(2)
     << std::setfill('0') << static_cast<int>(error_code_)
     << ", Vendor Error Code:0x" << std::hex << std::setw(2)
     << std::setfill('0') << static_cast<int>(vendor_error_code_)
     << ", Vendor Parameter Size:" << std::dec << vendor_param_size;

  return ss.str();
}
}  // namespace bqr
}  // namespace bluetooth_hal
