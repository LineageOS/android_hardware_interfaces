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
#include <cutils/properties.h>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <hidl/HidlTransportSupport.h>

#include <wifi_system/interface_tool.h>
#include <wifi_system/supplicant_manager.h>

#include "supplicant_hidl_test_utils.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantIface;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantNetwork;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantP2pIface;
using ::android::hardware::wifi::supplicant::V1_0::IfaceType;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::wifi_system::InterfaceTool;
using ::android::wifi_system::SupplicantManager;

namespace {
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

// Helper function to wait for supplicant to be started by framework on wifi
// enable.
bool waitForSupplicantStart() { return waitForSupplicantState(true); }

// Helper function to wait for supplicant to be stopped by framework on wifi
// disable.
bool waitForSupplicantStop() { return waitForSupplicantState(false); }

// Helper function to initialize the driver and firmware to STA mode
// using the vendor HAL HIDL interface.
void initilializeDriverAndFirmware(const std::string& wifi_instance_name) {
    // Skip if wifi instance is not set.
    if (wifi_instance_name == "") {
        return;
    }
    if (getWifi(wifi_instance_name) != nullptr) {
        sp<IWifiChip> wifi_chip = getWifiChip(wifi_instance_name);
        ChipModeId mode_id;
        EXPECT_TRUE(configureChipToSupportIfaceType(
            wifi_chip, ::android::hardware::wifi::V1_0::IfaceType::STA, &mode_id));
    } else {
        LOG(WARNING) << __func__ << ": Vendor HAL not supported";
    }
}

// Helper function to deinitialize the driver and firmware
// using the vendor HAL HIDL interface.
void deInitilializeDriverAndFirmware(const std::string& wifi_instance_name) {
    // Skip if wifi instance is not set.
    if (wifi_instance_name == "") {
        return;
    }
    if (getWifi(wifi_instance_name) != nullptr) {
        stopWifi(wifi_instance_name);
    } else {
        LOG(WARNING) << __func__ << ": Vendor HAL not supported";
    }
}

// Helper function to find any iface of the desired type exposed.
bool findIfaceOfType(sp<ISupplicant> supplicant, IfaceType desired_type,
                     ISupplicant::IfaceInfo* out_info) {
    bool operation_failed = false;
    std::vector<ISupplicant::IfaceInfo> iface_infos;
    supplicant->listInterfaces([&](const SupplicantStatus& status,
                                   hidl_vec<ISupplicant::IfaceInfo> infos) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
            operation_failed = true;
            return;
        }
        iface_infos = infos;
    });
    if (operation_failed) {
        return false;
    }
    for (const auto& info : iface_infos) {
        if (info.type == desired_type) {
            *out_info = info;
            return true;
        }
    }
    return false;
}

std::string getStaIfaceName() {
    std::array<char, PROPERTY_VALUE_MAX> buffer;
    property_get("wifi.interface", buffer.data(), "wlan0");
    return buffer.data();
}

std::string getP2pIfaceName() {
    std::array<char, PROPERTY_VALUE_MAX> buffer;
    property_get("wifi.direct.interface", buffer.data(), "p2p0");
    return buffer.data();
}
}  // namespace

bool startWifiFramework() {
    std::system("svc wifi enable");
    std::system("cmd wifi set-scan-always-available enabled");
    return waitForSupplicantStart();  // wait for wifi to start.
}

bool stopWifiFramework() {
    std::system("svc wifi disable");
    std::system("cmd wifi set-scan-always-available disabled");
    return waitForSupplicantStop();  // wait for wifi to shutdown.
}

void stopSupplicant() { stopSupplicant(""); }

void stopSupplicant(const std::string& wifi_instance_name) {
    SupplicantManager supplicant_manager;

    ASSERT_TRUE(supplicant_manager.StopSupplicant());
    deInitilializeDriverAndFirmware(wifi_instance_name);
    ASSERT_FALSE(supplicant_manager.IsSupplicantRunning());
}

void startSupplicantAndWaitForHidlService(
    const std::string& wifi_instance_name,
    const std::string& supplicant_instance_name) {
    initilializeDriverAndFirmware(wifi_instance_name);

    SupplicantManager supplicant_manager;
    ASSERT_TRUE(supplicant_manager.StartSupplicant());
    ASSERT_TRUE(supplicant_manager.IsSupplicantRunning());

    // Wait for supplicant service to come up.
    ISupplicant::getService(supplicant_instance_name);
}

bool is_1_1(const sp<ISupplicant>& supplicant) {
    sp<::android::hardware::wifi::supplicant::V1_1::ISupplicant>
        supplicant_1_1 =
            ::android::hardware::wifi::supplicant::V1_1::ISupplicant::castFrom(
                supplicant);
    return supplicant_1_1.get() != nullptr;
}

