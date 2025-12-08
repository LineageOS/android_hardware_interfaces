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

#include <string>

#include "bluetooth_hal/bqr/bqr_link_quality_event_v6.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {

class BqrLinkQualityEventV7 : public BqrLinkQualityEventV6 {
 public:
  explicit BqrLinkQualityEventV7(const ::bluetooth_hal::hci::HalPacket& packet);

  // Checks if the BQR Link Quality Event V6 is valid.
  bool IsValid() const override;

  // Returns a string representation of the event.
  std::string ToString() const;

 protected:
  std::string ToBqrString() const;
};

}  // namespace bqr
}  // namespace bluetooth_hal
