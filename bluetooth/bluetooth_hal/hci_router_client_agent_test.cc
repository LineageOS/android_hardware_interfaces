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

#include "bluetooth_hal/hci_router_client_agent.h"

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_client_callback.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace hci {
namespace {

using ::testing::Return;
using ::testing::Test;

class MockHciRouterClient : public HciRouterClientCallback {
 public:
  MOCK_METHOD(void, OnCommandCallback, (const HalPacket& packet), (override));
  MOCK_METHOD(MonitorMode, OnPacketCallback, (const HalPacket& packet),
              (override));
  MOCK_METHOD(void, OnHalStateChanged, (HalState new_state, HalState old_state),
              (override));
  MOCK_METHOD(void, OnBluetoothChipReady, (), (override));
  MOCK_METHOD(void, OnBluetoothChipClosed, (), (override));
  MOCK_METHOD(void, OnBluetoothEnabled, (), (override));
  MOCK_METHOD(void, OnBluetoothDisabled, (), (override));
};

class HciRouterClientAgentTest : public Test {
 protected:
  static void SetUpTestSuite() {}

  void SetUp() override {
    agent_ = &HciRouterClientAgent::GetAgent();
    ShutdownBluetooth();
    EXPECT_FALSE(agent_->IsBluetoothEnabled());
    EXPECT_FALSE(agent_->IsBluetoothChipReady());
  }

  void TearDown() override { agent_ = nullptr; }

  void ShutdownBluetooth() {
    agent_->NotifyHalStateChange(HalState::kInit, HalState::kShutdown);
  }

  void PowerOnBluetooth() {
    agent_->NotifyHalStateChange(HalState::kBtChipReady,
                                 HalState::kFirmwareReady);
  }

  void EnableBluetooth() {
    HalPacket reset_packet({0x04, 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00});
    agent_->NotifyHalStateChange(HalState::kRunning, HalState::kBtChipReady);
    ASSERT_EQ(agent_->DispatchPacketToClients(reset_packet),
              MonitorMode::kNone);
  }

  void DisableBluetooth() {
    agent_->NotifyHalStateChange(HalState::kBtChipReady, HalState::kRunning);
  }

  HciRouterClientAgent* agent_;
};

TEST_F(HciRouterClientAgentTest, HandleDispatchPacketToClients) {
  MonitorMode expected_mode = MonitorMode::kMonitor;
  HalPacket packet({0x01, 0x02, 0x03, 0x04});
  MockHciRouterClient mock_router_client;

  ON_CALL(mock_router_client, OnPacketCallback(packet))
      .WillByDefault(Return(expected_mode));
  EXPECT_CALL(mock_router_client, OnPacketCallback).Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));
  EXPECT_EQ(expected_mode, agent_->DispatchPacketToClients(packet));

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
  EXPECT_EQ(MonitorMode::kNone, agent_->DispatchPacketToClients(packet));
}

TEST_F(HciRouterClientAgentTest,
       HandleDispatchPacketToClientsWithMultipleClients) {
  HalPacket packet({0x01, 0x02, 0x03, 0x04});
  MockHciRouterClient mock_router_client1;
  MockHciRouterClient mock_router_client2;
  MockHciRouterClient mock_router_client3;

  ON_CALL(mock_router_client1, OnPacketCallback(packet))
      .WillByDefault(Return(MonitorMode::kIntercept));
  ON_CALL(mock_router_client2, OnPacketCallback(packet))
      .WillByDefault(Return(MonitorMode::kMonitor));
  ON_CALL(mock_router_client3, OnPacketCallback(packet))
      .WillByDefault(Return(MonitorMode::kMonitor));
  EXPECT_CALL(mock_router_client1, OnPacketCallback).Times(1);
  EXPECT_CALL(mock_router_client2, OnPacketCallback).Times(1);
  EXPECT_CALL(mock_router_client3, OnPacketCallback).Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client1));
  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client2));
  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client3));
  EXPECT_EQ(MonitorMode::kIntercept, agent_->DispatchPacketToClients(packet));

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client1));
  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client2));
  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client3));
  EXPECT_EQ(MonitorMode::kNone, agent_->DispatchPacketToClients(packet));
}

TEST_F(HciRouterClientAgentTest, HandleNotifyHalStateChangeShutdownToInit) {
  MockHciRouterClient mock_router_client;
  EXPECT_CALL(mock_router_client, OnBluetoothChipReady()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothDisabled()).Times(0);
  EXPECT_CALL(mock_router_client,
              OnHalStateChanged(HalState::kInit, HalState::kShutdown))
      .Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));

  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());
  agent_->NotifyHalStateChange(HalState::kInit, HalState::kShutdown);
  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
}

TEST_F(HciRouterClientAgentTest,
       HandleNotifyHalStateChangeInitToFirmwareDownloading) {
  MockHciRouterClient mock_router_client;
  EXPECT_CALL(mock_router_client, OnBluetoothChipReady()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothDisabled()).Times(0);
  EXPECT_CALL(
      mock_router_client,
      OnHalStateChanged(HalState::kFirmwareDownloading, HalState::kInit))
      .Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));

  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());
  agent_->NotifyHalStateChange(HalState::kFirmwareDownloading, HalState::kInit);
  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
}

