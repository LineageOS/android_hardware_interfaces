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

#define LOG_TAG "GnssHidlHal"

#include "GnssHidlHal.h"

namespace aidl::android::hardware::gnss {

using GnssSvInfo = ::android::hardware::gnss::V2_1::IGnssCallback::GnssSvInfo;

GnssHidlHal::GnssHidlHal(const std::shared_ptr<Gnss>& gnssAidl) : mGnssAidl(gnssAidl) {
    Gnss* iGnss = mGnssAidl.get();
    std::shared_ptr<IGnssConfiguration> iGnssConfiguration;
    auto status = iGnss->getExtensionGnssConfiguration(&iGnssConfiguration);
    if (!status.isOk()) {
        ALOGE("Failed to getExtensionGnssConfiguration.");
    } else {
        mGnssConfigurationAidl = iGnss->mGnssConfiguration;
    }

    std::shared_ptr<IGnssPowerIndication> iGnssPowerIndication;
    status = iGnss->getExtensionGnssPowerIndication(&iGnssPowerIndication);
    if (!status.isOk()) {
        ALOGE("Failed to getExtensionGnssPowerIndication.");
    } else {
        mGnssPowerIndicationAidl = iGnss->mGnssPowerIndication;
    }
};

hidl_vec<GnssSvInfo> GnssHidlHal::filterBlocklistedSatellitesV2_1(
        hidl_vec<GnssSvInfo> gnssSvInfoList) {
    if (mGnssConfigurationAidl == nullptr) {
        ALOGE("Handle to AIDL GnssConfiguration is not available.");
        return gnssSvInfoList;
    }
    for (uint32_t i = 0; i < gnssSvInfoList.size(); i++) {
        if (mGnssConfigurationAidl->isBlocklistedV2_1(gnssSvInfoList[i])) {
            ALOGD("Blocklisted constellation: %d, svid: %d",
                  (int)gnssSvInfoList[i].v2_0.constellation, gnssSvInfoList[i].v2_0.v1_0.svid);
            gnssSvInfoList[i].v2_0.v1_0.svFlag &= ~static_cast<uint8_t>(
                    ::android::hardware::gnss::V1_0::IGnssCallback::GnssSvFlags::USED_IN_FIX);
        }
    }
    return gnssSvInfoList;
}

void GnssHidlHal::notePowerConsumption() {
    mGnssPowerIndicationAidl->notePowerConsumption();
}

}  // namespace aidl::android::hardware::gnss
