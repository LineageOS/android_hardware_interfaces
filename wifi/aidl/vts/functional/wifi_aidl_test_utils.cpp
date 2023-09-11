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

#include "wifi_aidl_test_utils.h"

using ::android::wifi_system::InterfaceTool;

namespace {
bool findAnyModeSupportingConcurrencyType(IfaceConcurrencyType desired_type,
                                          const std::vector<IWifiChip::ChipMode>& modes,
                                          int* mode_id) {
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

bool configureChipToSupportConcurrencyTypeInternal(const std::shared_ptr<IWifiChip>& wifi_chip,
                                                   IfaceConcurrencyType type,
                                                   int* configured_mode_id) {
    if (!configured_mode_id) {
        return false;
    }
    std::vector<IWifiChip::ChipMode> chip_modes;
    auto status = wifi_chip->getAvailableModes(&chip_modes);
    if (!status.isOk()) {
        return false;
    }
    if (!findAnyModeSupportingConcurrencyType(type, chip_modes, configured_mode_id)) {
        return false;
    }
    if (!wifi_chip->configureChip(*configured_mode_id).isOk()) {
        return false;
    }
    return true;
}

bool configureChipToSupportConcurrencyTypeInternal(const std::shared_ptr<IWifiChip>& wifi_chip,
                                                   IfaceConcurrencyType type) {
    int mode_id;
    return configureChipToSupportConcurrencyTypeInternal(wifi_chip, type, &mode_id);
}
}  // namespace

bool checkStatusCode(ndk::ScopedAStatus* status, WifiStatusCode expected_code) {
    if (status == nullptr) {
        return false;
    }
    return status->getServiceSpecificError() == static_cast<int32_t>(expected_code);
}

std::shared_ptr<IWifi> getWifi(const char* instance_name) {
    return IWifi::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService(instance_name)));
}

std::shared_ptr<IWifiChip> getWifiChip(const char* instance_name) {
    std::shared_ptr<IWifi> wifi = getWifi(instance_name);
    if (!wifi.get()) {
        return nullptr;
    }

    const int retry_interval_ms = 2;
    const int max_retries = 5;
    int retry_count = 0;
    auto status = wifi->start();
    while (retry_count < max_retries && !status.isOk()) {
        retry_count++;
        usleep(retry_interval_ms * 1000);
        status = wifi->start();
    }
    if (!status.isOk()) {
        return nullptr;
    }

    std::vector<int> chip_ids = {};
    status = wifi->getChipIds(&chip_ids);
    if (!status.isOk() || chip_ids.size() == 0) {
        return nullptr;
    }
    std::shared_ptr<IWifiChip> chip;
    status = wifi->getChip(chip_ids[0], &chip);
    if (!status.isOk()) {
        return nullptr;
    }
    return chip;
}

void setupStaIface(const std::shared_ptr<IWifiStaIface>& iface) {
    std::string iface_name;
    auto status = iface->getName(&iface_name);
    if (status.isOk()) {
        InterfaceTool iface_tool;
        iface_tool.SetUpState(iface_name.c_str(), true);
    }
}

void setupNanIface(const std::shared_ptr<IWifiNanIface>& iface) {
    std::string iface_name;
    auto status = iface->getName(&iface_name);
    if (status.isOk()) {
        InterfaceTool iface_tool;
        iface_tool.SetUpState(iface_name.c_str(), true);
    }
}

std::shared_ptr<IWifiStaIface> getWifiStaIface(const char* instance_name) {
    std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(instance_name);
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportConcurrencyTypeInternal(wifi_chip, IfaceConcurrencyType::STA)) {
        return nullptr;
    }
    std::shared_ptr<IWifiStaIface> iface;
    auto status = wifi_chip->createStaIface(&iface);
    if (!status.isOk()) {
        return nullptr;
    }
    setupStaIface(iface);
    return iface;
}

std::shared_ptr<IWifiNanIface> getWifiNanIface(const char* instance_name) {
    std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(instance_name);
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportConcurrencyTypeInternal(wifi_chip,
                                                       IfaceConcurrencyType::NAN_IFACE)) {
        return nullptr;
    }
    std::shared_ptr<IWifiNanIface> iface;
    auto status = wifi_chip->createNanIface(&iface);
    if (!status.isOk()) {
        return nullptr;
    }
    setupNanIface(iface);
    return iface;
}

std::shared_ptr<IWifiApIface> getWifiApIface(const char* instance_name) {
    std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(instance_name);
    if (!wifi_chip.get()) {
        return nullptr;
    }
    if (!configureChipToSupportConcurrencyTypeInternal(wifi_chip, IfaceConcurrencyType::AP)) {
        return nullptr;
    }
    std::shared_ptr<IWifiApIface> iface;
    auto status = wifi_chip->createApIface(&iface);
    if (!status.isOk()) {
        return nullptr;
    }
    return iface;
}

std::shared_ptr<IWifiApIface> getBridgedWifiApIface(std::shared_ptr<IWifiChip> wifi_chip) {
    if (!wifi_chip.get()) {
        return nullptr;
    }
    int mode_id;
    std::shared_ptr<IWifiApIface> iface;
    configureChipToSupportConcurrencyTypeInternal(wifi_chip, IfaceConcurrencyType::AP, &mode_id);
    auto status = wifi_chip->createBridgedApIface(&iface);
    if (!status.isOk()) {
        return nullptr;
    }
    return iface;
}

std::shared_ptr<IWifiApIface> getBridgedWifiApIface(const char* instance_name) {
    std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(instance_name);
    return getBridgedWifiApIface(wifi_chip);
}

bool configureChipToSupportConcurrencyType(const std::shared_ptr<IWifiChip>& wifi_chip,
                                           IfaceConcurrencyType type, int* configured_mode_id) {
    if (!wifi_chip.get()) {
        return false;
    }
    return configureChipToSupportConcurrencyTypeInternal(wifi_chip, type, configured_mode_id);
}

bool doesChipSupportConcurrencyType(const std::shared_ptr<IWifiChip>& wifi_chip,
                                    IfaceConcurrencyType type) {
    if (!wifi_chip.get()) {
        return false;
    }
    std::vector<IWifiChip::ChipMode> chip_modes;
    auto status = wifi_chip->getAvailableModes(&chip_modes);
    if (!status.isOk()) {
        return false;
    }
    int mode_id;
    return findAnyModeSupportingConcurrencyType(type, chip_modes, &mode_id);
}

void stopWifiService(const char* instance_name) {
    std::shared_ptr<IWifi> wifi = getWifi(instance_name);
    if (wifi != nullptr) {
        wifi->stop();
    }
}

int32_t getChipFeatureSet(const std::shared_ptr<IWifiChip>& wifi_chip) {
    if (!wifi_chip.get()) {
        return 0;
    }
    int32_t features = 0;
    if (wifi_chip->getFeatureSet(&features).isOk()) {
        return features;
    }
    return 0;
}

bool isAidlServiceAvailable(const char* instance_name) {
    return AServiceManager_isDeclared(instance_name);
}
