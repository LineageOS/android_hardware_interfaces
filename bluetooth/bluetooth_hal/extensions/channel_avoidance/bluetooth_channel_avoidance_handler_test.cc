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

#include "bluetooth_hal/extensions/channel_avoidance/bluetooth_channel_avoidance_handler.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "bluetooth_hal/hci_router_client_callback.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace extensions {
namespace channel_avoidance {
namespace {

using ::testing::_;
using ::testing::Action;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::hci::CommandOpCode;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::HciRouterCallback;
using ::bluetooth_hal::hci::HciRouterClientCallback;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;

constexpr uint16_t kTestHciChannelAvoidanceOpcode = 0x0c3f;
constexpr uint8_t kTestHciChannelAvoidanceMapSize = 10;
constexpr int kTestMaxCommandWaitTimeMs = 1000;

class TestableBluetoothChannelAvoidanceHandler
    : public BluetoothChannelAvoidanceHandler {
 public:
  TestableBluetoothChannelAvoidanceHandler() = default;

  HalPacket BuildSetChannelAvoidanceCommandWrapper(
      const std::array<uint8_t, kTestHciChannelAvoidanceMapSize>& channel_map) {
    return BluetoothChannelAvoidanceHandler::BuildSetChannelAvoidanceCommand(
        channel_map);
  }
};

class BluetoothChannelAvoidanceHandlerTest : public Test {
 protected:
  void SetUp() override {
    MockHciRouter::SetMockRouter(&mock_hci_router_);

    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    EXPECT_CALL(mock_hci_router_client_agent_, RegisterClient(NotNull()))
        .WillOnce(
            DoAll(SaveArg<0>(&registered_callback_on_router_), Return(true)));

    handler_ = std::make_unique<TestableBluetoothChannelAvoidanceHandler>();
    ASSERT_NE(handler_, nullptr);
    ASSERT_EQ(registered_callback_on_router_, handler_.get());
  }

  void TearDown() override {
    EXPECT_CALL(mock_hci_router_client_agent_, UnregisterClient(handler_.get()))
        .WillOnce(Return(true));
    handler_.reset();
    MockHciRouter::SetMockRouter(nullptr);
  }

  HalPacket CreateCommandCompleteEvent(uint16_t opcode,
                                       EventResultCode status) {
    std::vector<uint8_t> parameters;
    parameters.push_back(static_cast<uint8_t>(HciPacketType::kCommand));
    parameters.push_back(opcode & 0xFF);
    parameters.push_back((opcode >> 8) & 0xFF);
    parameters.push_back(static_cast<uint8_t>(status));

    std::vector<uint8_t> event_data;
    event_data.push_back(static_cast<uint8_t>(HciPacketType::kEvent));
    event_data.push_back(static_cast<uint8_t>(EventCode::kCommandComplete));
    event_data.push_back(static_cast<uint8_t>(parameters.size()));
    event_data.insert(event_data.end(), parameters.begin(), parameters.end());
    return HalPacket(event_data);
  }

  // Helper to set Bluetooth enabled/disabled state by simulating
  // HciRouterClient behavior.
  void SetBluetoothState(bool enabled) {
    if (enabled) {
      EXPECT_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
          .WillRepeatedly(Return(true));
      EXPECT_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
          .WillRepeatedly(Return(true));
      EXPECT_CALL(mock_hci_router_, GetHalState())
          .WillRepeatedly(Return(HalState::kRunning));

      registered_callback_on_router_->OnBluetoothChipReady();
      registered_callback_on_router_->OnBluetoothEnabled();
    } else {
      EXPECT_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
          .WillRepeatedly(Return(false));
      EXPECT_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
          .WillRepeatedly(Return(false));
      EXPECT_CALL(mock_hci_router_, GetHalState())
          .WillRepeatedly(Return(HalState::kShutdown));

      registered_callback_on_router_->OnBluetoothDisabled();
      registered_callback_on_router_->OnBluetoothChipClosed();
    }
  }

  std::unique_ptr<TestableBluetoothChannelAvoidanceHandler> handler_;
  StrictMock<MockHciRouter> mock_hci_router_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
  HciRouterClientCallback* registered_callback_on_router_ = nullptr;

