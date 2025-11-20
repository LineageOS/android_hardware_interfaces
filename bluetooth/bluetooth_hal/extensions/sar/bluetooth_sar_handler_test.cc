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

#include "bluetooth_hal/extensions/sar/bluetooth_sar_handler.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_client_callback.h"
#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace extensions {
namespace sar {

using ::testing::_;
using ::testing::DoAll;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::MockHalConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciRouterClientCallback;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;

class TestSarHandler : public BluetoothSarHandler {
 public:
  HalPacket BuildCommandHRModeWrapper(
      const std::array<uint8_t, 4>& chain_0_cap,
      const std::array<uint8_t, 4>& chain_1_cap,
      const std::array<uint8_t, 8>& beamforming_cap, bool high_resolution_cap,
      bool is_ble_non_connection_enabled) {
    return BuildCommandHRMode(chain_0_cap, chain_1_cap, beamforming_cap,
                              high_resolution_cap,
                              is_ble_non_connection_enabled);
  }

  HalPacket BuildCommandWrapper(const std::array<uint8_t, 3>& chain_0_cap,
                                const std::array<uint8_t, 3>& chain_1_cap,
                                const std::array<uint8_t, 6>& beamforming_cap,
                                bool high_resolution_cap) {
    return BuildCommand(chain_0_cap, chain_1_cap, beamforming_cap,
                        high_resolution_cap);
  }

  HalPacket BuildCommandWrapper(uint8_t br_cap, uint8_t edr_cap,
                                uint8_t ble_cap, bool high_resolution_cap) {
    return BuildCommand(br_cap, edr_cap, ble_cap, high_resolution_cap);
  }
};

class BluetoothSarTest : public Test {
 protected:
  void SetUp() override {
    MockHalConfigLoader::SetMockLoader(&mock_hal_config_loader_);

    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
    EXPECT_CALL(mock_hci_router_client_agent_, RegisterRouterClient(NotNull()))
        .WillOnce(DoAll(SaveArg<0>(&router_client_callback_), Return(true)));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(false));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
        .WillByDefault(Return(false));

    MockHciRouter::SetMockRouter(&mock_hci_router_);
    ON_CALL(mock_hci_router_, SendCommand(_, _)).WillByDefault(Return(true));

    bluetooth_sar_handler_ = std::make_unique<TestSarHandler>();
  }

  void TearDown() override {
    EXPECT_CALL(mock_hci_router_client_agent_,
                UnregisterRouterClient(bluetooth_sar_handler_.get()))
        .WillOnce(Return(true));
    bluetooth_sar_handler_ = nullptr;
  }

  void EnableBluetooth() {
    ASSERT_NE(router_client_callback_, nullptr) << "Callback was not captured";
    ON_CALL(mock_hci_router_, GetHalState())
        .WillByDefault(Return(HalState::kRunning));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothChipReady())
        .WillByDefault(Return(true));
    ON_CALL(mock_hci_router_client_agent_, IsBluetoothEnabled())
        .WillByDefault(Return(true));
    router_client_callback_->OnBluetoothChipReady();
    router_client_callback_->OnBluetoothEnabled();
  }

  std::unique_ptr<TestSarHandler> bluetooth_sar_handler_;
  MockHciRouter mock_hci_router_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
  MockHalConfigLoader mock_hal_config_loader_;
  HciRouterClientCallback* router_client_callback_ = nullptr;
};

class BleNonConnectionModeTest
    : public BluetoothSarTest,
      public WithParamInterface<std::tuple<bool, uint8_t>> {};

TEST_P(BleNonConnectionModeTest, HandleModeWhenBuildCommandHRModeWrapper) {
  const auto& [is_ble_non_connection_enabled, expected_sub_op_code] =
      GetParam();
  HalPacket command_hr = bluetooth_sar_handler_->BuildCommandHRModeWrapper(
      {}, {}, {}, false, is_ble_non_connection_enabled);

  EXPECT_EQ(command_hr[4], expected_sub_op_code);
}

INSTANTIATE_TEST_SUITE_P(
    BleNonConnectionMode, BleNonConnectionModeTest,
    Values(std::make_tuple(true,
                           kHciVscSetPowerCapSubOpCodeLENonConnectionMode),
           std::make_tuple(false, kHciVscSetPowerCapSubOpCodeHRMode)));

