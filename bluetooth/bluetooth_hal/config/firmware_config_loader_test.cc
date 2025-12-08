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
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "bluetooth_hal/config/config_constants.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/common/test_helper.h"
#include "bluetooth_hal/test/mock/mock_android_base_wrapper.h"
#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"
#include "bluetooth_hal/test/mock/mock_system_call_wrapper.h"
#include "bluetooth_hal/test/mock/mock_transport_interface.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace config {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

using ::bluetooth_hal::config::MockHalConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::transport::MockTransportInterface;
using ::bluetooth_hal::transport::TransportType;
using ::bluetooth_hal::util::MatcherFactory;
using ::bluetooth_hal::util::MockAndroidBaseWrapper;
using ::bluetooth_hal::util::MockSystemCallWrapper;

namespace cfg_consts = ::bluetooth_hal::config::constants;

constexpr int kLoadMiniDrvDelayMs = 100;
constexpr int kLaunchRamDelayMs = 100;

constexpr std::string_view kMultiTransportValidContent = R"({
    "firmware_configs": [
      {
        "transport_type": 1,
        "firmware_folder_name": "/uart/fw/",
        "firmware_file_name": "uart_fw.bin",
        "chip_id": 101,
        "load_mini_drv_delay_ms": 51,
        "launch_ram_delay_ms": 251,
        "firmware_data_loading_type": "PACKET_BY_PACKET",
        "setup_commands": {
          "hci_reset": [1,3,12,0],
          "hci_read_chip_id": [1,121,252,0]
        }
      },
      {
        "transport_type": 100,
        "firmware_folder_name": "/vendor/fw/",
        "firmware_file_name": "vendor_fw.bin",
        "chip_id": 102,
        "load_mini_drv_delay_ms": 71,
        "launch_ram_delay_ms": 301,
        "firmware_data_loading_type": "ACCUMULATED_BUFFER",
        "fixed_size_reading": {
          "chunk_size": 200
        },
        "setup_commands": {
          "hci_update_chip_baud_rate": [1,24,252,6,0,0,0,9,61,0]
        }
      }
    ]
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
    MockHalConfigLoader::SetMockLoader(&mock_hal_config_loader_);
    MockTransportInterface::SetMockTransport(&mock_transport_interface_);

    ON_CALL(mock_transport_interface_, GetTransportType())
        .WillByDefault(Return(TransportType::kUnknown));

    FirmwareConfigLoader::ResetLoader();
  }

  void TearDown() override { MockHalConfigLoader::SetMockLoader(nullptr); }

  MockSystemCallWrapper mock_system_call_wrapper_;
  MockAndroidBaseWrapper mock_android_base_wrapper_;
  MockTransportInterface mock_transport_interface_;
  StrictMock<MockHalConfigLoader> mock_hal_config_loader_;

  static constexpr int kFile1Fd = 1;
  static constexpr int kFile2Fd = 2;
  static constexpr int kFile3Fd = 3;
};

TEST_F(FirmwareConfigLoaderTestBase, GetNextFirmwareDataOnInit) {
  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

TEST_F(FirmwareConfigLoaderTestBase, GetLoadMiniDrvDelayMsOnInit) {
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLoadMiniDrvDelayMs(),
            cfg_consts::kDefaultLoadMiniDrvDelayMs);
}

TEST_F(FirmwareConfigLoaderTestBase, GetLaunchRamDelayMsOnInit) {
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLaunchRamDelayMs(),
            cfg_consts::kDefaultLaunchRamDelayMs);
}

TEST_F(FirmwareConfigLoaderTestBase, GetFirmwareFileCountOnInit) {
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetFirmwareFileCount(), 0);
}

TEST_F(FirmwareConfigLoaderTestBase, GetFirmwareFileCountAfterLoadingConfig) {
  std::vector<TransportType> priority_list = {TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priority_list));
  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kMultiTransportValidContent));

  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetFirmwareFileCount(), 1);
}

TEST_F(FirmwareConfigLoaderTestBase, LoadConfigWithActiveTransport) {
  std::vector<TransportType> priority_list = {TransportType::kUartH4};
  EXPECT_CALL(mock_transport_interface_, GetTransportType())
      .WillOnce(Return(TransportType::kVendorStart));
  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kMultiTransportValidContent));

  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetFirmwareFileCount(), 1);
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLoadMiniDrvDelayMs(), 71);
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLaunchRamDelayMs(), 301);
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

    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(empty_transport_priority_list_));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kMultiTransportValidContent));
  }
  const std::vector<TransportType> empty_transport_priority_list_{};
};

TEST_P(LoadConfigSetupCommandTypeTest, ParsesSetupCommandOnInit) {
  const auto& [command_type, expected_command] = GetParam();
  const auto command_packet =
      FirmwareConfigLoader::GetLoader().GetSetupCommandPacket(command_type);

  EXPECT_TRUE(!command_packet.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    TestAllSetupCommandTypeConfig, LoadConfigSetupCommandTypeTest,
    Values(SetupCommandValueTestParam{.command_type = SetupCommandType::kReset,
                                      .expected_command = {}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kReadChipId,
               .expected_command = {}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kUpdateChipBaudRate,
               .expected_command = {}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kSetFastDownload,
               .expected_command = {}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kDownloadMinidrv,
               .expected_command = {}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kReadFwVersion,
               .expected_command = {}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kSetupLowPowerMode,
               .expected_command = {}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kWriteBdAddress,
               .expected_command = {}}));

class LoadConfigSetupCommandTypeTestVendor
    : public FirmwareConfigLoaderTestBase,
      public WithParamInterface<SetupCommandValueTestParam> {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();
    std::vector<TransportType> priority_list = {
        static_cast<TransportType>(TransportType::kVendorStart)};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));

    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kMultiTransportValidContent));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().SelectFirmwareConfiguration(
        static_cast<TransportType>(TransportType::kVendorStart)));
  }
};

