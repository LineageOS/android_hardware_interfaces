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

#ifndef SUPPLICANT_HIDL_TEST_UTILS_H
#define SUPPLICANT_HIDL_TEST_UTILS_H

#include <VtsCoreUtil.h>
#include <android-base/logging.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantP2pIface.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaNetwork.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicant.h>

#include <getopt.h>

#include "wifi_hidl_test_utils.h"

// Used to start the android wifi framework after every test.
bool startWifiFramework();

// Used to stop the android wifi framework before every test.
bool stopWifiFramework(const std::string& wifi_instance_name);

void stopSupplicant(const std::string& wifi_instance_name);
// Used to configure the chip, driver and start wpa_supplicant before every
// test.
void startSupplicantAndWaitForHidlService(
    const std::string& wifi_instance_name,
    const std::string& supplicant_instance_name);

// Used to initialize/deinitialize the driver and firmware at the
// beginning and end of each test.
void initializeDriverAndFirmware(const std::string& wifi_instance_name);
void deInitializeDriverAndFirmware(const std::string& wifi_instance_name);

// Helper functions to obtain references to the various HIDL interface objects.
// Note: We only have a single instance of each of these objects currently.
// These helper functions should be modified to return vectors if we support
// multiple instances.
android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicant>
getSupplicant(const std::string& supplicant_instance_name, bool isP2pOn);
android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicantStaIface>
getSupplicantStaIface(
    const android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicant>&
        supplicant);
android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork>
createSupplicantStaNetwork(
    const android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicant>&
        supplicant);
android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicantP2pIface>
getSupplicantP2pIface(
    const android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicant>&
        supplicant);
bool turnOnExcessiveLogging(
    const android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicant>&
        supplicant);

bool turnOnExcessiveLogging();

bool waitForFrameworkReady();

class SupplicantHidlTestBase
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
   public:
    virtual void SetUp() override {
        wifi_v1_0_instance_name_ =
            std::get<0>(GetParam());  // should always be v1.0 wifi
        supplicant_instance_name_ = std::get<1>(GetParam());

        // Stop & wait for wifi to shutdown.
        ASSERT_TRUE(stopWifiFramework(wifi_v1_0_instance_name_));

        std::system("/system/bin/start");
        ASSERT_TRUE(waitForFrameworkReady());
        isP2pOn_ =
            testing::deviceSupportsFeature("android.hardware.wifi.direct");
        stopSupplicant(wifi_v1_0_instance_name_);
        startSupplicantAndWaitForHidlService(wifi_v1_0_instance_name_,
                                             supplicant_instance_name_);
        LOG(INFO) << "SupplicantHidlTestBase isP2pOn_: " << isP2pOn_;
    }

    virtual void TearDown() override {
        stopSupplicant(wifi_v1_0_instance_name_);
        // Start Wi-Fi
        startWifiFramework();
    }

   protected:
    bool isP2pOn_ = false;
    std::string wifi_v1_0_instance_name_;
    std::string supplicant_instance_name_;
};

class SupplicantHidlTestBaseV1_0 : public SupplicantHidlTestBase {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBase::SetUp();
        supplicant_ = getSupplicant(supplicant_instance_name_, isP2pOn_);
        ASSERT_NE(supplicant_.get(), nullptr);
        EXPECT_TRUE(turnOnExcessiveLogging(supplicant_));
    }

   protected:
    android::sp<android::hardware::wifi::supplicant::V1_0::ISupplicant>
        supplicant_;
};
#endif /* SUPPLICANT_HIDL_TEST_UTILS_H */
