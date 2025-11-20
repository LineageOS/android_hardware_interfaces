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

#include "bluetooth_hal/hci_router.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "bluetooth_hal/test/mock/mock_transport_interface.h"
#include "bluetooth_hal/test/mock/mock_vnd_snoop_logger.h"
#include "bluetooth_hal/test/mock/mock_wakelock.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace hci {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Test;

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::MockHalConfigLoader;
using ::bluetooth_hal::debug::MockVndSnoopLogger;
using ::bluetooth_hal::transport::MockTransportInterface;
using ::bluetooth_hal::transport::TransportInterfaceCallback;
using ::bluetooth_hal::util::power::MockWakelock;

HalPacketCallback EmptyHalPacketCallback =
    []([[maybe_unused]] const HalPacket& packet) {};

class FakeHciRouterCallback : public HciRouterCallback {
 public:
  FakeHciRouterCallback() = default;
  void OnCommandCallback(const HalPacket& packet) override {
    OnPacketCallback(packet);
  };
  MOCK_METHOD(MonitorMode, OnPacketCallback, (const HalPacket& packet),
              (override));
  MOCK_METHOD(void, OnHalStateChanged,
              (const HalState new_state, const HalState old_state), (override));
};

class HciRouterTest : public Test {
 protected:
  static void SetUpTestSuite() {}

  void SetUp() override {
    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    fake_hci_callback_ = std::make_shared<FakeHciRouterCallback>();

    ON_CALL(mock_transport_interface_, IsTransportActive())
        .WillByDefault(Return(true));
    ON_CALL(mock_transport_interface_, Send(_))
        .WillByDefault(Invoke(this, &HciRouterTest::OnSendToTransport));
    ON_CALL(mock_transport_interface_, Initialize(_))
        .WillByDefault(Invoke(
            [this](TransportInterfaceCallback* transport_interface_callback) {
              this->transport_interface_callback_ =
                  transport_interface_callback;
              on_transport_packet_ready_ = [this](const HalPacket& packet) {
                // The on_packet_ready should only be called after the
                // command is sent to the transport.
                WaitForCommandSent(packet);
                // Drop this packet as the purpose of empty_packet_ is just
                // to block the test, we don't have to actually send it.
                if (packet != empty_packet_) {
                  transport_interface_callback_->OnTransportPacketReady(packet);
                }
              };
              return true;
            }));
    ON_CALL(mock_transport_interface_, SetHciRouterBusy(_))
        .WillByDefault(
            Invoke(this, &HciRouterTest::OnSetHciRouterBusyInTransport));
    ON_CALL(mock_hal_config_loader_, IsAcceleratedBtOnSupported())
        .WillByDefault(Return(false));
    ON_CALL(*fake_hci_callback_, OnHalStateChanged(_, _))
        .WillByDefault(DoAll(SaveArg<0>(&new_state_), SaveArg<1>(&old_state_)));
    ON_CALL(*fake_hci_callback_, OnPacketCallback(_))
        .WillByDefault(
            DoAll(SaveArg<0>(&hal_packet_), Return(MonitorMode::kNone)));

    MockTransportInterface::SetMockTransport(&mock_transport_interface_);
    MockHalConfigLoader::SetMockLoader(&mock_hal_config_loader_);
    MockWakelock::SetMockWakelock(&mock_wakelock_);
    MockVndSnoopLogger::SetMockVndSnoopLogger(&mock_vnd_snoop_logger_);

    router_ = &HciRouter::GetRouter();
    InitializeHciRouter();
  }

  void TearDown() override {
    CleanupHciRouter();
    command_sent_promises_.clear();
    command_sent_futures_.clear();
  }

  void InitializeHciRouter() {
    EXPECT_CALL(mock_transport_interface_, Initialize(_)).Times(1);
    router_->Initialize(fake_hci_callback_);
    CompleteFirmwareDownloadAndStackInit();
  }

