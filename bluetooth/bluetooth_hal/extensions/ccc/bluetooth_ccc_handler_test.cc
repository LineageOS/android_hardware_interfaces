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

#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_handler.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_handler_callback.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_timesync_event.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "bluetooth_hal/test/mock/mock_system_call_wrapper.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {
namespace {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::util::MockSystemCallWrapper;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArrayArgument;
using ::testing::Test;

class BluetoothCccHandlerTestInstance : public BluetoothCccHandler {
 public:
  void OnCommandCallback(const HalPacket& packet) override {
    BluetoothCccHandler::OnCommandCallback(packet);
  }

  void EnableBluetooth() {
    OnBluetoothChipReady();
    OnBluetoothEnabled();
    OnHalStateChanged(HalState::kRunning, HalState::kBtChipReady);
    OnPacketCallback(HalPacket({0x04, 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00}));
  }

  void DisableBluetooth() {
    OnBluetoothDisabled();
    OnBluetoothChipClosed();
  }
};

class MockCallbackVerifier : public BluetoothCccHandlerCallback {
 public:
  MockCallbackVerifier()
      : BluetoothCccHandlerCallback(BluetoothAddress({}),
                                    std::vector<CccLmpEventId>()) {}
  MOCK_METHOD(void, OnEventGenerated,
              (const CccTimestamp& timestamp, const BluetoothAddress& address,
               CccDirection direction, CccLmpEventId lmp_event_id,
               uint8_t event_counter),
              (override));
  MOCK_METHOD(void, OnRegistered, (bool status), (override));
};

class MockBluetoothCccHandlerCallback : public BluetoothCccHandlerCallback {
 public:
  MockBluetoothCccHandlerCallback(
      BluetoothAddress& address, std::vector<CccLmpEventId> lmp_ids,
      std::shared_ptr<MockCallbackVerifier> verifier,
      AddressType address_type = AddressType::kRandom)
      : BluetoothCccHandlerCallback(address, address_type, lmp_ids),
        verifier_(verifier) {};

  void OnEventGenerated(const CccTimestamp& timestamp,
                        const BluetoothAddress& address, CccDirection direction,
                        CccLmpEventId lmp_event_id,
                        uint8_t event_counter) override {
    verifier_->OnEventGenerated(timestamp, address, direction, lmp_event_id,
                                event_counter);
  }

  void OnRegistered(bool status) override { verifier_->OnRegistered(status); }

 private:
  std::shared_ptr<MockCallbackVerifier> verifier_;
};

class BluetoothCccHandlerTest : public Test {
 protected:
  static void SetUpTestSuite() {}

  void SetUp() override {
    MockSystemCallWrapper::SetMockWrapper(&mock_system_call_wrapper_);
    ON_CALL(mock_system_call_wrapper_, Open(_, _)).WillByDefault(Return(1));

    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    EXPECT_CALL(mock_hci_router_client_agent_, RegisterClient(NotNull()))
        .WillOnce(Return(true));

    MockHciRouter::SetMockRouter(&mock_hci_router_);
    ON_CALL(mock_hci_router_, Send(_)).WillByDefault(Return(true));
    ON_CALL(mock_hci_router_, SendCommand(_, _)).WillByDefault(Return(true));

    ccc_handler_ = new BluetoothCccHandlerTestInstance();
  }

  void TearDown() override {
    EXPECT_CALL(mock_hci_router_client_agent_, UnregisterClient(ccc_handler_))
        .WillOnce(Return(true));
    delete (ccc_handler_);
  }

  void EnableBluetooth() {
    ON_CALL(mock_hci_router_, GetHalState())
        .WillByDefault(Return(HalState::kRunning));
    ON_CALL(mock_system_call_wrapper_, Read(_, _, _)).WillByDefault(Return(0));

    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(true));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
        .WillByDefault(Return(true));
    ccc_handler_->EnableBluetooth();

    ON_CALL(mock_system_call_wrapper_, Read(_, _, _))
        .WillByDefault(
            Invoke([&]([[maybe_unused]] int fd, void* buf, size_t count) {
              size_t bytes_to_copy = std::min(count, sizeof(timestamp_data_));
              std::memcpy(buf, timestamp_data_, bytes_to_copy);
              return static_cast<int>(bytes_to_copy);
            }));
  }

