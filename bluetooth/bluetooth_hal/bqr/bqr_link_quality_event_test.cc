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

#include "bluetooth_hal/bqr/bqr_link_quality_event.h"

#include <cstdint>
#include <vector>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v1_to_v3.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v4.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v5.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v6.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v7.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::HalPacket;

std::vector<uint8_t> CreateCommonLinkQualityData() {
  return {
      // BQR Event V3/V4/V5/V6 Common Link Quality Data
      0x51,                    // Packet Types
      0x60, 0x00,              // Connection Handle (0x0060)
      0x00,                    // Connection Role
      0x02,                    // TX Power Level (signed 8-bit integer)
      0xbe,                    // RSSI (signed 8-bit integer)
      0x00,                    // SNR
      0x16,                    // Unused AFH Channel Count
      0x00,                    // AFH Select Unideal Channel Count
      0x40, 0x1f,              // Link Supervision Timeout (0x1f40)
      0x95, 0x04, 0x00, 0x00,  // Connection Piconet Clock (0x00000495)
      0x30, 0x00, 0x00, 0x00,  // Retransmission Count (0x00000030)
      0x32, 0x00, 0x00, 0x00,  // No RX Count (0x00000032)
      0x14, 0x00, 0x00, 0x00,  // NAK Count (0x00000014)
      0x95, 0x04, 0x00, 0x00,  // Last TX ACK Timestamp (0x00000495)
      0x00, 0x00, 0x00, 0x00,  // Flow Off Count (0x00000000)
      0x95, 0x04, 0x00, 0x00,  // Last Flow On Timestamp (0x00000495)
      0x00, 0x00, 0x00, 0x00,  // Buffer Overflow Bytes (0x00000000)
      0x00, 0x00, 0x00, 0x00   // Buffer Underflow Bytes (0x00000000)
  };
}

HalPacket CreateBqrLinkQualityEventV6V5V3() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      0xf6,  // Parameter Total Length
      0x58,  // Sub Event: Quality Monitor Event (0x58)
      0x07   // Report ID: LE Audio (0x07)
  };
  std::vector<uint8_t> common_data = CreateCommonLinkQualityData();
  data.insert(data.end(), common_data.begin(), common_data.end());

  // BQR Event V5 Specific Fields (starts after common Link Quality data)
  data.insert(
      data.end(),
      {
          0xd4, 0xc9, 0x3d, 0x8e,
          0xa7, 0x75,              // Remote Address (75:A7:8E:3D:C9:D4)
          0x00,                    // Call Failed Item Count
          0x80, 0x04, 0x00, 0x00,  // TX Total Packets (0x00000480)
          0x30, 0x00, 0x00, 0x00,  // TX Unacked Packets (0x00000030)
          0x46, 0x00, 0x00, 0x00,  // TX Flushed Packets (0x00000046)
          0x09, 0x00, 0x00, 0x00,  // TX Last Subevent Packets (0x00000009)
          0x04, 0x00, 0x00, 0x00,  // CRC Error Packets (0x00000004)
          0x03, 0x00, 0x00, 0x00   // RX Duplicate Packets (0x00000003)
      });

  // BQR Event V6 Specific Fields (starts after V5 fields)
  data.insert(data.end(),
              {
                  0x00, 0x00, 0x00, 0x00,  // RX Unreceived Packets (0x00000000)
                  0x08, 0x00               // Coex Info Mask (0x0008)
              });

  // Random vendor data after kEnd
  data.insert(data.end(), {0x01, 0x02, 0x03, 0x04});

  return HalPacket(data);
}

