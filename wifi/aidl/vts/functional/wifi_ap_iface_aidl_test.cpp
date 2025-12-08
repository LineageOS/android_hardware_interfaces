/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <vector>

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/BnWifi.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::IWifiApIface;
using aidl::android::hardware::wifi::WifiBand;

class WifiApIfaceAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        if (!::testing::deviceSupportsFeature("android.hardware.wifi")) {
            GTEST_SKIP() << "Skipping this test since wifi is not supported.";
        }
        isBridgedSupport_ = testing::checkSubstringInCommandOutput(
                "/system/bin/cmd wifi get-softap-supported-features",
                "wifi_softap_bridged_ap_supported");
        stopWifiService(getInstanceName());
        sleep(1);  // Ensure that all chips/interfaces are invalidated.
        wifi_chip_ = getWifiChip(getInstanceName());
        ASSERT_NE(nullptr, wifi_chip_.get());

        bool isApSupported = doesChipSupportConcurrencyType(wifi_chip_, IfaceConcurrencyType::AP);
        if (!isApSupported) {
            GTEST_SKIP() << "AP interfaces are not supported";
        }
    }

    void TearDown() override { stopWifiService(getInstanceName()); }

  protected:
    bool isBridgedSupport_ = false;
    std::shared_ptr<IWifiChip> wifi_chip_;
    const char* getInstanceName() { return GetParam().c_str(); }

    // Retrieve the AP interface, retrying if the initial retrieval fails.
    std::shared_ptr<IWifiApIface> getApIfaceInternal() {
        std::shared_ptr<IWifiApIface> wifi_ap_iface = getWifiApIface(wifi_chip_);
        if (wifi_ap_iface == nullptr) {
            LOG(INFO) << "Retrying AP interface retrieval";
            sleep(2);
            wifi_ap_iface = getWifiApIface(wifi_chip_);
        }
        return wifi_ap_iface;
    }
};

/*
 * SetMacAddress
 */
TEST_P(WifiApIfaceAidlTest, SetMacAddress) {
    std::shared_ptr<IWifiApIface> wifi_ap_iface = getApIfaceInternal();
    ASSERT_NE(nullptr, wifi_ap_iface.get());
    std::array<uint8_t, 6> mac = {0x12, 0x22, 0x33, 0x52, 0x10, 0x44};
    EXPECT_TRUE(wifi_ap_iface->setMacAddress(mac).isOk());
}

/*
 * SetCountryCode
 */
TEST_P(WifiApIfaceAidlTest, SetCountryCode) {
    std::shared_ptr<IWifiApIface> wifi_ap_iface = getApIfaceInternal();
    ASSERT_NE(nullptr, wifi_ap_iface.get());

    const std::array<uint8_t, 2> country_code = {0x55, 0x53};
    EXPECT_TRUE(wifi_ap_iface->setCountryCode(country_code).isOk());
}

/*
 * GetFactoryMacAddress
 */
TEST_P(WifiApIfaceAidlTest, GetFactoryMacAddress) {
    std::shared_ptr<IWifiApIface> wifi_ap_iface = getApIfaceInternal();
    ASSERT_NE(nullptr, wifi_ap_iface.get());

    std::array<uint8_t, 6> mac;
    EXPECT_TRUE(wifi_ap_iface->getFactoryMacAddress(&mac).isOk());
    std::array<uint8_t, 6> all_zero_mac = {0, 0, 0, 0, 0, 0};
    EXPECT_NE(mac, all_zero_mac);
}

/**
 * GetBridgedInstances - non-bridged mode
 */
TEST_P(WifiApIfaceAidlTest, GetBridgedInstances) {
    std::shared_ptr<IWifiApIface> wifi_ap_iface = getApIfaceInternal();
    ASSERT_NE(nullptr, wifi_ap_iface.get());

    std::vector<std::string> instances;
    EXPECT_TRUE(wifi_ap_iface->getBridgedInstances(&instances).isOk());
    EXPECT_EQ(instances.size(), 0);
}

/**
 * GetBridgedInstances - bridged AP mode.
 */
TEST_P(WifiApIfaceAidlTest, GetBridgedInstances_Bridged) {
    if (!isBridgedSupport_) {
        GTEST_SKIP() << "Missing Bridged AP support";
    }
    std::shared_ptr<IWifiApIface> wifi_ap_iface = getBridgedWifiApIface(wifi_chip_);
    ASSERT_NE(nullptr, wifi_ap_iface.get());

    std::vector<std::string> instances;
    EXPECT_TRUE(wifi_ap_iface->getBridgedInstances(&instances).isOk());
    EXPECT_EQ(instances.size(), 2);
}

/**
 * ResetToFactoryMacAddress - non-bridged mode
 */
TEST_P(WifiApIfaceAidlTest, ResetToFactoryMacAddress) {
    std::shared_ptr<IWifiApIface> wifi_ap_iface = getApIfaceInternal();
    ASSERT_NE(nullptr, wifi_ap_iface.get());
    EXPECT_TRUE(wifi_ap_iface->resetToFactoryMacAddress().isOk());
}

/**
 * ResetToFactoryMacAddress - bridged AP mode
 */
TEST_P(WifiApIfaceAidlTest, ResetToFactoryMacAddress_Bridged) {
    if (!isBridgedSupport_) {
        GTEST_SKIP() << "Missing Bridged AP support";
    }
    std::shared_ptr<IWifiApIface> wifi_ap_iface = getBridgedWifiApIface(wifi_chip_);
    ASSERT_NE(nullptr, wifi_ap_iface.get());
    EXPECT_TRUE(wifi_ap_iface->resetToFactoryMacAddress().isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiApIfaceAidlTest);
INSTANTIATE_TEST_SUITE_P(WifiTest, WifiApIfaceAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IWifi::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