TEST_F(BluetoothSarTest, HandleBuildCommandHRModeWrapper) {
  // Create sample input data.
  std::array<uint8_t, 4> chain0_cap = {10, 20, 30, 40};
  std::array<uint8_t, 4> chain1_cap = {50, 60, 70, 80};
  std::array<uint8_t, 8> beamforming_cap = {10, 20, 30, 40, 50, 60, 70, 80};

  // Test with high resolution cap.
  bool high_resolution_cap = true;
  HalPacket command_hr = bluetooth_sar_handler_->BuildCommandHRModeWrapper(
      chain0_cap, chain1_cap, beamforming_cap, high_resolution_cap, false);

  // Verify expected opcode and subopcode.
  EXPECT_EQ(command_hr[0], 0x01);
  EXPECT_EQ(command_hr[1], kHciVscSetPowerCapOpcode & 0xff);
  EXPECT_EQ(command_hr[2], (kHciVscSetPowerCapOpcode >> 8u) & 0xff);
  EXPECT_EQ(command_hr[4], kHciVscSetPowerCapSubOpCodeHRMode);
  EXPECT_EQ(command_hr[5], kHciVscSetPowerCapPlusHRCommandVersion);

  // Verify chain 0 cap values (no scaling expected in high resolution mode).
  for (size_t i = 0; i < kHciVscSetPowerCapChain0PowerLimitSizePlusHR; i++) {
    EXPECT_EQ(command_hr[6 + i], chain0_cap[i]);
  }

  // Verify chain 1 cap values (no scaling expected in high resolution mode).
  for (size_t i = 0; i < kHciVscSetPowerCapChain1PowerLimitSizePlusHR; i++) {
    EXPECT_EQ(command_hr[6 + kHciVscSetPowerCapChain0PowerLimitSizePlusHR + i],
              chain1_cap[i]);
  }

  // Verify beamforming cap values (no scaling expected in high resolution
  // mode).
  for (size_t i = 0; i < kHciVscSetPowerCapBeamformingPowerLimitSizePlusHR;
       i++) {
    EXPECT_EQ(command_hr[6 + kHciVscSetPowerCapChain0PowerLimitSizePlusHR +
                         kHciVscSetPowerCapChain1PowerLimitSizePlusHR + i],
              beamforming_cap[i]);
  }

  // Test with low resolution cap.
  high_resolution_cap = false;
  HalPacket command_lr = bluetooth_sar_handler_->BuildCommandHRModeWrapper(
      chain0_cap, chain1_cap, beamforming_cap, high_resolution_cap, false);

  // Verify cap values are scaled down for low resolution mode (assuming
  // kHciVscPowerCapScale is 2 for this example).
  for (size_t i = 0; i < kHciVscSetPowerCapChain0PowerLimitSizePlusHR; i++) {
    EXPECT_EQ(command_lr[6 + i], chain0_cap[i] / kHciVscPowerCapScale);
  }
  for (size_t i = 0; i < kHciVscSetPowerCapChain1PowerLimitSizePlusHR; i++) {
    EXPECT_EQ(command_lr[6 + kHciVscSetPowerCapChain0PowerLimitSizePlusHR + i],
              chain1_cap[i] / kHciVscPowerCapScale);
  }
  for (size_t i = 0; i < kHciVscSetPowerCapBeamformingPowerLimitSizePlusHR;
       i++) {
    EXPECT_EQ(command_lr[6 + kHciVscSetPowerCapChain0PowerLimitSizePlusHR +
                         kHciVscSetPowerCapChain1PowerLimitSizePlusHR + i],
              beamforming_cap[i] / kHciVscPowerCapScale);
  }
}

TEST_F(BluetoothSarTest, HandleBuildCommandHRModeBoundary) {
  // Test with cap exceeding 80.
  std::array<uint8_t, 4> chain0_cap = {100, 20, 30, 40};
  std::array<uint8_t, 4> chain1_cap = {50, 60, 70, 80};
  std::array<uint8_t, 8> beamforming_cap = {10, 20, 30, 40, 50, 60, 70, 80};
  HalPacket command = bluetooth_sar_handler_->BuildCommandHRModeWrapper(
      chain0_cap, chain1_cap, beamforming_cap, true, false);
  EXPECT_EQ(command[6], 80);
}