HalPacket CreateBqrLinkQualityEventV4() {
  std::vector<uint8_t> data = {
      0x04,  // H4 Type: HCI Event
      0xff,  // Event Code: Vendor Specific Event (0xFF)
      0xf6,  // Parameter Total Length
      0x58,  // Sub Event: Quality Monitor Event (0x58)
      0x07   // Report ID: LE Audio (0x07)
  };
  std::vector<uint8_t> common_data = CreateCommonLinkQualityData();
  data.insert(data.end(), common_data.begin(), common_data.end());

  // BQR Event V4 Specific Fields (starts after common Link Quality data)
  data.insert(
      data.end(),
      {
          0x80, 0x04, 0x00, 0x00,  // TX Total Packets (0x00000480)
          0x30, 0x00, 0x00, 0x00,  // TX Unacked Packets (0x00000030)
          0x46, 0x00, 0x00, 0x00,  // TX Flushed Packets (0x00000046)
          0x09, 0x00, 0x00, 0x00,  // TX Last Subevent Packets (0x00000009)
          0x04, 0x00, 0x00, 0x00,  // CRC Error Packets (0x00000004)
          0x03, 0x00, 0x00, 0x00   // RX Duplicate Packets (0x00000003)
      });

  // Random vendor data after kEnd
  data.insert(data.end(), {0x01, 0x02, 0x03, 0x04});

  return HalPacket(data);
}

HalPacket CreateIncorrectBqrHalPacket() {
  return HalPacket({0x01, 0x02, 0x03, 0x04, 0x05});
}

HalPacket CreateShortBqrPacket() {
  return HalPacket({
      0x04, 0xff, 0x03, 0x58,
      0x02,  // A2DP Choppy
      0x01,  // Packet Types (but packet ends here)
  });
}

HalPacket CreateWrongReportIdPacket() {
  std::vector<uint8_t> data = {
      0x04, 0xff, 0x4e, 0x58,
      0x05,  // Report ID: kRootInflammation (0x05)
  };
  // 5 bytes header + 83 bytes event data(LinkQualityOffsetV5::kEnd)
  size_t link_quality_event_v5_length = 88;

  // Create a packet with BQR header and 83 zeros.
  data.resize(link_quality_event_v5_length, 0x00);
  return HalPacket(data);
}

void VerifyV1ToV3(const BqrLinkQualityEventV1ToV3& packet) {
  // Assertions for common Link Quality fields
  ASSERT_EQ(packet.GetPacketTypes(), 0x51);
  ASSERT_EQ(packet.GetConnectionHandle(), 0x0060);
  ASSERT_EQ(packet.GetConnectionRole(), 0x00);
  ASSERT_EQ(packet.GetTxPowerLevel(), 0x02);
  ASSERT_EQ(packet.GetRssi(), static_cast<int8_t>(0xbe));  // -66
  ASSERT_EQ(packet.GetSnr(), 0x00);
  ASSERT_EQ(packet.GetUnusedAfhChannelCount(), 0x16);
  ASSERT_EQ(packet.GetAfhSelectUnidealChannelCount(), 0x00);
  ASSERT_EQ(packet.GetLsto(), 0x1f40);
  ASSERT_EQ(packet.GetConnectionPiconetClock(), 0x00000495);
  ASSERT_EQ(packet.GetRetransmissionCount(), 0x00000030);
  ASSERT_EQ(packet.GetNoRxCount(), 0x00000032);
  ASSERT_EQ(packet.GetNakCount(), 0x00000014);
  ASSERT_EQ(packet.GetLastTxAckTimestamp(), 0x00000495);
  ASSERT_EQ(packet.GetFlowOffCount(), 0x00000000);
  ASSERT_EQ(packet.GetLastFlowOnTimestamp(), 0x00000495);
  ASSERT_EQ(packet.GetBufferOverflowBytes(), 0x00000000);
  ASSERT_EQ(packet.GetBufferUnderflowBytes(), 0x00000000);
}

void VerifyV4(const BqrLinkQualityEventV4& packet) {
  // Assertions for V4 specific fields
  ASSERT_EQ(packet.GetTxTotalPackets(), 0x00000480);
  ASSERT_EQ(packet.GetTxUnackedPackets(), 0x00000030);
  ASSERT_EQ(packet.GetTxFlushedPackets(), 0x00000046);
  ASSERT_EQ(packet.GetTxLastSubeventPackets(), 0x00000009);
  ASSERT_EQ(packet.GetCrcErrorPackets(), 0x00000004);
  ASSERT_EQ(packet.GetRxDuplicatePackets(), 0x00000003);
}

