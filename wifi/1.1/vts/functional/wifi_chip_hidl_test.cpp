/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android/hardware/wifi/1.1/IWifi.h>
#include <android/hardware/wifi/1.1/IWifiChip.h>
#include <android/hardware/wifi/1.3/IWifiChip.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::ChipId;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_1::IWifi;
using ::android::hardware::wifi::V1_1::IWifiChip;
using ::android::hardware::wifi::V1_0::IWifiStaIface;

namespace {
constexpr IWifiChip::TxPowerScenario kFakePowerScenario =
    IWifiChip::TxPowerScenario::VOICE_CALL;
}; //namespace

/**
 * Fixture to use for all Wifi chip HIDL interface tests.
 */
class WifiChipHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure to start with a clean state
        stopWifi(GetInstanceName());

        wifi_chip_ = IWifiChip::castFrom(getWifiChip(GetInstanceName()));
        ASSERT_NE(nullptr, wifi_chip_.get());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    uint32_t configureChipForStaIfaceAndGetCapabilities() {
        ChipModeId mode_id;
        EXPECT_TRUE(configureChipToSupportIfaceType(
            wifi_chip_, IfaceType::STA, &mode_id));

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
 * SelectTxPowerScenario
 */
TEST_P(WifiChipHidlTest, SelectTxPowerScenario) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    const auto& status =
        HIDL_INVOKE(wifi_chip_, selectTxPowerScenario, kFakePowerScenario);
    if (caps & IWifiChip::ChipCapabilityMask::SET_TX_POWER_LIMIT) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
    }
}

/*
 * ResetTxPowerScenario
 */
TEST_P(WifiChipHidlTest, ResetTxPowerScenario) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    const auto& status =
        HIDL_INVOKE(wifi_chip_, resetTxPowerScenario);
    if (caps & IWifiChip::ChipCapabilityMask::SET_TX_POWER_LIMIT) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
    }
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiChipHidlTest,
    testing::ValuesIn(
        android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
