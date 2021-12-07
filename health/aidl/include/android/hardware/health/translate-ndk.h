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

#include <aidl/android/hardware/health/BatteryCapacityLevel.h>
#include <aidl/android/hardware/health/DiskStats.h>
#include <aidl/android/hardware/health/HealthInfo.h>
#include <aidl/android/hardware/health/StorageInfo.h>
#include <android/hardware/health/2.0/types.h>
#include <android/hardware/health/2.1/types.h>
#include <limits>

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::StorageInfo& in,
        aidl::android::hardware::health::StorageInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::DiskStats& in,
        aidl::android::hardware::health::DiskStats* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_0::HealthInfo& in,
        aidl::android::hardware::health::HealthInfo* out);
__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::health::V2_1::HealthInfo& in,
        aidl::android::hardware::health::HealthInfo* out);

}  // namespace android::h2a
