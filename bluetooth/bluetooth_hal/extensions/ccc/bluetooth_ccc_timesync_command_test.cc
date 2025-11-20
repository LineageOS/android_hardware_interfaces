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

#include <vector>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {
namespace {

using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::HalPacket;

// Test Suite for BluetoothCccTimesyncCommand
TEST(BluetoothCccTimesyncCommandTest, CreateAddCommandBasic) {
  BluetoothAddress address = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  AddressType addr_type = AddressType::kRandom;
  CccDirection direction = CccDirection::kTx;
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};

  HalPacket command_packet = BluetoothCccTimesyncCommand::CreateAddCommand(
      address, addr_type, direction, lmp_ids);

  // Expected packet construction:
  // Header (4 bytes): PacketType (0x01), Opcode LSB (0x63), Opcode MSB (0xFD),
  // Payload Length (0x0B) Payload (11 bytes): CommandType::kAdd (0x01), Address
  // (6 bytes), AddrType (0x01), Direction (0x00), LmpIds (0xFF, 0x18)
  HalPacket expected_packet({
      0x01, 0x63, 0xFD, 0x0B,  // Fixed Header: PacketType, Opcode (L,M), Length
      0x01,                    // Command Type: kAdd
      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,  // Address
      0x01,                                // Address Type: kRandom
      0x00,                                // Direction: kTx
      0xFF, 0x18  // LMP IDs: kConnectInd (0xFF), kLlPhyUpdateInd (0x18)
  });

  EXPECT_EQ(command_packet.size(),
            expected_packet.size());           // Sanity check length
  EXPECT_EQ(command_packet, expected_packet);  // Compare entire packets
}

TEST(BluetoothCccTimesyncCommandTest, CreateAddCommandEmptyLmpIds) {
  BluetoothAddress address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  AddressType addr_type = AddressType::kPublic;
  CccDirection direction = CccDirection::kRx;
  std::vector<CccLmpEventId> lmp_ids = {};  // Empty LMP IDs

  HalPacket command_packet = BluetoothCccTimesyncCommand::CreateAddCommand(
      address, addr_type, direction, lmp_ids);

  // Expected packet construction:
  // Header (4 bytes): PacketType (0x01), Opcode LSB (0x63), Opcode MSB (0xFD),
  // Payload Length (0x09) Payload (9 bytes): CommandType::kAdd (0x01), Address
  // (6 bytes), AddrType (0x00), Direction (0x01)
  HalPacket expected_packet({
      0x01, 0x63, 0xFD, 0x09,  // Fixed Header: PacketType, Opcode (L,M), Length
      0x01,                    // Command Type: kAdd
      0x66, 0x55, 0x44, 0x33, 0x22, 0x11,  // Address
      0x00,                                // Address Type: kPublic
      0x01                                 // Direction: kRx
  });

  EXPECT_EQ(command_packet.size(), expected_packet.size());
  EXPECT_EQ(command_packet, expected_packet);
}

TEST(BluetoothCccTimesyncCommandTest, CreateAddCommandUndefinedLmpIdMapping) {
  BluetoothAddress address = {0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA};
  AddressType addr_type = AddressType::kRandom;
  CccDirection direction = CccDirection::kTx;
  std::vector<CccLmpEventId> lmp_ids = {
      CccLmpEventId::kUndefined};  // Test kUndefined mapping

  HalPacket command_packet = BluetoothCccTimesyncCommand::CreateAddCommand(
      address, addr_type, direction, lmp_ids);

  // Expected packet construction:
  // Header (4 bytes): PacketType (0x01), Opcode LSB (0x63), Opcode MSB (0xFD),
  // Payload Length (0x0A) Payload (10 bytes): CommandType::kAdd (0x01), Address
  // (6 bytes), AddrType (0x01), Direction (0x00), LmpId (0x00)
  HalPacket expected_packet({
      0x01, 0x63, 0xFD, 0x0A,  // Fixed Header: PacketType, Opcode (L,M), Length
      0x01,                    // Command Type: kAdd
      0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA,  // Address
      0x01,                                // Address Type: kRandom
      0x00,                                // Direction: kTx
      0x00                                 // LMP ID: kUndefined maps to 0x00
  });

  EXPECT_EQ(command_packet.size(), expected_packet.size());
  EXPECT_EQ(command_packet, expected_packet);
}

TEST(BluetoothCccTimesyncCommandTest, CreateRemoveCommand) {
  BluetoothAddress address = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  AddressType addr_type = AddressType::kPublic;

  HalPacket command_packet =
      BluetoothCccTimesyncCommand::CreateRemoveCommand(address, addr_type);

  // Expected packet construction:
  // Header (4 bytes): PacketType (0x01), Opcode LSB (0x63), Opcode MSB (0xFD),
  // Payload Length (0x08) Payload (8 bytes): CommandType::kRemove (0x02),
  // Address (6 bytes), AddressType (0x00)
  HalPacket expected_packet({
      0x01, 0x63, 0xFD, 0x08,  // Fixed Header: PacketType, Opcode (L,M), Length
      0x02,                    // Command Type: kRemove
      0x06, 0x05, 0x04, 0x03, 0x02, 0x01,  // Address
      0x00                                 // Address Type: kPublic
  });

  EXPECT_EQ(command_packet.size(), expected_packet.size());
  EXPECT_EQ(command_packet, expected_packet);
}

TEST(BluetoothCccTimesyncCommandTest, CreateClearCommand) {
  HalPacket command_packet = BluetoothCccTimesyncCommand::CreateClearCommand();

  // Expected packet construction:
  // Header (4 bytes): PacketType (0x01), Opcode LSB (0x63), Opcode MSB (0xFD),
  // Payload Length (0x01) Payload (1 byte): CommandType::kClear (0x03)
  HalPacket expected_packet({
      0x01, 0x63, 0xFD, 0x01,  // Fixed Header: PacketType, Opcode (L,M), Length
      0x03                     // Command Type: kClear
  });

  EXPECT_EQ(command_packet.size(), expected_packet.size());
  EXPECT_EQ(command_packet, expected_packet);
}

}  // namespace
}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
