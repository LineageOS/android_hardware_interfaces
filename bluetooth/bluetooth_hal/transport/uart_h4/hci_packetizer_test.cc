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

#include "bluetooth_hal/transport/uart_h4/hci_packetizer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/common/test_helper.h"
#include "bluetooth_hal/test/mock/mock_bluetooth_activities.h"
#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace transport {
namespace {

using ::testing::_;
using ::testing::Return;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

using ::bluetooth_hal::config::MockHalConfigLoader;
using ::bluetooth_hal::debug::MockBluetoothActivities;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HalPacketCallback;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::util::MatcherFactory;
using ::bluetooth_hal::util::MockPacketHandler;

struct HciPacketTestParam {
  HciPacketType packet_type;
  std::vector<uint8_t> preamble;
  std::vector<uint8_t> payload;
};

std::vector<uint8_t> GenerateStreamWithHciPacket(
    const HciPacketTestParam& param) {
  const auto& [packet_type, preamble, payload] = param;

  std::vector<uint8_t> data_stream;
  data_stream.reserve(1 + preamble.size() + payload.size());

  data_stream.push_back(static_cast<uint8_t>(packet_type));
  data_stream.insert(data_stream.end(), preamble.begin(), preamble.end());
  data_stream.insert(data_stream.end(), payload.begin(), payload.end());

  return data_stream;
}

class HciPacketizerTest : public Test {
 protected:
  void SetUp() override {
    test_hci_packetizer_ = std::make_unique<HciPacketizer>(std::bind_front(
        &MockPacketHandler::HalPacketCallback, &mock_packet_handler_));
    MockHalConfigLoader::SetMockLoader(&mock_hal_config_loader_);
    MockBluetoothActivities::SetMockBluetoothActivities(
        &mock_bluetooth_activities_);
  }

  void TearDown() override {
    MockBluetoothActivities::SetMockBluetoothActivities(nullptr);
  }

  void ProcessDataStream(std::span<const uint8_t> data_stream,
                         size_t chunk_size = 0) {
    const size_t total_size = data_stream.size();

    // Does not split packet into several small chunks.
    if (!chunk_size) {
      chunk_size = total_size;
    }

    while (data_stream.size()) {
      size_t cur_chunk_size = std::min(data_stream.size(), chunk_size);
      const size_t cur_bytes_read = test_hci_packetizer_->ProcessData(
          data_stream.subspan(0, cur_chunk_size));
      if (!cur_bytes_read) {
        break;
      }
      data_stream = data_stream.subspan(cur_bytes_read);
    }
  }

