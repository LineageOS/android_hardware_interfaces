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

#include "bluetooth_hal/transport/uart_h4/data_processor.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "bluetooth_hal/debug/debug_types.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/common/test_helper.h"
#include "bluetooth_hal/test/mock/mock_debug_central.h"
#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"
#include "bluetooth_hal/test/mock/mock_system_call_wrapper.h"
#include "com_android_bluetooth_bluetooth_hal_flags.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace transport {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

using ::bluetooth_hal::config::MockHalConfigLoader;
using ::bluetooth_hal::debug::CoredumpErrorCode;
using ::bluetooth_hal::debug::MockDebugCentral;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HalPacketCallback;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::util::MatcherFactory;
using ::bluetooth_hal::util::MockPacketHandler;
using ::bluetooth_hal::util::MockSystemCallWrapper;

class DataProcessorTest : public Test {
 protected:
  void SetUp() override {
    MockSystemCallWrapper::SetMockWrapper(&mock_system_call_wrapper_);
    MockHalConfigLoader::SetMockLoader(&mock_hal_config_loader_);
    MockDebugCentral::SetMockDebugCentral(&mock_debug_central_);
    data_processor_ = std::make_unique<DataProcessor>(
        test_fd_, std::bind_front(&MockPacketHandler::HalPacketCallback,
                                  &mock_packet_handler_));
  }

  void TearDown() override {}

  std::unique_ptr<DataProcessor> data_processor_;
  MockSystemCallWrapper mock_system_call_wrapper_;
  MockPacketHandler mock_packet_handler_;
  MockHalConfigLoader mock_hal_config_loader_;
  MockDebugCentral mock_debug_central_;
  int test_fd_ = 1;
};

TEST_F(DataProcessorTest, SendEmptyPacket) {
  const std::vector<uint8_t> empty_packet;
  EXPECT_EQ(data_processor_->Send(empty_packet), 0);
}

TEST_F(DataProcessorTest, SendPacketAtOnceReturnSuccess) {
  const std::vector<uint8_t> packet = {0x00, 0x01, 0x02, 0x03};
  EXPECT_CALL(mock_system_call_wrapper_, Writev(test_fd_, _, _))
      .WillOnce(Return(packet.size()));

  EXPECT_EQ(data_processor_->Send(packet), packet.size());
}

TEST_F(DataProcessorTest, SendPacketReturnFailWithSystemFailure) {
  const std::vector<uint8_t> packet = {0x00, 0x01, 0x02, 0x03};
  EXPECT_CALL(mock_system_call_wrapper_, Writev(test_fd_, _, _))
      .WillOnce(Return(-1));

  EXPECT_EQ(data_processor_->Send(packet), 0);
}

TEST_F(DataProcessorTest, SendPacketReturnFailWithEmptyWritten) {
  const std::vector<uint8_t> packet = {0x00, 0x01, 0x02, 0x03};
  EXPECT_CALL(mock_system_call_wrapper_, Writev(test_fd_, _, _))
      .WillOnce(Return(0));

  EXPECT_EQ(data_processor_->Send(packet), 0);
}

TEST_F(DataProcessorTest, SendPacketInMultiplePiecesReturnSuccess) {
  const std::vector<uint8_t> packet = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
  EXPECT_CALL(mock_system_call_wrapper_, Writev(test_fd_, _, _))
      .WillOnce(Return(2))
      .WillOnce(Return(2))
      .WillOnce(Return(2));

  EXPECT_EQ(data_processor_->Send(packet), packet.size());
}

TEST_F(DataProcessorTest, ReadDataWithSystemFailureNoCallbackInvoked) {
  ON_CALL(mock_system_call_wrapper_, Read(test_fd_, _, _))
      .WillByDefault(Return(-1));
  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);

  EXPECT_DEATH(data_processor_->Recv(test_fd_), "");
}

TEST_F(DataProcessorTest,
       ReadDataReturnFailWithConnectionClosedNoCallbackInvoked) {
  EXPECT_CALL(mock_system_call_wrapper_, Read(_, _, _)).WillOnce(Return(0));
  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);

  data_processor_->Recv(test_fd_);
}

