/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android/hardware/wifi/1.4/IWifi.h>
#include <android/hardware/wifi/1.4/IWifiApIface.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_4::IWifi;
using ::android::hardware::wifi::V1_4::IWifiApIface;

/**
 * Fixture to use for all STA Iface HIDL interface tests.
 */
class WifiApIfaceHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure to start with a clean state
        stopWifi(GetInstanceName());

        wifi_ap_iface_ =
            IWifiApIface::castFrom(getWifiApIface(GetInstanceName()));
        ASSERT_NE(nullptr, wifi_ap_iface_.get());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    sp<IWifiApIface> wifi_ap_iface_;

   private:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * SetMacAddress:
 * Ensures that calls to set MAC address will return a success status
 * code.
 */
TEST_P(WifiApIfaceHidlTest, SetMacAddress) {
    const hidl_array<uint8_t, 6> kMac{{0x12, 0x22, 0x33, 0x52, 0x10, 0x41}};
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_ap_iface_, setMacAddress, kMac).code);
}

/*
 * GetFactoryMacAddress:
 * Ensures that calls to get factory MAC address will retrieve a non-zero MAC
 * and return a success status code.
 */
TEST_P(WifiApIfaceHidlTest, GetFactoryMacAddress) {
    std::pair<WifiStatus, hidl_array<uint8_t, 6> > status_and_mac =
        HIDL_INVOKE(wifi_ap_iface_, getFactoryMacAddress);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_mac.first.code);
    hidl_array<uint8_t, 6> all_zero{};
    EXPECT_NE(all_zero, status_and_mac.second);
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiApIfaceHidlTest,
    testing::ValuesIn(
        android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