  std::array<uint8_t, kTestHciChannelAvoidanceMapSize> test_channel_map_ = {
      {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A}};
};

TEST_F(BluetoothChannelAvoidanceHandlerTest, BuildCommandCorrectly) {
  HalPacket command =
      handler_->BuildSetChannelAvoidanceCommandWrapper(test_channel_map_);

  ASSERT_EQ(command.GetType(), HciPacketType::kCommand);
  ASSERT_EQ(command.size(), 1u + HciConstants::kHciCommandPreambleSize +
                                kTestHciChannelAvoidanceMapSize);

  uint16_t opcode = command[1] | (command[2] << 8);
  ASSERT_EQ(opcode, kTestHciChannelAvoidanceOpcode);

  uint8_t param_length = command[3];
  ASSERT_EQ(param_length, kTestHciChannelAvoidanceMapSize);

  std::vector<uint8_t> sent_map_data(command.begin() + 4, command.end());
  ASSERT_EQ(sent_map_data.size(), kTestHciChannelAvoidanceMapSize);
  EXPECT_THAT(sent_map_data, ElementsAreArray(test_channel_map_));
}

TEST_F(BluetoothChannelAvoidanceHandlerTest,
       SetStatusWhenBluetoothDisabledReturnsFalse) {
  SetBluetoothState(false);
  ASSERT_FALSE(handler_->SetBluetoothChannelStatus(test_channel_map_));
}

TEST_F(BluetoothChannelAvoidanceHandlerTest,
       SetStatusWhenSendCommandFailsReturnsFalse) {
  SetBluetoothState(true);
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).WillOnce(Return(false));

  ASSERT_FALSE(handler_->SetBluetoothChannelStatus(test_channel_map_));
}

TEST_F(BluetoothChannelAvoidanceHandlerTest,
       SetStatusCommandTimeoutReturnsFalse) {
  SetBluetoothState(true);
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).WillOnce(Return(true));

  ASSERT_FALSE(handler_->SetBluetoothChannelStatus(test_channel_map_));
}

struct SetStatusTestParams {
  EventResultCode command_complete_status;
  bool expected_return_value;
  std::string test_name_suffix;
};

// Helper Action to signal when SendCommand is called.
ACTION_P(SignalPromise, p) {
  p->set_value();
  return true;
}

class BluetoothChannelAvoidanceHandlerSetStatusTest
    : public BluetoothChannelAvoidanceHandlerTest,
      public WithParamInterface<SetStatusTestParams> {};

TEST_P(BluetoothChannelAvoidanceHandlerSetStatusTest, SetStatusAndVerify) {
  SetBluetoothState(true);
  const auto& params = GetParam();

  std::promise<void> command_sent_promise;
  std::future<void> command_sent_future = command_sent_promise.get_future();

  HalPacket expected_command_packet =
      handler_->BuildSetChannelAvoidanceCommandWrapper(test_channel_map_);

  EXPECT_CALL(mock_hci_router_, SendCommand(expected_command_packet, _))
      .WillOnce(SignalPromise(&command_sent_promise));

  auto result_future = std::async(std::launch::async, [&]() {
    return handler_->SetBluetoothChannelStatus(test_channel_map_);
  });

  ASSERT_EQ(command_sent_future.wait_for(
                std::chrono::milliseconds(kTestMaxCommandWaitTimeMs)),
            std::future_status::ready)
      << "SendCommand was not called or timed out waiting for it.";

  if (params.test_name_suffix != "Timeout") {
    HalPacket complete_event = CreateCommandCompleteEvent(
        kTestHciChannelAvoidanceOpcode, params.command_complete_status);
    registered_callback_on_router_->OnCommandCallback(complete_event);
  }

  std::chrono::milliseconds wait_duration =
      (params.test_name_suffix == "Timeout")
          ? std::chrono::milliseconds(kTestMaxCommandWaitTimeMs + 200)
          : std::chrono::milliseconds(kTestMaxCommandWaitTimeMs);

  ASSERT_EQ(result_future.wait_for(wait_duration), std::future_status::ready)
      << "SetBluetoothChannelStatus did not return as expected for test case: "
      << params.test_name_suffix;
  EXPECT_EQ(result_future.get(), params.expected_return_value);
}

INSTANTIATE_TEST_SUITE_P(
    SetStatusCommandResults, BluetoothChannelAvoidanceHandlerSetStatusTest,
    Values(SetStatusTestParams{EventResultCode::kSuccess, true, "Success"},
           SetStatusTestParams{EventResultCode::kFailure, false, "Failure"},
           SetStatusTestParams{EventResultCode::kFailure, false, "Timeout"}));

}  // namespace
}  // namespace channel_avoidance
}  // namespace extensions
}  // namespace bluetooth_hal
