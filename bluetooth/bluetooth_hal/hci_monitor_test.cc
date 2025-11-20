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

#include "bluetooth_hal/hci_monitor.h"

#include <cstdint>

#include "bluetooth_hal/hal_packet.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace hci {
namespace {

using ::bluetooth_hal::hci::HalPacket;

class HciMonitorTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(HciMonitorTest, HciMonitorCommandEqual) {
  // Command
  uint16_t primary_code = 0x0c03;
  HciMonitor monitor1(MonitorType::kCommand, primary_code);
  HciMonitor monitor2(MonitorType::kCommand, primary_code);
  EXPECT_TRUE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciMonitorEventEqual) {
  // Event
  uint16_t primary_code = 0x02;
  HciMonitor monitor1(MonitorType::kEvent, primary_code);
  HciMonitor monitor2(MonitorType::kEvent, primary_code);
  EXPECT_TRUE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciMonitorTypeNotEqual) {
  // Types not equal
  uint16_t primary_code = 0x0c03;
  HciMonitor monitor1(MonitorType::kCommand, primary_code);
  HciMonitor monitor2(MonitorType::kEvent, primary_code);
  EXPECT_FALSE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciMonitorEventNotEqual) {
  // primary_codes not equal
  uint16_t code1 = 0x02;
  uint16_t code2 = 0x03;
  HciMonitor monitor1(MonitorType::kEvent, code1);
  HciMonitor monitor2(MonitorType::kEvent, code2);
  EXPECT_FALSE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciMonitorCommandNotEqual) {
  // Sub-codes not equal
  uint16_t code1 = 0xfd2b;
  uint16_t code2 = 0x1234;
  HciMonitor monitor1(MonitorType::kCommand, code1);
  HciMonitor monitor2(MonitorType::kCommand, code2);
  EXPECT_FALSE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, BluetoothPacketTypeNotEqual) {
  // Type not equal
  uint16_t primary_code = 0x0c03;
  HciMonitor monitor(MonitorType::kEvent, primary_code);
  HalPacket packet({0x01, 0x03, 0x0c, 0x00});
  EXPECT_FALSE(packet == monitor);
}

TEST_F(HciMonitorTest, BluetoothPacketSubCodeNotEqual) {
  // Sub-code not equal
  uint16_t primary_code = 0xfd54;
  uint16_t secondary_code = 0x02;  // should be 0x01
  uint16_t offset = 4;
  HciMonitor monitor(MonitorType::kCommand, primary_code);
  monitor.MonitorOffset(secondary_code, offset);

  // LE Multi ADV Command
  HalPacket packet({0x01, 0x54, 0xfd, 0x18, 0x01, 0x90, 0x01, 0xc2, 0x01, 0x00,
                    0x01, 0x9e, 0x46, 0x7e, 0x8f, 0x96, 0x66, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x01, 0xf1});
  EXPECT_FALSE(packet == monitor);
}

TEST_F(HciMonitorTest, BluetoothPacketPrimaryCodeNotEqual) {
  // primary_code not equal
  uint16_t primary_code = 0x15;  // should be 0x14
  HciMonitor monitor(MonitorType::kEvent, primary_code);
  HalPacket packet({0x04, 0x14, 0x06, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00});
  EXPECT_FALSE(packet == monitor);
}

TEST_F(HciMonitorTest, BluetoothPacketOffsetNotEqual) {
  // Offset not equal
  uint16_t primary_code = 0x3e;
  uint16_t secondary_code = 0x03;
  uint16_t offset = 6;  // should be 3
  HciMonitor monitor(MonitorType::kEvent, primary_code);
  monitor.MonitorOffset(secondary_code, offset);

  // LE Connection Update Complete event
  HalPacket packet({0x04, 0x3e, 0x0a, 0x03, 0x00, 0x40, 0x00, 0x00, 0x06, 0x00,
                    0x00, 0x00, 0x0a});
  EXPECT_FALSE(packet == monitor);
}

TEST_F(HciMonitorTest, BluetoothPacketOverflowNotEqual) {
  // Offset overflow
  uint16_t primary_code = 0x3e;
  uint16_t secondary_code = 0x03;
  uint16_t offset = 999;  // should be 3
  HciMonitor monitor(MonitorType::kEvent, primary_code);
  monitor.MonitorOffset(secondary_code, offset);
  // LE Connection Update Complete event
  HalPacket packet({0x04, 0x3e, 0x0a, 0x03, 0x00, 0x40, 0x00, 0x00, 0x06, 0x00,
                    0x00, 0x00, 0x0a});
  EXPECT_FALSE(packet == monitor);
}

TEST_F(HciMonitorTest, HciEventMonitorWithEventCodeOnly) {
  uint8_t event_code = 0xff;
  HciEventMonitor monitor1(event_code);
  HciEventMonitor monitor2(event_code);
  EXPECT_TRUE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciEventMonitorWithSubEventCode) {
  // BLE Meta Event
  uint8_t primary_code = 0x3e;
  uint8_t secondary_code = 0x01;
  int offset = 4;
  HciEventMonitor monitor1(primary_code, secondary_code, offset);
  HciEventMonitor monitor2(primary_code, secondary_code, offset);
  EXPECT_TRUE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciEventMonitorOffsetNotEqual) {
  // Offsets not equal
  uint8_t event_code = 0x12;
  uint8_t sub_event_code = 0x34;
  int offset1 = 5;
  int offset2 = 6;
  HciEventMonitor monitor1(event_code, sub_event_code, offset1);
  HciEventMonitor monitor2(event_code, sub_event_code, offset2);
  EXPECT_FALSE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, BluetoothPacketEventEqual) {
  // HCI Event
  uint8_t event_code = 0x14;
  HciEventMonitor monitor(event_code);
  HalPacket packet({0x04, 0x14, 0x06, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00});
  EXPECT_TRUE(packet == monitor);
}

TEST_F(HciMonitorTest, BluetoothPacketEventWithSubCodeEqual) {
  // HCI Event with sub-code
  uint8_t event_code = 0x3e;
  uint8_t sub_event_code = 0x03;
  int offset = 3;
  HciEventMonitor monitor(event_code, sub_event_code, offset);
  // LE Connection Update Complete event
  HalPacket packet({0x04, 0x3e, 0x0a, 0x03, 0x00, 0x40, 0x00, 0x00, 0x06, 0x00,
                    0x00, 0x00, 0x0a});
  EXPECT_TRUE(packet == monitor);
}

TEST_F(HciMonitorTest, HciCommandMonitorWithOpcodeOnly) {
  uint16_t opcode = 0xff;
  HciCommandMonitor monitor1(opcode);
  HciCommandMonitor monitor2(opcode);
  EXPECT_TRUE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciCommandMonitorWithSubOpcode) {
  // Vendor Command
  uint16_t primary_code = 0xfd2b;
  uint8_t secondary_code = 0x01;
  int offset = 5;
  HciCommandMonitor monitor1(primary_code, secondary_code, offset);
  HciCommandMonitor monitor2(primary_code, secondary_code, offset);
  EXPECT_TRUE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciCommandMonitorOffsetNotEqual) {
  // Offsets not equal
  uint16_t opcode = 0x1234;
  uint8_t sub_opcode = 0x56;
  int offset1 = 5;
  int offset2 = 6;
  HciCommandMonitor monitor1(opcode, sub_opcode, offset1);
  HciCommandMonitor monitor2(opcode, sub_opcode, offset2);
  EXPECT_FALSE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, BluetoothPacketCommandEqual) {
  // HCI Command
  uint16_t opcode = 0x0c03;
  HciCommandMonitor monitor(opcode);
  HalPacket packet({0x01, 0x03, 0x0c, 0x00});
  EXPECT_TRUE(packet == monitor);
}

TEST_F(HciMonitorTest, BluetoothPacketCommandWithSubCodeEqual) {
  // HCI Command with sub-code
  uint16_t opcode = 0xfd54;
  uint8_t sub_opcode = 0x01;
  int offset = 4;
  HciCommandMonitor monitor(opcode, sub_opcode, offset);
  // LE Multi ADV Command
  HalPacket packet({0x01, 0x54, 0xfd, 0x18, 0x01, 0x90, 0x01, 0xc2, 0x01, 0x00,
                    0x01, 0x9e, 0x46, 0x7e, 0x8f, 0x96, 0x66, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x01, 0xf1});
  EXPECT_TRUE(packet == monitor);
}

TEST_F(HciMonitorTest, HciBleMetaEventMonitorEqual) {
  // LE Connection Update Complete event
  uint8_t correct_ble_event = 0x03;
  uint8_t incorrect_ble_event = 0x05;
  HalPacket packet({0x04, 0x3e, 0x0a, 0x03, 0x00, 0x40, 0x00, 0x00, 0x06, 0x00,
                    0x00, 0x00, 0x0a});

  HciBleMetaEventMonitor monitor1(correct_ble_event);
  HciBleMetaEventMonitor monitor2(incorrect_ble_event);

  EXPECT_TRUE(monitor1 == monitor1);
  EXPECT_TRUE(packet == monitor1);
  EXPECT_FALSE(packet == monitor2);
  EXPECT_FALSE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciBqrEventMonitorEqual) {
  // BQR root inflammation event
  uint8_t root_inflammation_report_id = 0x05;
  HalPacket correct_packet({0x04, 0xff, 0x04, 0x58, 0x05, 0x00, 0x01});
  HalPacket incorrect_packet({0x01, 0x03, 0x0c, 0x00});

  HciBqrEventMonitor monitor1;
  HciBqrEventMonitor monitor2(root_inflammation_report_id);

  EXPECT_TRUE(correct_packet == monitor1);
  EXPECT_TRUE(correct_packet == monitor2);
  EXPECT_FALSE(incorrect_packet == monitor1);
  EXPECT_FALSE(incorrect_packet == monitor2);
}

TEST_F(HciMonitorTest, HciCommandCompleteEventMonitorEqual) {
  // HCI RESET command opcode
  uint16_t opcode = 0x0c03;

  // HCI RESET command complete event
  HalPacket correct_packet({0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  HalPacket incorrect_packet({0x01, 0x03, 0x0c, 0x00});

  HciCommandCompleteEventMonitor monitor(opcode);

  EXPECT_TRUE(correct_packet == monitor);
  EXPECT_FALSE(incorrect_packet == monitor);
}

TEST_F(HciMonitorTest, HciCommandStatusEventMonitorEqual) {
  // HCI INQUIRY command opcode
  uint16_t opcode = 0x0401;

  // HCI INQUIRY command status event
  HalPacket correct_packet({0x04, 0x0f, 0x04, 0x00, 0x01, 0x01, 0x04});
  HalPacket incorrect_packet({0x01, 0x03, 0x0c, 0x00});

  HciCommandStatusEventMonitor monitor(opcode);

  EXPECT_TRUE(correct_packet == monitor);
  EXPECT_FALSE(incorrect_packet == monitor);
}

TEST_F(HciMonitorTest, HciThreadMonitorEqual) {
  // Default constructor.
  HciThreadMonitor monitor1;
  HciThreadMonitor monitor2;
  EXPECT_TRUE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciThreadMonitorWithOffsetEqual) {
  // Constructor with offset and data.
  int offset = 2;
  uint8_t data = 0xab;
  HciThreadMonitor monitor1(offset, data);
  HciThreadMonitor monitor2(offset, data);
  EXPECT_TRUE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciThreadMonitorOffsetNotEqual) {
  // Different offsets.
  int offset1 = 2;
  int offset2 = 3;
  uint8_t data = 0xab;
  HciThreadMonitor monitor1(offset1, data);
  HciThreadMonitor monitor2(offset2, data);
  EXPECT_FALSE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, HciThreadMonitorDataNotEqual) {
  // Different data at the same offset.
  int offset = 2;
  uint8_t data1 = 0xab;
  uint8_t data2 = 0xcd;
  HciThreadMonitor monitor1(offset, data1);
  HciThreadMonitor monitor2(offset, data2);
  EXPECT_FALSE(monitor1 == monitor2);
}

TEST_F(HciMonitorTest, BluetoothPacketThreadEqual) {
  // Default thread monitor matches any thread packet.
  HciThreadMonitor monitor;
  HalPacket packet({0x70, 0x01, 0x02, 0x03});
  EXPECT_TRUE(packet == monitor);
}

TEST_F(HciMonitorTest, BluetoothPacketThreadWithOffsetEqual) {
  // Thread monitor with specific offset and data.
  int offset = 2;
  uint8_t data = 0xab;
  HciThreadMonitor monitor(offset, data);
  HalPacket packet({0x70, 0x01, 0xab, 0x03});
  HalPacket wrong_data_packet({0x70, 0x01, 0xcd, 0x03});
  EXPECT_TRUE(packet == monitor);
  EXPECT_FALSE(wrong_data_packet == monitor);
}

}  // namespace
}  // namespace hci
}  // namespace bluetooth_hal
