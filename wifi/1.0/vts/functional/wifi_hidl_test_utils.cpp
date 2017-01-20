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

#include <gtest/gtest.h>

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
    ASSERT_EQ(std::system("svc wifi disable"), 0);
    sleep(5);
}

void startFramework() { ASSERT_EQ(std::system("svc wifi enable"), 0); }

sp<IWifi> getWifi() {
    sp<IWifi> wifi = IWifi::getService();
    return wifi;
}

sp<IWifiChip> getWifiChip() {
    sp<IWifi> wifi = getWifi();
    if (!wifi.get()) {
        return nullptr;
    }

    bool operation_failed = false;
    wifi->start([&](WifiStatus status) {
        if (status.code != WifiStatusCode::SUCCESS) {
            operation_failed = true;
        }
    });
    if (operation_failed) {
        return nullptr;
    }

    std::vector<ChipId> wifi_chip_ids;
    wifi->getChipIds(
        [&](const WifiStatus& status, const hidl_vec<ChipId>& chip_ids) {
            if (status.code != WifiStatusCode::SUCCESS) {
                operation_failed = true;
            }
            wifi_chip_ids = chip_ids;
        });
    // We don't expect more than 1 chip currently.
    if (operation_failed || wifi_chip_ids.size() != 1) {
        return nullptr;
    }

    sp<IWifiChip> wifi_chip;
    wifi->getChip(wifi_chip_ids[0],
                  [&](const WifiStatus& status, const sp<IWifiChip>& chip) {
                      if (status.code != WifiStatusCode::SUCCESS) {
                          operation_failed = true;
                      }
                      wifi_chip = chip;
                  });
    if (operation_failed) {
        return nullptr;
    }
    return wifi_chip;
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
    bool operation_failed = false;
    std::vector<IWifiChip::ChipMode> chip_modes;
    wifi_chip->getAvailableModes(
        [&](WifiStatus status, const hidl_vec<IWifiChip::ChipMode>& modes) {
            if (status.code != WifiStatusCode::SUCCESS) {
                operation_failed = true;
            }
            chip_modes = modes;
        });
    if (operation_failed) {
        return false;
    }

    ChipModeId mode_id;
    if (!findModeToSupportIfaceType(type, chip_modes, &mode_id)) {
        return false;
    }

    wifi_chip->configureChip(mode_id, [&](WifiStatus status) {
        if (status.code != WifiStatusCode::SUCCESS) {
            operation_failed = true;
        }
    });
    if (operation_failed) {
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

    bool operation_failed = false;
    sp<IWifiApIface> wifi_ap_iface;
    wifi_chip->createApIface(
        [&](const WifiStatus& status, const sp<IWifiApIface>& iface) {
            if (status.code != WifiStatusCode::SUCCESS) {
                operation_failed = true;
            }
            wifi_ap_iface = iface;
        });
    if (operation_failed) {
        return nullptr;
    }
    return wifi_ap_iface;
}

sp<IWifiNanIface> getWifiNanIface() {
    sp<IWifiChip> wifi_chip = getWifiChip();
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceType(wifi_chip, IfaceType::NAN)) {
        return nullptr;
    }

    bool operation_failed = false;
    sp<IWifiNanIface> wifi_nan_iface;
    wifi_chip->createNanIface(
        [&](const WifiStatus& status, const sp<IWifiNanIface>& iface) {
            if (status.code != WifiStatusCode::SUCCESS) {
                operation_failed = true;
            }
            wifi_nan_iface = iface;
        });
    if (operation_failed) {
        return nullptr;
    }
    return wifi_nan_iface;
}

sp<IWifiP2pIface> getWifiP2pIface() {
    sp<IWifiChip> wifi_chip = getWifiChip();
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceType(wifi_chip, IfaceType::P2P)) {
        return nullptr;
    }

    bool operation_failed = false;
    sp<IWifiP2pIface> wifi_p2p_iface;
    wifi_chip->createP2pIface(
        [&](const WifiStatus& status, const sp<IWifiP2pIface>& iface) {
            if (status.code != WifiStatusCode::SUCCESS) {
                operation_failed = true;
            }
            wifi_p2p_iface = iface;
        });
    if (operation_failed) {
        return nullptr;
    }
    return wifi_p2p_iface;
}

sp<IWifiStaIface> getWifiStaIface() {
    sp<IWifiChip> wifi_chip = getWifiChip();
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceType(wifi_chip, IfaceType::STA)) {
        return nullptr;
    }

    bool operation_failed = false;
    sp<IWifiStaIface> wifi_sta_iface;
    wifi_chip->createStaIface(
        [&](const WifiStatus& status, const sp<IWifiStaIface>& iface) {
            if (status.code != WifiStatusCode::SUCCESS) {
                operation_failed = true;
            }
            wifi_sta_iface = iface;
        });
    if (operation_failed) {
        return nullptr;
    }
    return wifi_sta_iface;
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

    bool operation_failed = false;
    sp<IWifiRttController> wifi_rtt_controller;
    wifi_chip->createRttController(
        wifi_sta_iface, [&](const WifiStatus& status,
                            const sp<IWifiRttController>& controller) {
            if (status.code != WifiStatusCode::SUCCESS) {
                operation_failed = true;
            }
            wifi_rtt_controller = controller;
        });
    if (operation_failed) {
        return nullptr;
    }
    return wifi_rtt_controller;
}

void stopWifi() {
    sp<IWifi> wifi = getWifi();
    ASSERT_NE(wifi, nullptr);
    wifi->stop([](const WifiStatus& status) {
        ASSERT_EQ(status.code, WifiStatusCode::SUCCESS);
    });
}