  void CleanupHciRouter() {
    EXPECT_CALL(mock_transport_interface_, Cleanup()).Times(1);
    router_->Cleanup();
    ASSERT_EQ(new_state_, HalState::kShutdown);
    ASSERT_EQ(router_->GetHalState(), HalState::kShutdown);
  }

  void OnSetHciRouterBusyInTransport(bool busy) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      is_router_busy_ = busy;
    }
    cv_.notify_one();
  }

  /**
   * @brief Get the HCI router busy state, waiting up to 100ms for updates.
   *
   * As HCI router busy state could be changed asynchronously, we wait a bit for
   * `is_router_busy_` to change (notified by `OnSetHciRouterBusyInTransport`).
   * Timeout may indicate the value was not set or has set earlier.
   *
   * @return The current or last known value of `is_router_busy_`.
   *
   */
  bool GetIsRouterBusy() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(10));
    return is_router_busy_;
  }

  void CompleteFirmwareDownloadAndStackInit() {
    // Mock the chip provisioner firmware download behaivor.
    router_->UpdateHalState(HalState::kFirmwareDownloading);
    router_->UpdateHalState(HalState::kFirmwareDownloadCompleted);
    router_->UpdateHalState(HalState::kFirmwareReady);

    Mock::VerifyAndClearExpectations(&(*fake_hci_callback_));

    // Check state is Running.
    std::vector<HalState> state_changes;
    EXPECT_CALL(*fake_hci_callback_, OnHalStateChanged(_, _))
        .Times(2)
        .WillRepeatedly(Invoke(
            [&](HalState new_state, [[maybe_unused]] HalState old_state) {
              state_changes.push_back(new_state);
            }));

    router_->UpdateHalState(HalState::kBtChipReady);

    EXPECT_EQ(state_changes.size(), 2);
    EXPECT_EQ(state_changes[0], HalState::kBtChipReady);
    EXPECT_EQ(state_changes[1], HalState::kRunning);

    // Without accelerated BT enabled, once HAL changes to `kBtChipReady`, it
    // will automatically update to the `kRunning`.
    EXPECT_EQ(router_->GetHalState(), HalState::kRunning);

    Mock::VerifyAndClearExpectations(&(*fake_hci_callback_));
  }

  void CompleteResetFirmwareWithAcceleratedBtOn() {
    // Mock the chip provisioner reset behavior.
    if (router_->GetHalState() != HalState::kBtChipReady &&
        router_->GetHalState() != HalState::kRunning) {
      return;
    }

    HalState target_state = router_->GetHalState() == HalState::kBtChipReady
                                ? HalState::kRunning
                                : HalState::kBtChipReady;
    router_->UpdateHalState(target_state);

    ASSERT_EQ(new_state_, target_state);
    ASSERT_EQ(router_->GetHalState(), target_state);
  }

  std::pair<HalPacket, HalPacket> CreateCommandEventPacketsWithOrderEnsured(
      std::vector<uint8_t> command, std::vector<uint8_t> event) {
    HalPacket command_packet(command);
    HalPacket event_packet(event);
    // Use promise and future to synchronize the command and event.
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    command_sent_promises_[command] = std::move(promise);
    command_sent_futures_[event] = std::move(future);
    return std::make_pair(command_packet, event_packet);
  }

  void MarkCommandAsSent(const HalPacket& command) {
    auto it = command_sent_promises_.find(command);
    if (it != command_sent_promises_.end()) {
      it->second.set_value();
    }
  }

  bool OnSendToTransport(const HalPacket& packet) {
    // To let the on_packet_ready know the command is sent.
    MarkCommandAsSent(packet);
    return true;
  }

  void WaitForCommandSent(const HalPacket& event) {
    auto it = command_sent_futures_.find(event);
    if (it != command_sent_futures_.end()) {
      it->second.get();
    }
  }

  void ExpectHalStateChange(HalState new_state, HalState old_state,
                            int count = 1) {
    EXPECT_CALL(*fake_hci_callback_, OnHalStateChanged(new_state, old_state))
        .Times(count);
    EXPECT_CALL(mock_hci_router_client_agent_,
                NotifyHalStateChange(new_state, old_state))
        .Times(count);
    EXPECT_CALL(mock_transport_interface_, NotifyHalStateChange(new_state))
        .Times(count);
  }

  FakeHciRouterCallback fake_router_callback_;
  std::shared_ptr<FakeHciRouterCallback> fake_hci_callback_;
  HciRouter* router_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool is_router_busy_ = false;
  HalState new_state_;
  HalState old_state_;
  HalPacket hal_packet_;
  // An empty RX for blocking the test until command has sent to transport.
  HalPacket empty_packet_ = HalPacket({0x02, 0x00, 0x00, 0x00, 0x00});
  HalPacketCallback on_transport_packet_ready_;
  TransportInterfaceCallback* transport_interface_callback_;
  MockTransportInterface mock_transport_interface_;
  MockHalConfigLoader mock_hal_config_loader_;
  MockWakelock mock_wakelock_;
  MockVndSnoopLogger mock_vnd_snoop_logger_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
  std::map<HalPacket, std::promise<void>> command_sent_promises_;
  std::map<HalPacket, std::future<void>> command_sent_futures_;
};