TEST_F(BluetoothSarTest, HandleBuildCommandWithArray) {
  // Create sample input data.
  std::array<uint8_t, 3> chain0_cap = {10, 20, 30};
  std::array<uint8_t, 3> chain1_cap = {50, 60, 70};
  std::array<uint8_t, 6> beamforming_cap = {10, 20, 30, 40, 50, 60};

  // Test with high resolution cap.
  bool high_resolution_cap = true;
  HalPacket command_hr = bluetooth_sar_handler_->BuildCommandWrapper(
      chain0_cap, chain1_cap, beamforming_cap, high_resolution_cap);

  // Verify expected opcode and subopcode.
  EXPECT_EQ(command_hr[0], 0x01);
  EXPECT_EQ(command_hr[1], kHciVscSetPowerCapOpcode & 0xff);
  EXPECT_EQ(command_hr[2], (kHciVscSetPowerCapOpcode >> 8u) & 0xff);
  EXPECT_EQ(command_hr[4], kHciVscSetPowerCapSubOpCodeHighResolution);

  // Verify chain 0 cap values (no scaling expected in high resolution mode).
  for (size_t i = 0; i < kHciVscSetPowerCapChain0PowerLimitSize; i++) {
    EXPECT_EQ(command_hr[5 + i], chain0_cap[i]);
  }

  // Verify chain 1 cap values (no scaling expected in high resolution mode).
  for (size_t i = 0; i < kHciVscSetPowerCapChain1PowerLimitSize; i++) {
    EXPECT_EQ(command_hr[5 + kHciVscSetPowerCapChain0PowerLimitSize + i],
              chain1_cap[i]);
  }

  // Verify beamforming cap values (no scaling expected in high resolution
  // mode).
  for (size_t i = 0; i < kHciVscSetPowerCapBeamformingPowerLimitSize; i++) {
    EXPECT_EQ(command_hr[5 + kHciVscSetPowerCapChain0PowerLimitSize +
                         kHciVscSetPowerCapChain1PowerLimitSize + i],
              beamforming_cap[i]);
  }

  // Test with low resolution cap.
  high_resolution_cap = false;
  HalPacket command_lr = bluetooth_sar_handler_->BuildCommandWrapper(
      chain0_cap, chain1_cap, beamforming_cap, high_resolution_cap);

  // Verify cap values are scaled down for low resolution mode (assuming
  // kHciVscPowerCapScale is 2 for this example).
  for (size_t i = 0; i < kHciVscSetPowerCapChain0PowerLimitSize; i++) {
    EXPECT_EQ(command_lr[5 + i], chain0_cap[i] / kHciVscPowerCapScale);
  }
  for (size_t i = 0; i < kHciVscSetPowerCapChain1PowerLimitSize; i++) {
    EXPECT_EQ(command_lr[5 + kHciVscSetPowerCapChain0PowerLimitSize + i],
              chain1_cap[i] / kHciVscPowerCapScale);
  }
  for (size_t i = 0; i < kHciVscSetPowerCapBeamformingPowerLimitSize; i++) {
    EXPECT_EQ(command_lr[5 + kHciVscSetPowerCapChain0PowerLimitSize +
                         kHciVscSetPowerCapChain1PowerLimitSize + i],
              beamforming_cap[i] / kHciVscPowerCapScale);
  }
}

TEST_F(BluetoothSarTest, HandleBuildCommandWithArrayBoundary) {
  // Test with cap exceeding 80 (should be clamped to 80).
  std::array<uint8_t, 3> chain0_cap = {100, 20, 30};
  std::array<uint8_t, 3> chain1_cap = {50, 60, 70};
  std::array<uint8_t, 6> beamforming_cap = {10, 20, 30, 40, 50, 60};
  HalPacket command = bluetooth_sar_handler_->BuildCommandWrapper(
      chain0_cap, chain1_cap, beamforming_cap, true);
  EXPECT_EQ(command[5], 80);
}

TEST_F(BluetoothSarTest, HandleBuildCommand) {
  // Create sample input data.
  uint8_t br_cap = 10;
  uint8_t edr_cap = 20;
  uint8_t ble_cap = 30;

  // Test with high resolution cap.
  bool high_resolution_cap = true;
  HalPacket command_hr = bluetooth_sar_handler_->BuildCommandWrapper(
      br_cap, edr_cap, ble_cap, high_resolution_cap);

  // Verify expected opcode and subopcode.
  EXPECT_EQ(command_hr[0], 0x01);
  EXPECT_EQ(command_hr[1], kHciVscSetPowerCapOpcode & 0xff);
  EXPECT_EQ(command_hr[2], (kHciVscSetPowerCapOpcode >> 8u) & 0xff);
  EXPECT_EQ(command_hr[4], kHciVscSetPowerCapSubOpCodeHighResolution);

  // Verify chain 0 cap values (no scaling expected in high resolution mode).
  EXPECT_EQ(command_hr[5], br_cap);
  EXPECT_EQ(command_hr[6], edr_cap);
  EXPECT_EQ(command_hr[7], ble_cap);

  // Verify chain 1 cap values (no scaling expected in high resolution mode).
  EXPECT_EQ(command_hr[8], br_cap);
  EXPECT_EQ(command_hr[9], edr_cap);
  EXPECT_EQ(command_hr[10], ble_cap);

  // Verify beamforming cap values (no scaling expected in high resolution
  // mode).
  EXPECT_EQ(command_hr[11], br_cap);
  EXPECT_EQ(command_hr[12], edr_cap);
  EXPECT_EQ(command_hr[13], ble_cap);
  EXPECT_EQ(command_hr[14], br_cap);
  EXPECT_EQ(command_hr[15], edr_cap);
  EXPECT_EQ(command_hr[16], ble_cap);

  // Test with low resolution cap.
  high_resolution_cap = false;
  HalPacket command_lr = bluetooth_sar_handler_->BuildCommandWrapper(
      br_cap, edr_cap, ble_cap, high_resolution_cap);

  // Verify cap values are scaled down for low resolution mode (assuming
  // kHciVscPowerCapScale is 2 for this example).
  EXPECT_EQ(command_lr[5], br_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[6], edr_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[7], ble_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[8], br_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[9], edr_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[10], ble_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[11], br_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[12], edr_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[13], ble_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[14], br_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[15], edr_cap / kHciVscPowerCapScale);
  EXPECT_EQ(command_lr[16], ble_cap / kHciVscPowerCapScale);
}