TEST_P(LoadConfigSetupCommandTypeTestVendor, ParsesSetupCommandForVendor) {
  const auto& [command_type, expected_command] = GetParam();
  const auto command_packet =
      FirmwareConfigLoader::GetLoader().GetSetupCommandPacket(command_type);

  EXPECT_EQ(command_packet.has_value(), !expected_command.empty());
  if (!expected_command.empty()) {
    EXPECT_EQ(command_packet.value().get().GetPayload(), expected_command);
  }
}

INSTANTIATE_TEST_SUITE_P(
    TestVendorSetupCommandTypeConfig, LoadConfigSetupCommandTypeTestVendor,
    Values(SetupCommandValueTestParam{.command_type = SetupCommandType::kReset,
                                      .expected_command = {}},
           SetupCommandValueTestParam{
               .command_type = SetupCommandType::kUpdateChipBaudRate,
               .expected_command = {0x01, 0x18, 0xfc, 0x06, 0x00, 0x00, 0x00,
                                    0x09, 0x3d, 0x00}}));

TEST_F(FirmwareConfigLoaderTestBase, DumpConfigToString) {
  std::vector<TransportType> priority_list = {TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priority_list));
  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kMultiTransportValidContent));

  std::string dump = FirmwareConfigLoader::GetLoader().DumpConfigToString();
  EXPECT_NE(dump.find("Transport Type: 1"), std::string::npos);
  EXPECT_NE(dump.find("Firmware Folder: \"/uart/fw/\""), std::string::npos);
  EXPECT_NE(dump.find("Active Configuration for Transport Type: 1"),
            std::string::npos);
  EXPECT_NE(
      dump.find(
          "Data Reading Method: (Default) COMMAND_BASED"),  // Check default.
      std::string::npos);

  EXPECT_NE(dump.find("Transport Type: 100"), std::string::npos);
  EXPECT_NE(dump.find("Firmware Folder: \"/vendor/fw/\""), std::string::npos);
  EXPECT_NE(dump.find("Data Reading Method: FIXED_SIZE"),  // Check fixed size.
            std::string::npos);
  EXPECT_NE(dump.find("Chunk Size: 200 bytes"), std::string::npos);

  FirmwareConfigLoader::GetLoader().SelectFirmwareConfiguration(
      static_cast<TransportType>(TransportType::kVendorStart));
  dump = FirmwareConfigLoader::GetLoader().DumpConfigToString();
  EXPECT_NE(dump.find("Active Configuration for Transport Type: 100"),
            std::string::npos);
}

TEST_F(FirmwareConfigLoaderTestBase, LoadMultiTransportConfigAndSelect) {
  std::vector<TransportType> priority_list = {
      TransportType::kUartH4,
      static_cast<TransportType>(TransportType::kVendorStart)};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priority_list));

  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kMultiTransportValidContent));

  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLoadMiniDrvDelayMs(), 51);
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLaunchRamDelayMs(), 251);
  auto reset_cmd = FirmwareConfigLoader::GetLoader().GetSetupCommandPacket(
      SetupCommandType::kReset);
  EXPECT_TRUE(reset_cmd.has_value());
  EXPECT_EQ(reset_cmd.value().get().GetPayload(),
            std::vector<uint8_t>({0x01, 0x03, 0x0c, 0x00}));

  // Select the vendor transport (type 100).
  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().SelectFirmwareConfiguration(
      static_cast<TransportType>(TransportType::kVendorStart)));
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLoadMiniDrvDelayMs(), 71);
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLaunchRamDelayMs(), 301);
  auto baud_rate_cmd = FirmwareConfigLoader::GetLoader().GetSetupCommandPacket(
      SetupCommandType::kUpdateChipBaudRate);
  EXPECT_TRUE(baud_rate_cmd.has_value());
  EXPECT_EQ(baud_rate_cmd.value().get().GetPayload(),
            std::vector<uint8_t>(
                {0x01, 0x18, 0xfc, 0x06, 0x00, 0x00, 0x00, 0x09, 0x3d, 0x00}));

  auto non_existent_cmd =
      FirmwareConfigLoader::GetLoader().GetSetupCommandPacket(
          SetupCommandType::kReset);
  EXPECT_FALSE(non_existent_cmd.has_value());
}

TEST_F(FirmwareConfigLoaderTestBase, SelectNonExistentTransport) {
  std::vector<TransportType> priority_list = {TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priority_list));

  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kMultiTransportValidContent));
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLoadMiniDrvDelayMs(),
            51);  // kUartH4 selected.

  EXPECT_FALSE(FirmwareConfigLoader::GetLoader().SelectFirmwareConfiguration(
      static_cast<TransportType>(200)));

  // Active config is reset.
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLoadMiniDrvDelayMs(),
            cfg_consts::kDefaultLoadMiniDrvDelayMs);
}

