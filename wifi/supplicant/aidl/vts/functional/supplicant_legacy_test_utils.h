/*
 * Copyright (C) 2022 The Android Open Source Project
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

#pragma once

#include <VtsCoreUtil.h>
#include <android-base/logging.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <hidl/ServiceManagement.h>
#include <supplicant_hidl_test_utils.h>
#include <wifi_system/supplicant_manager.h>

using android::wifi_system::SupplicantManager;

// Helper methods to interact with supplicant_hidl_test_utils
namespace SupplicantLegacyTestUtils {
std::string getWifiInstanceName() {
    const std::vector<std::string> instances = android::hardware::getAllHalInstanceNames(
            ::android::hardware::wifi::V1_0::IWifi::descriptor);
    EXPECT_NE(0, instances.size());
    return instances.size() != 0 ? instances[0] : "";
}

void stopSupplicantService() {
    stopSupplicant(getWifiInstanceName());
}

void startSupplicant() {
    initializeDriverAndFirmware(getWifiInstanceName());
    SupplicantManager supplicant_manager;
    ASSERT_TRUE(supplicant_manager.StartSupplicant());
    ASSERT_TRUE(supplicant_manager.IsSupplicantRunning());
}

void initializeService() {
    ASSERT_TRUE(stopWifiFramework(getWifiInstanceName()));
    std::system("/system/bin/start");
    ASSERT_TRUE(waitForFrameworkReady());
    stopSupplicantService();
    startSupplicant();
}
}  // namespace SupplicantLegacyTestUtils
