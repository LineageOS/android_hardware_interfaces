/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <android/hardware/wifi/1.3/IWifiStaIface.h>

#include <VtsHalHidlTargetTestBase.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_3::IWifiStaIface;

/**
 * Fixture to use for all STA Iface HIDL interface tests.
 */
class WifiStaIfaceHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        wifi_sta_iface_ = IWifiStaIface::castFrom(getWifiStaIface());
        ASSERT_NE(nullptr, wifi_sta_iface_.get());
    }

    virtual void TearDown() override { stopWifi(); }

   protected:
    bool isCapabilitySupported(IWifiStaIface::StaIfaceCapabilityMask cap_mask) {
        const auto& status_and_caps =
            HIDL_INVOKE(wifi_sta_iface_, getCapabilities);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
        return (status_and_caps.second & cap_mask) != 0;
    }

    sp<IWifiStaIface> wifi_sta_iface_;
};

/*
 * GetFactoryMacAddress:
 * Ensures that calls to get factory MAC address will retrieve a non-zero MAC
 * and return a success status code.
 */
TEST_F(WifiStaIfaceHidlTest, GetFactoryMacAddress) {
    const auto& status_and_mac =
        HIDL_INVOKE(wifi_sta_iface_, getFactoryMacAddress);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_mac.first.code);
    const int num_elements = sizeof(status_and_mac.second) / sizeof(uint8_t);
    EXPECT_EQ(6, num_elements);
    for (int i = 0; i < num_elements; i++) {
        EXPECT_NE(0, status_and_mac.second[i]);
    }
}

/*
 * GetLinkLayerStats_1_3
 * Ensures that calls to get link layer stats V1_3 will retrieve a non-empty
 * StaLinkLayerStats after link layer stats collection is enabled.
 */
TEST_F(WifiStaIfaceHidlTest, GetLinkLayerStats_1_3) {
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
        HIDL_INVOKE(wifi_sta_iface_, getLinkLayerStats_1_3);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_stats.first.code);
    EXPECT_GT(status_and_stats.second.timeStampInMs, 0u);
    // Disable link layer stats collection.
    EXPECT_EQ(
        WifiStatusCode::SUCCESS,
        HIDL_INVOKE(wifi_sta_iface_, disableLinkLayerStatsCollection).code);
}
