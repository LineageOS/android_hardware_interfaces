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

#pragma once

#include <android/hardware/wifi/1.4/IWifi.h>
#include <android/hardware/wifi/1.4/IWifiApIface.h>
#include <android/hardware/wifi/1.4/IWifiChip.h>

#include <getopt.h>

#include <VtsHalHidlTargetTestEnvBase.h>
// Helper functions to obtain references to the various HIDL interface objects.
// Note: We only have a single instance of each of these objects currently.
// These helper functions should be modified to return vectors if we support
// multiple instances.
android::sp<android::hardware::wifi::V1_4::IWifiChip> getWifiChip_1_4(
    const std::string& instance_name);
android::sp<android::hardware::wifi::V1_4::IWifiApIface> getWifiApIface_1_4(
    const std::string& instance_name);
