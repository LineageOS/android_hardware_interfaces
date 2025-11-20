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

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace bluetooth_hal {
namespace hci {

inline constexpr int kBluetoothAddressLength = 6;
inline constexpr int kBluetoothAddressHiddenBytes = 4;

class BluetoothAddress : public std::array<uint8_t, kBluetoothAddressLength> {
 public:
  std::string ToString() const {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < kBluetoothAddressLength; ++i) {
      if (i < kBluetoothAddressHiddenBytes) {
        ss << "XX";
      } else {
        ss << std::setw(2) << static_cast<int>((*this)[i]);
      }
      if (i < kBluetoothAddressLength - 1) {
        ss << ":";
      }
    }
    return ss.str();
  }

  std::string ToFullString() const {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < kBluetoothAddressLength; ++i) {
      ss << std::setw(2) << static_cast<int>((*this)[i]);
      if (i < kBluetoothAddressLength - 1) {
        ss << ":";
      }
    }
    return ss.str();
  }
};

}  // namespace hci
}  // namespace bluetooth_hal
