/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/1.0/IWifiChip.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::IWifi;
using ::android::hardware::wifi::V1_0::IWifiApIface;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::hardware::wifi::V1_0::IWifiIface;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;

/**
 * Fixture for IWifiChip tests that are conditioned on SoftAP support.
 */
class WifiChipHidlApTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure test starts with a clean state
        stopWifi(GetInstanceName());

        wifi_chip_ = getWifiChip(GetInstanceName());
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

    std::string getIfaceName(const sp<IWifiIface>& iface) {
        const auto& status_and_name = HIDL_INVOKE(iface, getName);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_name.first.code);
        return status_and_name.second;
    }

    WifiStatusCode createApIface(sp<IWifiApIface>* ap_iface) {
        const auto& status_and_iface = HIDL_INVOKE(wifi_chip_, createApIface);
        *ap_iface = status_and_iface.second;
        return status_and_iface.first.code;
    }

    WifiStatusCode removeApIface(const std::string& name) {
        return HIDL_INVOKE(wifi_chip_, removeApIface, name).code;
    }

    sp<IWifiChip> wifi_chip_;

   private:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * CreateApIface
 * Configures the chip in AP mode and ensures that at least 1 iface creation
 * succeeds.
 */
TEST_P(WifiChipHidlApTest, CreateApIface) {
    configureChipForIfaceType(IfaceType::AP, true);

    sp<IWifiApIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&iface));
    EXPECT_NE(nullptr, iface.get());
}

/*
 * GetApIfaceNames
 * Configures the chip in AP mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_P(WifiChipHidlApTest, GetApIfaceNames) {
    configureChipForIfaceType(IfaceType::AP, true);

    const auto& status_and_iface_names1 =
        HIDL_INVOKE(wifi_chip_, getApIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names1.first.code);
    EXPECT_EQ(0u, status_and_iface_names1.second.size());

    sp<IWifiApIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&iface));
    EXPECT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    const auto& status_and_iface_names2 =
        HIDL_INVOKE(wifi_chip_, getApIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names2.first.code);
    EXPECT_EQ(1u, status_and_iface_names2.second.size());
    EXPECT_EQ(iface_name, status_and_iface_names2.second[0]);

    EXPECT_EQ(WifiStatusCode::SUCCESS, removeApIface(iface_name));
    const auto& status_and_iface_names3 =
        HIDL_INVOKE(wifi_chip_, getApIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names3.first.code);
    EXPECT_EQ(0u, status_and_iface_names3.second.size());
}

/*
 * GetApIface
 * Configures the chip in AP mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_P(WifiChipHidlApTest, GetApIface) {
    configureChipForIfaceType(IfaceType::AP, true);

    sp<IWifiApIface> ap_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&ap_iface));
    EXPECT_NE(nullptr, ap_iface.get());

    std::string iface_name = getIfaceName(ap_iface);
    const auto& status_and_iface1 =
        HIDL_INVOKE(wifi_chip_, getApIface, iface_name);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface1.first.code);
    EXPECT_NE(nullptr, status_and_iface1.second.get());

    std::string invalid_name = iface_name + "0";
    const auto& status_and_iface2 =
        HIDL_INVOKE(wifi_chip_, getApIface, invalid_name);
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, status_and_iface2.first.code);
    EXPECT_EQ(nullptr, status_and_iface2.second.get());
}

/*
 * RemoveApIface
 * Configures the chip in AP mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_P(WifiChipHidlApTest, RemoveApIface) {
    configureChipForIfaceType(IfaceType::AP, true);

    sp<IWifiApIface> ap_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createApIface(&ap_iface));
    EXPECT_NE(nullptr, ap_iface.get());

    std::string iface_name = getIfaceName(ap_iface);
    std::string invalid_name = iface_name + "0";
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeApIface(invalid_name));
    EXPECT_EQ(WifiStatusCode::SUCCESS, removeApIface(iface_name));

    // No such iface exists now. So, this should return failure.
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeApIface(iface_name));
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiChipHidlApTest,
    testing::ValuesIn(
        android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