TEST_F(
    HciRouterClientAgentTest,
    HandleNotifyHalStateChangeFirmwaredownloadingToFirmwaredownloadCompleted) {
  MockHciRouterClient mock_router_client;
  EXPECT_CALL(mock_router_client, OnBluetoothChipReady()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothDisabled()).Times(0);
  EXPECT_CALL(mock_router_client,
              OnHalStateChanged(HalState::kFirmwareDownloadCompleted,
                                HalState::kFirmwareDownloading))
      .Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));

  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());
  agent_->NotifyHalStateChange(HalState::kFirmwareDownloadCompleted,
                               HalState::kFirmwareDownloading);
  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
}

TEST_F(HciRouterClientAgentTest,
       HandleNotifyHalStateChangeFirmwaredownloadCompletedToFirmwareReady) {
  MockHciRouterClient mock_router_client;
  EXPECT_CALL(mock_router_client, OnBluetoothChipReady()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothDisabled()).Times(0);
  EXPECT_CALL(mock_router_client,
              OnHalStateChanged(HalState::kFirmwareReady,
                                HalState::kFirmwareDownloadCompleted))
      .Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));

  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());
  agent_->NotifyHalStateChange(HalState::kFirmwareReady,
                               HalState::kFirmwareDownloadCompleted);
  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
}

TEST_F(HciRouterClientAgentTest,
       HandleNotifyHalStateChangeFirmwareReadyToBtChipReady) {
  MockHciRouterClient mock_router_client;
  EXPECT_CALL(mock_router_client, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(mock_router_client, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothDisabled()).Times(0);
  EXPECT_CALL(mock_router_client, OnHalStateChanged(HalState::kBtChipReady,
                                                    HalState::kFirmwareReady))
      .Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));

  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());
  agent_->NotifyHalStateChange(HalState::kBtChipReady,
                               HalState::kFirmwareReady);
  ASSERT_TRUE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
}

TEST_F(HciRouterClientAgentTest,
       HandleNotifyHalStateChangeBtChipReadyToRunning) {
  MockHciRouterClient mock_router_client;
  EXPECT_CALL(mock_router_client, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(mock_router_client, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothEnabled()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothDisabled()).Times(0);
  EXPECT_CALL(mock_router_client,
              OnHalStateChanged(HalState::kRunning, HalState::kBtChipReady))
      .Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));

  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());
  agent_->NotifyHalStateChange(HalState::kRunning, HalState::kBtChipReady);
  ASSERT_TRUE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
}

TEST_F(HciRouterClientAgentTest,
       HandleNotifyHalStateChangeBtChipReadyToRunningWithReset) {
  MockHciRouterClient mock_router_client;
  EXPECT_CALL(mock_router_client, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(mock_router_client, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothEnabled()).Times(1);
  EXPECT_CALL(mock_router_client, OnBluetoothDisabled()).Times(0);
  EXPECT_CALL(mock_router_client,
              OnHalStateChanged(HalState::kRunning, HalState::kBtChipReady))
      .Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));

  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());
  EnableBluetooth();
  ASSERT_TRUE(agent_->IsBluetoothChipReady());
  ASSERT_TRUE(agent_->IsBluetoothEnabled());

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
}

TEST_F(HciRouterClientAgentTest,
       HandleNotifyHalStateChangeWithMultipleClients) {
  MockHciRouterClient mock_router_client1;
  EXPECT_CALL(mock_router_client1, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(mock_router_client1, OnBluetoothChipClosed()).Times(1);
  EXPECT_CALL(mock_router_client1, OnBluetoothEnabled()).Times(1);
  EXPECT_CALL(mock_router_client1, OnBluetoothDisabled()).Times(1);

  MockHciRouterClient mock_router_client2;
  EXPECT_CALL(mock_router_client2, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(mock_router_client2, OnBluetoothChipClosed()).Times(1);
  EXPECT_CALL(mock_router_client2, OnBluetoothEnabled()).Times(1);
  EXPECT_CALL(mock_router_client2, OnBluetoothDisabled()).Times(1);

  MockHciRouterClient mock_router_client3;
  EXPECT_CALL(mock_router_client3, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(mock_router_client3, OnBluetoothChipClosed()).Times(1);
  EXPECT_CALL(mock_router_client3, OnBluetoothEnabled()).Times(1);
  EXPECT_CALL(mock_router_client3, OnBluetoothDisabled()).Times(1);

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client1));
  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client2));
  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client3));

  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  PowerOnBluetooth();
  ASSERT_TRUE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  EnableBluetooth();
  ASSERT_TRUE(agent_->IsBluetoothChipReady());
  ASSERT_TRUE(agent_->IsBluetoothEnabled());

  DisableBluetooth();
  ASSERT_TRUE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  ShutdownBluetooth();
  ASSERT_FALSE(agent_->IsBluetoothChipReady());
  ASSERT_FALSE(agent_->IsBluetoothEnabled());

  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client1));
  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client2));
  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client3));
}

TEST_F(HciRouterClientAgentTest, HandleRegisterRouterClientWhenEnabled) {
  MockHciRouterClient mock_router_client;
  EXPECT_CALL(mock_router_client, OnBluetoothChipReady()).Times(1);
  EXPECT_CALL(mock_router_client, OnBluetoothChipClosed()).Times(0);
  EXPECT_CALL(mock_router_client, OnBluetoothEnabled()).Times(1);
  EXPECT_CALL(mock_router_client, OnBluetoothDisabled()).Times(0);

  EnableBluetooth();

  EXPECT_TRUE(agent_->RegisterRouterClient(&mock_router_client));
  EXPECT_TRUE(agent_->UnregisterRouterClient(&mock_router_client));
}

}  // namespace
}  // namespace hci
}  // namespace bluetooth_hal