TEST_F(FirmwareConfigLoaderTestBase, LoadConfigWithEmptyFirmwareConfigs) {
  constexpr std::string_view kEmptyFirmwareConfigs =
      R"({"firmware_configs": []})";
  std::vector<TransportType> priority_list = {TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priority_list));

  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kEmptyFirmwareConfigs));

  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLoadMiniDrvDelayMs(),
            cfg_consts::kDefaultLoadMiniDrvDelayMs);
  EXPECT_EQ(FirmwareConfigLoader::GetLoader().GetLaunchRamDelayMs(),
            cfg_consts::kDefaultLaunchRamDelayMs);
  auto cmd = FirmwareConfigLoader::GetLoader().GetSetupCommandPacket(
      SetupCommandType::kReset);
  EXPECT_FALSE(cmd.has_value());
}

TEST_F(FirmwareConfigLoaderTestBase,
       ResetFirmwareDataLoadingStateWithActiveConfig) {
  std::vector<TransportType> priority_list = {TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priority_list));
  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kMultiTransportValidContent));

  EXPECT_CALL(
      mock_system_call_wrapper_,
      Open(MatcherFactory::CreateStringMatcher("/uart/fw/uart_fw.bin"), _))
      .WillOnce(Return(123));

  EXPECT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());
}

TEST_F(FirmwareConfigLoaderTestBase,
       ResetFirmwareDataLoadingStateNoActiveConfig) {
  constexpr std::string_view kEmptyFirmwareConfigs =
      R"({"firmware_configs": []})";
  std::vector<TransportType> priority_list = {};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priority_list));

  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kEmptyFirmwareConfigs));
  EXPECT_CALL(mock_system_call_wrapper_, Open(_, _)).Times(0);
  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());
}

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

class FirmwareDataPacketByPacketTest : public FirmwareConfigLoaderTestBase {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();

    std::vector<TransportType> priority_list = {TransportType::kUartH4};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kConfigPacketByPacket));
  }

  static constexpr std::string_view kConfigPacketByPacket = R"({
    "firmware_configs": [
      {
        "transport_type": 1,
        "firmware_folder_name": "/test/fw/",
        "firmware_file_name": "test_fw_packet.bin",
        "firmware_data_loading_type": "PACKET_BY_PACKET"
      }
    ]
  })";
};

TEST_F(FirmwareDataPacketByPacketTest,
       GetNextFirmwareDataPacketByPacketReturnReadHeaderFailError) {
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Open(MatcherFactory::CreateStringMatcher("/test/fw/test_fw_packet.bin"),
           _))
      .WillOnce(Return(kFile1Fd));

  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(Return(-1));  // Error when reading header.

  auto data_packet = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  EXPECT_FALSE(data_packet.has_value());
}

TEST_F(FirmwareDataPacketByPacketTest,
       GetNextFirmwareDataPacketByPacketReturnReadPayloadFailError) {
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Open(MatcherFactory::CreateStringMatcher("/test/fw/test_fw_packet.bin"),
           _))
      .WillOnce(Return(kFile1Fd));
  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());
  std::vector<uint8_t> fw_data_packet1_header = {0x01, 0xFC, 0x02};
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_packet1_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 2))
      .WillOnce(Return(-1));  // Error.
  auto data_packet = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  EXPECT_FALSE(data_packet.has_value());
}

TEST_F(FirmwareDataPacketByPacketTest,
       GetNextFirmwareDataPacketByPacketSuccess) {
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Open(MatcherFactory::CreateStringMatcher("/test/fw/test_fw_packet.bin"),
           _))
      .WillOnce(Return(kFile1Fd));

  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> fw_data_packet1_header = {0x01, 0xFC, 0x02};
  std::vector<uint8_t> fw_data_packet1_payload = {0xAA, 0xBB};
  std::vector<uint8_t> fw_data_packet2_header = {0x4E, 0xFC, 0x01};
  std::vector<uint8_t> fw_data_packet2_payload = {0xCC};

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_packet1_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 2))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_packet1_payload.data(), 2);
                      }),
                      Return(2)));

  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);
  EXPECT_EQ(data_packet1->GetPayload(),
            HalPacket({0x01, 0x01, 0xFC, 0x02, 0xAA, 0xBB}));

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_packet2_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 1))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_packet2_payload.data(), 1);
                      }),
                      Return(1)));

  auto data_packet2 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet2.has_value());
  EXPECT_EQ(data_packet2->GetDataType(), DataType::kDataEnd);
  EXPECT_EQ(data_packet2->GetPayload(),
            HalPacket({0x01, 0x4E, 0xFC, 0x01, 0xCC}));

  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

class MultiFilePacketByPacketTest : public FirmwareConfigLoaderTestBase {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();

    std::vector<TransportType> priority_list = {TransportType::kUartH4};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kConfigMultiFilePacketByPacket));
  }

  static constexpr std::string_view kConfigMultiFilePacketByPacket = R"({
   "firmware_configs": [
     {
       "transport_type": 1,
       "firmware_folder_name": "/test/fw_multi/",
       "firmware_file_name": ["file1_packet.bin", "file2_packet.bin"],
       "firmware_data_loading_type": "PACKET_BY_PACKET"
     }
   ]
 })";
};

