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

#define LOG_TAG "GnssPsds"

#include "GnssPsds.h"

#include <log/log.h>

namespace android::hardware::gnss::V3_0::implementation {

sp<V3_0::IGnssPsdsCallback> GnssPsds::sCallback_3_0 = nullptr;

// Methods from V1_0::IGnssXtra follow.
Return<bool> GnssPsds::setCallback(const sp<V1_0::IGnssXtraCallback>&) {
    // TODO implement
    return bool{};
}

Return<bool> GnssPsds::injectXtraData(const hidl_string&) {
    // TODO implement
    return bool{};
}

// Methods from V3_0::IGnssPsds follow.
Return<bool> GnssPsds::setCallback_3_0(const sp<V3_0::IGnssPsdsCallback>& callback) {
    ALOGD("setCallback_3_0");
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback_3_0 = callback;
    return true;
}

Return<bool> GnssPsds::injectPsdsData_3_0(int32_t psdsType, const hidl_string& psdsData) {
    ALOGD("injectPsdsData_3_0. psdsType: %d, psdsData: %s", psdsType, psdsData.c_str());
    return true;
}
}  // namespace android::hardware::gnss::V3_0::implementation
