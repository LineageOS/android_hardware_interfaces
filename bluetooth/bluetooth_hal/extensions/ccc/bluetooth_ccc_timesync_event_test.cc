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

HalPacket CreateSamplePacket(BluetoothAddress& address) {
  // Packet format:
  // [Event:1][Eventcode:1][Length:1][Address:6][AddressType:1][Direction:1]
  // [Timestamp:8][EventId:1][ToggleCount:1][Timesync offset:2][Event count:2]
  // Total expected length: 25 bytes

  return HalPacket({0x04,  // HCI Event (1 byte)
                    0xFF,  // Vendor event code (1 byte)
                    0x17,  // Length (1 byte - 23 decimal, payload length)
                    0xD0,  // Time sync sub event code (1 byte)

                    address[5], address[4], address[3], address[2], address[1],
                    address[0],  // Address (6 bytes)

                    0x01,  // AddressType (1 byte - Random)
                    0x00,  // Direction (1 byte - Tx)

                    // Timestamp (8 bytes - 0xAABBCCDDEEFF0011, little-endian)
                    0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,

                    0xFF,  // EventId (1 byte - EventIdConnInd)
                    0x0B,  // ToggleCount (1 byte)

                    // Timesync offset (2 bytes - 0x1234, little-endian)
                    0x34, 0x12,

                    // Event count (2 bytes - 0x5678, little-endian)
                    0x78, 0x56});
}

void CheckEventDefaultValues(BluetoothCccTimesyncEvent& event) {
  BluetoothAddress address = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  ASSERT_FALSE(event.IsValid());
  ASSERT_EQ(event.GetAddress(), address);
  ASSERT_EQ(event.GetAddressType(), 0x00);
  ASSERT_EQ(event.GetDirection(), CccDirection::kUndefined);
  ASSERT_EQ(event.GetTimestamp(), 0x0000000000000000);
  ASSERT_EQ(event.GetEventId(), CccLmpEventId::kUndefined);
  ASSERT_EQ(event.GetToggleCount(), 0x00);
  ASSERT_EQ(event.GetTimesyncOffset(), 0x0000);
  ASSERT_EQ(event.GetEventCount(), 0x0000);
}

TEST(BluetoothCccTimesyncEventTest, ValidPacketParsing) {
  BluetoothAddress address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  BluetoothCccTimesyncEvent event(CreateSamplePacket(address));

  ASSERT_TRUE(event.IsValid());
  ASSERT_EQ(event.GetAddress(), address);
  ASSERT_EQ(event.GetAddressType(), 0x01);
  ASSERT_EQ(event.GetDirection(), CccDirection::kTx);
  ASSERT_EQ(event.GetTimestamp(), 0xAABBCCDDEEFF0011);
  ASSERT_EQ(event.GetEventId(), CccLmpEventId::kConnectInd);
  ASSERT_EQ(event.GetToggleCount(), 0x0B);
  ASSERT_EQ(event.GetTimesyncOffset(), 0x1234);
  ASSERT_EQ(event.GetEventCount(), 0x5678);
}

TEST(BluetoothCccTimesyncEventTest, InvalidPacketParsingIncorrectLength) {
  BluetoothCccTimesyncEvent event(HalPacket({0x01, 0x02, 0x03, 0x04}));
  CheckEventDefaultValues(event);
}

TEST(BluetoothCccTimesyncEventTest, InvalidPacketParsingIncorrectPacketType) {
  BluetoothAddress address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  HalPacket packet = CreateSamplePacket(address);

  // Set to the wrong packet type, should be 0x04 for HCI event.
  packet.at(0) = 0x00;
  BluetoothCccTimesyncEvent event(packet);

  CheckEventDefaultValues(event);
}

TEST(BluetoothCccTimesyncEventTest, InvalidPacketParsingIncorrectEventCode) {
  BluetoothAddress address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  HalPacket packet = CreateSamplePacket(address);

  // Set to the wrong event code, should be 0xFF for vendor event.
  packet.at(1) = 0x00;
  BluetoothCccTimesyncEvent event(packet);

  CheckEventDefaultValues(event);
}

TEST(BluetoothCccTimesyncEventTest, InvalidPacketParsingIncorrectSubEventCode) {
  BluetoothAddress address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  HalPacket packet = CreateSamplePacket(address);

  // Set to the wrong sub event code, should be 0xD0 for time sync event.
  packet.at(3) = 0x00;
  BluetoothCccTimesyncEvent event(packet);

  CheckEventDefaultValues(event);
}

TEST(BluetoothCccTimesyncEventTest, HandleGetDirection) {
  BluetoothAddress address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  HalPacket packet = CreateSamplePacket(address);

  // Set direction to TX.
  packet.at(11) = 0x00;
  BluetoothCccTimesyncEvent event1(packet);
  ASSERT_EQ(event1.GetDirection(), CccDirection::kTx);

  // Set direction to RX.
  packet.at(11) = 0x01;
  BluetoothCccTimesyncEvent event2(packet);
  ASSERT_EQ(event2.GetDirection(), CccDirection::kRx);

  // Set direction to a random value.
  packet.at(11) = 0x99;
  BluetoothCccTimesyncEvent event3(packet);
  ASSERT_EQ(event3.GetDirection(), CccDirection::kUndefined);
}

TEST(BluetoothCccTimesyncEventTest, HandleGetEventId) {
  BluetoothAddress address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  HalPacket packet = CreateSamplePacket(address);

  // Set event id to Conn IND.
  packet.at(20) = 0xFF;
  BluetoothCccTimesyncEvent event1(packet);
  ASSERT_EQ(event1.GetEventId(), CccLmpEventId::kConnectInd);

  // Set event id to LL PHY Update IND.
  packet.at(20) = 0x18;
  BluetoothCccTimesyncEvent event2(packet);
  ASSERT_EQ(event2.GetEventId(), CccLmpEventId::kLlPhyUpdateInd);

  // Set event id to a random value.
  packet.at(20) = 0x99;
  BluetoothCccTimesyncEvent event3(packet);
  ASSERT_EQ(event3.GetEventId(), CccLmpEventId::kUndefined);
}

}  // namespace
}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
