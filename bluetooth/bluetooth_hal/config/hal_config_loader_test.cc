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

#include "bluetooth_hal/config/hal_config_loader.h"

#include <string>
#include <string_view>
#include <vector>

#include "bluetooth_hal/config/config_constants.h"
#include "bluetooth_hal/config/config_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/mock/mock_android_base_wrapper.h"
#include "bluetooth_hal/test/mock/mock_transport_interface.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace config {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

using ::bluetooth_hal::Property;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::transport::MockTransportInterface;
using ::bluetooth_hal::transport::TransportInterface;
using ::bluetooth_hal::transport::TransportType;
using ::bluetooth_hal::uart::BaudRate;
using ::bluetooth_hal::util::MockAndroidBaseWrapper;

namespace cfg_consts = ::bluetooth_hal::config::constants;

constexpr std::string_view kTestUartDevicePort = "/dev/ttySAC18";
constexpr int kTestVendorTransportCrashIntervalSec = 3000;
constexpr int kTestBtRegOnDelayMs = 500;
constexpr int kTestKernelRxWakelockTimeMs = 200;
constexpr std::string_view kTestLpmEnableProcNode =
    "/test/proc/bluetooth/sleep/lpm";
constexpr std::string_view kTestLpmWakingProcNode =
    "/test/proc/bluetooth/sleep/btwrite";
constexpr std::string_view kTestLpmWakelockCtrlProcNode =
    "/test/proc/bluetooth/sleep/wakelock_ctrl";
constexpr std::string_view kTestRfkillFolderPrefix =
    "/test/sys/class/rfkill/rfkill";
constexpr std::string_view kTestRfkillTypeBluetooth = "testbluetooth";

constexpr std::string_view kValidContent = R"({
  "fast_download_enabled": true,
  "sar_backoff_high_resolution_enabled": true,
  "reg_on_delay_ms": 500,
  "uart_device_port": "/dev/ttySAC18",
  "transport_type_priority": [1],
  "accelerated_bt_on_enabled": true,
  "thread_dispatcher_enabled": true,
  "bt_power_controlled_by_lpp": true,
  "hw_stages_without_lpp_control_bt_power_pin": [
    "stage1",
    "stage2"
  ],
  "fw_unsupported_hw_stages": [
    "stage1",
    "stage2"
  ],
  "vendor_transport_crash_interval_sec": 3000,
  "hp_uart_skip_suspend_enabled": true,
  "energy_controller_logging_enabled": true,
  "self_restart_recovery_enabled": true,
  "ble_non_connection_sar_enabled": true,
  "kernel_rx_wakelock_time_ms": 200,
  "low_power_mode_enabled": true,
  "bqr_event_mask": "456",
  "ldac_quality_mode": "mode1",
  "transport_fallback_type": 1,
  "lpm_enable_proc_node": "/test/proc/bluetooth/sleep/lpm",
  "lpm_waking_proc_node": "/test/proc/bluetooth/sleep/btwrite",
  "lpm_wakelock_ctrl_proc_node": "/test/proc/bluetooth/sleep/wakelock_ctrl",
  "rfkill_folder_prefix": "/test/sys/class/rfkill/rfkill",
  "rfkill_type_bluetooth": "testbluetooth"
})";

class ConfigLoaderTestBase : public Test {
 protected:
  void SetUp() override {
    MockAndroidBaseWrapper::SetMockWrapper(&mock_android_base_wrapper_);
    MockTransportInterface::SetMockTransport(&mock_transport_interface_);

    HalConfigLoader::ResetLoader();
  }

  void SetupSetPropertyExpectations(const std::string& name,
                                    const std::string& value) {
    EXPECT_CALL(mock_android_base_wrapper_, SetProperty(name, value))
        .Times(1)
        .WillOnce(Return(true));
  }

  MockAndroidBaseWrapper mock_android_base_wrapper_;
  MockTransportInterface mock_transport_interface_;
};

TEST_F(ConfigLoaderTestBase, IsFastDownloadEnabledOnInit) {
  EXPECT_FALSE(HalConfigLoader::GetLoader().IsFastDownloadEnabled());
}

TEST_F(ConfigLoaderTestBase, IsSarBackoffHighResolutionEnabledOnInit) {
  EXPECT_FALSE(
      HalConfigLoader::GetLoader().IsSarBackoffHighResolutionEnabled());
}

TEST_F(ConfigLoaderTestBase, GetBtRegOnDelayMsOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetBtRegOnDelayMs(),
            cfg_consts::kDefaultBtRegOnDelay);
}

TEST_F(ConfigLoaderTestBase, GetBtUartDevicePortOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetBtUartDevicePort(),
            cfg_consts::kDefaultBtUartDevicePort);
}

TEST_F(ConfigLoaderTestBase, GetTransportTypePriorityOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetTransportTypePriority(),
            (std::vector<TransportType>{TransportType::kUartH4}));
}

