/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef SUPPLICANT_TEST_UTILS_H
#define SUPPLICANT_TEST_UTILS_H

#include <VtsCoreUtil.h>
#include <android-base/logging.h>
#include <wifi_system/supplicant_manager.h>

using aidl::android::hardware::wifi::supplicant::ISupplicantStaIface;
using aidl::android::hardware::wifi::supplicant::KeyMgmtMask;
using android::wifi_system::SupplicantManager;

std::string getStaIfaceName() {
    std::array<char, PROPERTY_VALUE_MAX> buffer;
    property_get("wifi.interface", buffer.data(), "wlan0");
    return std::string(buffer.data());
}

std::string getP2pIfaceName() {
    std::array<char, PROPERTY_VALUE_MAX> buffer;
    property_get("wifi.direct.interface", buffer.data(), "p2p0");
    return std::string(buffer.data());
}

bool keyMgmtSupported(std::shared_ptr<ISupplicantStaIface> iface,
                      KeyMgmtMask expected) {
    KeyMgmtMask caps;
    if (!iface->getKeyMgmtCapabilities(&caps).isOk()) {
        return false;
    }
    return !!(static_cast<uint32_t>(caps) & static_cast<uint32_t>(expected));
}

bool isFilsSupported(std::shared_ptr<ISupplicantStaIface> iface) {
    KeyMgmtMask filsMask = static_cast<KeyMgmtMask>(
        static_cast<uint32_t>(KeyMgmtMask::FILS_SHA256) |
        static_cast<uint32_t>(KeyMgmtMask::FILS_SHA384));
    return keyMgmtSupported(iface, filsMask);
}

bool waitForSupplicantState(bool is_running) {
    SupplicantManager supplicant_manager;
    int count = 50; /* wait at most 5 seconds for completion */
    while (count-- > 0) {
        if (supplicant_manager.IsSupplicantRunning() == is_running) {
            return true;
        }
        usleep(100000);
    }
    LOG(ERROR) << "Supplicant not " << (is_running ? "running" : "stopped");
    return false;
}

bool waitForFrameworkReady() {
    int waitCount = 15;
    do {
        // Check whether package service is ready or not.
        if (!testing::checkSubstringInCommandOutput(
                "/system/bin/service check package", ": not found")) {
            return true;
        }
        LOG(INFO) << "Framework is not ready";
        sleep(1);
    } while (waitCount-- > 0);
    return false;
}

bool waitForSupplicantStart() { return waitForSupplicantState(true); }

bool waitForSupplicantStop() { return waitForSupplicantState(false); }

void stopSupplicant() {
    SupplicantManager supplicant_manager;
    ASSERT_TRUE(supplicant_manager.StopSupplicant());
    ASSERT_FALSE(supplicant_manager.IsSupplicantRunning());
}

bool startWifiFramework() {
    std::system("svc wifi enable");
    std::system("cmd wifi set-scan-always-available enabled");
    return waitForSupplicantStart();
}

bool stopWifiFramework() {
    std::system("svc wifi disable");
    std::system("cmd wifi set-scan-always-available disabled");
    return waitForSupplicantStop();
}

void initializeService() {
    ASSERT_TRUE(stopWifiFramework());
    std::system("/system/bin/start");
    ASSERT_TRUE(waitForFrameworkReady());
    stopSupplicant();
}

#endif  // SUPPLICANT_TEST_UTILS_H