/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android-base/macros.h>
#include <android/hardware/automotive/can/1.0/types.h>

namespace android::hardware::automotive::protocan {

/**
 * TODO(twasilczyk): right now, only Little Endian signals are supported.
 */
class Signal {
 public:
  using value = uint64_t;

  const value maxValue;

  Signal(uint16_t start, uint8_t length, value defVal = 0);

  value get(const can::V1_0::CanMessage& msg) const;
  void set(can::V1_0::CanMessage& msg, value val) const;
  void setDefault(can::V1_0::CanMessage& msg) const;

 private:
  const uint16_t kFirstByte;     ///< Index of first byte that holds the signal
  const uint8_t kFirstBit;       ///< Index of first bit within first byte
  const uint8_t kFirstByteBits;  ///< How many bits of the first byte belong to the signal
  const uint16_t kLastByte;      ///< Index of last byte that holds the signal
  const uint8_t kLastMask;       ///< Bits of the last byte that belong to the signal
  const uint8_t kFirstMask;      ///< Bits of the first byte that belong to the signal

  const value kDefVal;
};

}  // namespace android::hardware::automotive::protocan
