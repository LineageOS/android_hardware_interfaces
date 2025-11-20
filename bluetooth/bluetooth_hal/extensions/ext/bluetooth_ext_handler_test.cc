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

#include "bluetooth_hal/extensions/ext/bluetooth_ext_handler.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <memory>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace extensions {
namespace ext {
namespace {

using ::testing::_;
using ::testing::Action;
using ::testing::DoAll;
using ::testing::Mock;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;
using ::testing::Test;

using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::HciRouterCallback;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;
using ::bluetooth_hal::hci::MonitorMode;

// Helper Action to signal when SendCommand is called.
ACTION_P(SignalPromise, p) { p->set_value(); }

// Helper Action to save HalPacket argument.
ACTION_P(SaveHalPacketArg, ptr) {
  *ptr = arg0;  // Assuming arg0 is the HalPacket.
}

class BluetoothExtHandlerTest : public Test {
 protected:
  void SetUp() override {
    MockHciRouter::SetMockRouter(&mock_hci_router_);
    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    EXPECT_CALL(mock_hci_router_client_agent_, RegisterRouterClient(NotNull()))
        .WillOnce(DoAll(SaveArg<0>(&router_callback_), Return(true)));

    handler_ = std::make_unique<BluetoothExtHandler>();

    ON_CALL(mock_hci_router_, SendCommand(_, _)).WillByDefault(Return(true));
  }

  void TearDown() override {
    EXPECT_CALL(mock_hci_router_client_agent_,
                UnregisterRouterClient(handler_.get()))
        .WillOnce(Return(true));
    handler_.reset();
    MockHciRouter::SetMockRouter(nullptr);
  }

  HalPacket CreateCommandCompleteEvent(uint16_t opcode,
                                       EventResultCode status) {
    HalPacket event_packet({
        static_cast<uint8_t>(HciPacketType::kEvent),
        static_cast<uint8_t>(EventCode::kCommandComplete),
        0x04,  // Parameter Total Length (Num Packets(1) + OpCode(2) + Status(1)
               // = 4)
        0x01,  // Number of HCI command packets allowed to be sent.
        static_cast<uint8_t>(opcode & 0xff),
        static_cast<uint8_t>((opcode >> 8) & 0xff),
        static_cast<uint8_t>(status),
    });
    return event_packet;
  }

  // Helper to build expected command packet
  HalPacket BuildExpectedCommand(uint16_t opcode,
                                 const std::vector<uint8_t>& params) {
    HalPacket expected_cmd;
    uint8_t params_len = params.size();
    uint8_t cmd_length = HciConstants::kHciCommandPreambleSize + params_len;
    expected_cmd.resize(1 + cmd_length);  // Type + Preamble + Params

    expected_cmd[0] = static_cast<uint8_t>(HciPacketType::kCommand);
    expected_cmd[1] = opcode & 0xff;
    expected_cmd[2] = (opcode >> 8u) & 0xff;
    expected_cmd[3] = params_len;
    if (params_len > 0) {
      memcpy(expected_cmd.data() + 1 + HciConstants::kHciCommandPreambleSize,
             params.data(), params_len);
    }
    return expected_cmd;
  }

  std::unique_ptr<BluetoothExtHandler> handler_;
  HciRouterCallback* router_callback_ = nullptr;
  StrictMock<MockHciRouter> mock_hci_router_;
  StrictMock<MockHciRouterClientAgent> mock_hci_router_client_agent_;
};

TEST_F(BluetoothExtHandlerTest, SetBluetoothCmdPacketReturnSuccess) {
  ASSERT_NE(router_callback_, nullptr);
  const uint16_t test_opcode = 0xfc01;
  const std::vector<uint8_t> test_params = {0x01, 0x02, 0x03};
  bool result = false;

  HalPacket expected_cmd = BuildExpectedCommand(test_opcode, test_params);
  HalPacket sent_cmd;  // To capture the actual sent command

  std::promise<void> cmd_sent_promise;
  std::future<void> cmd_sent_future = cmd_sent_promise.get_future();

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _))
      .WillOnce(DoAll(SaveHalPacketArg(&sent_cmd),
                      SignalPromise(&cmd_sent_promise), Return(true)));

  auto future_result = std::async(std::launch::async, [&]() {
    return handler_->SetBluetoothCmdPacket(test_opcode, test_params, &result);
  });

  // Wait for SendCommand to be called.
  ASSERT_EQ(cmd_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready)
      << "SendCommand was not called";

  // Verify the sent command packet contents.
  EXPECT_EQ(sent_cmd, expected_cmd);

