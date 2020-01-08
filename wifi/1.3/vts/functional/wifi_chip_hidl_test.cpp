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

#include <android/hardware/wifi/1.3/IWifi.h>
#include <android/hardware/wifi/1.3/IWifiChip.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_3::IWifiChip;

namespace {
constexpr IWifiChip::LatencyMode kLatencyModeNormal =
    IWifiChip::LatencyMode::NORMAL;

constexpr IWifiChip::LatencyMode kLatencyModeLow = IWifiChip::LatencyMode::LOW;
};  // namespace

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
    // Helper function to configure the Chip in one of the supported modes.
    // Most of the non-mode-configuration-related methods require chip
    // to be first configured.
    ChipModeId configureChipForIfaceType(IfaceType type, bool expectSuccess) {
        ChipModeId mode_id;
        EXPECT_EQ(expectSuccess,
                  configureChipToSupportIfaceType(wifi_chip_, type, &mode_id));
        return mode_id;
    }

    uint32_t configureChipForStaIfaceAndGetCapabilities() {
        ChipModeId mode_id;
        EXPECT_TRUE(configureChipToSupportIfaceType(wifi_chip_, IfaceType::STA,
                                                    &mode_id));
        const auto& status_and_caps =
            HIDL_INVOKE(wifi_chip_, getCapabilities_1_3);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
        return status_and_caps.second;
    }

    sp<IWifiChip> wifi_chip_;

   private:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * SetLatencyMode_normal
 * This test case tests the setLatencyMode() API with
 * Latency mode NORMAL
 */
TEST_P(WifiChipHidlTest, SetLatencyMode_normal) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    const auto& status =
        HIDL_INVOKE(wifi_chip_, setLatencyMode, kLatencyModeNormal);
    if (caps & (IWifiChip::ChipCapabilityMask::SET_LATENCY_MODE)) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
    }
}

/*
 * SetLatencyMode_low
 * This test case tests the setLatencyMode() API with Latency mode LOW
 */
TEST_P(WifiChipHidlTest, SetLatencyMode_low) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    const auto& status =
        HIDL_INVOKE(wifi_chip_, setLatencyMode, kLatencyModeLow);
    if (caps & (IWifiChip::ChipCapabilityMask::SET_LATENCY_MODE)) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
    }
}

/*
 * GetCapabilities_1_3
 */
TEST_P(WifiChipHidlTest, GetCapabilities_1_3) {
    configureChipForIfaceType(IfaceType::STA, true);
    const auto& status_and_caps = HIDL_INVOKE(wifi_chip_, getCapabilities_1_3);
    if (status_and_caps.first.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
                  status_and_caps.first.code);
        return;
    }
    EXPECT_NE(0u, status_and_caps.second);
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiChipHidlTest,
    testing::ValuesIn(android::hardware::getAllHalInstanceNames(
        ::android::hardware::wifi::V1_3::IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
