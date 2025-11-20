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

#include <vector>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {

class BluetoothCccTimesyncCommand : public ::bluetooth_hal::hci::HalPacket {
 public:
  /**
   * @brief Creates an ADD CCC Timesync Command packet.
   *
   * Format:
   * [PacketType:1][Opcode:2][Length:1][ADD:1][Address:6][AddressType:1][Direction:1][LmpId:X]
   *
   * @param address The 6-byte Bluetooth address to add.
   * @param lmp_ids The variable-length LMP ID. This can be empty if the LMP ID
   * is not present.
   * @return A `HalPacket` object representing the ADD command packet.
   */
  static ::bluetooth_hal::hci::HalPacket CreateAddCommand(
      const ::bluetooth_hal::hci::BluetoothAddress& address,
      const AddressType address_type, const CccDirection direction,
      const std::vector<CccLmpEventId>& lmp_ids);

  /**
   * @brief Creates a REMOVE CCC Timesync Command packet.
   *
   * Format:
   * [PacketType:1][Opcode:2][Length:1][REMOVE:1][Address:6][AddressType:1]
   *
   * @param address The 6-byte Bluetooth address to remove.
   * @return A `HalPacket` object representing the REMOVE command packet.
   */
  static ::bluetooth_hal::hci::HalPacket CreateRemoveCommand(
      const ::bluetooth_hal::hci::BluetoothAddress& address,
      const AddressType address_type);

  /**
   * @brief Creates a CLEAR CCC Timesync Command packet.
   *
   * Format: [PacketType:1][Opcode:2][Length:1(1)][CLEAR:1]
   *
   * @return A `HalPacket` object representing the CLEAR command packet.
   */
  static ::bluetooth_hal::hci::HalPacket CreateClearCommand();
};

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
