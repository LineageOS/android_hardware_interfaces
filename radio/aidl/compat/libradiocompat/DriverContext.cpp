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

#include <libradiocompat/DriverContext.h>

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio;

void DriverContext::addDataProfile(const aidl::data::DataProfileInfo& profile) {
    mDataProfiles[profile.apn] = profile;
}

aidl::data::DataProfileInfo DriverContext::getDataProfile(const std::string& apn) {
    const auto it = mDataProfiles.find(apn);
    if (it != mDataProfiles.end()) return it->second;

    // if not found in cache, return a made up default
    return {
            .apn = apn,
    };
}

void DriverContext::expectSignalCriteriaResponses(int32_t serial, size_t count) {
    std::lock_guard<std::mutex> lock(mSignalCriteriaLock);
    mSignalCriteriaResponses[serial] = {
            .remaining = count,
            .error = aidl::RadioError::NONE,
    };
}

bool DriverContext::collectSignalCriteriaResponse(aidl::RadioResponseInfo& info) {
    std::lock_guard<std::mutex> lock(mSignalCriteriaLock);
    const auto it = mSignalCriteriaResponses.find(info.serial);
    if (it == mSignalCriteriaResponses.end()) return true;

    auto& pending = it->second;
    if (pending.error == aidl::RadioError::NONE) pending.error = info.error;
    if (pending.remaining > 1) {
        pending.remaining--;
        return false;
    }

    info.error = pending.error;
    mSignalCriteriaResponses.erase(it);
    return true;
}

}  // namespace android::hardware::radio::compat