TEST_F(MultiFilePacketByPacketTest, ReadsDataFromMultipleFilesSuccessfully) {
  // File 1.
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw_multi/file1_packet.bin"),
                   _))
      .WillOnce(Return(kFile1Fd));

  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> file1_packet1_header = {0x01, 0xFC, 0x02};
  std::vector<uint8_t> file1_packet1_payload = {0xAA, 0xBB};
  std::vector<uint8_t> file1_launch_ram_header = {0x4E, 0xFC, 0x01};
  std::vector<uint8_t> file1_launch_ram_payload = {0xCC};

  // Read first packet from file1.
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file1_packet1_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 2))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file1_packet1_payload.data(), 2);
                      }),
                      Return(2)));

  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);
  EXPECT_EQ(data_packet1->GetPayload(),
            HalPacket({0x01, 0x01, 0xFC, 0x02, 0xAA, 0xBB}));

  // Read launch RAM from file1.
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file1_launch_ram_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 1))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file1_launch_ram_payload.data(), 1);
                      }),
                      Return(1)));
  EXPECT_CALL(mock_system_call_wrapper_, Close(kFile1Fd)).Times(1);

  auto data_packet2 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet2.has_value());
  EXPECT_EQ(data_packet2->GetDataType(), DataType::kDataEnd);
  EXPECT_EQ(data_packet2->GetPayload(),
            HalPacket({0x01, 0x4E, 0xFC, 0x01, 0xCC}));

  // File 2.
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw_multi/file2_packet.bin"),
                   _))
      .WillOnce(Return(kFile2Fd));

  std::vector<uint8_t> file2_packet1_header = {0x01, 0xFC, 0x02};
  std::vector<uint8_t> file2_packet1_payload = {0xDD, 0xEE};
  std::vector<uint8_t> file2_launch_ram_header = {0x4E, 0xFC, 0x01};
  std::vector<uint8_t> file2_launch_ram_payload = {0xFF};

  // Read first packet from file2.
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file2_packet1_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 2))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file2_packet1_payload.data(), 2);
                      }),
                      Return(2)));

  auto data_packet3 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet3.has_value());
  EXPECT_EQ(data_packet3->GetDataType(), DataType::kDataFragment);
  EXPECT_EQ(data_packet3->GetPayload(),
            HalPacket({0x01, 0x01, 0xFC, 0x02, 0xDD, 0xEE}));

  // Read launch RAM from file2 (last file).
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file2_launch_ram_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 1))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file2_launch_ram_payload.data(), 1);
                      }),
                      Return(1)));
  EXPECT_CALL(mock_system_call_wrapper_, Close(kFile2Fd)).Times(1);

  auto data_packet4 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet4.has_value());
  EXPECT_EQ(data_packet4->GetDataType(), DataType::kDataEnd);
  EXPECT_EQ(data_packet4->GetPayload(),
            HalPacket({0x01, 0x4E, 0xFC, 0x01, 0xFF}));

  // No more data.
  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

TEST_F(MultiFilePacketByPacketTest, HandlesErrorOpeningSubsequentFile) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw_multi/file1_packet.bin"),
                   _))
      .WillOnce(Return(kFile1Fd));
  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> file1_launch_ram_header = {0x4E, 0xFC, 0x01};
  std::vector<uint8_t> file1_launch_ram_payload = {0xCC};

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file1_launch_ram_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 1))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file1_launch_ram_payload.data(), 1);
                      }),
                      Return(1)));
  EXPECT_CALL(mock_system_call_wrapper_, Close(kFile1Fd)).Times(1);

  // Process the launch RAM packet from the first file
  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());

  // Mock opening the second file to fail
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw_multi/file2_packet.bin"),
                   _))
      .WillOnce(Return(-1));

  // Next call should return nullopt as the second file cannot be opened
  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

class FirmwareAccumulatedBufferTest : public FirmwareConfigLoaderTestBase {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();

    std::vector<TransportType> priority_list = {TransportType::kUartH4};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kConfigAccumulated));
  }

  static constexpr std::string_view kConfigAccumulated = R"({
       "firmware_configs": [
         {
           "transport_type": 1,
           "firmware_folder_name": "/test/fw/",
           "firmware_file_name": "test_fw_accum.bin",
           "firmware_data_loading_type": "ACCUMULATED_BUFFER"
         }
       ]
     })";
};

TEST_F(FirmwareAccumulatedBufferTest,
       GetNextFirmwareDataAccumulatedBufferReturnReadHeaderFailError) {
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Open(MatcherFactory::CreateStringMatcher("/test/fw/test_fw_accum.bin"),
           _))
      .WillOnce(Return(kFile1Fd));

  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(Return(-1));

  auto data_packet = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  EXPECT_FALSE(data_packet.has_value());
}

TEST_F(FirmwareAccumulatedBufferTest,
       GetNextFirmwareDataAccumulatedBufferReturnReadPayloadFailError) {
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Open(MatcherFactory::CreateStringMatcher("/test/fw/test_fw_accum.bin"),
           _))
      .WillOnce(Return(kFile1Fd));
  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());
  std::vector<uint8_t> fw_data_packet_header = {0x01, 0xFC, 0x05};
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_packet_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 5))
      .WillOnce(Return(-1));
  auto data_packet = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  EXPECT_FALSE(data_packet.has_value());
}

