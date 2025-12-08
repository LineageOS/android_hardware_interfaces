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

#include "bluetooth_hal/extensions/finder/bluetooth_finder_handler.h"

#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <vector>

#include "aidl/android/hardware/bluetooth/finder/Eid.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "bluetooth_hal/test/mock/mock_android_base_wrapper.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace extensions {
namespace finder {
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

using ::aidl::android::hardware::bluetooth::finder::Eid;
using ::bluetooth_hal::HalState;
using ::bluetooth_hal::Property;
using ::bluetooth_hal::hci::CommandOpCode;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::HciRouterCallback;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;
using ::bluetooth_hal::util::MockAndroidBaseWrapper;

constexpr int kMaxKeyNumPerVsc = 12;
constexpr int kBytesPerKey = 20;
constexpr uint16_t kHciVscPofOpcode = 0xfd62;

std::vector<Eid> CreateEids(size_t count, uint8_t start_value = 1) {
  std::vector<Eid> eids;
  eids.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    Eid eid;
    eid.bytes.fill(start_value + static_cast<uint8_t>(i));
    eids.push_back(eid);
  }
  return eids;
}

class TestBluetoothFinderHandler : public BluetoothFinderHandler {
 public:
  TestBluetoothFinderHandler() = default;
  ~TestBluetoothFinderHandler() override = default;

  void SimulateBluetoothChipReady() { OnBluetoothChipReady(); }

  void SimulateBluetoothChipNotReady() { OnBluetoothChipClosed(); }

  HalPacket BuildFinderResetCommandWrapper() {
    return BuildFinderResetCommand();
  }

  HalPacket BuildPrecomputedKeyCommandWrapper(const std::vector<Eid>& keys,
                                              uint_t cur_key_idx) {
    return BuildPrecomputedKeyCommand(keys, cur_key_idx);
  }

  HalPacket BuildStartPoweredOffFinderModeCommandWrapper(int32_t cur_key_idx) {
    return BuildStartPoweredOffFinderModeCommand(cur_key_idx);
  }

  int GetCurrentKeyIndex() const { return current_key_index_; }

  void SetCurrentKeyIndex(int index) { current_key_index_ = index; }

  BluetoothFinderHandler::State GetState() const { return state_; }

  void SetState(BluetoothFinderHandler::State state) { state_ = state; }

  std::vector<Eid>& GetKeys() { return keys_; }
};

class BluetoothFinderHandlerTest : public Test {
 protected:
  void SetUp() override {
    MockHciRouter::SetMockRouter(&mock_hci_router_);
    MockAndroidBaseWrapper::SetMockWrapper(&mock_android_base_wrapper_);

    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    EXPECT_CALL(mock_hci_router_client_agent_, RegisterClient(NotNull()))
        .WillOnce(DoAll(SaveArg<0>(&router_callback_), Return(true)));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(false));

    handler_ = std::make_unique<TestBluetoothFinderHandler>();

    ON_CALL(mock_hci_router_, SendCommand(_, _)).WillByDefault(Return(true));

    test_keys_ = CreateEids(5);
  }

  void TearDown() override {
    EXPECT_CALL(mock_hci_router_client_agent_, UnregisterClient(handler_.get()))
        .WillOnce(Return(true));
    handler_.reset();
    MockHciRouter::SetMockRouter(nullptr);
    MockAndroidBaseWrapper::SetMockWrapper(nullptr);
  }

  HalPacket CreateCommandCompleteEvent(uint16_t opcode,
                                       EventResultCode status) {
    HalPacket event_packet({
        static_cast<uint8_t>(HciPacketType::kEvent),
        static_cast<uint8_t>(EventCode::kCommandComplete),
        0x04,  // Parameter Total Length (Num Packets(1) + OpCode(2) +
               // Status(1) = 4)
        0x01,  // Number of HCI command packets allowed to be sent.
        static_cast<uint8_t>(opcode & 0xff),
        static_cast<uint8_t>((opcode >> 8) & 0xff),
        static_cast<uint8_t>(status),
    });
    return event_packet;
  }

  void SimulateBluetoothChipReady() {
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(true));
    handler_->SimulateBluetoothChipReady();
  }

  void SimulateBluetoothChipNotReady() {
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(false));
    handler_->SimulateBluetoothChipNotReady();
  }

  std::unique_ptr<TestBluetoothFinderHandler> handler_;
  HciRouterCallback* router_callback_ = nullptr;
  StrictMock<MockHciRouter> mock_hci_router_;
  StrictMock<MockAndroidBaseWrapper> mock_android_base_wrapper_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;

  std::vector<Eid> test_keys_;
};