TEST_F(BluetoothSarTest, HandleSetBluetoothTxPowerCap) {
  uint8_t cap = 10;
  HalPacket command =
      bluetooth_sar_handler_->BuildCommandWrapper(cap, cap, cap, false);
  EXPECT_CALL(mock_hci_router_, SendCommand(command, _)).Times(1);

  EXPECT_FALSE(bluetooth_sar_handler_->SetBluetoothTxPowerCap(cap));

  EnableBluetooth();
  EXPECT_TRUE(bluetooth_sar_handler_->SetBluetoothTxPowerCap(cap));
}

TEST_F(BluetoothSarTest, HandleSetBluetoothTechBasedTxPowerCap) {
  uint8_t br_cap = 10;
  uint8_t edr_cap = 20;
  uint8_t ble_cap = 30;
  HalPacket command = bluetooth_sar_handler_->BuildCommandWrapper(
      br_cap, edr_cap, ble_cap, false);
  EXPECT_CALL(mock_hci_router_, SendCommand(command, _)).Times(1);

  EXPECT_FALSE(bluetooth_sar_handler_->SetBluetoothTechBasedTxPowerCap(
      br_cap, edr_cap, ble_cap));

  EnableBluetooth();
  EXPECT_TRUE(bluetooth_sar_handler_->SetBluetoothTechBasedTxPowerCap(
      br_cap, edr_cap, ble_cap));
}

TEST_F(BluetoothSarTest, SetBluetoothModeBasedTxPowerCap) {
  std::array<uint8_t, 3> chain0_cap = {10, 20, 30};
  std::array<uint8_t, 3> chain1_cap = {50, 60, 70};
  std::array<uint8_t, 6> beamforming_cap = {10, 20, 30, 40, 50, 60};

  HalPacket command = bluetooth_sar_handler_->BuildCommandWrapper(
      chain0_cap, chain1_cap, beamforming_cap, false);
  EXPECT_CALL(mock_hci_router_, SendCommand(command, _)).Times(1);

  EXPECT_FALSE(bluetooth_sar_handler_->SetBluetoothModeBasedTxPowerCap(
      chain0_cap, chain1_cap, beamforming_cap));

  EnableBluetooth();
  EXPECT_TRUE(bluetooth_sar_handler_->SetBluetoothModeBasedTxPowerCap(
      chain0_cap, chain1_cap, beamforming_cap));
}

TEST_F(BluetoothSarTest, SetBluetoothModeBasedTxPowerCapPlusHR) {
  std::array<uint8_t, 4> chain0_cap = {10, 20, 30, 40};
  std::array<uint8_t, 4> chain1_cap = {50, 60, 70, 80};
  std::array<uint8_t, 8> beamforming_cap = {10, 20, 30, 40, 50, 60, 70, 80};

  HalPacket command = bluetooth_sar_handler_->BuildCommandHRModeWrapper(
      chain0_cap, chain1_cap, beamforming_cap, false, false);
  EXPECT_CALL(mock_hci_router_, SendCommand(command, _)).Times(1);

  EXPECT_FALSE(bluetooth_sar_handler_->SetBluetoothModeBasedTxPowerCapPlusHR(
      chain0_cap, chain1_cap, beamforming_cap));

  EnableBluetooth();
  EXPECT_TRUE(bluetooth_sar_handler_->SetBluetoothModeBasedTxPowerCapPlusHR(
      chain0_cap, chain1_cap, beamforming_cap));
}

}  // namespace sar
}  // namespace extensions
}  // namespace bluetooth_hal
