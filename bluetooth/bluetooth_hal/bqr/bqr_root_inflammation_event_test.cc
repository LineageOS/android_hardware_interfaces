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

#include "bluetooth_hal/bqr/bqr_root_inflammation_event.h"

#include <cstdint>
#include <vector>

#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr uint8_t kSampleErrorCode = 0xAA;
constexpr uint8_t kSampleVendorErrorCode = 0xBB;

HalPacket CreateRootInflammationEvent() {
  std::vector<uint8_t> data = {
      0x04,                    // H4 Type: HCI Event
      0xff,                    // Event Code: Vendor Specific Event (0xFF)
      0xf6,                    // Parameter Total Length
      0x58,                    // Sub Event: BQR event (0x58)
      0x05,                    // Report ID: Root inflammation (0x05)
      kSampleErrorCode,        // Error Code
      kSampleVendorErrorCode,  // Vendor Error Code
      0x01,
      0x02,
      0x03  // Random vendor parameters
  };
  return HalPacket(data);
}

HalPacket CreateIncorrectBqrHalPacket() {
  return HalPacket({0x01, 0x02, 0x03, 0x04, 0x05});
}

HalPacket CreateShortBqrPacket() {
  std::vector<uint8_t> data = {
      0x04, 0xff, 0x4e, 0x58,
      0x05,  // Report ID: kRootInflammation (0x05)
  };
  return HalPacket(data);
}

TEST(BqrRootInflammationTest, ValidPacketParsing) {
  auto packet = BqrRootInflammationEvent(CreateRootInflammationEvent());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kRootInflammation);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kRootInflammation);

  ASSERT_EQ(packet.GetErrorCode(), kSampleErrorCode);
  ASSERT_EQ(packet.GetVendorErrorCode(), kSampleVendorErrorCode);

  std::vector<uint8_t> expected_vendor_param = {0x01, 0x02, 0x03};
  ASSERT_EQ(packet.GetVendorParameter(), expected_vendor_param);
}

TEST(BqrRootInflammationTest, InvalidPacketParsingIncorrectFormat) {
  auto packet = BqrRootInflammationEvent(CreateIncorrectBqrHalPacket());
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kNone);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kNone);

  ASSERT_EQ(packet.GetErrorCode(), 0);
  ASSERT_EQ(packet.GetVendorErrorCode(), 0);

  std::vector<uint8_t> expected_vendor_param = {};
  ASSERT_EQ(packet.GetVendorParameter(), expected_vendor_param);
}

TEST(BqrRootInflammationTest, InvalidPacketParsingPacketTooShort) {
  auto packet = BqrRootInflammationEvent(CreateShortBqrPacket());
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kRootInflammation);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kRootInflammation);

  ASSERT_EQ(packet.GetErrorCode(), 0);
  ASSERT_EQ(packet.GetVendorErrorCode(), 0);

  std::vector<uint8_t> expected_vendor_param = {};
  ASSERT_EQ(packet.GetVendorParameter(), expected_vendor_param);
}

}  // namespace
}  // namespace bqr
}  // namespace bluetooth_hal