TEST_F(FirmwareAccumulatedBufferTest, GetNextFirmwareDataAccumulatedBuffer) {
  std::vector<TransportType> priority_list = {TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priority_list));
  EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
      kConfigAccumulated));

  EXPECT_CALL(
      mock_system_call_wrapper_,
      Open(MatcherFactory::CreateStringMatcher("/test/fw/test_fw_accum.bin"),
           _))
      .WillOnce(Return(kFile2Fd));

  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> fw_data_p1_hdr = {0x01, 0xFC, 0x02};
  std::vector<uint8_t> fw_data_p1_pld = {0xAA, 0xBB};
  std::vector<uint8_t> fw_data_p2_hdr = {0x02, 0xFC, 0x01};
  std::vector<uint8_t> fw_data_p2_pld = {0xDD};
  std::vector<uint8_t> fw_data_p3_hdr = {0x4E, 0xFC, 0x01};
  std::vector<uint8_t> fw_data_p3_pld = {0xEE};

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_p1_hdr.data(), 3);
                      }),
                      Return(3)))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_p2_hdr.data(), 3);
                      }),
                      Return(3)))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_p3_hdr.data(), 3);
                      }),
                      Return(3)));

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 2))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_p1_pld.data(), 2);
                      }),
                      Return(2)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 1))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_p2_pld.data(), 1);
                      }),
                      Return(1)))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, fw_data_p3_pld.data(), 1);
                      }),
                      Return(1)));

  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);
  HalPacket expected_accumulated_p1_p2(
      {0x01, 0x01, 0xFC, 0x02, 0xAA, 0xBB, 0x01, 0x02, 0xFC, 0x01, 0xDD});
  EXPECT_EQ(data_packet1->GetPayload(), expected_accumulated_p1_p2);

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);

  auto data_packet2 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet2.has_value());
  EXPECT_EQ(data_packet2->GetDataType(), DataType::kDataEnd);
  EXPECT_EQ(data_packet2->GetPayload(),
            HalPacket({0x01, 0x4E, 0xFC, 0x01, 0xEE}));

  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

TEST_F(FirmwareAccumulatedBufferTest,
       GetNextFirmwareDataAccumulatedBufferExceedsInternalBuffer) {
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Open(MatcherFactory::CreateStringMatcher("/test/fw/test_fw_accum.bin"),
           _))
      .WillOnce(Return(kFile3Fd));

  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  // kBufferSize is 32 * 1024 = 32768.

  // Packet 1: total size in buffer = 1 (type) + 3 (hdr) + 255 (pld) = 259
  // Here we have 126 packet 1.
  std::vector<uint8_t> p1_hdr = {0x01, 0xFC, 0xff};
  std::vector<uint8_t> p1_pld(255, 0xAA);

  // Packet 2: total size in buffer = 1 (type) + 3 (hdr) + 62 (pld) = 66
  std::vector<uint8_t> p2_hdr = {0x01, 0xFC, 0x3e};
  std::vector<uint8_t> p2_pld(62, 0xAA);

  // Packet 3: total size in buffer = 1 (type) + 3 (hdr) + 100 (pld) = 104.
  // P1*126 (32634) + P2 (66) + P2 (104) = 32804 > 32768. So P1*126 + P2 will be
  // returned first.
  std::vector<uint8_t> p3_hdr = {0x02, 0xFC, 0x64};
  std::vector<uint8_t> p3_pld(100, 0xBB);

  // Packet 3 (Launch RAM): total size in buffer = 1 (type) + 3 (hdr) + 1 (pld)
  // = 5.
  std::vector<uint8_t> p4_hdr = {0x4E, 0xFC, 0x01};  // kHciVscLaunchRamOpcode.
  std::vector<uint8_t> p4_pld = {0xCC};

  {
    InSequence s;

    for (int i = 0; i < 126; ++i) {
      EXPECT_CALL(mock_system_call_wrapper_, Read(kFile3Fd, _, 3))
          .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                            memcpy(buf, p1_hdr.data(), 3);
                          }),
                          Return(3)));
      EXPECT_CALL(mock_system_call_wrapper_, Read(kFile3Fd, _, p1_pld.size()))
          .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                            memcpy(buf, p1_pld.data(), p1_pld.size());
                          }),
                          Return(p1_pld.size())));
    }

    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile3Fd, _, 3))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                          memcpy(buf, p2_hdr.data(), 3);
                        }),
                        Return(3)));
    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile3Fd, _, p2_pld.size()))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                          memcpy(buf, p2_pld.data(), p2_pld.size());
                        }),
                        Return(p2_pld.size())));
    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile3Fd, _, 3))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                          memcpy(buf, p3_hdr.data(), 3);
                        }),
                        Return(3)));

    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile3Fd, _, p3_pld.size()))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                          memcpy(buf, p3_pld.data(), p3_pld.size());
                        }),
                        Return(p3_pld.size())));
  }

  // First call: 126 * P1 + P2 should be returned as it fills the buffer.
  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);
  HalPacket expected_p1;
  for (int i = 0; i < 126; ++i) {
    expected_p1.push_back(static_cast<uint8_t>(HciPacketType::kCommand));
    expected_p1.insert(expected_p1.end(), p1_hdr.begin(), p1_hdr.end());
    expected_p1.insert(expected_p1.end(), p1_pld.begin(), p1_pld.end());
  }
  expected_p1.push_back(static_cast<uint8_t>(HciPacketType::kCommand));
  expected_p1.insert(expected_p1.end(), p2_hdr.begin(), p2_hdr.end());
  expected_p1.insert(expected_p1.end(), p2_pld.begin(), p2_pld.end());
  EXPECT_EQ(data_packet1->GetPayload(), expected_p1);

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile3Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, p4_hdr.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile3Fd, _, p4_pld.size()))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, p4_pld.data(), p4_pld.size());
                      }),
                      Return(p4_pld.size())));

  // Second call: P3 should be returned.
  auto data_packet3 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet3.has_value());
  EXPECT_EQ(data_packet3->GetDataType(), DataType::kDataFragment);
  HalPacket expected_p3;
  expected_p3.push_back(static_cast<uint8_t>(HciPacketType::kCommand));
  expected_p3.insert(expected_p3.end(), p3_hdr.begin(), p3_hdr.end());
  expected_p3.insert(expected_p3.end(), p3_pld.begin(), p3_pld.end());
  EXPECT_EQ(data_packet3->GetPayload(), expected_p3);

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);

  // Third call: P4 (Launch RAM) should be returned as kDataEnd.
  auto data_packet4 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet4.has_value());
  EXPECT_EQ(data_packet4->GetDataType(), DataType::kDataEnd);
  HalPacket expected_p4;
  expected_p4.push_back(static_cast<uint8_t>(HciPacketType::kCommand));
  expected_p4.insert(expected_p4.end(), p4_hdr.begin(), p4_hdr.end());
  expected_p4.insert(expected_p4.end(), p4_pld.begin(), p4_pld.end());
  EXPECT_EQ(data_packet4->GetPayload(), expected_p4);

  // Fourth call: No more data.
  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

