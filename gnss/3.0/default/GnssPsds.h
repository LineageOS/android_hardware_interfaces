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

#pragma once

#include <android/hardware/gnss/3.0/IGnssPsds.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android::hardware::gnss::V3_0::implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct GnssPsds : public V3_0::IGnssPsds {
    // Methods from V1_0::IGnssXtra follow.
    Return<bool> setCallback(const sp<V1_0::IGnssXtraCallback>& callback) override;
    Return<bool> injectXtraData(const hidl_string& xtraData) override;

    // Methods from V3_0::IGnssPsds follow.
    Return<bool> setCallback_3_0(const sp<V3_0::IGnssPsdsCallback>& callback) override;
    Return<bool> injectPsdsData_3_0(int32_t psdsType, const hidl_string& psdsData) override;

  private:
    // Guarded by mMutex
    static sp<V3_0::IGnssPsdsCallback> sCallback_3_0;

    // Synchronization lock for sCallback_3_0
    mutable std::mutex mMutex;
};

}  // namespace android::hardware::gnss::V3_0::implementation