TEST_F(ConfigLoaderTestBase, IsAcceleratedBtOnSupportedOnInit) {
  EXPECT_FALSE(HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported());
}

TEST_F(ConfigLoaderTestBase, IsThreadDispatcherEnabledOnInit) {
  EXPECT_FALSE(HalConfigLoader::GetLoader().IsThreadDispatcherEnabled());
}

TEST_F(ConfigLoaderTestBase, IsBtPowerControlledByLppOnInit) {
  EXPECT_FALSE(HalConfigLoader::GetLoader().IsBtPowerControlledByLpp());
}

TEST_F(ConfigLoaderTestBase, GetHwStagesWithoutLppControlBtPowerPinOnInit) {
  EXPECT_TRUE(HalConfigLoader::GetLoader()
                  .GetHwStagesWithoutLppControlBtPowerPin()
                  .empty());
}

TEST_F(ConfigLoaderTestBase, GetFwUnsupportedHwStagesOnInit) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().GetFwUnsupportedHwStages().empty());
}

TEST_F(ConfigLoaderTestBase, GetVendorTransportCrashIntervalSecOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetVendorTransportCrashIntervalSec(),
            cfg_consts::kDefaultVendorTransportCrashIntervalSec);
}

TEST_F(ConfigLoaderTestBase, IsHpUartSkipSuspendSupportedOnInit) {
  EXPECT_FALSE(HalConfigLoader::GetLoader().IsHpUartSkipSuspendSupported());
}

TEST_F(ConfigLoaderTestBase, IsEnergyControllerLoggingSupportedOnInit) {
  EXPECT_FALSE(
      HalConfigLoader::GetLoader().IsEnergyControllerLoggingSupported());
}

TEST_F(ConfigLoaderTestBase, IsBtHalRestartRecoverySupportedOnInit) {
  EXPECT_FALSE(HalConfigLoader::GetLoader().IsBtHalRestartRecoverySupported());
}

TEST_F(ConfigLoaderTestBase, IsBleNonConnectionSarEnabledOnInit) {
  EXPECT_FALSE(HalConfigLoader::GetLoader().IsBleNonConnectionSarEnabled());
}

TEST_F(ConfigLoaderTestBase, GetKernelRxWakelockTimeMillisecondsOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetKernelRxWakelockTimeMilliseconds(),
            0);
}

TEST_F(ConfigLoaderTestBase, GetLpmEnableProcNodeOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetLpmEnableProcNode(),
            cfg_consts::kLpmEnableProcNode);
}

TEST_F(ConfigLoaderTestBase, GetLpmWakingProcNodeOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetLpmWakingProcNode(),
            cfg_consts::kLpmWakingProcNode);
}

TEST_F(ConfigLoaderTestBase, GetLpmWakelockCtrlProcNodeOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetLpmWakelockCtrlProcNode(),
            cfg_consts::kLpmWakelockCtrlProcNode);
}

TEST_F(ConfigLoaderTestBase, GetRfkillFolderPrefixOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetRfkillFolderPrefix(),
            cfg_consts::kRfkillFolderPrefix);
}

TEST_F(ConfigLoaderTestBase, GetRfkillTypeBluetoothOnInit) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetRfkillTypeBluetooth(),
            cfg_consts::kRfkillTypeBluetooth);
}

class ConfigLoaderProtoTest : public ConfigLoaderTestBase {
 protected:
  void SetUp() override {
    ConfigLoaderTestBase::SetUp();
    EXPECT_TRUE(
        HalConfigLoader::GetLoader().LoadConfigFromString(kValidContent));
  }
};

TEST_F(ConfigLoaderProtoTest, IsFastDownloadEnabled) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsFastDownloadEnabled());
}

TEST_F(ConfigLoaderProtoTest, IsSarBackoffHighResolutionEnabled) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsSarBackoffHighResolutionEnabled());
}

TEST_F(ConfigLoaderProtoTest, GetBtRegOnDelayMs) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetBtRegOnDelayMs(),
            kTestBtRegOnDelayMs);
}

TEST_F(ConfigLoaderProtoTest, GetBtUartDevicePort) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetBtUartDevicePort(),
            kTestUartDevicePort);
}

TEST_F(ConfigLoaderProtoTest, GetTransportTypePriority) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetTransportTypePriority(),
            (std::vector<TransportType>{TransportType::kUartH4}));
}

TEST_F(ConfigLoaderProtoTest, IsAcceleratedBtOnSupported) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported());
}

TEST_F(ConfigLoaderProtoTest, IsThreadDispatcherEnabled) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsThreadDispatcherEnabled());
}

TEST_F(ConfigLoaderProtoTest, IsBtPowerControlledByLpp) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsBtPowerControlledByLpp());
}

