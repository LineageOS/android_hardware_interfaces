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

#include <android/hardware/wifi/1.2/IWifi.h>
#include <android/hardware/wifi/1.2/IWifiStaIface.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_0::CommandId;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_2::IWifiStaIface;

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

    sp<IWifiStaIface> wifi_sta_iface_;

   private:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * SetMacAddress:
 * Ensures that calls to set MAC address will return a success status
 * code.
 */
TEST_P(WifiStaIfaceHidlTest, SetMacAddress) {
    const android::hardware::hidl_array<uint8_t, 6> kMac{
        std::array<uint8_t, 6>{{0x12, 0x22, 0x33, 0x52, 0x10, 0x41}}};
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, setMacAddress, kMac).code);
}

/*
 * ReadApfPacketFilterData:
 * Ensures that we can read the APF working memory when supported.
 *
 * TODO: Test disabled because we can't even test reading and writing the APF
 * memory while the interface is in disconnected state (b/73804303#comment25).
 * There's a pending bug on VTS infra to add such support (b/32974062).
 * TODO: We can't execute APF opcodes from this test because there's no way
 * to loop test packets through the wifi firmware (b/73804303#comment29).
 */
TEST_P(WifiStaIfaceHidlTest, DISABLED_ReadApfPacketFilterData) {
    if (!isCapabilitySupported(IWifiStaIface::StaIfaceCapabilityMask::APF)) {
        // Disable test if APF packet filer is not supported.
        LOG(WARNING) << "TEST SKIPPED: APF packet filtering not supported";
        return;
    }

    const auto& status_and_caps =
        HIDL_INVOKE(wifi_sta_iface_, getApfPacketFilterCapabilities);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
    LOG(WARNING) << "StaApfPacketFilterCapabilities: version="
                 << status_and_caps.second.version
                 << " maxLength=" << status_and_caps.second.maxLength;

    const CommandId kCmd = 0;  // Matches what WifiVendorHal.java uses.
    const uint32_t kDataSize =
        std::min(status_and_caps.second.maxLength, static_cast<uint32_t>(500));

    // Create a buffer and fill it with some values.
    std::vector<uint8_t> data(kDataSize);
    std::iota(data.begin(), data.end(), 0);

    EXPECT_EQ(
        HIDL_INVOKE(wifi_sta_iface_, installApfPacketFilter, kCmd, data).code,
        WifiStatusCode::SUCCESS);
    const auto& status_and_data =
        HIDL_INVOKE(wifi_sta_iface_, readApfPacketFilterData);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_data.first.code);

    EXPECT_EQ(status_and_data.second, data);
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiStaIfaceHidlTest,
    testing::ValuesIn(android::hardware::getAllHalInstanceNames(
        ::android::hardware::wifi::V1_2::IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