class MultiFileAccumulatedTest : public FirmwareConfigLoaderTestBase {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();

    std::vector<TransportType> priority_list = {TransportType::kUartH4};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kConfigMultiFileAccumulated));
  }

  static constexpr std::string_view kConfigMultiFileAccumulated = R"({
   "firmware_configs": [
     {
       "transport_type": 1,
       "firmware_folder_name": "/test/fw_multi_accum/",
       "firmware_file_name": ["file1_accum.bin", "file2_accum.bin"],
       "firmware_data_loading_type": "ACCUMULATED_BUFFER"
     }
   ]
 })";
};

TEST_F(MultiFileAccumulatedTest, ReadsDataFromMultipleFilesSuccessfully) {
  // File 1.
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw_multi_accum/file1_accum.bin"),
                   _))
      .WillOnce(Return(kFile1Fd));

  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> file1_packet1_hdr = {0x01, 0xFC, 0x02};
  std::vector<uint8_t> file1_packet1_pld = {0xAA, 0xBB};
  std::vector<uint8_t> file1_launch_ram_hdr = {0x4E, 0xFC, 0x01};
  std::vector<uint8_t> file1_launch_ram_pld = {0xCC};

  {
    InSequence s;
    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                          memcpy(buf, file1_packet1_hdr.data(), 3);
                        }),
                        Return(3)));
    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 2))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                          memcpy(buf, file1_packet1_pld.data(), 2);
                        }),
                        Return(2)));

    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                          memcpy(buf, file1_launch_ram_hdr.data(), 3);
                        }),
                        Return(3)));
    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 1))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                          memcpy(buf, file1_launch_ram_pld.data(), 1);
                        }),
                        Return(1)));

    EXPECT_CALL(mock_system_call_wrapper_, Close(kFile1Fd)).Times(1);
  }

  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);
  EXPECT_EQ(data_packet1->GetPayload(),
            HalPacket({0x01, 0x01, 0xFC, 0x02, 0xAA, 0xBB}));

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);

  auto data_packet2 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet2.has_value());
  EXPECT_EQ(data_packet2->GetDataType(), DataType::kDataEnd);
  EXPECT_EQ(data_packet2->GetPayload(),
            HalPacket({0x01, 0x4E, 0xFC, 0x01, 0xCC}));

  // File 2.
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw_multi_accum/file2_accum.bin"),
                   _))
      .WillOnce(Return(kFile2Fd));

  std::vector<uint8_t> file2_launch_ram_hdr = {0x4E, 0xFC, 0x01};
  std::vector<uint8_t> file2_launch_ram_pld = {0xFF};

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file2_launch_ram_hdr.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile2Fd, _, 1))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, file2_launch_ram_pld.data(), 1);
                      }),
                      Return(1)));
  EXPECT_CALL(mock_system_call_wrapper_, Close(kFile2Fd)).Times(1);

  auto data_packet3 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet3.has_value());
  EXPECT_EQ(data_packet3->GetDataType(), DataType::kDataEnd);
  EXPECT_EQ(data_packet3->GetPayload(),
            HalPacket({0x01, 0x4E, 0xFC, 0x01, 0xFF}));

  // No more data.
  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

class FirmwareDataPacketByPacketFixedSizeTest
    : public FirmwareConfigLoaderTestBase {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();
    std::vector<TransportType> priority_list = {TransportType::kUartH4};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kConfigPacketByPacketFixedSize));
  }

  static constexpr std::string_view kConfigPacketByPacketFixedSize = R"({
   "firmware_configs": [
     {
       "transport_type": 1,
       "firmware_folder_name": "/test/fw/",
       "firmware_file_name": "test_fw_packet_fixed.bin",
       "firmware_data_loading_type": "PACKET_BY_PACKET",
       "fixed_size_reading": { "chunk_size": 64 }
     }
   ]
 })";
  static constexpr size_t kChunkSize = 64;
};