TEST_F(HciRouterTest, InitializeWithAcceleratedBtOn) {
  // Power up the Bluetooth chip.
  ON_CALL(mock_hal_config_loader_, IsAcceleratedBtOnSupported())
      .WillByDefault(Return(true));

  // Turn off Bluetooth, but without cleanup the transport layer.
  router_->Cleanup();
  CompleteResetFirmwareWithAcceleratedBtOn();

  // Turn on Bluetooth from kBtChipReady state, skip firmware download.
  router_->Initialize(fake_hci_callback_);
  CompleteResetFirmwareWithAcceleratedBtOn();

  auto [cmd_reset, evt_reset] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  EXPECT_TRUE(router_->Send(cmd_reset));
  EXPECT_TRUE(GetIsRouterBusy());
  on_transport_packet_ready_(evt_reset);
  EXPECT_FALSE(GetIsRouterBusy());
  EXPECT_EQ(hal_packet_, evt_reset);

  // Disable Accelerated BT ON for test tear down.
  ON_CALL(mock_hal_config_loader_, IsAcceleratedBtOnSupported())
      .WillByDefault(Return(false));
}

TEST_F(HciRouterTest, HandleSendAclData) {
  HalPacket acl_data({0x02, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  EXPECT_CALL(mock_transport_interface_, Send(acl_data)).Times(1);

  EXPECT_TRUE(router_->Send(acl_data));
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommand) {
  auto [cmd, blocker] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, empty_packet_);
  EXPECT_CALL(mock_transport_interface_, Send(cmd)).Times(1);

  EXPECT_TRUE(router_->Send(cmd));
  EXPECT_TRUE(GetIsRouterBusy());

  on_transport_packet_ready_(blocker);
}

TEST_F(HciRouterTest, HandleSendHciCommandTwiceWithoutEvent) {
  auto [cmd_reset, blocker] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, empty_packet_);
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(0);

  EXPECT_TRUE(router_->Send(cmd_reset));
  EXPECT_TRUE(GetIsRouterBusy());
  EXPECT_TRUE(router_->Send(cmd_set_host_le_support));
  EXPECT_TRUE(GetIsRouterBusy());

  on_transport_packet_ready_(blocker);
}

