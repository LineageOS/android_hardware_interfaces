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

#include "bluetooth_hal/bqr/bqr_event.h"

#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

class TestBqrEvent : public BqrEvent {
 public:
  TestBqrEvent(const HalPacket& packet) : BqrEvent(packet) {}

  bool IsValid() const override { return BqrEvent::IsValid(); }
};

HalPacket CreateBqrHalPacket() {
  return HalPacket({0x04, 0xFF, 0x05,
                    // BQR event code
                    0x58,
                    // MonitorMode
                    0x01,
                    // Random payload
                    0x02, 0x03, 0x04});
}

HalPacket CreateIncorrectBqrHalPacket() {
  return HalPacket({0x01, 0x02, 0x03, 0x04, 0x05});
}

HalPacket empty_packet_;

TEST(BqrEventTest, ValidPacketParsing) {
  auto packet = TestBqrEvent(CreateBqrHalPacket());
  ASSERT_TRUE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kMonitorMode);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kLinkQuality);
}

TEST(BqrEventTest, InvalidPacketParsing) {
  auto packet = TestBqrEvent(CreateIncorrectBqrHalPacket());
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kNone);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kNone);
}

TEST(BqrEventTest, EmptyPacketParsing) {
  TestBqrEvent packet(empty_packet_);
  ASSERT_FALSE(packet.IsValid());
  ASSERT_EQ(packet.GetBqrReportId(), BqrReportId::kNone);
  ASSERT_EQ(packet.GetBqrEventType(), BqrEventType::kNone);
}

TEST(BqrEventTest, HandleGetBqrEventTypeFromReportId) {
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kNone),
            BqrEventType::kNone);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kMonitorMode),
            BqrEventType::kLinkQuality);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kApproachLsto),
            BqrEventType::kLinkQuality);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kA2dpAudioChoppy),
            BqrEventType::kLinkQuality);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kScoVoiceChoppy),
            BqrEventType::kLinkQuality);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kLeAudioChoppy),
            BqrEventType::kLinkQuality);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kConnectFail),
            BqrEventType::kLinkQuality);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kRootInflammation),
            BqrEventType::kRootInflammation);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kEnergyMonitoring),
            BqrEventType::kEnergyMonitoring);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kAdvanceRfStats),
            BqrEventType::kAdvancedRfStat);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kAdvanceRfStatsPeriodic),
            BqrEventType::kAdvancedRfStat);
  ASSERT_EQ(GetBqrEventTypeFromReportId(BqrReportId::kControllerHealthMonitor),
            BqrEventType::kControllerHealthMonitor);
  ASSERT_EQ(GetBqrEventTypeFromReportId(
                BqrReportId::kControllerHealthMonitorPeriodic),
            BqrEventType::kControllerHealthMonitor);
  ASSERT_EQ(GetBqrEventTypeFromReportId(static_cast<BqrReportId>(0xFF)),
            BqrEventType::kNone);
}

}  // namespace
}  // namespace bqr
}  // namespace bluetooth_hal