TEST_F(DataProcessorTest, ReadInavlidHciPacketNoCallbackInvoked) {
  set_com_android_bluetooth_bluetooth_hal_flags_coredump_when_receiving_unimplemented_packet_type(
      false);

  std::vector<uint8_t> packet = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
  ON_CALL(mock_system_call_wrapper_, Read(test_fd_, _, _))
      .WillByDefault(DoAll(
          Invoke([&]([[maybe_unused]] int fd, [[maybe_unused]] void* buffer,
                     [[maybe_unused]] size_t count) {
            buffer = static_cast<void*>(packet.data());
          }),
          Return(6)));
  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);

  EXPECT_DEATH(data_processor_->Recv(test_fd_), "");
}

TEST_F(DataProcessorTest,
       ReadInavlidHciPacketNoCallbackInvokedWithCoredumpFeatureFlagEnabled) {
  set_com_android_bluetooth_bluetooth_hal_flags_coredump_when_receiving_unimplemented_packet_type(
      true);

  std::vector<uint8_t> packet = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
  ON_CALL(mock_system_call_wrapper_, Read(test_fd_, _, _))
      .WillByDefault(DoAll(
          Invoke([&]([[maybe_unused]] int fd, [[maybe_unused]] void* buffer,
                     [[maybe_unused]] size_t count) {
            buffer = static_cast<void*>(packet.data());
          }),
          Return(6)));
  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);
  EXPECT_CALL(mock_debug_central_,
              GenerateCoredump(
                  CoredumpErrorCode::kControllerUnimplementedPacketType, _))
      .Times(1);
  data_processor_->Recv(test_fd_);
}

struct HciPacketTestParam {
  HciPacketType type;
  std::vector<uint8_t> preamble;
  std::vector<uint8_t> payload;
};

class HciPacketTest : public DataProcessorTest,
                      public WithParamInterface<HciPacketTestParam> {};

TEST_P(HciPacketTest, ReadValidHciPacketCallbackInvoked) {
  set_com_android_bluetooth_bluetooth_hal_flags_coredump_when_receiving_unimplemented_packet_type(
      true);

  const auto& [type, preamble, payload] = GetParam();
  std::vector<uint8_t> test_buffer;
  test_buffer.push_back(static_cast<uint8_t>(type));
  test_buffer.insert(test_buffer.end(), preamble.begin(), preamble.end());
  test_buffer.insert(test_buffer.end(), payload.begin(), payload.end());
  EXPECT_CALL(mock_system_call_wrapper_, Read(test_fd_, _, _))
      .WillOnce(DoAll(Invoke([&]([[maybe_unused]] int fd, void* buffer,
                                 [[maybe_unused]] size_t count) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        memcpy(buffer, test_buffer.data(), test_buffer.size());
                      }),
                      Return(test_buffer.size())));
  HalPacket hal_packet(test_buffer);
  EXPECT_CALL(
      mock_packet_handler_,
      HalPacketCallback(MatcherFactory::CreateHalPacketMatcher(hal_packet)))
      .Times(1);
  EXPECT_CALL(mock_debug_central_,
              GenerateCoredump(
                  CoredumpErrorCode::kControllerUnimplementedPacketType, _))
      .Times(0);

  data_processor_->Recv(test_fd_);
}

INSTANTIATE_TEST_SUITE_P(
    HandleValidPacketFromFd, HciPacketTest,
    Values(
        HciPacketTestParam{
            .type = HciPacketType::kAclData,
            .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03, 0x00},
            .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{.type = HciPacketType::kScoData,
                           .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03},
                           .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{.type = HciPacketType::kEvent,
                           .preamble = std::vector<uint8_t>{0x00, 0x03},
                           .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{
            .type = HciPacketType::kIsoData,
            .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03, 0x00},
            .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}},
        HciPacketTestParam{
            .type = HciPacketType::kThreadData,
            .preamble = std::vector<uint8_t>{0x00, 0x00, 0x03, 0x00},
            .payload = std::vector<uint8_t>{0x01, 0x02, 0x03}}));

}  // namespace
}  // namespace transport
}  // namespace bluetooth_hal
