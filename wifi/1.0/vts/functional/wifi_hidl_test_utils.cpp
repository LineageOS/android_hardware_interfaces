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

#include <VtsHalHidlTargetBaseTest.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::hardware::wifi::V1_0::IWifi;
using ::android::hardware::wifi::V1_0::IWifiApIface;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::hardware::wifi::V1_0::IWifiNanIface;
using ::android::hardware::wifi::V1_0::IWifiP2pIface;
using ::android::hardware::wifi::V1_0::IWifiRttController;
using ::android::hardware::wifi::V1_0::IWifiStaIface;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::ChipId;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;

void stopFramework() {
    ASSERT_EQ(std::system("stop"), 0);
    stopWifi();
    sleep(5);
}

void startFramework() { ASSERT_EQ(std::system("start"), 0); }

sp<IWifi> getWifi() {
    sp<IWifi> wifi = ::testing::VtsHalHidlTargetBaseTest::getService<IWifi>();
    return wifi;
}

sp<IWifiChip> getWifiChip() {
    sp<IWifi> wifi = getWifi();
    if (!wifi.get()) {
        return nullptr;
    }

    if (HIDL_INVOKE(wifi, start).code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }

    const auto& status_and_chip_ids = HIDL_INVOKE(wifi, getChipIds);
    const auto& chip_ids = status_and_chip_ids.second;
    if (status_and_chip_ids.first.code != WifiStatusCode::SUCCESS ||
        chip_ids.size() != 1) {
        return nullptr;
    }

    const auto& status_and_chip = HIDL_INVOKE(wifi, getChip, chip_ids[0]);
    if (status_and_chip.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }

    return status_and_chip.second;
}

// Since we currently only support one iface of each type. Just iterate thru the
// modes of operation and find the mode ID to use for that iface type.
bool findModeToSupportIfaceType(IfaceType type,
                                const std::vector<IWifiChip::ChipMode>& modes,
                                ChipModeId* mode_id) {
    for (const auto& mode : modes) {
        std::vector<IWifiChip::ChipIfaceCombination> combinations =
            mode.availableCombinations;
        for (const auto& combination : combinations) {
            std::vector<IWifiChip::ChipIfaceCombinationLimit> iface_limits =
                combination.limits;
            for (const auto& iface_limit : iface_limits) {
                std::vector<IfaceType> iface_types = iface_limit.types;
                for (const auto& iface_type : iface_types) {
                    if (iface_type == type) {
                        *mode_id = mode.id;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool configureChipToSupportIfaceType(const sp<IWifiChip>& wifi_chip,
                                     IfaceType type) {
    const auto& status_and_modes = HIDL_INVOKE(wifi_chip, getAvailableModes);
    if (status_and_modes.first.code != WifiStatusCode::SUCCESS) {
        return false;
    }

    ChipModeId mode_id;
    if (!findModeToSupportIfaceType(type, status_and_modes.second, &mode_id)) {
        return false;
    }

    if (HIDL_INVOKE(wifi_chip, configureChip, mode_id).code !=
        WifiStatusCode::SUCCESS) {
        return false;
    }
    return true;
}

sp<IWifiApIface> getWifiApIface() {
    sp<IWifiChip> wifi_chip = getWifiChip();
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceType(wifi_chip, IfaceType::AP)) {
        return nullptr;
    }

    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createApIface);
    if (status_and_iface.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    return status_and_iface.second;
}

sp<IWifiNanIface> getWifiNanIface() {
    sp<IWifiChip> wifi_chip = getWifiChip();
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceType(wifi_chip, IfaceType::NAN)) {
        return nullptr;
    }

    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createNanIface);
    if (status_and_iface.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    return status_and_iface.second;
}

sp<IWifiP2pIface> getWifiP2pIface() {
    sp<IWifiChip> wifi_chip = getWifiChip();
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceType(wifi_chip, IfaceType::P2P)) {
        return nullptr;
    }

    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createP2pIface);
    if (status_and_iface.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    return status_and_iface.second;
}

sp<IWifiStaIface> getWifiStaIface() {
    sp<IWifiChip> wifi_chip = getWifiChip();
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceType(wifi_chip, IfaceType::STA)) {
        return nullptr;
    }

    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createStaIface);
    if (status_and_iface.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    return status_and_iface.second;
}

sp<IWifiRttController> getWifiRttController() {
    sp<IWifiChip> wifi_chip = getWifiChip();
    if (!wifi_chip.get()) {
        return nullptr;
    }
    sp<IWifiStaIface> wifi_sta_iface = getWifiStaIface();
    if (!wifi_sta_iface.get()) {
        return nullptr;
    }

    const auto& status_and_controller =
        HIDL_INVOKE(wifi_chip, createRttController, wifi_sta_iface);
    if (status_and_controller.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    return status_and_controller.second;
}

void stopWifi() {
    sp<IWifi> wifi = getWifi();
    ASSERT_NE(wifi, nullptr);
    ASSERT_EQ(HIDL_INVOKE(wifi, stop).code, WifiStatusCode::SUCCESS);
}
