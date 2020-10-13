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

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <hidl/HidlTransportSupport.h>

#include <wifi_system/hostapd_manager.h>
#include <wifi_system/interface_tool.h>
#include <wifi_system/supplicant_manager.h>

#include "hostapd_hidl_test_utils.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::joinRpcThreadpool;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatus;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatusCode;
using ::android::hardware::wifi::hostapd::V1_0::IHostapd;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::wifi_system::HostapdManager;
using ::android::wifi_system::SupplicantManager;

namespace {
// Helper function to initialize the driver and firmware to AP mode
// using the vendor HAL HIDL interface.
void initilializeDriverAndFirmware(const std::string& wifi_instance_name) {
    if (getWifi(wifi_instance_name) != nullptr) {
        sp<IWifiChip> wifi_chip = getWifiChip(wifi_instance_name);
        ChipModeId mode_id;
        EXPECT_TRUE(configureChipToSupportIfaceType(
            wifi_chip, ::android::hardware::wifi::V1_0::IfaceType::AP, &mode_id));
    } else {
        LOG(WARNING) << __func__ << ": Vendor HAL not supported";
    }
}

// Helper function to deinitialize the driver and firmware
// using the vendor HAL HIDL interface.
void deInitilializeDriverAndFirmware(const std::string& wifi_instance_name) {
    if (getWifi(wifi_instance_name) != nullptr) {
        stopWifi(wifi_instance_name);
    } else {
        LOG(WARNING) << __func__ << ": Vendor HAL not supported";
    }
}
}  // namespace

void stopSupplicantIfNeeded(const std::string& instance_name) {
    SupplicantManager supplicant_manager;
    if (supplicant_manager.IsSupplicantRunning()) {
        LOG(INFO) << "Supplicant is running, stop supplicant first.";
        ASSERT_TRUE(supplicant_manager.StopSupplicant());
        deInitilializeDriverAndFirmware(instance_name);
        ASSERT_FALSE(supplicant_manager.IsSupplicantRunning());
    }
}

void stopHostapd(const std::string& instance_name) {
    HostapdManager hostapd_manager;

    ASSERT_TRUE(hostapd_manager.StopHostapd());
    deInitilializeDriverAndFirmware(instance_name);
}

void startHostapdAndWaitForHidlService(
    const std::string& wifi_instance_name,
    const std::string& hostapd_instance_name) {
    initilializeDriverAndFirmware(wifi_instance_name);

    HostapdManager hostapd_manager;
    ASSERT_TRUE(hostapd_manager.StartHostapd());

    // Wait for hostapd service to come up.
    IHostapd::getService(hostapd_instance_name);
}

bool is_1_1(const sp<IHostapd>& hostapd) {
    sp<::android::hardware::wifi::hostapd::V1_1::IHostapd> hostapd_1_1 =
        ::android::hardware::wifi::hostapd::V1_1::IHostapd::castFrom(hostapd);
    return hostapd_1_1.get() != nullptr;
}
