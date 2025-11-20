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

#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_timesync_command.h"

#include <cstdint>
#include <vector>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {

namespace {
using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;

static constexpr uint8_t kHeaderSize =
    4;  // H4 header(1) + opcode(2) + length(1)

static void AppendFixedHeader(HalPacket& packet, uint8_t payload_length) {
  packet.push_back(static_cast<uint8_t>(HciPacketType::kCommand));
  packet.push_back(TimesyncConstants::kCommandOpCode & 0xFF);
  packet.push_back((TimesyncConstants::kCommandOpCode >> 8) & 0xFF);
  packet.push_back(payload_length);
}

}  // namespace

HalPacket BluetoothCccTimesyncCommand::CreateAddCommand(
    const BluetoothAddress& address, const AddressType address_type,
    const CccDirection direction, const std::vector<CccLmpEventId>& lmp_ids) {
  // Command Type (1) + Address (6) + Address Type (1) + Direction (1) + LmpId
  // (X)
  uint8_t payload_length =
      TimesyncConstants::kCommandCommandTypeLength + address.size() +
      TimesyncConstants::kCommandAddressTypeLength +
      TimesyncConstants::kCommandDirectionLength + lmp_ids.size();

  HalPacket packet;
  packet.reserve(kHeaderSize + payload_length);  // Fixed header (4) + payload
  AppendFixedHeader(packet, payload_length);

  // Command-specific Payload
  packet.push_back(
      static_cast<uint8_t>(TimesyncCommandType::kAdd));  // Command Type: ADD
  for (int i = address.size() - 1; i >= 0; --i) {
    packet.push_back(address[i]);  // Address
  }
  packet.push_back(static_cast<uint8_t>(address_type));  // Address Type
  packet.push_back(static_cast<uint8_t>(direction));     // Direction
  for (CccLmpEventId id : lmp_ids) {
    uint8_t byte;
    switch (id) {
      case CccLmpEventId::kConnectInd:
        byte = static_cast<uint8_t>(CccLmpEventIdByte::kConnectInd);
        break;
      case CccLmpEventId::kLlPhyUpdateInd:
        byte = static_cast<uint8_t>(CccLmpEventIdByte::kLlPhyUpdateInd);
        break;
      default:
        byte = static_cast<uint8_t>(CccLmpEventIdByte::kUndefined);
        break;
    }
    packet.push_back(byte);  // LmpId
  }

  return packet;
}

HalPacket BluetoothCccTimesyncCommand::CreateRemoveCommand(
    const BluetoothAddress& address, const AddressType address_type) {
  // Command Type (1) + Address (6) + AddressType (1)
  uint8_t payload_length = TimesyncConstants::kCommandCommandTypeLength +
                           address.size() +
                           TimesyncConstants::kCommandAddressTypeLength;

  HalPacket packet;
  packet.reserve(kHeaderSize + payload_length);  // Fixed header (4) + payload
  AppendFixedHeader(packet, payload_length);

  // Command-specific Payload
  packet.push_back(static_cast<uint8_t>(
      TimesyncCommandType::kRemove));  // Command Type: REMOVE
  for (int i = address.size() - 1; i >= 0; --i) {
    packet.push_back(address[i]);  // Address
  }
  packet.push_back(static_cast<uint8_t>(address_type));  // Address Type

  return packet;
}

HalPacket BluetoothCccTimesyncCommand::CreateClearCommand() {
  // Command Type (1)
  uint8_t payload_length = TimesyncConstants::kCommandCommandTypeLength;
  HalPacket packet;
  packet.reserve(kHeaderSize + payload_length);  // Fixed header (4) + payload
  AppendFixedHeader(packet, payload_length);

  // Command-specific Payload
  packet.push_back(static_cast<uint8_t>(
      TimesyncCommandType::kClear));  // Command Type: CLEAR

  return packet;
}

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
