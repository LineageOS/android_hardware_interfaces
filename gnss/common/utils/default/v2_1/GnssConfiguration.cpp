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

#define LOG_TAG "GnssConfiguration"

#include "v2_1/GnssConfiguration.h"
#include <log/log.h>

namespace android::hardware::gnss::V2_1::implementation {

using GnssSvInfoV2_1 = V2_1::IGnssCallback::GnssSvInfo;
using BlacklistedSourceV2_1 = V2_1::IGnssConfiguration::BlacklistedSource;

// Methods from ::android::hardware::gnss::V1_0::IGnssConfiguration follow.
Return<bool> GnssConfiguration::setSuplEs(bool enable) {
    ALOGD("setSuplEs enable: %d", enable);
    // Method deprecated in 2.0 and not expected to be called by the framework.
    return false;
}

Return<bool> GnssConfiguration::setSuplVersion(uint32_t) {
    return true;
}

Return<bool> GnssConfiguration::setSuplMode(hidl_bitfield<SuplMode>) {
    return true;
}

Return<bool> GnssConfiguration::setGpsLock(hidl_bitfield<GpsLock> gpsLock) {
    ALOGD("setGpsLock gpsLock: %hhu", static_cast<GpsLock>(gpsLock));
    // Method deprecated in 2.0 and not expected to be called by the framework.
    return false;
}

Return<bool> GnssConfiguration::setLppProfile(hidl_bitfield<LppProfile>) {
    return true;
}

Return<bool> GnssConfiguration::setGlonassPositioningProtocol(hidl_bitfield<GlonassPosProtocol>) {
    return true;
}

Return<bool> GnssConfiguration::setEmergencySuplPdn(bool) {
    return true;
}

// Methods from ::android::hardware::gnss::V1_1::IGnssConfiguration follow.
Return<bool> GnssConfiguration::setBlacklist(
        const hidl_vec<V1_1::IGnssConfiguration::BlacklistedSource>&) {
    // TODO (b/122463906): Reuse 1.1 implementation.
    return bool{};
}

// Methods from ::android::hardware::gnss::V2_0::IGnssConfiguration follow.
Return<bool> GnssConfiguration::setEsExtensionSec(uint32_t emergencyExtensionSeconds) {
    ALOGD("setEsExtensionSec emergencyExtensionSeconds: %d", emergencyExtensionSeconds);
    return true;
}

// Methods from ::android::hardware::gnss::V2_1::IGnssConfiguration follow.
Return<bool> GnssConfiguration::setBlacklist_2_1(
        const hidl_vec<BlacklistedSourceV2_1>& sourceList) {
    std::unique_lock<std::recursive_mutex> lock(mMutex);
    mBlacklistedConstellationSet.clear();
    mBlacklistedSourceSet.clear();
    for (auto source : sourceList) {
        if (source.svid == 0) {
            // Wildcard blacklist, i.e., blacklist entire constellation.
            mBlacklistedConstellationSet.insert(source.constellation);
        } else {
            mBlacklistedSourceSet.insert(source);
        }
    }
    return true;
}

Return<bool> GnssConfiguration::isBlacklistedV2_1(const GnssSvInfoV2_1& gnssSvInfo) const {
    std::unique_lock<std::recursive_mutex> lock(mMutex);
    if (mBlacklistedConstellationSet.find(gnssSvInfo.v2_0.constellation) !=
        mBlacklistedConstellationSet.end()) {
        return true;
    }
    BlacklistedSourceV2_1 source = {.constellation = gnssSvInfo.v2_0.constellation,
                                    .svid = gnssSvInfo.v2_0.v1_0.svid};
    return (mBlacklistedSourceSet.find(source) != mBlacklistedSourceSet.end());
}

}  // namespace android::hardware::gnss::V2_1::implementation