void VerifyV5(const BqrLinkQualityEventV5& packet) {
  // Assertions for V5 specific fields
  BluetoothAddress expected_remote_addr = {0x75, 0xa7, 0x8e, 0x3d, 0xc9, 0xd4};
  ASSERT_EQ(packet.GetRemoteAddress(), expected_remote_addr);
  ASSERT_EQ(packet.GetCallFailedItemCount(), 0x00);
  ASSERT_EQ(packet.GetTxTotalPackets(), 0x00000480);
  ASSERT_EQ(packet.GetTxUnackedPackets(), 0x00000030);
  ASSERT_EQ(packet.GetTxFlushedPackets(), 0x00000046);
  ASSERT_EQ(packet.GetTxLastSubeventPackets(), 0x00000009);
  ASSERT_EQ(packet.GetCrcErrorPackets(), 0x00000004);
  ASSERT_EQ(packet.GetRxDuplicatePackets(), 0x00000003);
}

void VerifyDefaultV1ToV3(const BqrLinkQualityEventV1ToV3& packet) {
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetVersion(), BqrVersion::kNone);
  ASSERT_EQ(packet.GetPacketTypes(), 0);
  ASSERT_EQ(packet.GetConnectionHandle(), 0);
  ASSERT_EQ(packet.GetConnectionRole(), 0);
  ASSERT_EQ(packet.GetTxPowerLevel(), 0);
  ASSERT_EQ(packet.GetRssi(), 0);
  ASSERT_EQ(packet.GetSnr(), 0);
  ASSERT_EQ(packet.GetUnusedAfhChannelCount(), 0);
  ASSERT_EQ(packet.GetAfhSelectUnidealChannelCount(), 0);
  ASSERT_EQ(packet.GetLsto(), 0);
  ASSERT_EQ(packet.GetConnectionPiconetClock(), 0);
  ASSERT_EQ(packet.GetRetransmissionCount(), 0);
  ASSERT_EQ(packet.GetNoRxCount(), 0);
  ASSERT_EQ(packet.GetNakCount(), 0);
  ASSERT_EQ(packet.GetLastTxAckTimestamp(), 0);
  ASSERT_EQ(packet.GetFlowOffCount(), 0);
  ASSERT_EQ(packet.GetLastFlowOnTimestamp(), 0);
  ASSERT_EQ(packet.GetBufferOverflowBytes(), 0);
  ASSERT_EQ(packet.GetBufferUnderflowBytes(), 0);
}

void VerifyDefaultV4(const BqrLinkQualityEventV4& packet) {
  VerifyDefaultV1ToV3(packet);

  // Assertions for V4 specific fields
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetVersion(), BqrVersion::kNone);
  ASSERT_EQ(packet.GetTxTotalPackets(), 0);
  ASSERT_EQ(packet.GetTxUnackedPackets(), 0);
  ASSERT_EQ(packet.GetTxFlushedPackets(), 0);
  ASSERT_EQ(packet.GetTxLastSubeventPackets(), 0);
  ASSERT_EQ(packet.GetCrcErrorPackets(), 0);
  ASSERT_EQ(packet.GetRxDuplicatePackets(), 0);
}

void VerifyDefaultV5(const BqrLinkQualityEventV5& packet) {
  VerifyDefaultV1ToV3(packet);

  // Assertions for V5 specific fields
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetVersion(), BqrVersion::kNone);
  BluetoothAddress expected_remote_addr = {};
  ASSERT_EQ(packet.GetRemoteAddress(), expected_remote_addr);
  ASSERT_EQ(packet.GetCallFailedItemCount(), 0);
  ASSERT_EQ(packet.GetTxTotalPackets(), 0);
  ASSERT_EQ(packet.GetTxUnackedPackets(), 0);
  ASSERT_EQ(packet.GetTxFlushedPackets(), 0);
  ASSERT_EQ(packet.GetTxLastSubeventPackets(), 0);
  ASSERT_EQ(packet.GetCrcErrorPackets(), 0);
  ASSERT_EQ(packet.GetRxDuplicatePackets(), 0);
}

void VerifyDefaultV6(const BqrLinkQualityEventV6& packet) {
  VerifyDefaultV5(packet);
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetVersion(), BqrVersion::kNone);
  ASSERT_EQ(packet.GetRxUnreceivedPackets(), 0);
  ASSERT_EQ(packet.GetCoexInfoMask(), 0);
}

void VerifyDefaultV7(const BqrLinkQualityEventV7& packet) {
  VerifyDefaultV6(packet);
}

