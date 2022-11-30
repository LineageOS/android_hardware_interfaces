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

#pragma once

#include <VtsCoreUtil.h>
#include <aidl/android/hardware/wifi/IWifi.h>
#include <android-base/logging.h>
#include <wifi_system/supplicant_manager.h>

#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::IfaceConcurrencyType;
using aidl::android::hardware::wifi::IWifi;
using aidl::android::hardware::wifi::IWifiChip;
using aidl::android::hardware::wifi::supplicant::IfaceInfo;
using aidl::android::hardware::wifi::supplicant::ISupplicant;
using aidl::android::hardware::wifi::supplicant::ISupplicantP2pIface;
using aidl::android::hardware::wifi::supplicant::ISupplicantStaIface;
using aidl::android::hardware::wifi::supplicant::KeyMgmtMask;
using android::wifi_system::SupplicantManager;

const std::string kWifiInstanceName = std::string() + IWifi::descriptor + "/default";

// Initialize the driver and firmware to STA mode using the vendor HAL.
void initializeDriverAndFirmware(const std::string& wifi_instance_name) {
    // Skip if wifi instance is not set.
    if (wifi_instance_name == "") {
        return;
    }
    if (getWifi(wifi_instance_name.c_str()) != nullptr) {
        std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(wifi_instance_name.c_str());
        int mode_id;
        EXPECT_TRUE(configureChipToSupportConcurrencyType(wifi_chip, IfaceConcurrencyType::STA,
                                                          &mode_id));
    } else {
        LOG(WARNING) << __func__ << ": Vendor HAL not supported";
    }
}

// Deinitialize the driver and firmware using the vendor HAL.
void deInitializeDriverAndFirmware(const std::string& wifi_instance_name) {
    // Skip if wifi instance is not set.
    if (wifi_instance_name == "") {
        return;
    }
    if (getWifi(wifi_instance_name.c_str()) != nullptr) {
        stopWifiService(wifi_instance_name.c_str());
    } else {
        LOG(WARNING) << __func__ << ": Vendor HAL not supported";
    }
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
    LOG(ERROR) << "Supplicant not " << is_running ? "running" : "stopped";
    return false;
}

bool waitForSupplicantStart() {
    return waitForSupplicantState(true);
}

bool waitForSupplicantStop() {
    return waitForSupplicantState(false);
}

bool waitForWifiHalStop(const std::string& wifi_instance_name) {
    std::shared_ptr<IWifi> wifi = getWifi(wifi_instance_name.c_str());
    int count = 50; /* wait at most 5 seconds for completion */
    while (count-- > 0) {
        if (wifi != nullptr) {
            bool started = false;
            auto status = wifi->isStarted(&started);
            if (status.isOk() && !started) {
                return true;
            }
        }
        usleep(100000);
        wifi = getWifi(wifi_instance_name.c_str());
    }
    LOG(ERROR) << "Wifi HAL was not stopped";
    return false;
}

bool waitForFrameworkReady() {
    int waitCount = 15;
    do {
        // Check whether package service is ready or not.
        if (!testing::checkSubstringInCommandOutput("/system/bin/service check package",
                                                    ": not found")) {
            return true;
        }
        LOG(INFO) << "Framework is not ready";
        sleep(1);
    } while (waitCount-- > 0);
    return false;
}

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

void startSupplicant() {
    initializeDriverAndFirmware(kWifiInstanceName);
    SupplicantManager supplicant_manager;
    ASSERT_TRUE(supplicant_manager.StartSupplicant());
    ASSERT_TRUE(supplicant_manager.IsSupplicantRunning());
}

void stopSupplicantService() {
    SupplicantManager supplicant_manager;
    ASSERT_TRUE(supplicant_manager.StopSupplicant());
    deInitializeDriverAndFirmware(kWifiInstanceName);
    ASSERT_FALSE(supplicant_manager.IsSupplicantRunning());
}

bool startWifiFramework() {
    std::system("svc wifi enable");
    std::system("cmd wifi set-scan-always-available enabled");
    return waitForSupplicantStart();  // wait for wifi to start.
}

bool stopWifiFramework(const std::string& wifi_instance_name) {
    std::system("svc wifi disable");
    std::system("cmd wifi set-scan-always-available disabled");
    return waitForSupplicantStop() && waitForWifiHalStop(wifi_instance_name);
}

void initializeService() {
    ASSERT_TRUE(stopWifiFramework(kWifiInstanceName));
    std::system("/system/bin/start");
    ASSERT_TRUE(waitForFrameworkReady());
    stopSupplicantService();
    startSupplicant();
}

void addStaIface(const std::shared_ptr<ISupplicant> supplicant) {
    ASSERT_TRUE(supplicant.get());
    std::shared_ptr<ISupplicantStaIface> iface;
    ASSERT_TRUE(supplicant->addStaInterface(getStaIfaceName(), &iface).isOk());
}

void addP2pIface(const std::shared_ptr<ISupplicant> supplicant) {
    ASSERT_TRUE(supplicant.get());
    std::shared_ptr<ISupplicantP2pIface> iface;
    ASSERT_TRUE(supplicant->addP2pInterface(getP2pIfaceName(), &iface).isOk());
}

std::shared_ptr<ISupplicant> getSupplicant(const char* supplicant_name) {
    std::shared_ptr<ISupplicant> supplicant = ISupplicant::fromBinder(
        ndk::SpAIBinder(AServiceManager_waitForService(supplicant_name)));
    addStaIface(supplicant);
    if (testing::deviceSupportsFeature("android.hardware.wifi.direct")) {
        addP2pIface(supplicant);
    }
    return supplicant;
}
