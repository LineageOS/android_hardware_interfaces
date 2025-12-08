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

#include "bluetooth_hal/transport/hci_packet_rescuer.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/mock/mock_bluetooth_activities.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace transport {
namespace {

using ::testing::Return;
using ::testing::Test;
using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

using ::bluetooth_hal::debug::MockBluetoothActivities;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::HciPacketType;

struct HciPacketRescuerTestParam {
  std::string test_name;
  std::vector<uint8_t> data_stream;
  size_t expected_offset;
  std::function<void(MockBluetoothActivities&)> mock_setup =
      [](MockBluetoothActivities&) {};
};

class HciPacketRescuerTest : public Test {
 protected:
  void SetUp() override {
    hci_packet_rescuer_ = std::make_unique<HciPacketRescuer>();
    MockBluetoothActivities::SetMockBluetoothActivities(
        &mock_bluetooth_activities_);
  }

  void TearDown() override {
    hci_packet_rescuer_.reset();
    MockBluetoothActivities::SetMockBluetoothActivities(nullptr);
  }

  std::unique_ptr<HciPacketRescuer> hci_packet_rescuer_;
  MockBluetoothActivities mock_bluetooth_activities_;
};

class HciPacketRescuerParamTest
    : public HciPacketRescuerTest,
      public WithParamInterface<HciPacketRescuerTestParam> {};

TEST_P(HciPacketRescuerParamTest, FindValidPacketOffset) {
  const auto& param = GetParam();
  param.mock_setup(mock_bluetooth_activities_);

  size_t offset =
      hci_packet_rescuer_->FindValidPacketOffset(std::span(param.data_stream));

  EXPECT_EQ(offset, param.expected_offset) << "Test: " << param.test_name;
}

INSTANTIATE_TEST_SUITE_P(
    HciPacketRescuerTests, HciPacketRescuerParamTest,
    Values(
        HciPacketRescuerTestParam{.test_name = "EmptyStream",
                                  .data_stream = {},
                                  .expected_offset = 0},
        HciPacketRescuerTestParam{.test_name = "NoValidPacketType",
                                  .data_stream = {0xAA, 0xBB, 0xCC},
                                  .expected_offset = 3},
        HciPacketRescuerTestParam{
            .test_name = "ValidAclPacketAtStart",
            .data_stream = {static_cast<uint8_t>(HciPacketType::kAclData), 0x23,
                            0x01},
            .expected_offset = 0,
            .mock_setup =
                [](MockBluetoothActivities& mock) {
                  EXPECT_CALL(mock, IsConnected(0x0123)).WillOnce(Return(true));
                }},
        HciPacketRescuerTestParam{
            .test_name = "ValidAclPacketWithOffset",
            .data_stream = {0xFF, 0xFF,
                            static_cast<uint8_t>(HciPacketType::kAclData), 0x23,
                            0x01},
            .expected_offset = 2,
            .mock_setup =
                [](MockBluetoothActivities& mock) {
                  EXPECT_CALL(mock, IsConnected(0x0123)).WillOnce(Return(true));
                }},
        HciPacketRescuerTestParam{
            .test_name = "AclPacketWhenNotConnected",
            .data_stream = {static_cast<uint8_t>(HciPacketType::kAclData), 0x23,
                            0x01},
            .expected_offset = 3,
            .mock_setup =
                [](MockBluetoothActivities& mock) {
                  EXPECT_CALL(mock, IsConnected(0x0123))
                      .WillOnce(Return(false));
                }},
        HciPacketRescuerTestParam{
            .test_name = "AclPacketTooShort",
            .data_stream = {static_cast<uint8_t>(HciPacketType::kAclData),
                            0x23},
            .expected_offset = 2},
        HciPacketRescuerTestParam{
            .test_name = "ValidThreadPacketAtStart",
            .data_stream = {static_cast<uint8_t>(HciPacketType::kThreadData),
                            0x00, 0x00, 0x01, 0x02, 0x85},
            .expected_offset = 0},
        HciPacketRescuerTestParam{
            .test_name = "ValidThreadPacketWithOffset",
            .data_stream = {0xFF, 0xFA, 0xFB, 0xFC,
                            static_cast<uint8_t>(HciPacketType::kThreadData),
                            0x00, 0x00, 0x01, 0x02, 0x80},
            .expected_offset = 4},
        HciPacketRescuerTestParam{
            .test_name = "InvalidThreadPacket",
            .data_stream = {static_cast<uint8_t>(HciPacketType::kThreadData),
                            0x01, 0x00, 0x01, 0x02, 0x85},
            .expected_offset = 6},
        HciPacketRescuerTestParam{
            .test_name = "ValidEventPacketAtStart",
            .data_stream = {static_cast<uint8_t>(HciPacketType::kEvent),
                            static_cast<uint8_t>(EventCode::kCommandComplete),
                            0x05, 0x01, 0x02, 0x03, 0x04, 0x05},
            .expected_offset = 0},
        HciPacketRescuerTestParam{
            .test_name = "ValidEventPacketWithOffset",
            .data_stream = {0xAB, 0xCD, 0xEF, 0xFF, 0xAA, 0xBB,
                            static_cast<uint8_t>(HciPacketType::kEvent),
                            static_cast<uint8_t>(EventCode::kCommandComplete),
                            0x05, 0x01, 0x02, 0x03, 0x04, 0x05},
            .expected_offset = 6},
        HciPacketRescuerTestParam{
            .test_name = "InvalidEventPacket",
            .data_stream = {static_cast<uint8_t>(HciPacketType::kEvent), 0x01,
                            0x01},
            .expected_offset = 3}),
    [](const TestParamInfo<HciPacketRescuerParamTest::ParamType>& info) {
      return info.param.test_name;
    });

}  // namespace
}  // namespace transport
}  // namespace bluetooth_hal
