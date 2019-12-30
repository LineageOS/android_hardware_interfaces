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

#include <android/log.h>

#include <VtsHalHidlTargetTestBase.h>

#include <wifi_system/interface_tool.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::hardware::wifi::V1_0::IWifi;
using ::android::hardware::wifi::V1_0::IWifiApIface;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::hardware::wifi::V1_0::IWifiIface;
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
using ::android::wifi_system::InterfaceTool;

extern WifiHidlEnvironment* gEnv;

namespace {
constexpr uint32_t kHalStartRetryMaxCount = 5;
constexpr uint32_t kHalStartRetryIntervalInMs = 2;

bool findAnyModeSupportingIfaceType(
    IfaceType desired_type, const std::vector<IWifiChip::ChipMode>& modes,
    ChipModeId* mode_id) {
    for (const auto& mode : modes) {
        for (const auto& combination : mode.availableCombinations) {
            for (const auto& iface_limit : combination.limits) {
                const auto& iface_types = iface_limit.types;
                if (std::find(iface_types.begin(), iface_types.end(),
                              desired_type) != iface_types.end()) {
                    *mode_id = mode.id;
                    return true;
                }
            }
        }
    }
    return false;
}

bool configureChipToSupportIfaceTypeInternal(const sp<IWifiChip>& wifi_chip,
                                             IfaceType type,
                                             ChipModeId* configured_mode_id) {
    if (!configured_mode_id) {
        return false;
    }
    const auto& status_and_modes = HIDL_INVOKE(wifi_chip, getAvailableModes);
    if (status_and_modes.first.code != WifiStatusCode::SUCCESS) {
        return false;
    }
    if (!findAnyModeSupportingIfaceType(type, status_and_modes.second,
                                        configured_mode_id)) {
        return false;
    }
    if (HIDL_INVOKE(wifi_chip, configureChip, *configured_mode_id).code !=
        WifiStatusCode::SUCCESS) {
        return false;
    }
    return true;
}

bool configureChipToSupportIfaceTypeInternal(const sp<IWifiChip>& wifi_chip,
                                             IfaceType type) {
    ChipModeId mode_id;
    return configureChipToSupportIfaceTypeInternal(wifi_chip, type, &mode_id);
}
}  // namespace

sp<IWifi> getWifi(const std::string& instance_name) {
    if ((!gEnv && instance_name.empty()) || (gEnv && !instance_name.empty())) {
        ALOGE("instance_name and gEnv must have one and only one set.");
        return nullptr;
    }
    if (gEnv) {
        return ::testing::VtsHalHidlTargetTestBase::getService<IWifi>(
            gEnv->getServiceName<IWifi>());
    } else {
        return IWifi::getService(instance_name);
    }
}

sp<IWifiChip> getWifiChip(const std::string& instance_name) {
    sp<IWifi> wifi = getWifi(instance_name);
    if (!wifi.get()) {
        return nullptr;
    }
    uint32_t retry_count = 0;
    auto status = HIDL_INVOKE(wifi, start);
    while (retry_count < kHalStartRetryMaxCount &&
           status.code == WifiStatusCode::ERROR_NOT_AVAILABLE) {
        retry_count++;
        usleep(kHalStartRetryIntervalInMs * 1000);
        status = HIDL_INVOKE(wifi, start);
    }
    if (status.code != WifiStatusCode::SUCCESS) {
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

void setIfaceUp(const sp<IWifiIface>& iface) {
    // Set the iface up before retrurning the object.
    const auto& status_and_name = HIDL_INVOKE(iface, getName);
    if (status_and_name.first.code == WifiStatusCode::SUCCESS) {
        const auto& iface_name = status_and_name.second;
        InterfaceTool iface_tool;
        iface_tool.SetUpState(iface_name.c_str(), true);
    }
}

sp<IWifiApIface> getWifiApIface(const std::string& instance_name) {
    sp<IWifiChip> wifi_chip = getWifiChip(instance_name);
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceTypeInternal(wifi_chip, IfaceType::AP)) {
        return nullptr;
    }
    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createApIface);
    if (status_and_iface.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    setIfaceUp(status_and_iface.second);
    return status_and_iface.second;
}

sp<IWifiNanIface> getWifiNanIface(const std::string& instance_name) {
    sp<IWifiChip> wifi_chip = getWifiChip(instance_name);
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceTypeInternal(wifi_chip, IfaceType::NAN)) {
        return nullptr;
    }
    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createNanIface);
    if (status_and_iface.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    setIfaceUp(status_and_iface.second);
    return status_and_iface.second;
}

sp<IWifiP2pIface> getWifiP2pIface(const std::string& instance_name) {
    sp<IWifiChip> wifi_chip = getWifiChip(instance_name);
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceTypeInternal(wifi_chip, IfaceType::P2P)) {
        return nullptr;
    }
    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createP2pIface);
    if (status_and_iface.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    setIfaceUp(status_and_iface.second);
    return status_and_iface.second;
}

sp<IWifiStaIface> getWifiStaIface(const std::string& instance_name) {
    sp<IWifiChip> wifi_chip = getWifiChip(instance_name);
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportIfaceTypeInternal(wifi_chip, IfaceType::STA)) {
        return nullptr;
    }
    const auto& status_and_iface = HIDL_INVOKE(wifi_chip, createStaIface);
    if (status_and_iface.first.code != WifiStatusCode::SUCCESS) {
        return nullptr;
    }
    setIfaceUp(status_and_iface.second);
    return status_and_iface.second;
}

bool configureChipToSupportIfaceType(const sp<IWifiChip>& wifi_chip,
                                     IfaceType type,
                                     ChipModeId* configured_mode_id) {
    return configureChipToSupportIfaceTypeInternal(wifi_chip, type,
                                                   configured_mode_id);
}

void stopWifi(const std::string& instance_name) {
    sp<IWifi> wifi = getWifi(instance_name);
    ASSERT_NE(wifi, nullptr);
    HIDL_INVOKE(wifi, stop);
}
