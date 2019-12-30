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

#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/1.0/IWifiRttController.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_0::IWifi;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::hardware::wifi::V1_0::IWifiRttController;
using ::android::hardware::wifi::V1_0::IWifiStaIface;
using ::android::hardware::wifi::V1_0::WifiStatusCode;

/**
 * Fixture to use for all RTT controller HIDL interface tests.
 */
class WifiRttControllerHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {}

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * Create:
 * Ensures that an instance of the IWifiRttController proxy object is
 * successfully created.
 */
TEST_P(WifiRttControllerHidlTest, Create) {
    stopWifi(GetInstanceName());

    const std::string& instance_name = GetInstanceName();

    sp<IWifiChip> wifi_chip = getWifiChip(instance_name);
    EXPECT_NE(nullptr, wifi_chip.get());

    sp<IWifiStaIface> wifi_sta_iface = getWifiStaIface(instance_name);
    EXPECT_NE(nullptr, wifi_sta_iface.get());

    const auto& status_and_controller =
        HIDL_INVOKE(wifi_chip, createRttController, wifi_sta_iface);
    if (status_and_controller.first.code !=
        WifiStatusCode::ERROR_NOT_SUPPORTED) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_controller.first.code);
        EXPECT_NE(nullptr, status_and_controller.second.get());
    }
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiRttControllerHidlTest,
    testing::ValuesIn(
        android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
