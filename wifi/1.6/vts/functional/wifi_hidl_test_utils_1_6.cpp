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

#include <VtsHalHidlTargetCallbackBase.h>

#undef NAN  // NAN is defined in bionic/libc/include/math.h:38

#include <android/hardware/wifi/1.5/IWifiApIface.h>
#include <android/hardware/wifi/1.6/IWifiChip.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_5::IWifiApIface;
using ::android::hardware::wifi::V1_6::IfaceConcurrencyType;
using ::android::hardware::wifi::V1_6::IWifiChip;

namespace {

bool findAnyModeSupportingConcurrencyType(IfaceConcurrencyType desired_type,
                                          const std::vector<IWifiChip::ChipMode>& modes,
                                          ChipModeId* mode_id) {
    for (const auto& mode : modes) {
        for (const auto& combination : mode.availableCombinations) {
            for (const auto& iface_limit : combination.limits) {
                const auto& iface_types = iface_limit.types;
                if (std::find(iface_types.begin(), iface_types.end(), desired_type) !=
                    iface_types.end()) {
                    *mode_id = mode.id;
                    return true;
                }
            }
        }
    }
    return false;
}

bool configureChipToSupportConcurrencyType(const sp<IWifiChip>& wifi_chip,
                                           IfaceConcurrencyType type,
                                           ChipModeId* configured_mode_id) {
    const auto& status_and_modes = HIDL_INVOKE(wifi_chip, getAvailableModes_1_6);
    if (status_and_modes.first.code != WifiStatusCode::SUCCESS) {
        return false;
    }
    if (!findAnyModeSupportingConcurrencyType(type, status_and_modes.second, configured_mode_id)) {
        return false;
    }
    if (HIDL_INVOKE(wifi_chip, configureChip, *configured_mode_id).code !=
        WifiStatusCode::SUCCESS) {
        return false;
    }
    return true;
}

sp<IWifiChip> getWifiChip_1_6(const std::string& instance_name) {
    return IWifiChip::castFrom(getWifiChip(instance_name));
}

}  // namespace

sp<IWifiApIface> getBridgedWifiApIface_1_6(const std::string& instance_name) {
    ChipModeId mode_id;
    sp<IWifiChip> wifi_chip = getWifiChip_1_6(instance_name);
    if (!wifi_chip.get()) return nullptr;
    configureChipToSupportConcurrencyType(wifi_chip, IfaceConcurrencyType::AP_BRIDGED, &mode_id);
    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createBridgedApIface);
    return IWifiApIface::castFrom(status_and_iface.second);
}
