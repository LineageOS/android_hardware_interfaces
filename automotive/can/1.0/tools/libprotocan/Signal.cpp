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

#include <libprotocan/Signal.h>

#include <android-base/logging.h>

namespace android::hardware::automotive::protocan {

static uint8_t calculateLastByteMask(uint16_t start, uint8_t length) {
  unsigned lastByteBits = (start + length) % 8;
  unsigned lastBytePadding = (8 - lastByteBits) % 8;
  return 0xFF >> lastBytePadding;
}

static uint8_t calculateFirstByteMask(uint16_t firstByte, uint8_t firstBit, uint16_t lastByte,
                                      uint8_t lastMask) {
  uint8_t firstMask = 0xFF << firstBit;
  if (firstByte == lastByte) firstMask &= lastMask;
  return firstMask;
}

Signal::Signal(uint16_t start, uint8_t length, value defVal)
    : maxValue((1u << length) - 1),
      kFirstByte(start / 8),
      kFirstBit(start % 8),
      kFirstByteBits(8 - kFirstBit),
      kLastByte((start + length - 1) / 8),
      kLastMask(calculateLastByteMask(start, length)),
      kFirstMask(calculateFirstByteMask(kFirstByte, kFirstBit, kLastByte, kLastMask)),
      kDefVal(defVal) {
  CHECK(length > 0) << "Signal length must not be zero";
}

Signal::value Signal::get(const can::V1_0::CanMessage& msg) const {
    CHECK(msg.payload.size() > kLastByte)
            << "Message is too short. Did you call MessageDef::validate?";

    Signal::value v = 0;
    if (kLastByte != kFirstByte) v = kLastMask & msg.payload[kLastByte];

    for (int i = kLastByte - 1; i > kFirstByte; i--) {
        v = (v << 8) | msg.payload[i];
    }

    return (v << kFirstByteBits) | ((msg.payload[kFirstByte] & kFirstMask) >> kFirstBit);
}

void Signal::set(can::V1_0::CanMessage& msg, Signal::value val) const {
  CHECK(msg.payload.size() > kLastByte)
      << "Signal requires message of length " << (kLastByte + 1)
      << " which is beyond message length of " << msg.payload.size();

  uint8_t firstByte = val << kFirstBit;
  val >>= kFirstByteBits;

  msg.payload[kFirstByte] = (msg.payload[kFirstByte] & ~kFirstMask) | (firstByte & kFirstMask);

  for (int i = kFirstByte + 1; i < kLastByte; i++) {
    msg.payload[i] = val & 0xFF;
    val >>= 8;
  }

  if (kLastByte != kFirstByte) {
    msg.payload[kLastByte] = (msg.payload[kLastByte] & ~kLastMask) | (val & kLastMask);
  }
}

void Signal::setDefault(can::V1_0::CanMessage& msg) const { set(msg, kDefVal); }

}  // namespace android::hardware::automotive::protocan
