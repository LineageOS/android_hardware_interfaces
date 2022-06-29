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

#include <android/hardware/gnss/BnGnssAntennaInfoCallback.h>
#include <vector>
#include "GnssCallbackEventQueue.h"

/** Implementation for IGnssAntennaInfoCallback. */
class GnssAntennaInfoCallbackAidl : public android::hardware::gnss::BnGnssAntennaInfoCallback {
  public:
    GnssAntennaInfoCallbackAidl() : antenna_info_cbq_("info"){};
    ~GnssAntennaInfoCallbackAidl(){};

    android::binder::Status gnssAntennaInfoCb(
            const std::vector<IGnssAntennaInfoCallback::GnssAntennaInfo>& gnssAntennaInfos)
            override;

    android::hardware::gnss::common::GnssCallbackEventQueue<
            std::vector<IGnssAntennaInfoCallback::GnssAntennaInfo>>
            antenna_info_cbq_;
};
