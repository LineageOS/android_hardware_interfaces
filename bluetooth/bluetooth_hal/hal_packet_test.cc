/*
 * Copyright 2024 The Android Open Source Project
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

#include "bluetooth_hal/hal_packet.h"

#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace hci {
namespace {

using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;

enum class TestEnumInt : int {
  kTest = 0,
  kOutOfBond = 99,
};

enum class TestEnumUint8 : uint8_t {
  kTest = 0,
  kOutOfBond = 99,
};

enum class TestEnumUint32 : uint32_t {
  kTest = 0,
  kOutOfBond = 99,
};

enum class TestEnumUint64 : uint64_t {
  kTest = 0,
  kOutOfBond = 99,
};

TEST(HalPacketTest, HandleAt) {
  HalPacket packet({0x01, 0x02, 0x03, 0x04});
  EXPECT_EQ(packet.At(0), 0x01);
  EXPECT_EQ(packet.At(TestEnumInt::kTest), 0x01);
  EXPECT_EQ(packet.At(TestEnumUint8::kTest), 0x01);
  EXPECT_EQ(packet.At(TestEnumUint32::kTest), 0x01);
  EXPECT_EQ(packet.At(TestEnumUint64::kTest), 0x01);

  EXPECT_EQ(packet.At(99), 0);
  EXPECT_EQ(packet.At(TestEnumInt::kOutOfBond), 0);
  EXPECT_EQ(packet.At(TestEnumUint8::kOutOfBond), 0);
  EXPECT_EQ(packet.At(TestEnumUint32::kOutOfBond), 0);
  EXPECT_EQ(packet.At(TestEnumUint64::kOutOfBond), 0);
}

TEST(HalPacketTest, HandleAtUint16LittleEndian) {
  HalPacket packet({0x01, 0x02, 0x03, 0x04});
  EXPECT_EQ(packet.AtUint16LittleEndian(0), 0x0201);
  EXPECT_EQ(packet.AtUint16LittleEndian(TestEnumInt::kTest), 0x0201);
  EXPECT_EQ(packet.AtUint16LittleEndian(TestEnumUint8::kTest), 0x0201);
  EXPECT_EQ(packet.AtUint16LittleEndian(TestEnumUint32::kTest), 0x0201);
  EXPECT_EQ(packet.AtUint16LittleEndian(TestEnumUint64::kTest), 0x0201);

  EXPECT_EQ(packet.AtUint16LittleEndian(99), 0);
  EXPECT_EQ(packet.AtUint16LittleEndian(TestEnumInt::kOutOfBond), 0);
  EXPECT_EQ(packet.AtUint16LittleEndian(TestEnumUint8::kOutOfBond), 0);
  EXPECT_EQ(packet.AtUint16LittleEndian(TestEnumUint32::kOutOfBond), 0);
  EXPECT_EQ(packet.AtUint16LittleEndian(TestEnumUint64::kOutOfBond), 0);
}

TEST(HalPacketTest, HandleAtUint64LittleEndian) {
  HalPacket packet({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
  EXPECT_EQ(packet.AtUint64LittleEndian(0), 0x0807060504030201);
  EXPECT_EQ(packet.AtUint64LittleEndian(TestEnumInt::kTest),
            0x0807060504030201);
  EXPECT_EQ(packet.AtUint64LittleEndian(TestEnumUint8::kTest),
            0x0807060504030201);
  EXPECT_EQ(packet.AtUint64LittleEndian(TestEnumUint32::kTest),
            0x0807060504030201);
  EXPECT_EQ(packet.AtUint64LittleEndian(TestEnumUint64::kTest),
            0x0807060504030201);

  EXPECT_EQ(packet.AtUint64LittleEndian(99), 0);
  EXPECT_EQ(packet.AtUint64LittleEndian(TestEnumInt::kOutOfBond), 0);
  EXPECT_EQ(packet.AtUint64LittleEndian(TestEnumUint8::kOutOfBond), 0);
  EXPECT_EQ(packet.AtUint64LittleEndian(TestEnumUint32::kOutOfBond), 0);
  EXPECT_EQ(packet.AtUint64LittleEndian(TestEnumUint64::kOutOfBond), 0);
}

TEST(HalPacketTest, HandleToString) {
  HalPacket packet({0x01, 0x02, 0x03, 0x04});
  std::string expected = "(4)[01 02 03 04]";
  std::string actual = packet.ToString();
  EXPECT_EQ(actual, expected);
}

TEST(HalPacketTest, HandleToStringEmpty) {
  HalPacket packet;
  std::string expected = "(0)[]";
  std::string actual = packet.ToString();
  EXPECT_EQ(actual, expected);
}

TEST(HalPacketTest, HandleConstructorWithType) {
  // HCI Reset Command
  uint8_t type = static_cast<uint8_t>(HciPacketType::kCommand);
  std::vector<uint8_t> payload = {0x03, 0x0C, 0x00};

  HalPacket packet(type, payload);

  ASSERT_EQ(packet[0], type);
  ASSERT_EQ(packet.size(), payload.size() + 1);
  for (size_t i = 1; i < payload.size(); i++) {
    ASSERT_EQ(packet[i + 1], payload[i]);
  }

  ASSERT_EQ(packet.GetType(), HciPacketType::kCommand);
  ASSERT_EQ(packet.GetCommandOpcode(), 0x0c03);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandlePacketInit) {
  // Uninitialized packet
  HalPacket packet;

  ASSERT_EQ(packet.GetType(), HciPacketType::kUnknown);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleInvalidPacket) {
  // Invalid packet with an unimplmeneted type 0xFF
  HalPacket packet({0xFF, 0x00, 0x00, 0x01, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kUnknown);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleHciCommand) {
  // HCI Reset Command
  HalPacket packet({0x01, 0x03, 0x0c, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kCommand);
  ASSERT_EQ(packet.GetCommandOpcode(), 0x0c03);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleHciEvent) {
  // Mode Change Event (event code = 0x14)
  HalPacket packet({0x04, 0x14, 0x06, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kEvent);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0x14);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleHciCommandCompleteEvent) {
  // HCI Reset Complete Event
  HalPacket packet({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kEvent);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0x0e);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_TRUE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kSuccess));
  ASSERT_TRUE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0x0c03);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleHciCommandStatusEvent) {
  // HCI Create Connection Status Event
  HalPacket packet({0x04, 0x0f, 0x04, 0x00, 0x01, 0x05, 0x04});

  ASSERT_EQ(packet.GetType(), HciPacketType::kEvent);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0x0f);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_TRUE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kSuccess));
  ASSERT_TRUE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0x0405);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleBleMetaEvent) {
  // LE Connection Update Complete event (sub-event code = 0x03)
  HalPacket packet({0x04, 0x3e, 0x0a, 0x03, 0x00, 0x40, 0x00, 0x00, 0x06, 0x00,
                    0x00, 0x00, 0x0a});

  ASSERT_EQ(packet.GetType(), HciPacketType::kEvent);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0x3e);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_TRUE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0x03);
}

TEST(HalPacketTest, HandleAclData) {
  // ACL data
  HalPacket packet({0x02, 0x41, 0x00, 0x01, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kAclData);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleScoData) {
  // SCO data
  HalPacket packet({0x03, 0x41, 0x00, 0x01, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kScoData);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleIsoData) {
  // ISO data
  HalPacket packet({0x05, 0x41, 0x00, 0x01, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kIsoData);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleThreadData) {
  // Thread data
  HalPacket packet({0x70, 0x00, 0x00, 0x01, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kThreadData);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

TEST(HalPacketTest, HandleHdlcData) {
  // HDLC data
  HalPacket packet({0x7e, 0x00, 0x00, 0x01, 0x00});

  ASSERT_EQ(packet.GetType(), HciPacketType::kHdlcData);
  ASSERT_EQ(packet.GetCommandOpcode(), 0);
  ASSERT_FALSE(packet.IsVendorCommand());
  ASSERT_EQ(packet.GetEventCode(), 0);
  ASSERT_FALSE(packet.IsVendorEvent());
  ASSERT_FALSE(packet.IsCommandCompleteEvent());
  ASSERT_FALSE(packet.IsCommandStatusEvent());
  ASSERT_EQ(packet.GetCommandCompleteEventResult(),
            static_cast<uint8_t>(EventResultCode::kFailure));
  ASSERT_FALSE(packet.IsCommandCompleteStatusEvent());
  ASSERT_EQ(packet.GetCommandOpcodeFromGeneratedEvent(), 0);
  ASSERT_FALSE(packet.IsBleMetaEvent());
  ASSERT_EQ(packet.GetBleSubEventCode(), 0);
}

}  // namespace
}  // namespace hci
}  // namespace bluetooth_hal
