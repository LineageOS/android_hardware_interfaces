/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <android-base/logging.h>

#include <android/hardware/wifi/1.2/IWifi.h>
#include <android/hardware/wifi/1.2/IWifiChip.h>
#include <android/hardware/wifi/1.2/IWifiChipEventCallback.h>
#include <android/hardware/wifi/1.3/IWifiChip.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <VtsHalHidlTargetCallbackBase.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiDebugRingBufferStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_2::IWifiChip;
using ::android::hardware::wifi::V1_2::IWifiChipEventCallback;
using ::android::hardware::Void;
using ::android::sp;

namespace {
constexpr IWifiChip::TxPowerScenario kPowerScenarioBody =
    IWifiChip::TxPowerScenario::ON_BODY_CELL_OFF;

constexpr IWifiChip::TxPowerScenario kPowerScenarioVoiceCall =
    IWifiChip::TxPowerScenario::VOICE_CALL;
};  // namespace

/**
 * Fixture to use for all Wifi chip HIDL interface tests.
 */
class WifiChipHidlTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    wifi_chip_ = IWifiChip::castFrom(getWifiChip(GetInstanceName()));
    ASSERT_NE(nullptr, wifi_chip_.get());
  }

  virtual void TearDown() override { stopWifi(GetInstanceName()); }

  // A simple test implementation of WifiChipEventCallback.
  class WifiChipEventCallback
      : public ::testing::VtsHalHidlTargetCallbackBase<WifiChipHidlTest>,
        public IWifiChipEventCallback {

   public:
    WifiChipEventCallback() {};

    virtual ~WifiChipEventCallback() = default;

    Return<void> onChipReconfigured(uint32_t modeId __unused) {
      return Void();
    };

    Return<void> onChipReconfigureFailure(const WifiStatus& status __unused) {
      return Void();
    };

    Return<void> onIfaceAdded(IfaceType type __unused, const hidl_string& name __unused) {
      return Void();
    };

    Return<void> onIfaceRemoved(IfaceType type __unused, const hidl_string& name __unused) {
      return Void();
    };

    Return<void> onDebugRingBufferDataAvailable(const WifiDebugRingBufferStatus& status __unused,
        const hidl_vec<uint8_t>& data __unused) {
      return Void();
    };

    Return<void> onDebugErrorAlert(int32_t errorCode __unused,
        const hidl_vec<uint8_t>& debugData __unused) {
      return Void();
    };

    Return<void> onRadioModeChange(const hidl_vec<RadioModeInfo>& radioModeInfos __unused) {
      return Void();
    };
  };

 protected:
  uint32_t configureChipForStaIfaceAndGetCapabilities() {
    ChipModeId mode_id;
    EXPECT_TRUE(
        configureChipToSupportIfaceType(wifi_chip_, IfaceType::STA, &mode_id));

    sp<::android::hardware::wifi::V1_3::IWifiChip> chip_converted =
        ::android::hardware::wifi::V1_3::IWifiChip::castFrom(wifi_chip_);

    std::pair<WifiStatus, uint32_t> status_and_caps;

    if (chip_converted != nullptr) {
        // Call the newer HAL version
        status_and_caps = HIDL_INVOKE(chip_converted, getCapabilities_1_3);
    } else {
        status_and_caps = HIDL_INVOKE(wifi_chip_, getCapabilities);
    }

    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
    return status_and_caps.second;
  }

  sp<IWifiChip> wifi_chip_;

 private:
  std::string GetInstanceName() { return GetParam(); }
};

/*
 * SelectTxPowerScenario_1_2_body
 * This test case tests the selectTxPowerScenario_1_2() API with SAR scenarios
 * newly defined in 1.2
 */
TEST_P(WifiChipHidlTest, SelectTxPowerScenario_1_2_body) {
  uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
  const auto& status =
      HIDL_INVOKE(wifi_chip_, selectTxPowerScenario_1_2, kPowerScenarioBody);
  if (caps & (IWifiChip::ChipCapabilityMask::SET_TX_POWER_LIMIT |
              IWifiChip::ChipCapabilityMask::USE_BODY_HEAD_SAR)) {
    EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
  } else {
    EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
  }
}

/*
 * SelectTxPowerScenario_1_2_voiceCall
 * This test case tests the selectTxPowerScenario_1_2() API with previously
 * defined SAR scenarios
 */
TEST_P(WifiChipHidlTest, SelectTxPowerScenario_1_2_voiceCall) {
  uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
  const auto& status =
      HIDL_INVOKE(wifi_chip_, selectTxPowerScenario_1_2, kPowerScenarioVoiceCall);
  if (caps & (IWifiChip::ChipCapabilityMask::SET_TX_POWER_LIMIT)) {
    EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
  } else {
    EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
  }
}

/*
 * registerEventCallback_1_2
 * This test case tests the registerEventCallback_1_2() API which registers
 * a call back function with the hal implementation
 *
 * Note: it is not feasible to test the invocation of the call back function
 * since event is triggered internally in the HAL implementation, and can not be
 * triggered from the test case
 */
TEST_P(WifiChipHidlTest, registerEventCallback_1_2) {
    sp<WifiChipEventCallback> wifiChipEventCallback = new WifiChipEventCallback();
    const auto& status =
        HIDL_INVOKE(wifi_chip_, registerEventCallback_1_2, wifiChipEventCallback);

    if (status.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
        return;
    }
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiChipHidlTest,
    testing::ValuesIn(android::hardware::getAllHalInstanceNames(
        ::android::hardware::wifi::V1_2::IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
