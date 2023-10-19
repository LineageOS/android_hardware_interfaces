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

#include <android-base/logging.h>

#include "hostapd_hidl_test_utils.h"
#include "wifi_hidl_test_utils.h"

using ::android::hardware::wifi::V1_0::WifiStatus;

namespace {

std::string getWifiInstanceName() {
    const std::vector<std::string> instances = android::hardware::getAllHalInstanceNames(
            ::android::hardware::wifi::V1_0::IWifi::descriptor);
    EXPECT_NE(0, instances.size());
    return instances.size() != 0 ? instances[0] : "";
}

}  // namespace

namespace HostapdLegacyTestUtils {

void startAndConfigureVendorHal() {
    initializeDriverAndFirmware(getWifiInstanceName());
}

void stopVendorHal() {
    deInitializeDriverAndFirmware(getWifiInstanceName());
}

std::string setupApIfaceAndGetName(bool isBridged) {
    android::sp<::android::hardware::wifi::V1_0::IWifiApIface> wifi_ap_iface;
    if (isBridged) {
        wifi_ap_iface = getBridgedWifiApIface_1_6(getWifiInstanceName());
    } else {
        wifi_ap_iface = getWifiApIface_1_5(getWifiInstanceName());
    }

    EXPECT_TRUE(wifi_ap_iface.get() != nullptr);
    if (!wifi_ap_iface.get()) {
        LOG(ERROR) << "Unable to create iface. isBridged=" << isBridged;
        return "";
    }

    const auto& status_and_name = HIDL_INVOKE(wifi_ap_iface, getName);
    EXPECT_TRUE(status_and_name.first.code ==
                android::hardware::wifi::V1_0::WifiStatusCode::SUCCESS);
    if (status_and_name.first.code != android::hardware::wifi::V1_0::WifiStatusCode::SUCCESS) {
        LOG(ERROR) << "Unable to retrieve iface name. isBridged=" << isBridged;
        return "";
    }
    return status_and_name.second;
}

}  // namespace HostapdLegacyTestUtils
