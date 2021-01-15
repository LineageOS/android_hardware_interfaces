/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Staache License, Version 2.0 (the "License");
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

#include <numeric>
#include <vector>

#include <android-base/logging.h>

#include <android/hardware/wifi/1.5/IWifi.h>
#include <android/hardware/wifi/1.5/IWifiChip.h>
#include <android/hardware/wifi/1.5/IWifiStaIface.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_5::IWifiChip;
using ::android::hardware::wifi::V1_5::IWifiStaIface;

/**
 * Fixture to use for all STA Iface HIDL interface tests.
 */
class WifiStaIfaceHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure to start with a clean state
        stopWifi(GetInstanceName());

        wifi_sta_iface_ =
            IWifiStaIface::castFrom(getWifiStaIface(GetInstanceName()));
        ASSERT_NE(nullptr, wifi_sta_iface_.get());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    bool isCapabilitySupported(IWifiStaIface::StaIfaceCapabilityMask cap_mask) {
        const auto& status_and_caps =
            HIDL_INVOKE(wifi_sta_iface_, getCapabilities);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
        return (status_and_caps.second & cap_mask) != 0;
    }

    WifiStatusCode createStaIface(sp<IWifiStaIface>* sta_iface) {
        sp<IWifiChip> wifi_chip =
            IWifiChip::castFrom(getWifiChip(GetInstanceName()));
        EXPECT_NE(nullptr, wifi_chip.get());
        const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createStaIface);
        *sta_iface = IWifiStaIface::castFrom(status_and_iface.second);
        return status_and_iface.first.code;
    }

    sp<IWifiStaIface> wifi_sta_iface_;

   private:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * GetLinkLayerStats_1_5
 * Ensures that calls to get link layer stats V1_5 will retrieve a non-empty
 * StaLinkLayerStats after link layer stats collection is enabled.
 */
TEST_P(WifiStaIfaceHidlTest, GetLinkLayerStats_1_5) {
    if (!isCapabilitySupported(
            IWifiStaIface::StaIfaceCapabilityMask::LINK_LAYER_STATS)) {
        // No-op if link layer stats is not supported.
        return;
    }

    // Enable link layer stats collection.
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, enableLinkLayerStatsCollection, true)
                  .code);
    // Retrieve link layer stats.
    const auto& status_and_stats =
        HIDL_INVOKE(wifi_sta_iface_, getLinkLayerStats_1_5);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_stats.first.code);
    EXPECT_GT(status_and_stats.second.timeStampInMs, 0u);
    // Try to create 2nd iface. If yes, it should fill in the duty cycle field.
    sp<IWifiStaIface> iface;
    auto status = createStaIface(&iface);
    if (status == WifiStatusCode::SUCCESS) {
        EXPECT_GT(status_and_stats.second.iface.timeSliceDutyCycleInPercent,
                  0u);
    }
    // Disable link layer stats collection.
    EXPECT_EQ(
        WifiStatusCode::SUCCESS,
        HIDL_INVOKE(wifi_sta_iface_, disableLinkLayerStatsCollection).code);
}
/**
 * SetScanMode
 */
TEST_P(WifiStaIfaceHidlTest, SetScanMode) {
    auto statusCode =
        HIDL_INVOKE(wifi_sta_iface_, setScanMode, true).code;
    EXPECT_TRUE(statusCode == WifiStatusCode::SUCCESS ||
                statusCode == WifiStatusCode::ERROR_NOT_SUPPORTED);

    statusCode = HIDL_INVOKE(wifi_sta_iface_, setScanMode, false).code;
    EXPECT_TRUE(statusCode == WifiStatusCode::SUCCESS ||
                statusCode == WifiStatusCode::ERROR_NOT_SUPPORTED);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiStaIfaceHidlTest);
INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiStaIfaceHidlTest,
    testing::ValuesIn(android::hardware::getAllHalInstanceNames(
        ::android::hardware::wifi::V1_5::IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