  std::unique_ptr<HciPacketizer> test_hci_packetizer_;
  MockPacketHandler mock_packet_handler_;
  MockHalConfigLoader mock_hal_config_loader_;
  MockBluetoothActivities mock_bluetooth_activities_;
};

class LargePacketTest : public HciPacketizerTest,
                        public WithParamInterface<int> {};

TEST_P(LargePacketTest, HandleLargeSizePacket) {
  // Payload length is 243.
  const int chunk_size = GetParam();
  HciPacketTestParam test_param = {.packet_type = HciPacketType::kAclData,
                                   .preamble = {0x00, 0x00, 0xF3, 0x00},
                                   std::vector<uint8_t>(243)};

  const std::vector<uint8_t> data_stream =
      GenerateStreamWithHciPacket(test_param);

  EXPECT_CALL(mock_packet_handler_,
              HalPacketCallback(MatcherFactory::CreateHalPacketMatcher(
                  HalPacket(data_stream))))
      .Times(1);

  ProcessDataStream(std::span(data_stream), chunk_size);
}

INSTANTIATE_TEST_SUITE_P(ChunkSizeTest, LargePacketTest,
                         Values(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

class HciPacketizerParameterizedTest
    : public HciPacketizerTest,
      public WithParamInterface<HciPacketTestParam> {};

TEST_P(HciPacketizerParameterizedTest, HandlePacket) {
  const std::vector<uint8_t> data_stream =
      GenerateStreamWithHciPacket(GetParam());

  EXPECT_CALL(mock_packet_handler_,
              HalPacketCallback(MatcherFactory::CreateHalPacketMatcher(
                  HalPacket(data_stream))))
      .Times(1);

  ProcessDataStream(std::span(data_stream));
}

INSTANTIATE_TEST_SUITE_P(
    HciPacketTests, HciPacketizerParameterizedTest,
    Values(
        HciPacketTestParam{.packet_type = HciPacketType::kCommand,
                           .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03},
                           .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{
            .packet_type = HciPacketType::kAclData,
            .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03, 0x00},
            .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{.packet_type = HciPacketType::kScoData,
                           .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03},
                           .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{.packet_type = HciPacketType::kEvent,
                           .preamble = std::vector<uint8_t>{0x00, 0x03},
                           .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{
            .packet_type = HciPacketType::kIsoData,
            .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03, 0x00},
            .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{
            .packet_type = HciPacketType::kThreadData,
            .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03, 0x00},
            .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}}));

TEST_F(HciPacketizerTest, PacketFoundWithEnhancedValidationOn) {
  EXPECT_CALL(mock_hal_config_loader_, IsEnhancedPacketValidationSupported())
      .WillRepeatedly(Return(true));

  std::vector<uint8_t> valid_packet =
      GenerateStreamWithHciPacket(HciPacketTestParam{
          .packet_type = HciPacketType::kEvent,
          .preamble = std::vector<uint8_t>{0x05, 0x04},
          .payload = std::vector<uint8_t>{0x00, 0x0C, 0x00, 0x00}});

  std::vector<uint8_t> data_stream = {0xFF, 0xFF};
  data_stream.insert(data_stream.end(), valid_packet.begin(),
                     valid_packet.end());

  EXPECT_CALL(mock_packet_handler_,
              HalPacketCallback(MatcherFactory::CreateHalPacketMatcher(
                  HalPacket(valid_packet))))
      .Times(1);
  ProcessDataStream(std::span(data_stream));
}

TEST_F(HciPacketizerTest, PacketNotFoundWithEnhancedValidationOn) {
  EXPECT_CALL(mock_hal_config_loader_, IsEnhancedPacketValidationSupported())
      .WillRepeatedly(Return(true));

  const std::vector<uint8_t> data_stream = {0xFF, 0xFF, 0xAA, 0xBB, 0xCC};

  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);
  ProcessDataStream(std::span(data_stream));
}

TEST_F(HciPacketizerTest, EnhancedValidationOffWithValidPacket) {
  EXPECT_CALL(mock_hal_config_loader_, IsEnhancedPacketValidationSupported())
      .WillRepeatedly(Return(false));

  std::vector<uint8_t> valid_packet = GenerateStreamWithHciPacket(
      HciPacketTestParam{.packet_type = HciPacketType::kEvent,
                         .preamble = std::vector<uint8_t>{0x3e, 0x13},
                         .payload = std::vector<uint8_t>{0x01, 0x00}});

  std::vector<uint8_t> data_stream = {0xFF, 0xFF};
  data_stream.insert(data_stream.end(), valid_packet.begin(),
                     valid_packet.end());

  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);
  ProcessDataStream(std::span(data_stream));
}

TEST_F(HciPacketizerTest, EnhancedValidationOffWithoutValidPacket) {
  EXPECT_CALL(mock_hal_config_loader_, IsEnhancedPacketValidationSupported())
      .WillRepeatedly(Return(false));

  const std::vector<uint8_t> data_stream = {0xFF, 0xFF, 0xAA, 0xBB, 0xCC};

  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);
  ProcessDataStream(std::span(data_stream));
}

}  // namespace
}  // namespace transport
}  // namespace bluetooth_hal