  void DisableBluetooth() {
    ON_CALL(mock_hci_router_, GetHalState())
        .WillByDefault(Return(HalState::kBtChipReady));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(false));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
        .WillByDefault(Return(false));
    ccc_handler_->DisableBluetooth();
  }

  HalPacket GenerateTimesyncCommandCompleteEvent(bool success) {
    uint8_t status = (success ? 0x00 : 0x01);
    return HalPacket({0x04, 0x0E, 0x04, 0x01, 0x63, 0xFD, status});
  }

  BluetoothCccTimesyncEvent CreateTimesyncEvent(
      BluetoothAddress& address, uint8_t toggle_count = 0x01,
      uint16_t event_count = 0x5678,
      AddressType address_type = AddressType::kRandom) {
    auto packet = HalPacket({
        0x04,  // HCI Event (1 byte)
        0xFF,  // Vendor event code (1 byte)
        0x17,  // Length (1 byte - 23 decimal, payload length)
        0xD0,  // Time sync sub event code (1 byte)

        address[5], address[4], address[3], address[2], address[1],
        address[0],  // Address (6 bytes)

        static_cast<uint8_t>(address_type),  // Address type
        0x00,                                // Direction (1 byte - Tx)

        // Timestamp (8 bytes - 0xAABBCCDDEEFF0011, little-endian)
        0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA,

        0xFF,          // EventId (1 byte - EventIdConnInd)
        toggle_count,  // ToggleCount (1 byte)

        // Timesync offset (2 bytes - 0x1234, little-endian)
        0x34, 0x12,

        // Event count (2 bytes - 0x5678, little-endian)
        static_cast<uint8_t>(event_count & 0xFF),  // 0x78
        static_cast<uint8_t>(event_count >> 8)     // 0x56
    });

    return BluetoothCccTimesyncEvent(packet);
  }

  MockHciRouter mock_hci_router_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
  MockSystemCallWrapper mock_system_call_wrapper_;
  char timestamp_data_[20] = {'1', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
  BluetoothCccHandlerTestInstance* ccc_handler_;
  BluetoothAddress address_ = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  BluetoothAddress different_address_ = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
};

TEST_F(BluetoothCccHandlerTest, HandleRegisterForLmpEventsSuccess) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(2);
  EXPECT_CALL(*mock_callback, OnRegistered(true)).Times(1);

  EXPECT_FALSE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
  EnableBluetooth();
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  EXPECT_TRUE(ccc_handler_->UnregisterLmpEvents(address_));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));
}

TEST_F(BluetoothCccHandlerTest, HandleRegisterForLmpEventsFailed) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(1);
  EXPECT_CALL(*mock_callback, OnRegistered(false)).Times(1);

  EXPECT_FALSE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
  EnableBluetooth();
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(false));

  // This UnregisterLmpEvents does not invoke SendCommand to the HciRouter due
  // to the failure in the command complete event for the
  // RegisterForLmpEvents.
  EXPECT_FALSE(ccc_handler_->UnregisterLmpEvents(address_));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(false));
}

TEST_F(BluetoothCccHandlerTest, HandleRegisterForLmpEventsEmptyLmpEvent) {
  std::vector<CccLmpEventId> lmp_ids = {};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();
  EnableBluetooth();
  EXPECT_FALSE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
}

TEST_F(BluetoothCccHandlerTest, HandleRegisterForLmpEventsTooManyLmpEvent) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd,
                                        CccLmpEventId::kMax};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();
  EnableBluetooth();
  EXPECT_FALSE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
}

TEST_F(BluetoothCccHandlerTest, HandleUnregisterLmpEventsDefault) {
  EXPECT_FALSE(ccc_handler_->UnregisterLmpEvents(address_));
}

TEST_F(BluetoothCccHandlerTest, HandleUnregisterLmpEventsWhenBluetoothOff) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(1);
  EXPECT_CALL(*mock_callback, OnRegistered(true)).Times(1);

  EnableBluetooth();
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  DisableBluetooth();
  EXPECT_FALSE(ccc_handler_->UnregisterLmpEvents(address_));
}

TEST_F(BluetoothCccHandlerTest,
       HandleUnregisterLmpEventsBeforeCommandComplete) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();

  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(2);
  EXPECT_CALL(*mock_callback, OnRegistered(true)).Times(1);

  EnableBluetooth();
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
  std::thread event_thread([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));
  });
  event_thread.detach();
  // The thread should be blocked in UnregisterLmpEvents until the command
  // complete event is received.
  EXPECT_TRUE(ccc_handler_->UnregisterLmpEvents(address_));
}

TEST_F(BluetoothCccHandlerTest, HandleMonitoringTimeSyncEvent) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();
  auto timesync_event = CreateTimesyncEvent(address_);

  // One additional system wrapper invoke for Enabling BT
  EXPECT_CALL(mock_system_call_wrapper_, Open(_, _)).Times(2);
  EXPECT_CALL(mock_system_call_wrapper_, Read(_, _, _)).Times(2);
  EXPECT_CALL(mock_system_call_wrapper_, Close(_)).Times(2);
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(2);
  EXPECT_CALL(*mock_callback, OnRegistered(true)).Times(1);
  EXPECT_CALL(*mock_callback, OnEventGenerated(_, address_, _, _, _)).Times(1);

  // timesync event before register for LMP event, should not trigger a
  // callback.
  ccc_handler_->OnPacketCallback(timesync_event);

  // Register LMP event for address_
  EnableBluetooth();
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));

  // timesync event before register complete, should not trigger a callback.
  ccc_handler_->OnPacketCallback(timesync_event);

  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  // timesync event after register complete, should trigger a callback.
  ccc_handler_->OnPacketCallback(timesync_event);

  EXPECT_TRUE(ccc_handler_->UnregisterLmpEvents(address_));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  // timesync event after unregistery, should not trigger a callback.
  ccc_handler_->OnPacketCallback(timesync_event);
}

