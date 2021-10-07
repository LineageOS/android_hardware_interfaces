/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "GnssHalTest"

#include <android/hidl/manager/1.2/IServiceManager.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>
#include <vintf/VintfObject.h>

#include "gnss_hal_test.h"

using ::android::hardware::hidl_string;
using ::android::hidl::manager::V1_2::IServiceManager;

bool GnssHalTest::IsGnssHalVersion_2_1() const {
    sp<IServiceManager> manager = ::android::hardware::defaultServiceManager1_2();
    bool hasGnssHalVersion_2_1 = false;
    manager->listManifestByInterface(
            "android.hardware.gnss@2.1::IGnss",
            [&hasGnssHalVersion_2_1](const hidl_vec<hidl_string>& registered) {
                hasGnssHalVersion_2_1 = registered.size() > 0;
            });

    auto deviceManifest = ::android::vintf::VintfObject::GetDeviceHalManifest();
    bool hasGnssAidl =
            deviceManifest->getAidlInstances("android.hardware.gnss", "IGnss").size() > 0;

    return hasGnssHalVersion_2_1 && !hasGnssAidl;
}