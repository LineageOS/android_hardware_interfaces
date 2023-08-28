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

#include "supplicant_aidl_test_utils.h"
#include "supplicant_legacy_test_utils.h"

using aidl::android::hardware::wifi::supplicant::IfaceInfo;
using aidl::android::hardware::wifi::supplicant::ISupplicant;
using aidl::android::hardware::wifi::supplicant::ISupplicantP2pIface;
using aidl::android::hardware::wifi::supplicant::ISupplicantStaIface;
using aidl::android::hardware::wifi::supplicant::KeyMgmtMask;

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

bool keyMgmtSupported(std::shared_ptr<ISupplicantStaIface> iface, KeyMgmtMask expected) {
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

void stopSupplicantService() {
    // Select method based on whether the HIDL or AIDL
    // Vendor HAL is available.
    if (SupplicantAidlTestUtils::useAidlService()) {
        SupplicantAidlTestUtils::stopSupplicantService();
    } else {
        SupplicantLegacyTestUtils::stopSupplicantService();
    }
}

void initializeService() {
    if (SupplicantAidlTestUtils::useAidlService()) {
        SupplicantAidlTestUtils::initializeService();
    } else {
        SupplicantLegacyTestUtils::initializeService();
    }
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
