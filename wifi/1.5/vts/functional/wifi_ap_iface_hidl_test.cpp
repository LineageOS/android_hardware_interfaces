/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <VtsCoreUtil.h>
#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/logging.h>

#undef NAN  // NAN is defined in bionic/libc/include/math.h:38

#include <android/hardware/wifi/1.4/IWifiChipEventCallback.h>
#include <android/hardware/wifi/1.5/IWifi.h>
#include <android/hardware/wifi/1.5/IWifiApIface.h>
#include <android/hardware/wifi/1.5/IWifiChip.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"
#include "wifi_hidl_test_utils_1_5.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::IWifiIface;
using ::android::hardware::wifi::V1_0::WifiDebugRingBufferStatus;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_4::IWifiChipEventCallback;
using ::android::hardware::wifi::V1_5::IWifiApIface;
using ::android::hardware::wifi::V1_5::IWifiChip;

/**
 * Fixture for IWifiChip tests that are conditioned on SoftAP support.
 */
class WifiApIfaceHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        isBridgedSupport_ = testing::checkSubstringInCommandOutput(
            "/system/bin/cmd wifi get-softap-supported-features",
            "wifi_softap_bridged_ap_supported");
        // Make sure to start with a clean state
        stopWifi(GetInstanceName());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    bool isBridgedSupport_ = false;
    std::string GetInstanceName() { return GetParam(); }
};

/**
 * resetToFactoryMacAddress in bridged AP mode.
 */
TEST_P(WifiApIfaceHidlTest, resetToFactoryMacAddressInBridgedModeTest) {
    if (!isBridgedSupport_) GTEST_SKIP() << "Missing Bridged AP support";
    sp<IWifiApIface> wifi_ap_iface =
        getBridgedWifiApIface_1_5(GetInstanceName());
    ASSERT_NE(nullptr, wifi_ap_iface.get());
    const auto& status = HIDL_INVOKE(wifi_ap_iface, resetToFactoryMacAddress);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
}

/**
 * resetToFactoryMacAddress in non-bridged mode
 */
TEST_P(WifiApIfaceHidlTest, resetToFactoryMacAddressTest) {
    sp<IWifiApIface> wifi_ap_iface = getWifiApIface_1_5(GetInstanceName());
    ASSERT_NE(nullptr, wifi_ap_iface.get());
    const auto& status = HIDL_INVOKE(wifi_ap_iface, resetToFactoryMacAddress);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
}

/**
 * getBridgedInstances in non-bridged mode
 */
TEST_P(WifiApIfaceHidlTest, getBridgedInstancesTest) {
    sp<IWifiApIface> wifi_ap_iface = getWifiApIface_1_5(GetInstanceName());
    ASSERT_NE(nullptr, wifi_ap_iface.get());
    const auto& status_and_instances =
        HIDL_INVOKE(wifi_ap_iface, getBridgedInstances);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_instances.first.code);
    const auto& instances = status_and_instances.second;
    EXPECT_EQ(0, instances.size());
}

/**
 * getBridgedInstances in bridged AP mode.
 */
TEST_P(WifiApIfaceHidlTest, getBridgedInstancesInBridgedModeTest) {
    if (!isBridgedSupport_) GTEST_SKIP() << "Missing Bridged AP support";
    sp<IWifiApIface> wifi_ap_iface =
        getBridgedWifiApIface_1_5(GetInstanceName());
    ASSERT_NE(nullptr, wifi_ap_iface.get());
    const auto& status_and_instances =
        HIDL_INVOKE(wifi_ap_iface, getBridgedInstances);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_instances.first.code);
    const auto& instances = status_and_instances.second;
    EXPECT_EQ(2, instances.size());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiApIfaceHidlTest);
INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiApIfaceHidlTest,
    testing::ValuesIn(android::hardware::getAllHalInstanceNames(
        ::android::hardware::wifi::V1_5::IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
