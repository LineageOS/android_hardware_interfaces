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

#include "bluetooth_hal/debug/bluetooth_activities.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"

namespace bluetooth_hal {
namespace debug {
namespace {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;
using ::bluetooth_hal::hci::MonitorMode;
using ::testing::_;
using ::testing::Return;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

constexpr size_t kClassicConnectionCompleteEventLength = 14;
constexpr size_t kDisconnectionCompleteEventLength = 7;
constexpr size_t kBleConnectionCompleteEventLength = 22;
constexpr size_t kBleEnhancedConnectionCompleteV1EventLength = 34;
constexpr size_t kBleEnhancedConnectionCompleteV2EventLength = 37;

struct BtDeviceForTest {
  uint16_t connection_handle;
  BluetoothAddress bd_address;
};

HalPacket CreateBleConnectionCompleteEvent(BtDeviceForTest device,
                                           bool success) {
  HalPacket event(
      std::vector<uint8_t>(kBleConnectionCompleteEventLength, 0x00));
  event[0] = static_cast<uint8_t>(HciPacketType::kEvent);
  event[1] = static_cast<uint8_t>(EventCode::kBleMeta);
  event[2] = 0x13;                     // Length
  event[3] = 0x01;                     // LE Connection Complete Subevent
  event[4] = (success ? 0x00 : 0x01);  // Status
  event[5] = static_cast<uint8_t>(device.connection_handle & 0xFF);
  event[6] = static_cast<uint8_t>((device.connection_handle >> 8u) & 0x0F);
  for (int i = 0; i < device.bd_address.size(); ++i) {
    event[9 + i] = device.bd_address[device.bd_address.size() - 1 - i];
  }
  return event;
}

HalPacket CreateBleEnhancedConnectionCompleteV1Event(BtDeviceForTest device,
                                                     bool success) {
  HalPacket event(
      std::vector<uint8_t>(kBleEnhancedConnectionCompleteV1EventLength, 0x00));
  event[0] = static_cast<uint8_t>(HciPacketType::kEvent);
  event[1] = static_cast<uint8_t>(EventCode::kBleMeta);
  event[2] = 0x1f;  // Length
  event[3] = 0x0a;  // LE Enhanced Connection Complete Subevent
  event[4] = (success ? 0x00 : 0x01);  // Status
  event[5] = static_cast<uint8_t>(device.connection_handle & 0xFF);
  event[6] = static_cast<uint8_t>((device.connection_handle >> 8u) & 0x0F);
  for (int i = 0; i < device.bd_address.size(); ++i) {
    event[9 + i] = device.bd_address[device.bd_address.size() - 1 - i];
  }
  return event;
}

HalPacket CreateBleEnhancedConnectionCompleteV2Event(BtDeviceForTest device,
                                                     bool success) {
  HalPacket event = CreateBleEnhancedConnectionCompleteV1Event(device, success);
  event.resize(kBleEnhancedConnectionCompleteV2EventLength, 0x00);
  event[2] = 0x22;  // Length
  event[3] = 0x29;  // LE Enhanced Connection Complet Subevent
  return event;
}

HalPacket CreateClassicConnectionCompleteEvent(BtDeviceForTest device,
                                               bool success) {
  HalPacket event(
      std::vector<uint8_t>(kClassicConnectionCompleteEventLength, 0x00));
  event[0] = static_cast<uint8_t>(HciPacketType::kEvent);
  event[1] = static_cast<uint8_t>(EventCode::kConnectionComplete);
  event[2] = 0x0b;                     // Length
  event[3] = (success ? 0x00 : 0x01);  // Status
  event[4] = static_cast<uint8_t>(device.connection_handle & 0xFF);
  event[5] = static_cast<uint8_t>((device.connection_handle >> 8u) & 0x0F);
  for (int i = 0; i < device.bd_address.size(); ++i) {
    event[6 + i] = device.bd_address[device.bd_address.size() - 1 - i];
  }
  return event;
}

HalPacket CreateDisconnectionCompleteEvent(BtDeviceForTest device,
                                           bool success) {
  HalPacket event(
      std::vector<uint8_t>(kDisconnectionCompleteEventLength, 0x00));
  event[0] = static_cast<uint8_t>(HciPacketType::kEvent);
  event[1] = static_cast<uint8_t>(EventCode::kDisconnectionComplete);
  event[2] = 0x04;                     // Length
  event[3] = (success ? 0x00 : 0x01);  // Status
  event[4] = static_cast<uint8_t>(device.connection_handle & 0xFF);
  event[5] = static_cast<uint8_t>((device.connection_handle >> 8u) & 0x0F);
  return event;
}

BtDeviceForTest device_1{0x0123, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
BtDeviceForTest device_2{0x0456, {0x04, 0x05, 0x06, 0x07, 0x07, 0x09}};

class BluetoothActivitiesTest : public Test {
 protected:
  void SetUp() override {
    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    MockHciRouter::SetMockRouter(&mock_hci_router_);
    ON_CALL(mock_hci_router_, Send(_)).WillByDefault(Return(true));
    ON_CALL(mock_hci_router_, SendCommand(_, _)).WillByDefault(Return(true));

    BluetoothActivities::Start();
    EnableBluetooth();
  }

  void TearDown() override {
    BluetoothActivities::Stop();
    DisableBluetooth();
  }

  void EnableBluetooth() {
    ON_CALL(mock_hci_router_, GetHalState())
        .WillByDefault(Return(HalState::kRunning));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(true));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
        .WillByDefault(Return(true));
  }

  void DisableBluetooth() {
    ON_CALL(mock_hci_router_, GetHalState())
        .WillByDefault(Return(HalState::kBtChipReady));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(false));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
        .WillByDefault(Return(false));
  }
  MockHciRouter mock_hci_router_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
};

TEST_F(BluetoothActivitiesTest, InitialState) {
  EXPECT_FALSE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_FALSE(BluetoothActivities::Get().IsConnected(0x000a));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 0);
}

class ConnectionAndDisconnectionTest
    : public BluetoothActivitiesTest,
      public WithParamInterface<std::pair<HalPacket, uint16_t>> {};

TEST_P(ConnectionAndDisconnectionTest, ConnectionAndDisconnection) {
  const auto& [packet, connect_handle] = GetParam();
  EXPECT_FALSE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_FALSE(BluetoothActivities::Get().IsConnected(connect_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 0);

  BluetoothActivities::Get().OnMonitorPacketCallback(MonitorMode::kMonitor,
                                                     packet);
  EXPECT_TRUE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_TRUE(BluetoothActivities::Get().IsConnected(connect_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 1);

  BluetoothActivities::Get().OnMonitorPacketCallback(
      MonitorMode::kMonitor, CreateDisconnectionCompleteEvent(device_1, true));
  EXPECT_FALSE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_FALSE(BluetoothActivities::Get().IsConnected(connect_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 0);
}

INSTANTIATE_TEST_SUITE_P(
    ConnectionAndDisconnectionTest, ConnectionAndDisconnectionTest,
    Values(std::make_pair(CreateClassicConnectionCompleteEvent(device_1, true),
                          device_1.connection_handle),
           std::make_pair(CreateBleConnectionCompleteEvent(device_1, true),
                          device_1.connection_handle),
           std::make_pair(CreateBleEnhancedConnectionCompleteV1Event(device_1,
                                                                     true),
                          device_1.connection_handle),
           std::make_pair(CreateBleEnhancedConnectionCompleteV2Event(device_1,
                                                                     true),
                          device_1.connection_handle)));

class MultiDeviceConnectionsAndDisconnectionsTest
    : public BluetoothActivitiesTest,
      public WithParamInterface<std::pair<HalPacket, HalPacket>> {};

TEST_P(MultiDeviceConnectionsAndDisconnectionsTest,
       MultiDeviceConnectionsAndDisconnections) {
  const auto& [device_1_connection_event, device_2_connection_event] =
      GetParam();
  EXPECT_FALSE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 0);

  // Connect with device_1
  BluetoothActivities::Get().OnMonitorPacketCallback(MonitorMode::kMonitor,
                                                     device_1_connection_event);
  EXPECT_TRUE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_TRUE(
      BluetoothActivities::Get().IsConnected(device_1.connection_handle));
  EXPECT_FALSE(
      BluetoothActivities::Get().IsConnected(device_2.connection_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 1);

  // Connect with device_2
  BluetoothActivities::Get().OnMonitorPacketCallback(MonitorMode::kMonitor,
                                                     device_2_connection_event);
  EXPECT_TRUE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_TRUE(
      BluetoothActivities::Get().IsConnected(device_1.connection_handle));
  EXPECT_TRUE(
      BluetoothActivities::Get().IsConnected(device_2.connection_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 2);

  // Disconnect with device_1
  BluetoothActivities::Get().OnMonitorPacketCallback(
      MonitorMode::kMonitor, CreateDisconnectionCompleteEvent(device_1, true));
  EXPECT_TRUE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_FALSE(
      BluetoothActivities::Get().IsConnected(device_1.connection_handle));
  EXPECT_TRUE(
      BluetoothActivities::Get().IsConnected(device_2.connection_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 1);

  // Disconnect with device_2
  BluetoothActivities::Get().OnMonitorPacketCallback(
      MonitorMode::kMonitor, CreateDisconnectionCompleteEvent(device_2, true));
  EXPECT_FALSE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_FALSE(
      BluetoothActivities::Get().IsConnected(device_1.connection_handle));
  EXPECT_FALSE(
      BluetoothActivities::Get().IsConnected(device_2.connection_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 0);
}

INSTANTIATE_TEST_SUITE_P(
    MultiDeviceConnectionsAndDisconnectionsTest,
    MultiDeviceConnectionsAndDisconnectionsTest,
    Values(std::make_pair(CreateClassicConnectionCompleteEvent(device_1, true),
                          CreateClassicConnectionCompleteEvent(device_2, true)),
           std::make_pair(CreateBleConnectionCompleteEvent(device_1, true),
                          CreateBleConnectionCompleteEvent(device_2, true)),
           std::make_pair(
               CreateBleEnhancedConnectionCompleteV1Event(device_1, true),
               CreateBleEnhancedConnectionCompleteV1Event(device_2, true)),
           std::make_pair(
               CreateBleEnhancedConnectionCompleteV2Event(device_1, true),
               CreateBleEnhancedConnectionCompleteV2Event(device_2, true)),
           std::make_pair(CreateClassicConnectionCompleteEvent(device_1, true),
                          CreateBleConnectionCompleteEvent(device_2, true)),
           std::make_pair(
               CreateBleEnhancedConnectionCompleteV1Event(device_1, true),
               CreateClassicConnectionCompleteEvent(device_2, true))));

class ConnectionFailTest
    : public BluetoothActivitiesTest,
      public WithParamInterface<std::pair<HalPacket, uint16_t>> {};

TEST_P(ConnectionFailTest, ConnectionFail) {
  const auto& [packet, connect_handle] = GetParam();
  EXPECT_FALSE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_FALSE(BluetoothActivities::Get().IsConnected(connect_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 0);

  BluetoothActivities::Get().OnMonitorPacketCallback(MonitorMode::kMonitor,
                                                     packet);
  EXPECT_FALSE(BluetoothActivities::Get().HasConnectedDevice());
  EXPECT_FALSE(BluetoothActivities::Get().IsConnected(connect_handle));
  EXPECT_EQ(BluetoothActivities::Get().GetConnectionHandleCount(), 0);
}

INSTANTIATE_TEST_SUITE_P(
    ConnectionFailTest, ConnectionFailTest,
    Values(std::make_pair(CreateClassicConnectionCompleteEvent(device_1, false),
                          device_1.connection_handle),
           std::make_pair(CreateBleConnectionCompleteEvent(device_1, false),
                          device_1.connection_handle),
           std::make_pair(CreateBleEnhancedConnectionCompleteV1Event(device_1,
                                                                     false),
                          device_1.connection_handle),
           std::make_pair(CreateBleEnhancedConnectionCompleteV2Event(device_1,
                                                                     false),
                          device_1.connection_handle)));

}  // namespace
}  // namespace debug
}  // namespace bluetooth_hal
