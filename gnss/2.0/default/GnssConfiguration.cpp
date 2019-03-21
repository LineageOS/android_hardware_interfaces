/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "GnssConfiguration.h"
#include <log/log.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

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

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android