// Helper Action to signal when SendCommand is called.
ACTION_P(SignalPromise, p) {
  p->set_value();
  return true;
}

TEST_F(BluetoothFinderHandlerTest, InitialState) {
  bool enabled = true;
  EXPECT_TRUE(handler_->GetPoweredOffFinderMode(&enabled));
  EXPECT_FALSE(enabled);
  EXPECT_FALSE(handler_->IsPoweredOffFinderEnabled());
}

TEST_F(BluetoothFinderHandlerTest, SetModeOnAndGetMode) {
  bool enabled = false;

  // Enable POF.
  EXPECT_TRUE(handler_->SetPoweredOffFinderMode(true));
  EXPECT_TRUE(handler_->GetPoweredOffFinderMode(&enabled));
  EXPECT_TRUE(enabled);
  EXPECT_TRUE(handler_->IsPoweredOffFinderEnabled());
}

TEST_F(BluetoothFinderHandlerTest, SetOffAndGetMode) {
  bool enabled = false;
  handler_->SendEids(test_keys_);

  EXPECT_TRUE(handler_->SetPoweredOffFinderMode(false));
  EXPECT_TRUE(handler_->GetPoweredOffFinderMode(&enabled));
  EXPECT_FALSE(enabled);
  EXPECT_FALSE(handler_->IsPoweredOffFinderEnabled());
  EXPECT_TRUE(handler_->GetKeys().empty());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kIdle);
}

TEST_F(BluetoothFinderHandlerTest, SetModeWhenStarting) {
  handler_->SetState(BluetoothFinderHandler::State::kStartingPof);

  EXPECT_FALSE(handler_->SetPoweredOffFinderMode(true));
}

TEST_F(BluetoothFinderHandlerTest, SendEidsWhenIdle) {
  handler_->SetState(BluetoothFinderHandler::State::kIdle);
  handler_->SetCurrentKeyIndex(5);

  EXPECT_TRUE(handler_->SendEids(test_keys_));
  EXPECT_EQ(handler_->GetKeys().size(), test_keys_.size());
  EXPECT_EQ(handler_->GetCurrentKeyIndex(), 0);
}

TEST_F(BluetoothFinderHandlerTest, SendEidsWhenStarting) {
  handler_->SetState(BluetoothFinderHandler::State::kStartingPof);
  handler_->SetCurrentKeyIndex(3);

  EXPECT_FALSE(handler_->SendEids(test_keys_));
  EXPECT_NE(handler_->GetKeys().size(), test_keys_.size());
  EXPECT_EQ(handler_->GetCurrentKeyIndex(), 3);
}

class StartFinderProcessTest : public BluetoothFinderHandlerTest {
 protected:
  void SetUp() override {
    BluetoothFinderHandlerTest::SetUp();
    // Default enable finder.
    handler_->SetPoweredOffFinderMode(true);
    EXPECT_CALL(mock_android_base_wrapper_, GetProperty(_, _))
        .WillRepeatedly(Return("shut_down_action"));
  }
};

TEST_F(StartFinderProcessTest, StartPofButNotEnabled) {
  SimulateBluetoothChipReady();
  handler_->SetPoweredOffFinderMode(false);

  EXPECT_TRUE(handler_->SendEids(test_keys_));
  EXPECT_FALSE(handler_->StartPoweredOffFinderMode());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kIdle);
}

TEST_F(StartFinderProcessTest, StartPofButNotShuttingDown) {
  SimulateBluetoothChipReady();
  EXPECT_CALL(mock_android_base_wrapper_, GetProperty(_, _))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_TRUE(handler_->SendEids(test_keys_));
  EXPECT_FALSE(handler_->StartPoweredOffFinderMode());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kIdle);
}

TEST_F(StartFinderProcessTest, StartPofButAlreadyStarted) {
  handler_->SetState(BluetoothFinderHandler::State::kStarted);

  EXPECT_FALSE(handler_->StartPoweredOffFinderMode());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kStarted);
}

TEST_F(StartFinderProcessTest, StartPofButBluetoothOff) {
  SimulateBluetoothChipNotReady();

  EXPECT_FALSE(handler_->StartPoweredOffFinderMode());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kIdle);
}

TEST_F(StartFinderProcessTest, StartPofButNoKeys) {
  SimulateBluetoothChipReady();

  EXPECT_FALSE(handler_->StartPoweredOffFinderMode());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kIdle);
}

