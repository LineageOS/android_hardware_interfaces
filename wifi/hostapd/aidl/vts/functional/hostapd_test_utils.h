/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <aidl/android/hardware/wifi/hostapd/BnHostapd.h>
#include <android-base/logging.h>
#include <wifi_system/hostapd_manager.h>
#include <wifi_system/supplicant_manager.h>

#include "hostapd_aidl_test_utils.h"
#include "hostapd_legacy_test_utils.h"

using aidl::android::hardware::wifi::hostapd::IHostapd;
using android::wifi_system::HostapdManager;
using android::wifi_system::SupplicantManager;

namespace {

void startAndConfigureVendorHal() {
    if (HostapdAidlTestUtils::useAidlService()) {
        HostapdAidlTestUtils::startAndConfigureVendorHal();
    } else {
        HostapdLegacyTestUtils::startAndConfigureVendorHal();
    }
}

void stopVendorHal() {
    if (HostapdAidlTestUtils::useAidlService()) {
        HostapdAidlTestUtils::stopVendorHal();
    } else {
        HostapdLegacyTestUtils::stopVendorHal();
    }
}

void stopHostapd() {
    HostapdManager hostapd_manager;
    ASSERT_TRUE(hostapd_manager.StopHostapd());
}

void waitForSupplicantState(bool enable) {
    SupplicantManager supplicant_manager;
    int count = 50;  // wait at most 5 seconds
    while (count-- > 0) {
        if (supplicant_manager.IsSupplicantRunning() == enable) {
            return;
        }
        usleep(100000);  // 100 ms
    }
    LOG(ERROR) << "Unable to " << (enable ? "start" : "stop") << " supplicant";
}

void toggleWifiFrameworkAndScan(bool enable) {
    if (enable) {
        std::system("svc wifi enable");
        std::system("cmd wifi set-scan-always-available enabled");
        waitForSupplicantState(true);
    } else {
        std::system("svc wifi disable");
        std::system("cmd wifi set-scan-always-available disabled");
        waitForSupplicantState(false);
    }
}

}  // namespace

std::shared_ptr<IHostapd> getHostapd(const std::string& hostapd_instance_name) {
    return IHostapd::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(hostapd_instance_name.c_str())));
}

/**
 * Disable the Wifi framework, hostapd, and vendor HAL.
 *
 * Note: The framework should be disabled to avoid having
 *       any other clients to the HALs during testing.
 */
void disableHalsAndFramework() {
    toggleWifiFrameworkAndScan(false);
    stopHostapd();
    stopVendorHal();

    // Wait for the services to stop.
    sleep(3);
}

void initializeHostapdAndVendorHal(const std::string& hostapd_instance_name) {
    startAndConfigureVendorHal();
    HostapdManager hostapd_manager;
    ASSERT_TRUE(hostapd_manager.StartHostapd());
    getHostapd(hostapd_instance_name);
}

void stopHostapdAndVendorHal() {
    stopHostapd();
    stopVendorHal();
}

void startWifiFramework() {
    toggleWifiFrameworkAndScan(true);
}

std::string setupApIfaceAndGetName(bool isBridged) {
    if (HostapdAidlTestUtils::useAidlService()) {
        return HostapdAidlTestUtils::setupApIfaceAndGetName(isBridged);
    } else {
        return HostapdLegacyTestUtils::setupApIfaceAndGetName(isBridged);
    }
}
