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

#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_timesync_event.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {

using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;

BluetoothCccTimesyncEvent::BluetoothCccTimesyncEvent(const HalPacket& packet)
    : HalPacket(packet) {
  // Format of the CCC Time Sync event:
  //    [PacketType:1][EventCode:1][Length:1][SubEventCode:1][Address:6][AddressType:1][Direction:1]
  //    [Timestamp:8][EventId:1][ToggleCount:1][TimesyncOffset:2][EventCount:2]

  // Check length, and the bytes in the header (4 octets).
  is_valid_ = (size() == TimesyncConstants::kEventLength &&
               GetType() == HciPacketType::kEvent && IsVendorEvent() &&
               At(TimesyncEventOffset::kSubEventCode) ==
                   TimesyncConstants::kSubEventCode);

  if (!is_valid_) {
    return;
  }

  // Parse BluetoothAddress (6 octets, little-endian)
  std::reverse_copy(
      begin() + static_cast<uint8_t>(TimesyncEventOffset::kAddress),
      begin() + static_cast<uint8_t>(TimesyncEventOffset::kAddress) +
          address_.size(),
      address_.begin());

  // Parse address type (1 octet)
  address_type_ = At(TimesyncEventOffset::kAddressType);

  // Parse direction (1 octet)
  direction_ = At(TimesyncEventOffset::kDirection);

  // Parse Timestamp (8 octets, little-endian)
  timestamp_ = AtUint64LittleEndian(TimesyncEventOffset::kTimestamp);

  // Parse event ID (1 octet)
  event_id_ = At(TimesyncEventOffset::kEventId);

  // Parse toggle count (1 octet)
  toggle_count_ = At(TimesyncEventOffset::kToggleCount);

  // Parse Timesync Offset (2 octets, little-endian)
  timesync_offset_ = AtUint16LittleEndian(TimesyncEventOffset::kTimesyncOffset);

  // Parse Event Count (2 octets, little-endian)
  event_count_ = AtUint16LittleEndian(TimesyncEventOffset::kEventCount);
}

bool BluetoothCccTimesyncEvent::IsValid() { return is_valid_; }

BluetoothAddress BluetoothCccTimesyncEvent::GetAddress() const {
  return address_;
}

uint8_t BluetoothCccTimesyncEvent::GetAddressType() const {
  return address_type_;
}

CccDirection BluetoothCccTimesyncEvent::GetDirection() const {
  if (!is_valid_) {
    return CccDirection::kUndefined;
  }
  CccDirection direction = static_cast<CccDirection>(direction_);
  return (direction <= CccDirection::kMax) ? direction
                                           : CccDirection::kUndefined;
}

uint64_t BluetoothCccTimesyncEvent::GetTimestamp() const { return timestamp_; }

CccLmpEventId BluetoothCccTimesyncEvent::GetEventId() const {
  if (!is_valid_) {
    return CccLmpEventId::kUndefined;
  }
  if (event_id_ == static_cast<uint8_t>(CccLmpEventIdByte::kConnectInd)) {
    return CccLmpEventId::kConnectInd;
  } else if (event_id_ ==
             static_cast<uint8_t>(CccLmpEventIdByte::kLlPhyUpdateInd)) {
    return CccLmpEventId::kLlPhyUpdateInd;
  }
  return CccLmpEventId::kUndefined;
}

uint8_t BluetoothCccTimesyncEvent::GetToggleCount() const {
  return toggle_count_;
}

uint16_t BluetoothCccTimesyncEvent::GetTimesyncOffset() const {
  return timesync_offset_;
}

uint16_t BluetoothCccTimesyncEvent::GetEventCount() const {
  return event_count_;
}

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