TEST_F(StartFinderProcessTest, StartPofWithSingleKeyBatchReturnSuccess) {
  ASSERT_NE(router_callback_, nullptr) << "Callback was not captured";

  SimulateBluetoothChipReady();
  test_keys_ = CreateEids(5);

  EXPECT_TRUE(handler_->SendEids(test_keys_));

  // --- Promises for synchronization ---
  std::promise<void> reset_sent_promise;
  std::future<void> reset_sent_future = reset_sent_promise.get_future();
  std::promise<void> keys_sent_promise;
  std::future<void> keys_sent_future = keys_sent_promise.get_future();
  std::promise<void> pof_sent_promise;
  std::future<void> pof_sent_future = pof_sent_promise.get_future();

  // --- Set Expectations with Signaling Actions ---
  // 1. Expect Reset Command.
  HalPacket expected_reset_cmd = handler_->BuildFinderResetCommandWrapper();
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_reset_cmd, _))
      .WillOnce(SignalPromise(&reset_sent_promise));

  // 2. Expect Set Keys Command (after Reset callback).
  HalPacket expected_set_keys_cmd =
      handler_->BuildPrecomputedKeyCommandWrapper(test_keys_, 0);
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_set_keys_cmd, _))
      .WillOnce(SignalPromise(&keys_sent_promise));

  // 3. Expect Start POF Command (after Set Keys callback).
  HalPacket expected_start_pof_cmd =
      handler_->BuildStartPoweredOffFinderModeCommandWrapper(0);
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_start_pof_cmd, _))
      .WillOnce(SignalPromise(&pof_sent_promise));

  // --- Start the process asynchronously ---
  auto result_future = std::async(std::launch::async, [&]() {
    return handler_->StartPoweredOffFinderMode();
  });

  // --- Simulation and Synchronization ---

  // Wait for Reset command to be sent, then simulate its callback.
  ASSERT_EQ(reset_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready)
      << "Timeout waiting for Reset command";
  HalPacket reset_success_event = CreateCommandCompleteEvent(
      static_cast<uint16_t>(CommandOpCode::kHciReset),
      EventResultCode::kSuccess);
  router_callback_->OnCommandCallback(reset_success_event);

  // Wait for Set Keys command to be sent, then simulate its callback.
  ASSERT_EQ(keys_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready)
      << "Timeout waiting for Set Keys command";
  HalPacket set_keys_success_event = CreateCommandCompleteEvent(
      static_cast<uint16_t>(kHciVscPofOpcode), EventResultCode::kSuccess);
  router_callback_->OnCommandCallback(set_keys_success_event);

  // Wait for Start POF command to be sent, then simulate its callback.
  ASSERT_EQ(pof_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready)
      << "Timeout waiting for Start POF command";
  HalPacket start_pof_success_event = CreateCommandCompleteEvent(
      static_cast<uint16_t>(kHciVscPofOpcode), EventResultCode::kSuccess);
  router_callback_->OnCommandCallback(start_pof_success_event);

  // --- Verification ---
  // Wait for the async task to complete and verify the final state.
  EXPECT_TRUE(result_future.get());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kStarted);
  EXPECT_EQ(handler_->GetCurrentKeyIndex(), 5);
}