TEST_F(FirmwareDataPacketByPacketFixedSizeTest, ReadSingleFileSuccessfully) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw/test_fw_packet_fixed.bin"),
                   _))
      .WillOnce(Return(kFile1Fd));
  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> chunk1(kChunkSize, 0xAA);
  std::vector<uint8_t> chunk2(kChunkSize / 2, 0xBB);  // Last chunk, smaller.

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, kChunkSize))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t count) {
                        memcpy(buf, chunk1.data(), count);
                      }),
                      Return(kChunkSize)));
  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);
  EXPECT_EQ(data_packet1->GetPayload(), HalPacket(chunk1));

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, kChunkSize))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t /*count*/) {
                        memcpy(buf, chunk2.data(), chunk2.size());
                      }),
                      Return(chunk2.size())));
  EXPECT_CALL(mock_system_call_wrapper_, Close(kFile1Fd)).Times(1);

  auto data_packet2 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet2.has_value());
  EXPECT_EQ(data_packet2->GetDataType(), DataType::kDataEnd);
  EXPECT_EQ(data_packet2->GetPayload(), HalPacket(chunk2));

  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

class FirmwareAccumulatedFixedSizeTest : public FirmwareConfigLoaderTestBase {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();
    std::vector<TransportType> priority_list = {TransportType::kUartH4};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kConfigAccumulatedFixedSize));
  }

  static constexpr std::string_view kConfigAccumulatedFixedSize = R"({
    "firmware_configs": [
      {
        "transport_type": 1,
        "firmware_folder_name": "/test/fw/",
        "firmware_file_name": "test_fw_accum_fixed.bin",
        "firmware_data_loading_type": "ACCUMULATED_BUFFER",
        "fixed_size_reading": { "chunk_size": 32 }
      }
    ]
  })";
  static constexpr size_t kChunkSize = 32;
};

TEST_F(FirmwareAccumulatedFixedSizeTest, AccumulateMultipleChunks) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw/test_fw_accum_fixed.bin"),
                   _))
      .WillOnce(Return(kFile1Fd));
  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> chunk1(kChunkSize, 0xA1);
  std::vector<uint8_t> chunk2(kChunkSize, 0xA2);
  std::vector<uint8_t> chunk3(10, 0xA3);  // Last partial chunk.

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, kChunkSize))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t c) {
                        memcpy(buf, chunk1.data(), c);
                      }),
                      Return(kChunkSize)))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t c) {
                        memcpy(buf, chunk2.data(), c);
                      }),
                      Return(kChunkSize)))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t /*c*/) {
                        memcpy(buf, chunk3.data(), chunk3.size());
                      }),
                      Return(chunk3.size())));
  EXPECT_CALL(mock_system_call_wrapper_, Close(1)).Times(1);

  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);

  HalPacket expected_payload1;
  expected_payload1.insert(expected_payload1.end(), chunk1.begin(),
                           chunk1.end());
  expected_payload1.insert(expected_payload1.end(), chunk2.begin(),
                           chunk2.end());
  EXPECT_EQ(data_packet1->GetPayload(), expected_payload1);

  auto data_packet2 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet2.has_value());
  EXPECT_EQ(data_packet2->GetDataType(), DataType::kDataEnd);

  HalPacket expected_payload2;
  expected_payload2.insert(expected_payload2.end(), chunk3.begin(),
                           chunk3.end());
  EXPECT_EQ(data_packet2->GetPayload(), expected_payload2);

  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

TEST_F(FirmwareAccumulatedFixedSizeTest, ExceedsInternalBuffer) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw/test_fw_accum_fixed.bin"),
                   _))
      .WillOnce(Return(kFile1Fd));
  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  constexpr size_t kInternalBufferSize = 32 * 1024;
  const size_t num_full_chunks_to_fill_buffer =
      kInternalBufferSize / kChunkSize;  // 1024 chunks.

  std::vector<uint8_t> chunk(kChunkSize, 0xAA);
  std::vector<uint8_t> last_chunk(10, 0xBB);

  HalPacket expected_first_batch;
  {
    InSequence s;
    for (size_t i = 0; i < num_full_chunks_to_fill_buffer; ++i) {
      EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, kChunkSize))
          .WillOnce(DoAll(Invoke([&](int, void* buf, size_t c) {
                            memcpy(buf, chunk.data(), c);
                          }),
                          Return(kChunkSize)));
      expected_first_batch.insert(expected_first_batch.end(), chunk.begin(),
                                  chunk.end());
    }
    // This read will cause the buffer to be full, and its result stored in
    // previous_packet_
    EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, kChunkSize))
        .WillOnce(DoAll(Invoke([&](int, void* buf, size_t c) {
                          memcpy(buf, chunk.data(), c);
                        }),
                        Return(kChunkSize)));
  }

  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);
  EXPECT_EQ(data_packet1->GetPayload().size(), kInternalBufferSize);
  EXPECT_EQ(data_packet1->GetPayload(), expected_first_batch);

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);

  // Next call should return the chunk stored in previous_packet_ and then the
  // last_chunk.
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, kChunkSize))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t /*c*/) {
                        memcpy(buf, last_chunk.data(), last_chunk.size());
                      }),
                      Return(last_chunk.size())));
  EXPECT_CALL(mock_system_call_wrapper_, Close(kFile1Fd)).Times(1);

  auto data_packet2 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet2.has_value());
  EXPECT_EQ(data_packet2->GetDataType(), DataType::kDataFragment);

  HalPacket expected_second_batch;
  expected_second_batch.insert(expected_second_batch.end(), chunk.begin(),
                               chunk.end());  // From previous_packet_
  EXPECT_EQ(data_packet2->GetPayload(), expected_second_batch);

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);

  auto data_packet3 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet3.has_value());
  EXPECT_EQ(data_packet3->GetDataType(), DataType::kDataEnd);

  HalPacket expected_third_batch;
  expected_third_batch.insert(expected_third_batch.end(), last_chunk.begin(),
                              last_chunk.end());
  EXPECT_EQ(data_packet3->GetPayload(), expected_third_batch);

  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