  // Simulate the success callback.
  HalPacket success_event =
      CreateCommandCompleteEvent(test_opcode, EventResultCode::kSuccess);
  router_callback_->OnCommandCallback(success_event);

  // Wait for SetBluetoothCmdPacket to finish and check results.
  EXPECT_TRUE(
      future_result.get());  // Check the return value of SetBluetoothCmdPacket.
  EXPECT_TRUE(result);       // Check the output parameter 'ret'.
}

TEST_F(BluetoothExtHandlerTest,
       SetBluetoothCmdPacketReturnSuccessWithNoParams) {
  ASSERT_NE(router_callback_, nullptr);
  const uint16_t test_opcode = 0xfc02;
  const std::vector<uint8_t> test_params = {};
  bool result = false;

  HalPacket expected_cmd = BuildExpectedCommand(test_opcode, test_params);
  HalPacket sent_cmd;

  std::promise<void> cmd_sent_promise;
  std::future<void> cmd_sent_future = cmd_sent_promise.get_future();

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _))
      .WillOnce(DoAll(SaveHalPacketArg(&sent_cmd),
                      SignalPromise(&cmd_sent_promise), Return(true)));

  auto future_result = std::async(std::launch::async, [&]() {
    return handler_->SetBluetoothCmdPacket(test_opcode, test_params, &result);
  });

  ASSERT_EQ(cmd_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);

  EXPECT_EQ(sent_cmd, expected_cmd);

  HalPacket success_event =
      CreateCommandCompleteEvent(test_opcode, EventResultCode::kSuccess);
  router_callback_->OnCommandCallback(success_event);

  EXPECT_TRUE(future_result.get());
  EXPECT_TRUE(result);
}

TEST_F(BluetoothExtHandlerTest, SetBluetoothCmdPacketReturnCallbackFailure) {
  ASSERT_NE(router_callback_, nullptr);
  const uint16_t test_opcode = 0xfc03;
  const std::vector<uint8_t> test_params = {0x0a, 0x0b};
  bool result = true;  // Initialize to non-default

  HalPacket expected_cmd = BuildExpectedCommand(test_opcode, test_params);
  HalPacket sent_cmd;

  std::promise<void> cmd_sent_promise;
  std::future<void> cmd_sent_future = cmd_sent_promise.get_future();

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _))
      .WillOnce(DoAll(SaveHalPacketArg(&sent_cmd),
                      SignalPromise(&cmd_sent_promise), Return(true)));

  auto future_result = std::async(std::launch::async, [&]() {
    return handler_->SetBluetoothCmdPacket(test_opcode, test_params, &result);
  });

  ASSERT_EQ(cmd_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);

  EXPECT_EQ(sent_cmd, expected_cmd);

  // Simulate the failure callback.
  HalPacket failure_event =
      CreateCommandCompleteEvent(test_opcode, EventResultCode::kFailure);
  router_callback_->OnCommandCallback(failure_event);

  // Wait for SetBluetoothCmdPacket to finish and check results.
  EXPECT_TRUE(future_result.get());
  EXPECT_FALSE(result);
}

TEST_F(BluetoothExtHandlerTest, SetBluetoothCmdPacketReturnCommandTimeout) {
  ASSERT_NE(router_callback_, nullptr);
  const uint16_t test_opcode = 0xfc04;
  const std::vector<uint8_t> test_params = {0x0c};
  bool result = true;  // Initialize to non-default

  HalPacket expected_cmd = BuildExpectedCommand(test_opcode, test_params);
  HalPacket sent_cmd;

  std::promise<void> cmd_sent_promise;
  std::future<void> cmd_sent_future = cmd_sent_promise.get_future();

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _))
      .WillOnce(DoAll(SaveHalPacketArg(&sent_cmd),
                      SignalPromise(&cmd_sent_promise), Return(true)));

  // Run SetBluetoothCmdPacket. We expect it to block until timeout.
  // kMaxCommandWaitTimeMs is 1000ms.
  auto future_result = std::async(std::launch::async, [&]() {
    return handler_->SetBluetoothCmdPacket(test_opcode, test_params, &result);
  });

  // Wait for SendCommand to be called.
  ASSERT_EQ(cmd_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);

  EXPECT_EQ(sent_cmd, expected_cmd);

  std::future_status status = future_result.wait_for(
      std::chrono::milliseconds(1500));  // Wait > 1000ms.

  ASSERT_EQ(status, std::future_status::ready)
      << "SetBluetoothCmdPacket did not return after expected timeout.";

  // Check results after timeout.
  EXPECT_TRUE(future_result.get());
  EXPECT_FALSE(result);
}

}  // namespace
}  // namespace ext
}  // namespace extensions
}  // namespace bluetooth_hal
