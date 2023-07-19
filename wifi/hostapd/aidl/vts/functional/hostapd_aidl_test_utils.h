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

#include <aidl/android/hardware/wifi/IWifi.h>
#include <android-base/logging.h>

#include "wifi_aidl_test_utils.h"

namespace {

const std::string kWifiInstanceNameStr = std::string() + IWifi::descriptor + "/default";
const char* kWifiInstanceName = kWifiInstanceNameStr.c_str();

}  // namespace

namespace HostapdAidlTestUtils {

bool useAidlService() {
    return isAidlServiceAvailable(kWifiInstanceName);
}

void startAndConfigureVendorHal() {
    if (getWifi(kWifiInstanceName) != nullptr) {
        std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(kWifiInstanceName);
        int mode_id;
        EXPECT_TRUE(configureChipToSupportConcurrencyType(wifi_chip, IfaceConcurrencyType::AP,
                                                          &mode_id));
    } else {
        LOG(ERROR) << "Unable to initialize Vendor HAL";
    }
}

void stopVendorHal() {
    if (getWifi(kWifiInstanceName) != nullptr) {
        stopWifiService(kWifiInstanceName);
    } else {
        LOG(ERROR) << "Unable to stop Vendor HAL";
    }
}

std::string setupApIfaceAndGetName(bool isBridged) {
    std::shared_ptr<IWifiApIface> wifi_ap_iface;
    if (isBridged) {
        wifi_ap_iface = getBridgedWifiApIface(kWifiInstanceName);
    } else {
        wifi_ap_iface = getWifiApIface(kWifiInstanceName);
    }

    EXPECT_TRUE(wifi_ap_iface.get() != nullptr);
    if (!wifi_ap_iface.get()) {
        LOG(ERROR) << "Unable to create iface. isBridged=" << isBridged;
        return "";
    }

    std::string ap_iface_name;
    auto status = wifi_ap_iface->getName(&ap_iface_name);
    EXPECT_TRUE(status.isOk());
    if (!status.isOk()) {
        LOG(ERROR) << "Unable to retrieve iface name. isBridged=" << isBridged;
        return "";
    }
    return ap_iface_name;
}

}  // namespace HostapdAidlTestUtils