class FirmwareDataCommandBasedCustomOpcodeTest
    : public FirmwareConfigLoaderTestBase {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();
    std::vector<TransportType> priority_list = {TransportType::kUartH4};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kConfigCommandBasedCustomOpcode));
  }
  static constexpr uint16_t kCustomLaunchRamOpcode = 0xABCD;
  static constexpr std::string_view kConfigCommandBasedCustomOpcode = R"({
    "firmware_configs": [
      {
        "transport_type": 1,
        "firmware_folder_name": "/test/fw/",
        "firmware_file_name": "test_fw_cmd_custom_op.bin",
        "command_based_reading": { "launch_ram_opcode": 43981 }
      }
    ]
  })";
};

TEST_F(FirmwareDataCommandBasedCustomOpcodeTest, UsesCustomOpcode) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw/test_fw_cmd_custom_op.bin"),
                   _))
      .WillOnce(Return(kFile1Fd));
  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> packet1_header = {0x01, 0xFC, 0x02};  // Normal command.
  std::vector<uint8_t> packet1_payload = {0xAA, 0xBB};
  std::vector<uint8_t> packet2_header = {
      static_cast<uint8_t>(kCustomLaunchRamOpcode & 0xFF),
      static_cast<uint8_t>((kCustomLaunchRamOpcode >> 8) & 0xFF),
      0x01};  // Custom Launch RAM
  std::vector<uint8_t> packet2_payload = {0xCC};

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, packet1_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 2))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, packet1_payload.data(), 2);
                      }),
                      Return(2)));

  auto data_packet1 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet1.has_value());
  EXPECT_EQ(data_packet1->GetDataType(), DataType::kDataFragment);

  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 3))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, packet2_header.data(), 3);
                      }),
                      Return(3)));
  EXPECT_CALL(mock_system_call_wrapper_, Read(kFile1Fd, _, 1))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t) {
                        memcpy(buf, packet2_payload.data(), 1);
                      }),
                      Return(1)));
  EXPECT_CALL(mock_system_call_wrapper_, Close(kFile1Fd)).Times(1);

  auto data_packet2 = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet2.has_value());
  EXPECT_EQ(data_packet2->GetDataType(),
            DataType::kDataEnd);  // Should be kDataEnd due to custom opcode.

  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

class FirmwareDataFixedSizeDefaultChunkTest
    : public FirmwareConfigLoaderTestBase {
 protected:
  void SetUp() override {
    FirmwareConfigLoaderTestBase::SetUp();
    std::vector<TransportType> priority_list = {TransportType::kUartH4};
    EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
        .WillRepeatedly(ReturnRef(priority_list));
    EXPECT_TRUE(FirmwareConfigLoader::GetLoader().LoadConfigFromString(
        kConfigFixedSizeDefaultChunk));
  }

  static constexpr std::string_view kConfigFixedSizeDefaultChunk = R"({
   "firmware_configs": [
     {
       "transport_type": 1,
       "firmware_folder_name": "/test/fw/",
       "firmware_file_name": "test_fw_fixed_default_chunk.bin",
       "firmware_data_loading_type": "PACKET_BY_PACKET",
       "fixed_size_reading": { }
     }
   ]
 })";
};

TEST_F(FirmwareDataFixedSizeDefaultChunkTest, UsesDefaultChunkSize) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Open(MatcherFactory::CreateStringMatcher(
                       "/test/fw/test_fw_fixed_default_chunk.bin"),
                   _))
      .WillOnce(Return(kFile1Fd));
  ASSERT_TRUE(
      FirmwareConfigLoader::GetLoader().ResetFirmwareDataLoadingState());

  std::vector<uint8_t> chunk(cfg_consts::kDefaultFixedChunkSize, 0xCC);

  EXPECT_CALL(mock_system_call_wrapper_,
              Read(kFile1Fd, _, cfg_consts::kDefaultFixedChunkSize))
      .WillOnce(DoAll(Invoke([&](int, void* buf, size_t count) {
                        memcpy(buf, chunk.data(), count);
                      }),
                      Return(cfg_consts::kDefaultFixedChunkSize)));

  auto data_packet = FirmwareConfigLoader::GetLoader().GetNextFirmwareData();
  ASSERT_TRUE(data_packet.has_value());
  EXPECT_EQ(data_packet->GetDataType(), DataType::kDataFragment);
  EXPECT_EQ(data_packet->GetPayload(), HalPacket(chunk));

  // Simulate EOF for next read.
  EXPECT_CALL(mock_system_call_wrapper_,
              Read(kFile1Fd, _, cfg_consts::kDefaultFixedChunkSize))
      .WillOnce(Return(0));
  EXPECT_CALL(mock_system_call_wrapper_, Close(kFile1Fd)).Times(1);
  EXPECT_FALSE(
      FirmwareConfigLoader::GetLoader().GetNextFirmwareData().has_value());
}

}  // namespace
}  // namespace config
}  // namespace bluetooth_hal