TEST_F(StartFinderProcessTest, StartPofWithMultipleKeyBatchesReturnSuccess) {
  ASSERT_NE(router_callback_, nullptr) << "Callback was not captured";

  SimulateBluetoothChipReady();

  // Create more keys than fit in one VSC batch.
  const size_t num_keys_first_batch = kMaxKeyNumPerVsc;
  const size_t num_keys_second_batch = 5;
  const size_t total_keys = num_keys_first_batch + num_keys_second_batch;
  test_keys_ = CreateEids(total_keys);
  EXPECT_TRUE(handler_->SendEids(test_keys_));

  // --- Promises for synchronization ---
  std::promise<void> reset_sent_promise;
  std::future<void> reset_sent_future = reset_sent_promise.get_future();
  std::promise<void> keys1_sent_promise;
  std::future<void> keys1_sent_future = keys1_sent_promise.get_future();
  std::promise<void> keys2_sent_promise;
  std::future<void> keys2_sent_future = keys2_sent_promise.get_future();
  std::promise<void> pof_sent_promise;
  std::future<void> pof_sent_future = pof_sent_promise.get_future();

  // --- Set Expectations with Signaling Actions ---
  // 1. Expect Reset Command.
  HalPacket expected_reset_cmd = handler_->BuildFinderResetCommandWrapper();
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_reset_cmd, _))
      .WillOnce(SignalPromise(&reset_sent_promise));

  // 2. Expect Set Keys Command (Batch 1).
  HalPacket expected_set_keys1_cmd =
      handler_->BuildPrecomputedKeyCommandWrapper(test_keys_, 0);
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_set_keys1_cmd, _))
      .WillOnce(SignalPromise(&keys1_sent_promise));

  // 3. Expect Set Keys Command (Batch 2).
  // Note: BuildPrecomputedKeyCommandWrapper updates the internal index,
  // so we call it again, and it should build the command for the next batch.
  // We need to temporarily set the index back for the expectation setup.
  handler_->SetCurrentKeyIndex(
      num_keys_first_batch);  // Set index for expectation
  HalPacket expected_set_keys2_cmd =
      handler_->BuildPrecomputedKeyCommandWrapper(test_keys_,
                                                  num_keys_first_batch);
  handler_->SetCurrentKeyIndex(
      0);  // Reset index before starting the actual flow
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_set_keys2_cmd, _))
      .WillOnce(SignalPromise(&keys2_sent_promise));

  // 4. Expect Start POF Command
  HalPacket expected_start_pof_cmd =
      handler_->BuildStartPoweredOffFinderModeCommandWrapper(0);
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_start_pof_cmd, _))
      .WillOnce(SignalPromise(&pof_sent_promise));

  // --- Start the process asynchronously ---
  auto result_future = std::async(std::launch::async, [&]() {
    return handler_->StartPoweredOffFinderMode();
  });

  // --- Simulation and Synchronization ---
  // Simulate Reset success
  ASSERT_EQ(reset_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  router_callback_->OnCommandCallback(CreateCommandCompleteEvent(
      static_cast<uint16_t>(CommandOpCode::kHciReset),
      EventResultCode::kSuccess));

  // Simulate Set Keys Batch 1 success.
  ASSERT_EQ(keys1_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  router_callback_->OnCommandCallback(
      CreateCommandCompleteEvent(kHciVscPofOpcode, EventResultCode::kSuccess));

  // Simulate Set Keys Batch 2 success.
  ASSERT_EQ(keys2_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  router_callback_->OnCommandCallback(
      CreateCommandCompleteEvent(kHciVscPofOpcode, EventResultCode::kSuccess));

  // Simulate Start POF success.
  ASSERT_EQ(pof_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  router_callback_->OnCommandCallback(
      CreateCommandCompleteEvent(kHciVscPofOpcode, EventResultCode::kSuccess));

  // --- Verification ---
  EXPECT_TRUE(result_future.get());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kStarted);
  // Verify the index was updated correctly after sending all keys.
  EXPECT_EQ(handler_->GetCurrentKeyIndex(), total_keys);
}

TEST_F(StartFinderProcessTest, StartPofFailOnReset) {
  ASSERT_NE(router_callback_, nullptr);
  SimulateBluetoothChipReady();
  test_keys_ = CreateEids(5);
  EXPECT_TRUE(handler_->SendEids(test_keys_));

  // --- Promises ---
  std::promise<void> reset_sent_promise;
  std::future<void> reset_sent_future = reset_sent_promise.get_future();

  // --- Expectations ---
  // Expect Reset Command.
  HalPacket expected_reset_cmd = handler_->BuildFinderResetCommandWrapper();
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_reset_cmd, _))
      .WillOnce(SignalPromise(&reset_sent_promise));
  // Do NOT expect Set Keys or Start POF commands.

  // --- Start Async ---
  auto result_future = std::async(std::launch::async, [&]() {
    // StartPoweredOffFinderMode should return false if any step fails.
    return handler_->StartPoweredOffFinderMode();
  });

  // --- Simulation ---
  // Wait for Reset command, then simulate FAILURE.
  ASSERT_EQ(reset_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  HalPacket reset_failure_event = CreateCommandCompleteEvent(
      static_cast<uint16_t>(CommandOpCode::kHciReset),
      EventResultCode::kFailure);
  router_callback_->OnCommandCallback(reset_failure_event);

  // --- Verification ---
  EXPECT_FALSE(result_future.get());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kIdle);
}