TEST_F(HciRouterTest, HandleSendHciCommandTwiceWithEvent) {
  auto [cmd_reset, evt_reset] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  auto [cmd_set_host_le_support, blocker] =
      CreateCommandEventPacketsWithOrderEnsured(
          {0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00}, empty_packet_);

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);

  // Send the first command.
  EXPECT_TRUE(router_->Send(cmd_reset));
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the first command, and pass to the stack callback.
  on_transport_packet_ready_(evt_reset);
  EXPECT_FALSE(GetIsRouterBusy());
  EXPECT_EQ(hal_packet_, evt_reset);
  // Send the second command.
  EXPECT_TRUE(router_->Send(cmd_set_host_le_support));
  EXPECT_TRUE(GetIsRouterBusy());

  on_transport_packet_ready_(blocker);
}

TEST_F(HciRouterTest, HandleSendHciCommandTwiceWithLateEvent) {
  auto [cmd_reset, evt_reset] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  auto [cmd_set_host_le_support, evt_set_host_le_support] =
      CreateCommandEventPacketsWithOrderEnsured(
          {0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00},
          {0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(2);

  // Send the first command.
  EXPECT_TRUE(router_->Send(cmd_reset));
  EXPECT_TRUE(GetIsRouterBusy());
  // Send the second command.
  EXPECT_TRUE(router_->Send(cmd_set_host_le_support));
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the first command, and pass to the stack callback.
  on_transport_packet_ready_(evt_reset);
  EXPECT_EQ(hal_packet_, evt_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the second command.
  on_transport_packet_ready_(evt_set_host_le_support);
  EXPECT_EQ(hal_packet_, evt_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendCommandTwiceWithoutEvent) {
  auto [cmd_reset, blocker] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, empty_packet_);
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(0);

  EXPECT_TRUE(router_->SendCommand(cmd_reset, EmptyHalPacketCallback));
  EXPECT_TRUE(GetIsRouterBusy());
  EXPECT_TRUE(
      router_->SendCommand(cmd_set_host_le_support, EmptyHalPacketCallback));
  EXPECT_TRUE(GetIsRouterBusy());

  on_transport_packet_ready_(blocker);
}

TEST_F(HciRouterTest, HandleSendCommandTwiceWithEvent) {
  auto [cmd_reset, evt_reset] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  auto [cmd_set_host_le_support, blocker] =
      CreateCommandEventPacketsWithOrderEnsured(
          {0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00}, empty_packet_);

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  // Send the first command with a client callback.
  HalPacket event;
  EXPECT_TRUE(router_->SendCommand(
      cmd_reset, [&event](const HalPacket& packet) { event = packet; }));
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the first command, check if the event is sent to
  // the client callback.
  on_transport_packet_ready_(evt_reset);
  EXPECT_FALSE(GetIsRouterBusy());
  EXPECT_EQ(event, evt_reset);
  // Send the second command.
  EXPECT_TRUE(
      router_->SendCommand(cmd_set_host_le_support, EmptyHalPacketCallback));
  EXPECT_TRUE(GetIsRouterBusy());

  on_transport_packet_ready_(blocker);
}

TEST_F(HciRouterTest, HandleSendCommandTwiceWithLateEvent) {
  auto [cmd_reset, evt_reset] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  auto [cmd_set_host_le_support, evt_set_host_le_support] =
      CreateCommandEventPacketsWithOrderEnsured(
          {0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00},
          {0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  // Send the first command with a client callback.
  HalPacket event;
  EXPECT_TRUE(router_->SendCommand(
      cmd_reset, [&event](const HalPacket& packet) { event = packet; }));
  EXPECT_TRUE(GetIsRouterBusy());
  // Send the second command.
  EXPECT_TRUE(
      router_->SendCommand(cmd_set_host_le_support, EmptyHalPacketCallback));
  EXPECT_TRUE(GetIsRouterBusy());
  // Receive the event for the first command, check if the event is sent to
  // the client callback.
  on_transport_packet_ready_(evt_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  EXPECT_EQ(event, evt_reset);
  // Receive the event for the second command.
  on_transport_packet_ready_(evt_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommandInCallback) {
  auto [cmd_reset, evt_reset] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  auto [cmd_set_host_le_support, evt_set_host_le_support] =
      CreateCommandEventPacketsWithOrderEnsured(
          {0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00},
          {0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  // Expect both cmd_reset and cmd_set_host_le_support are sent to the
  // transport layer, and no callback to the stack.
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  // Send the first command with a client callback, and send the second
  // command in the client callback.
  HalPacket event;
  EXPECT_TRUE(router_->SendCommand(cmd_reset, [&event, &cmd_set_host_le_support,
                                               this](const HalPacket& packet) {
    event = packet;
    EXPECT_TRUE(
        router_->SendCommand(cmd_set_host_le_support, EmptyHalPacketCallback));
  }));
  EXPECT_TRUE(GetIsRouterBusy());

  // Receive the generated event for the first command, check if the second
  // command is properly sent.
  on_transport_packet_ready_(evt_reset);
  EXPECT_EQ(event, evt_reset);
  EXPECT_TRUE(GetIsRouterBusy());
  // Check the second command is properly handled.
  on_transport_packet_ready_(evt_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendHciCommandInCallbackAfterAnotherSendCommand) {
  auto [cmd_reset, evt_reset] = CreateCommandEventPacketsWithOrderEnsured(
      {0x01, 0x03, 0x0c, 0x00}, {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00});
  auto [cmd_set_min_enc_key_size, evt_set_min_enc_key_size] =
      CreateCommandEventPacketsWithOrderEnsured(
          {0x01, 0x84, 0x0c, 0x01, 0x07},
          {0x04, 0x0e, 0x04, 0x01, 0x84, 0x0c, 0x00});
  auto [cmd_set_host_le_support, evt_set_host_le_support] =
      CreateCommandEventPacketsWithOrderEnsured(
          {0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00},
          {0x04, 0x0e, 0x04, 0x01, 0x6d, 0x0c, 0x00});

  // Expect both cmd_reset and cmd_set_host_le_support are sent to the
  // transport layer, and no callback to the stack.
  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_min_enc_key_size))
      .Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  // Send the first command with a client callback, and send the second
  // command in the client callback.
  HalPacket event;
  EXPECT_TRUE(router_->SendCommand(cmd_reset, [&event, &cmd_set_host_le_support,
                                               this](const HalPacket& packet) {
    event = packet;
    EXPECT_TRUE(
        router_->SendCommand(cmd_set_host_le_support, EmptyHalPacketCallback));
  }));
  EXPECT_TRUE(GetIsRouterBusy());
  EXPECT_TRUE(router_->SendCommand(
      cmd_set_min_enc_key_size,
      [&event](const HalPacket& packet) { event = packet; }));
  EXPECT_TRUE(GetIsRouterBusy());

  // Receive three generated events in order
  on_transport_packet_ready_(evt_reset);
  EXPECT_EQ(event, evt_reset);
  EXPECT_TRUE(GetIsRouterBusy());

  on_transport_packet_ready_(evt_set_min_enc_key_size);
  EXPECT_EQ(event, evt_set_min_enc_key_size);
  EXPECT_TRUE(GetIsRouterBusy());

  on_transport_packet_ready_(evt_set_host_le_support);
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleSendCommandNoAck) {
  HalPacket cmd_reset({0x01, 0x03, 0x0c, 0x00});
  HalPacket cmd_set_host_le_support({0x01, 0x6d, 0x0c, 0x02, 0x01, 0x00});

  EXPECT_CALL(mock_transport_interface_, Send(cmd_reset)).Times(1);
  EXPECT_CALL(mock_transport_interface_, Send(cmd_set_host_le_support))
      .Times(1);

  // Send the first command.
  EXPECT_TRUE(router_->SendCommandNoAck(cmd_reset));
  EXPECT_FALSE(GetIsRouterBusy());
  // Send the second command.
  EXPECT_TRUE(router_->SendCommandNoAck(cmd_set_host_le_support));
  EXPECT_FALSE(GetIsRouterBusy());
}

TEST_F(HciRouterTest, HandleDispatchPacketToClientsMonitorNone) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  ON_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(_))
      .WillByDefault(Return(MonitorMode::kNone));

  // Check if the received event is dispatched to both callback and stack.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event)).Times(1);

  on_transport_packet_ready_(event);
  EXPECT_EQ(hal_packet_, event);
}

TEST_F(HciRouterTest, HandleDispatchPacketToClientsMonitorMonitor) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  ON_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(_))
      .WillByDefault(Return(MonitorMode::kMonitor));

  // Check if the received event is dispatched to both callback and stack.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(event)).Times(1);

  on_transport_packet_ready_(event);
  EXPECT_EQ(hal_packet_, event);
}

TEST_F(HciRouterTest, HandleDispatchPacketToClientsMonitorIntercept) {
  HalPacket event({0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  ON_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(_))
      .WillByDefault(Return(MonitorMode::kIntercept));

  // Check if the received event is only dispatched to the agent.
  EXPECT_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(event))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  on_transport_packet_ready_(event);
}

TEST_F(HciRouterTest, HandleOnAclDataCallback) {
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x02, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  on_transport_packet_ready_(packet);
  EXPECT_EQ(hal_packet_, packet);
}

TEST_F(HciRouterTest, HandleOnScoDataCallback) {
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x03, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  on_transport_packet_ready_(packet);
  EXPECT_EQ(hal_packet_, packet);
}

TEST_F(HciRouterTest, HandleOnIsoDataCallback) {
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  on_transport_packet_ready_(packet);
  EXPECT_EQ(hal_packet_, packet);
}

TEST_F(HciRouterTest, HandleDispatchPacketToClientsInterceptThreadData) {
  FakeHciRouterCallback fake_router_callback;
  HalPacket thread_data({0x70, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});

  ON_CALL(mock_hci_router_client_agent_, DispatchPacketToClients(_))
      .WillByDefault(Return(MonitorMode::kIntercept));

  // Expect router callback is called, but stack callback is not.
  EXPECT_CALL(mock_hci_router_client_agent_,
              DispatchPacketToClients(thread_data))
      .Times(1);
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(0);

  on_transport_packet_ready_(thread_data);
}

TEST_F(HciRouterTest, HandleSendPacketToStack) {
  EXPECT_CALL(*fake_hci_callback_, OnPacketCallback(_)).Times(1);
  HalPacket packet({0x02, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  router_->SendPacketToStack(packet);
  EXPECT_EQ(hal_packet_, packet);
}

TEST_F(HciRouterTest, HandleUpdateHalState) {
  // A second shutdown is called in the test Cleanup.
  ExpectHalStateChange(HalState::kShutdown, HalState::kRunning, 2);
  ExpectHalStateChange(HalState::kInit, HalState::kShutdown);
  ExpectHalStateChange(HalState::kFirmwareDownloading, HalState::kInit);
  ExpectHalStateChange(HalState::kFirmwareDownloadCompleted,
                       HalState::kFirmwareDownloading);
  ExpectHalStateChange(HalState::kFirmwareReady,
                       HalState::kFirmwareDownloadCompleted);
  ExpectHalStateChange(HalState::kBtChipReady, HalState::kFirmwareReady);
  ExpectHalStateChange(HalState::kRunning, HalState::kBtChipReady);
  router_->UpdateHalState(HalState::kShutdown);
  router_->UpdateHalState(HalState::kInit);
  router_->UpdateHalState(HalState::kFirmwareDownloading);
  router_->UpdateHalState(HalState::kFirmwareDownloadCompleted);
  router_->UpdateHalState(HalState::kFirmwareReady);
  // Without accelerated BT enabled, once HAL changes to `kBtChipReady`, it
  // will automatically update to the `kRunning`.
  router_->UpdateHalState(HalState::kBtChipReady);
}

}  // namespace
}  // namespace hci
}  // namespace bluetooth_hal
