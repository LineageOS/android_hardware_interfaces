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

#pragma once

#include <android/hardware/gnss/2.1/IGnssCallback.h>
#include <android/hardware/gnss/2.1/IGnssConfiguration.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <mutex>
#include <unordered_set>

namespace android::hardware::gnss::V2_1::implementation {

struct BlacklistedSourceHashV2_1 {
    inline int operator()(
            const ::android::hardware::gnss::V2_1::IGnssConfiguration::BlacklistedSource& source)
            const {
        return int(source.constellation) * 1000 + int(source.svid);
    }
};

struct BlacklistedSourceEqualV2_1 {
    inline bool operator()(
            const ::android::hardware::gnss::V2_1::IGnssConfiguration::BlacklistedSource& s1,
            const ::android::hardware::gnss::V2_1::IGnssConfiguration::BlacklistedSource& s2)
            const {
        return (s1.constellation == s2.constellation) && (s1.svid == s2.svid);
    }
};

using BlacklistedSourceSetV2_1 =
        std::unordered_set<::android::hardware::gnss::V2_1::IGnssConfiguration::BlacklistedSource,
                           BlacklistedSourceHashV2_1, BlacklistedSourceEqualV2_1>;
using BlacklistedConstellationSetV2_1 = std::unordered_set<V2_0::GnssConstellationType>;

struct GnssConfiguration : public IGnssConfiguration {
    // Methods from ::android::hardware::gnss::V1_0::IGnssConfiguration follow.
    Return<bool> setSuplEs(bool enabled) override;
    Return<bool> setSuplVersion(uint32_t version) override;
    Return<bool> setSuplMode(hidl_bitfield<SuplMode> mode) override;
    Return<bool> setGpsLock(hidl_bitfield<GpsLock> lock) override;
    Return<bool> setLppProfile(hidl_bitfield<LppProfile> lppProfile) override;
    Return<bool> setGlonassPositioningProtocol(hidl_bitfield<GlonassPosProtocol> protocol) override;
    Return<bool> setEmergencySuplPdn(bool enable) override;

    // Methods from ::android::hardware::gnss::V1_1::IGnssConfiguration follow.
    Return<bool> setBlacklist(
            const hidl_vec<V1_1::IGnssConfiguration::BlacklistedSource>& blacklist) override;

    std::recursive_mutex& getMutex() const;

    // Methods from ::android::hardware::gnss::V2_0::IGnssConfiguration follow.
    Return<bool> setEsExtensionSec(uint32_t emergencyExtensionSeconds) override;

    // Methods from ::android::hardware::gnss::V2_1::IGnssConfiguration follow.
    Return<bool> setBlacklist_2_1(
            const hidl_vec<V2_1::IGnssConfiguration::BlacklistedSource>& blacklist) override;

    Return<bool> isBlacklistedV2_1(const V2_1::IGnssCallback::GnssSvInfo& gnssSvInfo) const;

  private:
    mutable std::recursive_mutex mMutex;

    BlacklistedSourceSetV2_1 mBlacklistedSourceSet;
    BlacklistedConstellationSetV2_1 mBlacklistedConstellationSet;
};

}  // namespace android::hardware::gnss::V2_1::implementation