TEST_F(StartFinderProcessTest, StartPofFailOnSetKeys) {
  ASSERT_NE(router_callback_, nullptr);
  SimulateBluetoothChipReady();
  test_keys_ = CreateEids(5);
  EXPECT_TRUE(handler_->SendEids(test_keys_));

  // --- Promises ---
  std::promise<void> reset_sent_promise;
  std::future<void> reset_sent_future = reset_sent_promise.get_future();
  std::promise<void> keys_sent_promise;
  std::future<void> keys_sent_future = keys_sent_promise.get_future();

  // --- Expectations ---
  // 1. Expect Reset Command.
  HalPacket expected_reset_cmd = handler_->BuildFinderResetCommandWrapper();
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_reset_cmd, _))
      .WillOnce(SignalPromise(&reset_sent_promise));

  // 2. Expect Set Keys Command.
  HalPacket expected_set_keys_cmd =
      handler_->BuildPrecomputedKeyCommandWrapper(test_keys_, 0);
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_set_keys_cmd, _))
      .WillOnce(SignalPromise(&keys_sent_promise));
  // Do NOT expect Start POF command.

  // --- Start Async ---
  auto result_future = std::async(std::launch::async, [&]() {
    return handler_->StartPoweredOffFinderMode();
  });

  // --- Simulation ---
  // Simulate Reset success.
  ASSERT_EQ(reset_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  router_callback_->OnCommandCallback(CreateCommandCompleteEvent(
      static_cast<uint16_t>(CommandOpCode::kHciReset),
      EventResultCode::kSuccess));

  // Wait for Set Keys command, then simulate FAILURE.
  ASSERT_EQ(keys_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  HalPacket set_keys_failure_event = CreateCommandCompleteEvent(
      static_cast<uint16_t>(kHciVscPofOpcode), EventResultCode::kFailure);
  router_callback_->OnCommandCallback(set_keys_failure_event);

  // --- Verification ---
  EXPECT_FALSE(result_future.get());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kIdle);
}

TEST_F(StartFinderProcessTest, StartPofFailOnStartPof) {
  ASSERT_NE(router_callback_, nullptr);
  SimulateBluetoothChipReady();
  test_keys_ = CreateEids(5);
  EXPECT_TRUE(handler_->SendEids(test_keys_));

  // --- Promises ---
  std::promise<void> reset_sent_promise;
  std::future<void> reset_sent_future = reset_sent_promise.get_future();
  std::promise<void> keys_sent_promise;
  std::future<void> keys_sent_future = keys_sent_promise.get_future();
  std::promise<void> pof_sent_promise;
  std::future<void> pof_sent_future = pof_sent_promise.get_future();

  // --- Expectations ---
  // 1. Expect Reset Command.
  HalPacket expected_reset_cmd = handler_->BuildFinderResetCommandWrapper();
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_reset_cmd, _))
      .WillOnce(SignalPromise(&reset_sent_promise));

  // 2. Expect Set Keys Command.
  HalPacket expected_set_keys_cmd =
      handler_->BuildPrecomputedKeyCommandWrapper(test_keys_, 0);
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_set_keys_cmd, _))
      .WillOnce(SignalPromise(&keys_sent_promise));

  // 3. Expect Start POF Command.
  HalPacket expected_start_pof_cmd =
      handler_->BuildStartPoweredOffFinderModeCommandWrapper(0);
  EXPECT_CALL(mock_hci_router_, SendCommand(expected_start_pof_cmd, _))
      .WillOnce(SignalPromise(&pof_sent_promise));

  // --- Start Async ---
  auto result_future = std::async(std::launch::async, [&]() {
    return handler_->StartPoweredOffFinderMode();
  });

  // --- Simulation ---
  // Simulate Reset success.
  ASSERT_EQ(reset_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  router_callback_->OnCommandCallback(CreateCommandCompleteEvent(
      static_cast<uint16_t>(CommandOpCode::kHciReset),
      EventResultCode::kSuccess));

  // Simulate Set Keys success.
  ASSERT_EQ(keys_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  router_callback_->OnCommandCallback(CreateCommandCompleteEvent(
      static_cast<uint16_t>(kHciVscPofOpcode), EventResultCode::kSuccess));

  // Wait for Start POF command, then simulate FAILURE.
  ASSERT_EQ(pof_sent_future.wait_for(std::chrono::seconds(1)),
            std::future_status::ready);
  HalPacket start_pof_failure_event = CreateCommandCompleteEvent(
      static_cast<uint16_t>(kHciVscPofOpcode), EventResultCode::kFailure);
  router_callback_->OnCommandCallback(start_pof_failure_event);

  // --- Verification ---
  EXPECT_FALSE(result_future.get());
  EXPECT_EQ(handler_->GetState(), BluetoothFinderHandler::State::kIdle);
}

}  // namespace
}  // namespace finder
}  // namespace extensions
}  // namespace bluetooth_hal
