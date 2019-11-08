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

#ifndef HOSTAPD_HIDL_TEST_UTILS_H
#define HOSTAPD_HIDL_TEST_UTILS_H

#include <android/hardware/wifi/hostapd/1.0/IHostapd.h>
#include <android/hardware/wifi/hostapd/1.1/IHostapd.h>

// Used to stop the android wifi framework before every test.
void stopWifiFramework(const std::string& instance_name);
void startWifiFramework(const std::string& instance_name);
void stopSupplicantIfNeeded(const std::string& instance_name);
void stopHostapd(const std::string& instance_name);
// Used to configure the chip, driver and start wpa_hostapd before every
// test.
void startHostapdAndWaitForHidlService(
    const std::string& wifi_instance_name,
    const std::string& hostapd_instance_name);

bool is_1_1(const android::sp<android::hardware::wifi::hostapd::V1_0::IHostapd>&
                hostapd);

#endif /* HOSTAPD_HIDL_TEST_UTILS_H */