TEST_F(BluetoothCccHandlerTest,
       HandleMonitoringTimeSyncEventWithDifferentAddress) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();
  auto timesync_event = CreateTimesyncEvent(different_address_);

  // One additional system wrapper invoke for Enabling BT
  EXPECT_CALL(mock_system_call_wrapper_, Open(_, _)).Times(2);
  EXPECT_CALL(mock_system_call_wrapper_, Read(_, _, _)).Times(2);
  EXPECT_CALL(mock_system_call_wrapper_, Close(_)).Times(2);
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(1);
  EXPECT_CALL(*mock_callback, OnRegistered(true)).Times(1);
  EXPECT_CALL(*mock_callback, OnEventGenerated(_, _, _, _, _)).Times(0);

  // Register LMP event for address_
  EnableBluetooth();
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));

  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  // timesync event for different_address_, should not trigger callback.
  ccc_handler_->OnPacketCallback(timesync_event);
}

TEST_F(BluetoothCccHandlerTest,
       HandleMonitoringTimeSyncEventWithDifferentAddressType) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();
  auto timesync_event = CreateTimesyncEvent(address_, 1, 0x1);
  auto address_type_public =
      CreateTimesyncEvent(address_, 2, 0x2, AddressType::kPublic);

  // One additional system wrapper invoke for Enabling BT
  EXPECT_CALL(mock_system_call_wrapper_, Open(_, _)).Times(3);
  EXPECT_CALL(mock_system_call_wrapper_, Read(_, _, _)).Times(3);
  EXPECT_CALL(mock_system_call_wrapper_, Close(_)).Times(3);
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(1);
  EXPECT_CALL(*mock_callback, OnRegistered(true)).Times(1);
  EXPECT_CALL(*mock_callback, OnEventGenerated(_, address_, _, _, 0x2))
      .Times(1);

  EnableBluetooth();
  // Register LMP event for address_ kPublic
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(
          address_, lmp_ids, mock_callback, AddressType::kPublic)));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  // timesync events for each address type, should only trigger callbakc once
  ccc_handler_->OnPacketCallback(timesync_event);
  ccc_handler_->OnPacketCallback(address_type_public);
}

TEST_F(BluetoothCccHandlerTest, HandleMonitoringMultipleTimeSyncEvent) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback1 = std::make_shared<MockCallbackVerifier>();
  auto mock_callback2 = std::make_shared<MockCallbackVerifier>();
  auto timesync_event1 = CreateTimesyncEvent(address_, 1);
  auto timesync_event2 = CreateTimesyncEvent(different_address_, 2);

  // One additional system wrapper invoke for Enabling BT
  EXPECT_CALL(mock_system_call_wrapper_, Open(_, _)).Times(3);
  EXPECT_CALL(mock_system_call_wrapper_, Read(_, _, _)).Times(3);
  EXPECT_CALL(mock_system_call_wrapper_, Close(_)).Times(3);
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(2);
  EXPECT_CALL(*mock_callback1, OnRegistered(true)).Times(1);
  EXPECT_CALL(*mock_callback2, OnRegistered(true)).Times(1);
  EXPECT_CALL(*mock_callback1, OnEventGenerated(_, address_, _, _, _)).Times(1);
  EXPECT_CALL(*mock_callback2, OnEventGenerated(_, different_address_, _, _, _))
      .Times(1);

  EnableBluetooth();
  // Register LMP event for address_
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback1)));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  // Register LMP event for different_address_
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(
          different_address_, lmp_ids, mock_callback2)));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  // timesync events for each callback.
  ccc_handler_->OnPacketCallback(timesync_event1);
  ccc_handler_->OnPacketCallback(timesync_event2);
}

TEST_F(BluetoothCccHandlerTest, HandleMonitoringHighToggleCount) {
  std::vector<CccLmpEventId> lmp_ids = {CccLmpEventId::kConnectInd,
                                        CccLmpEventId::kLlPhyUpdateInd};
  auto mock_callback = std::make_shared<MockCallbackVerifier>();
  auto timesync_event = CreateTimesyncEvent(address_, 10);  // toggle count = 10

  // One additional system wrapper invoke for Enabling BT
  EXPECT_CALL(mock_system_call_wrapper_, Open(_, _)).Times(2);
  EXPECT_CALL(mock_system_call_wrapper_, Read(_, _, _)).Times(11);
  EXPECT_CALL(mock_system_call_wrapper_, Close(_)).Times(2);
  EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).Times(1);
  EXPECT_CALL(*mock_callback, OnRegistered(true)).Times(1);
  EXPECT_CALL(*mock_callback, OnEventGenerated(_, address_, _, _, _)).Times(1);

  EnableBluetooth();
  // Register LMP event for address_
  EXPECT_TRUE(ccc_handler_->RegisterForLmpEvents(
      std::make_unique<MockBluetoothCccHandlerCallback>(address_, lmp_ids,
                                                        mock_callback)));
  ccc_handler_->OnCommandCallback(GenerateTimesyncCommandCompleteEvent(true));

  // timesync events for each callback.
  ccc_handler_->OnPacketCallback(timesync_event);
}

}  // namespace
}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
