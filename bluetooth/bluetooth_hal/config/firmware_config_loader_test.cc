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

#include "bluetooth_hal/config/firmware_config_loader.h"

#include <cstddef>
#include <string_view>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/test/mock/mock_android_base_wrapper.h"
#include "bluetooth_hal/test/mock/mock_system_call_wrapper.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace config {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::util::MockAndroidBaseWrapper;
using ::bluetooth_hal::util::MockSystemCallWrapper;

constexpr int kDefaultLoadMiniDrvDelayMs = 50;
constexpr int kDefaultLaunchRamDelayMs = 250;

constexpr int kLoadMiniDrvDelayMs = 100;
constexpr int kLaunchRamDelayMs = 100;

const std::string kValidContent = R"({
  "firmware_folder_name": "/PATH/TO/FOLDER/",
  "firmware_file_name": "FILENAME",
  "chip_id": 123,
  "load_mini_drv_delay_ms": 100,
  "launch_ram_delay_ms": 100,
  "firmware_data_loading_type": "PACKET_BY_PACKET",
  "setup_commands": {
    "hci_reset": [1, 3, 12, 0],
    "hci_read_chip_id": [1, 121, 252, 0],
    "hci_update_chip_baud_rate": [1, 24, 252, 6, 0, 0, 0, 9, 61, 0],
    "hci_set_fast_download": [1, 114, 254, 2, 0, 1],
    "hci_download_minidrv": [1, 46, 252, 0],
    "hci_vsc_launch_ram": [1, 78, 252, 4, 255, 255, 255, 255],
    "hci_read_fw_version": [1, 20, 12, 0],
    "hci_setup_low_power_mode": [1, 39, 252, 12, 1, 24, 24, 1, 1, 1, 1, 0, 0, 0, 0, 0],
    "hci_write_bd_address": [1, 1, 252, 6, 30, 77, 19, 43, 213, 232]
  }
})";

TEST(FirmwareConfigLoaderTest, IsSameSingleton) {
  MockSystemCallWrapper mock_system_call_wrapper;
  MockSystemCallWrapper::SetMockWrapper(&mock_system_call_wrapper);

  MockAndroidBaseWrapper mock_android_base_wrapper;
  MockAndroidBaseWrapper::SetMockWrapper(&mock_android_base_wrapper);

  EXPECT_EQ(&FirmwareConfigLoader::GetLoader(),
            &FirmwareConfigLoader::GetLoader());
}

class FirmwareConfigLoaderTestBase : public Test {
 protected:
  void SetUp() override {
    MockSystemCallWrapper::SetMockWrapper(&mock_system_call_wrapper_);
    MockAndroidBaseWrapper::SetMockWrapper(&mock_android_base_wrapper_);

    FirmwareConfigLoader::ResetLoader();
  }

  void TearDown() override {}

  MockSystemCallWrapper mock_system_call_wrapper_;
  MockAndroidBaseWrapper mock_android_base_wrapper_;
};

TEST_F(FirmwareConfigLoaderTestBase, GetNextFirmwareDataOnInit) {
  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

TEST_F(FirmwareConfigLoaderTestBase, GetLoadMiniDrvDelayMsOnInit) {
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLoadMiniDrvDelayMs(),
            kDefaultLoadMiniDrvDelayMs);
}

TEST_F(FirmwareConfigLoaderTestBase, GetLaunchRamDelayMsOnInit) {
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLaunchRamDelayMs(),
            kDefaultLaunchRamDelayMs);
}

struct SetupCommandValueTestParam {
  SetupCommandType command_type;
  std::vector<uint8_t> expected_command;
};

class LoadConfigSetupCommandTypeTest
    : public FirmwareConfigLoaderTestBase,
      public WithParamInterface<SetupCommandValueTestParam> {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();
    EXPECT_TRUE(
        FirmwareConfigLoader::GetLoader().LoadConfigFromString(kValidContent));
  }
};

TEST_P(LoadConfigSetupCommandTypeTest, ParsesSetupCommand) {
  const auto& [command_type, expected_command] = GetParam();
  const auto command_packet =
      FirmwareConfigLoader::GetLoader().GetSetupCommandPacket(command_type);

  EXPECT_EQ(command_packet.has_value(), !expected_command.empty());
  if (!expected_command.empty()) {
    EXPECT_EQ(command_packet.value().get().GetPayload(), expected_command);
  }
}

INSTANTIATE_TEST_SUITE_P(
    TestAllSetupCommandTypeConfig, LoadConfigSetupCommandTypeTest,
    Values(SetupCommandValueTestParam{.command_type = SetupCommandType::kReset,
                                      .expected_command = {0x01, 0x03, 0x0c,
                                                           0x00}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kReadChipId,
               .expected_command = {0x01, 0x79, 0xfc, 0x00}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kUpdateChipBaudRate,
               .expected_command = {0x01, 0x18, 0xfc, 0x06, 0x00, 0x00, 0x00,
                                    0x09, 0x3d, 0x00}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kSetFastDownload,
               .expected_command = {0x01, 0x72, 0xfe, 0x02, 0x00, 0x01}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kDownloadMinidrv,
               .expected_command = {0x01, 0x2e, 0xfc, 0x00}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kLaunchRam,
               .expected_command = {0x01, 0x4e, 0xfc, 0x04, 0xff, 0xff, 0xff,
                                    0xff}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kReadFwVersion,
               .expected_command = {0x01, 0x14, 0x0c, 0x00}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kSetupLowPowerMode,
               .expected_command = {0x01, 0x27, 0xfc, 0x0c, 0x01, 0x18, 0x18,
                                    0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
                                    0x00, 0x00}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kWriteBdAddress,
               .expected_command = {0x01, 0x01, 0xfc, 0x06, 0x1e, 0x4d, 0x13,
                                    0x2b, 0xd5, 0xe8}}));

TEST(FirmwarePacketTest, HandleDifferentFirmwarePacketType) {
  std::vector<uint8_t> payload = {0x01};

  for (int i = 0; i <= static_cast<int>(FirmwarePacketType::kData); ++i) {
    FirmwarePacketType packet_type = static_cast<FirmwarePacketType>(i);
    FirmwarePacket packet(packet_type, payload);
    EXPECT_EQ(packet.GetPacketType(), packet_type);
  }
}

TEST(FirmwarePacketTest, HandlePayload) {
  std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
  FirmwarePacket packet(FirmwarePacketType::kData, payload);
  EXPECT_EQ(packet.GetPayload(), payload);
}

TEST(FirmwarePacketTest, HandleEmptyPayload) {
  FirmwarePacket packet(FirmwarePacketType::kData, {});
  EXPECT_TRUE(packet.GetPayload().empty());
}

TEST(SetupCommandPacketTest, HandleDifferentSetupCommandPacketTypes) {
  std::vector<uint8_t> payload = {0x01};

  for (int i = 0; i <= static_cast<int>(SetupCommandType::kWriteBdAddress);
       ++i) {
    SetupCommandType command_type = static_cast<SetupCommandType>(i);
    SetupCommandPacket packet(command_type, payload);
    EXPECT_EQ(packet.GetCommandType(), command_type);
  }
}

TEST(DataPacketTest, HandleDifferentDataPacketTypes) {
  std::vector<uint8_t> payload = {0x01};

  for (int i = 0; i <= static_cast<int>(DataType::kDataEnd); ++i) {
    DataType data_type = static_cast<DataType>(i);
    DataPacket packet(data_type, payload);
    EXPECT_EQ(packet.GetDataType(), data_type);
  }
}

}  // namespace
}  // namespace config
}  // namespace bluetooth_hal
