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

using ::android::hardware::gnss::common::implementation::GnssTemplate;
using GnssSvInfo = ::android::hardware::gnss::V2_1::IGnssCallback::GnssSvInfo;

class GnssHidlHal : public GnssTemplate<::android::hardware::gnss::V2_1::IGnss> {
  public:
    GnssHidlHal(const std::shared_ptr<Gnss>& gnssAidl);

  private:
    hidl_vec<GnssSvInfo> filterBlocklistedSatellitesV2_1(
            hidl_vec<GnssSvInfo> gnssSvInfoList) override;

    std::shared_ptr<Gnss> mGnssAidl;
    std::shared_ptr<GnssConfiguration> mGnssConfigurationAidl;
};

}  // namespace aidl::android::hardware::gnss
