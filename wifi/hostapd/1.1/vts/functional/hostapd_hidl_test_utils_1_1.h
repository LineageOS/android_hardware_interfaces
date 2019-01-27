/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef HOSTAPD_HIDL_TEST_UTILS_1_1_H
#define HOSTAPD_HIDL_TEST_UTILS_1_1_H

#include <android/hardware/wifi/hostapd/1.1/IHostapd.h>

#include <VtsHalHidlTargetTestEnvBase.h>

// Helper functions to obtain references to the various HIDL interface objects.
// Note: We only have a single instance of each of these objects currently.
// These helper functions should be modified to return vectors if we support
// multiple instances.
android::sp<android::hardware::wifi::hostapd::V1_1::IHostapd> getHostapd_1_1();

#endif /* HOSTAPD_HIDL_TEST_UTILS_1_1_H */