TEST_F(ConfigLoaderProtoTest, GetHwStagesWithoutLppControlBtPowerPin) {
  EXPECT_EQ(
      HalConfigLoader::GetLoader().GetHwStagesWithoutLppControlBtPowerPin(),
      (std::vector<std::string>{"stage1", "stage2"}));
}

TEST_F(ConfigLoaderProtoTest, GetFwUnsupportedHwStages) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetFwUnsupportedHwStages(),
            (std::vector<std::string>{"stage1", "stage2"}));
}

TEST_F(ConfigLoaderProtoTest, GetVendorTransportCrashIntervalSec) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetVendorTransportCrashIntervalSec(),
            kTestVendorTransportCrashIntervalSec);
}

TEST_F(ConfigLoaderProtoTest, IsHpUartSkipSuspendSupported) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsHpUartSkipSuspendSupported());
}

TEST_F(ConfigLoaderProtoTest, IsEnergyControllerLoggingSupported) {
  EXPECT_TRUE(
      HalConfigLoader::GetLoader().IsEnergyControllerLoggingSupported());
}

TEST_F(ConfigLoaderProtoTest, IsBtHalRestartRecoverySupported) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsBtHalRestartRecoverySupported());
}

TEST_F(ConfigLoaderProtoTest, IsBleNonConnectionSarEnabled) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsBleNonConnectionSarEnabled());
}

TEST_F(ConfigLoaderProtoTest, GetKernelRxWakelockTimeMilliseconds) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetKernelRxWakelockTimeMilliseconds(),
            kTestKernelRxWakelockTimeMs);
}

TEST_F(ConfigLoaderProtoTest, IsLowPowerModeSupported) {
  EXPECT_TRUE(HalConfigLoader::GetLoader().IsLowPowerModeSupported());
}

TEST_F(ConfigLoaderProtoTest, GetLpmEnableProcNode) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetLpmEnableProcNode(),
            kTestLpmEnableProcNode);
}

TEST_F(ConfigLoaderProtoTest, GetLpmWakingProcNode) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetLpmWakingProcNode(),
            kTestLpmWakingProcNode);
}

TEST_F(ConfigLoaderProtoTest, GetLpmWakelockCtrlProcNode) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetLpmWakelockCtrlProcNode(),
            kTestLpmWakelockCtrlProcNode);
}

TEST_F(ConfigLoaderProtoTest, GetRfkillFolderPrefix) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetRfkillFolderPrefix(),
            kTestRfkillFolderPrefix);
}

TEST_F(ConfigLoaderProtoTest, GetRfkillTypeBluetooth) {
  EXPECT_EQ(HalConfigLoader::GetLoader().GetRfkillTypeBluetooth(),
            kTestRfkillTypeBluetooth);
}

class ConfigLoaderUtilTest : public ConfigLoaderTestBase {};

TEST_F(ConfigLoaderUtilTest, IsBtSnoopLogFullModeOnReturnsTrue) {
  HalConfigLoader& config_loader = HalConfigLoader::GetLoader();
  Mock::VerifyAndClearExpectations(&mock_android_base_wrapper_);

  EXPECT_CALL(mock_android_base_wrapper_,
              GetProperty(Property::kBtSnoopLogMode, _))
      .Times(1)
      .WillOnce(Return("full"));
  EXPECT_TRUE(config_loader.IsBtSnoopLogFullModeOn());
}

TEST_F(ConfigLoaderUtilTest, GetUartBaudRateWithUartH4Interface) {
  EXPECT_EQ(
      HalConfigLoader::GetLoader().GetUartBaudRate(TransportType::kUartH4),
      BaudRate::kRate4000000);
}

TEST_F(ConfigLoaderUtilTest, IsUserDebugOrEngBuildReturnsTrue) {
  ON_CALL(mock_android_base_wrapper_, GetProperty(Property::kBuildType, _))
      .WillByDefault(Return("userdebug"));

  EXPECT_TRUE(HalConfigLoader::GetLoader().IsUserDebugOrEngBuild());
}

TEST_F(ConfigLoaderUtilTest, IsUserDebugOrEngBuildReturnsFalse) {
  ON_CALL(mock_android_base_wrapper_, GetProperty(Property::kBuildType, _))
      .WillByDefault(Return("user"));

  EXPECT_FALSE(HalConfigLoader::GetLoader().IsUserDebugOrEngBuild());
}

TEST_F(ConfigLoaderUtilTest, TransportFallbackEnabled) {
  SetupSetPropertyExpectations(Property::kIsAcceleratedBtOnEnabled, "false");
  SetupSetPropertyExpectations(Property::kTransportFallbackEnabled, "true");

  EnableTransportFallback();
}

TEST_F(ConfigLoaderUtilTest, IsSameSingleton) {
  EXPECT_EQ(&HalConfigLoader::GetLoader(), &HalConfigLoader::GetLoader());
}

}  // namespace
}  // namespace config
}  // namespace bluetooth_hal
