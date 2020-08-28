/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the P2pache License, Version 2.0 (the "License");
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
#include <android/hardware/wifi/1.0/IWifiP2pIface.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_0::IWifi;
using ::android::hardware::wifi::V1_0::IWifiP2pIface;

/**
 * Fixture to use for all P2P Iface HIDL interface tests.
 */
class WifiP2pIfaceHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure test starts with a clean state
        stopWifi(GetInstanceName());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * Create:
 * Ensures that an instance of the IWifiP2pIface proxy object is
 * successfully created.
 */
TEST_P(WifiP2pIfaceHidlTest, Create) {
    stopWifi(GetInstanceName());
    EXPECT_NE(nullptr, getWifiP2pIface(GetInstanceName()).get());
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiP2pIfaceHidlTest,
    testing::ValuesIn(
        android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
