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

#include "Gnss.h"
#include "GnssConfiguration.h"
#include "v2_1/GnssTemplate.h"

namespace aidl::android::hardware::gnss {

class GnssHidlHal : public ::android::hardware::gnss::common::implementation::GnssTemplate<
                            ::android::hardware::gnss::V2_1::IGnss> {
  public:
    GnssHidlHal(const std::shared_ptr<Gnss>& gnssAidl);

  private:
    hidl_vec<::android::hardware::gnss::V2_1::IGnssCallback::GnssSvInfo>
    filterBlocklistedSatellitesV2_1(
            hidl_vec<::android::hardware::gnss::V2_1::IGnssCallback::GnssSvInfo> gnssSvInfoList)
            override;
    void notePowerConsumption() override;

    std::shared_ptr<Gnss> mGnssAidl;
    std::shared_ptr<GnssConfiguration> mGnssConfigurationAidl;
    std::shared_ptr<GnssPowerIndication> mGnssPowerIndicationAidl;
};

}  // namespace aidl::android::hardware::gnss