void addSupplicantStaIface_1_1(const sp<ISupplicant>& supplicant) {
    sp<::android::hardware::wifi::supplicant::V1_1::ISupplicant>
        supplicant_1_1 =
            ::android::hardware::wifi::supplicant::V1_1::ISupplicant::castFrom(
                supplicant);
    ASSERT_TRUE(supplicant_1_1.get());
    ISupplicant::IfaceInfo info = {IfaceType::STA, getStaIfaceName()};
    supplicant_1_1->addInterface(
        info, [&](const SupplicantStatus& status,
                  const sp<ISupplicantIface>& /* iface */) {
            ASSERT_TRUE(
                (SupplicantStatusCode::SUCCESS == status.code) ||
                (SupplicantStatusCode::FAILURE_IFACE_EXISTS == status.code));
        });
}

void addSupplicantP2pIface_1_1(const sp<ISupplicant>& supplicant) {
    sp<::android::hardware::wifi::supplicant::V1_1::ISupplicant>
        supplicant_1_1 =
            ::android::hardware::wifi::supplicant::V1_1::ISupplicant::castFrom(
                supplicant);
    ASSERT_TRUE(supplicant_1_1.get());
    ISupplicant::IfaceInfo info = {IfaceType::P2P, getP2pIfaceName()};
    supplicant_1_1->addInterface(
        info, [&](const SupplicantStatus& status,
                  const sp<ISupplicantIface>& /* iface */) {
            ASSERT_TRUE(
                (SupplicantStatusCode::SUCCESS == status.code) ||
                (SupplicantStatusCode::FAILURE_IFACE_EXISTS == status.code));
        });
}

sp<ISupplicant> getSupplicant(const std::string& supplicant_instance_name,
                              bool isP2pOn) {
    sp<ISupplicant> supplicant =
        ISupplicant::getService(supplicant_instance_name);
    // For 1.1 supplicant, we need to add interfaces at initialization.
    if (is_1_1(supplicant)) {
        addSupplicantStaIface_1_1(supplicant);
        if (isP2pOn) {
            addSupplicantP2pIface_1_1(supplicant);
        }
    }
    return supplicant;
}

sp<ISupplicantStaIface> getSupplicantStaIface(
    const sp<ISupplicant>& supplicant) {
    if (!supplicant.get()) {
        return nullptr;
    }
    ISupplicant::IfaceInfo info;
    if (!findIfaceOfType(supplicant, IfaceType::STA, &info)) {
        return nullptr;
    }
    bool operation_failed = false;
    sp<ISupplicantStaIface> sta_iface;
    supplicant->getInterface(info, [&](const SupplicantStatus& status,
                                       const sp<ISupplicantIface>& iface) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
            operation_failed = true;
            return;
        }
        sta_iface = ISupplicantStaIface::castFrom(iface);
    });
    if (operation_failed) {
        return nullptr;
    }
    return sta_iface;
}

sp<ISupplicantStaNetwork> createSupplicantStaNetwork(
    const sp<ISupplicant>& supplicant) {
    sp<ISupplicantStaIface> sta_iface = getSupplicantStaIface(supplicant);
    if (!sta_iface.get()) {
        return nullptr;
    }
    bool operation_failed = false;
    sp<ISupplicantStaNetwork> sta_network;
    sta_iface->addNetwork([&](const SupplicantStatus& status,
                              const sp<ISupplicantNetwork>& network) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
            operation_failed = true;
            return;
        }
        sta_network = ISupplicantStaNetwork::castFrom(network);
    });
    if (operation_failed) {
        return nullptr;
    }
    return sta_network;
}

sp<ISupplicantP2pIface> getSupplicantP2pIface(
    const sp<ISupplicant>& supplicant) {
    if (!supplicant.get()) {
        return nullptr;
    }
    ISupplicant::IfaceInfo info;
    if (!findIfaceOfType(supplicant, IfaceType::P2P, &info)) {
        return nullptr;
    }
    bool operation_failed = false;
    sp<ISupplicantP2pIface> p2p_iface;
    supplicant->getInterface(info, [&](const SupplicantStatus& status,
                                       const sp<ISupplicantIface>& iface) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
            operation_failed = true;
            return;
        }
        p2p_iface = ISupplicantP2pIface::castFrom(iface);
    });
    if (operation_failed) {
        return nullptr;
    }
    return p2p_iface;
}

bool turnOnExcessiveLogging(const sp<ISupplicant>& supplicant) {
    if (!supplicant.get()) {
        return false;
    }
    bool operation_failed = false;
    supplicant->setDebugParams(
        ISupplicant::DebugLevel::EXCESSIVE,
        true,  // show timestamps
        true,  // show keys
        [&](const SupplicantStatus& status) {
            if (status.code != SupplicantStatusCode::SUCCESS) {
                operation_failed = true;
            }
        });
    return !operation_failed;
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
