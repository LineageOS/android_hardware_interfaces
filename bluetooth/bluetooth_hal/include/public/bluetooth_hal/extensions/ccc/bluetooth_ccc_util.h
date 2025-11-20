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

#include <cstdint>

namespace bluetooth_hal {
namespace extensions {
namespace ccc {

enum class CccDirection : uint8_t {
  kTx = 0x00,
  kRx = 0x01,
  kMax = 0x01,
  kUndefined = 0xFF,
};

struct CccTimestamp {
  /**
   * Timestamp in microsecond since system boot.
   */
  long system_time;
  /**
   * Timestamp in microsecond since Bluetooth controller power up.
   */
  long bluetooth_time;
};

enum class CccLmpEventId : uint8_t {
  kConnectInd = 0x00,
  kLlPhyUpdateInd = 0x01,
  kMax,
  kUndefined = 0xFF,
};

enum class CccLmpEventIdByte : uint8_t {
  kConnectInd = 0xFF,
  kLlPhyUpdateInd = 0x18,
  kUndefined = 0x00,
};

// Define constants for the event offsets
enum class TimesyncEventOffset : uint8_t {
  kSubEventCode = 3,
  kAddress = 4,
  kAddressType = 10,
  kDirection = 11,
  kTimestamp = 12,
  kEventId = 20,
  kToggleCount = 21,
  kTimesyncOffset = 22,
  kEventCount = 24,
};

enum class AddressType : uint8_t {
  kPublic = 0x00,
  kRandom = 0x01,
};

enum class TimesyncCommandType : uint8_t {
  kUndefined = 0x00,
  kAdd,
  kRemove,
  kClear,
};

class TimesyncConstants {
 public:
  static constexpr int kEventLength = 26;
  static constexpr int kEventTimestampLength = 8;
  static constexpr int kCommandCommandTypeLength = 1;
  static constexpr int kCommandAddressTypeLength = 1;
  static constexpr int kCommandDirectionLength = 1;
  static constexpr uint8_t kSubEventCode = 0xD0;
  static constexpr uint16_t kCommandOpCode = 0xFD63;
  static constexpr int kUint64MaxDigitInDec = 20;
};

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
