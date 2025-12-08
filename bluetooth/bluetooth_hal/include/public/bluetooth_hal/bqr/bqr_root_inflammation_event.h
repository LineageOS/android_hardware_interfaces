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
#include <vector>

#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {

class BqrRootInflammationEvent : public BqrEvent {
 public:
  explicit BqrRootInflammationEvent(
      const ::bluetooth_hal::hci::HalPacket& packet);
  ~BqrRootInflammationEvent() = default;

  bool IsValid() const override;
  uint8_t GetErrorCode() const;
  uint8_t GetVendorErrorCode() const;
  std::vector<uint8_t> GetVendorParameter() const;
  std::string ToString() const;

 protected:
  std::string ToBqrString() const;
  bool is_valid_;
  uint8_t error_code_;
  uint8_t vendor_error_code_;
};

}  // namespace bqr
}  // namespace bluetooth_hal