TEST(BqrLinkQualityEventTest, ValidV3PacketParsing) {
  auto packet = BqrLinkQualityEventV1ToV3(CreateBqrLinkQualityEventV6V5V3());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetVersion(), BqrVersion::kV1ToV3);
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kLeAudioChoppy);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kLinkQuality);

  VerifyV1ToV3(packet);
}

TEST(BqrLinkQualityEventTest, ValidV4PacketParsing) {
  auto packet = BqrLinkQualityEventV4(CreateBqrLinkQualityEventV4());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetVersion(), BqrVersion::kV4);
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kLeAudioChoppy);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kLinkQuality);

  VerifyV1ToV3(packet);
  VerifyV4(packet);
}

TEST(BqrLinkQualityEventTest, ValidV5PacketParsing) {
  auto packet = BqrLinkQualityEventV5(CreateBqrLinkQualityEventV6V5V3());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetVersion(), BqrVersion::kV5);
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kLeAudioChoppy);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kLinkQuality);

  VerifyV1ToV3(packet);
  VerifyV5(packet);
}

TEST(BqrLinkQualityEventTest, ValidV6PacketParsing) {
  auto packet = BqrLinkQualityEventV6(CreateBqrLinkQualityEventV6V5V3());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetVersion(), BqrVersion::kV6);
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kLeAudioChoppy);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kLinkQuality);

  VerifyV1ToV3(packet);
  VerifyV5(packet);

  // Verify V6 specific fields
  ASSERT_EQ(packet.GetRxUnreceivedPackets(), 0x00000000);
  ASSERT_EQ(packet.GetCoexInfoMask(), 0x0008);
}

TEST(BqrLinkQualityEventTest, InvalidPacketParsingIncorrectFormat) {
  auto packet_v3 = BqrLinkQualityEventV1ToV3(CreateIncorrectBqrHalPacket());
  auto packet_v4 = BqrLinkQualityEventV4(CreateIncorrectBqrHalPacket());
  auto packet_v5 = BqrLinkQualityEventV5(CreateIncorrectBqrHalPacket());
  auto packet_v6 = BqrLinkQualityEventV6(CreateIncorrectBqrHalPacket());
  auto packet_v7 = BqrLinkQualityEventV7(CreateIncorrectBqrHalPacket());
  VerifyDefaultV1ToV3(packet_v3);
  VerifyDefaultV4(packet_v4);
  VerifyDefaultV5(packet_v5);
  VerifyDefaultV6(packet_v6);
  VerifyDefaultV7(packet_v7);
}

TEST(BqrLinkQualityEventTest, InvalidPacketParsingPacketTooShort) {
  auto packet_v3 = BqrLinkQualityEventV1ToV3(CreateShortBqrPacket());
  auto packet_v4 = BqrLinkQualityEventV4(CreateShortBqrPacket());
  auto packet_v5 = BqrLinkQualityEventV5(CreateShortBqrPacket());
  auto packet_v6 = BqrLinkQualityEventV6(CreateShortBqrPacket());
  auto packet_v7 = BqrLinkQualityEventV7(CreateShortBqrPacket());
  VerifyDefaultV1ToV3(packet_v3);
  VerifyDefaultV4(packet_v4);
  VerifyDefaultV5(packet_v5);
  VerifyDefaultV6(packet_v6);
  VerifyDefaultV7(packet_v7);
}

TEST(BqrLinkQualityEventTest, InvalidPacketParsingWrongReportId) {
  auto packet_v3 = BqrLinkQualityEventV1ToV3(CreateWrongReportIdPacket());
  auto packet_v4 = BqrLinkQualityEventV4(CreateWrongReportIdPacket());
  auto packet_v5 = BqrLinkQualityEventV5(CreateWrongReportIdPacket());
  auto packet_v6 = BqrLinkQualityEventV6(CreateWrongReportIdPacket());
  auto packet_v7 = BqrLinkQualityEventV7(CreateWrongReportIdPacket());
  VerifyDefaultV1ToV3(packet_v3);
  VerifyDefaultV4(packet_v4);
  VerifyDefaultV5(packet_v5);
  VerifyDefaultV6(packet_v6);
  VerifyDefaultV7(packet_v7);
}

}  // namespace
}  // namespace bqr
}  // namespace bluetooth_hal
