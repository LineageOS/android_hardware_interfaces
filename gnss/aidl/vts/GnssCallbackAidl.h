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

#include <android/hardware/gnss/BnGnssCallback.h>
#include "GnssCallbackEventQueue.h"

/* Callback class for data & Event. */
class GnssCallbackAidl : public android::hardware::gnss::BnGnssCallback {
  public:
    GnssCallbackAidl()
        : capabilities_cbq_("capabilities"),
          info_cbq_("system_info"),
          location_cbq_("location"),
          sv_info_list_cbq_("sv_info"){};
    ~GnssCallbackAidl(){};

    android::binder::Status gnssSetCapabilitiesCb(const int capabilities) override;
    android::binder::Status gnssStatusCb(const GnssStatusValue status) override;
    android::binder::Status gnssSvStatusCb(const std::vector<GnssSvInfo>& svInfoList) override;
    android::binder::Status gnssLocationCb(
            const android::hardware::gnss::GnssLocation& location) override;
    android::binder::Status gnssNmeaCb(const int64_t timestamp, const std::string& nmea) override;
    android::binder::Status gnssAcquireWakelockCb() override;
    android::binder::Status gnssReleaseWakelockCb() override;
    android::binder::Status gnssSetSystemInfoCb(const GnssSystemInfo& info) override;
    android::binder::Status gnssRequestTimeCb() override;
    android::binder::Status gnssRequestLocationCb(const bool independentFromGnss,
                                                  const bool isUserEmergency) override;

    int last_capabilities_;
    android::hardware::gnss::IGnssCallback::GnssSystemInfo last_info_;
    android::hardware::gnss::GnssLocation last_location_;

    android::hardware::gnss::common::GnssCallbackEventQueue<int> capabilities_cbq_;
    android::hardware::gnss::common::GnssCallbackEventQueue<
            android::hardware::gnss::IGnssCallback::GnssSystemInfo>
            info_cbq_;
    android::hardware::gnss::common::GnssCallbackEventQueue<android::hardware::gnss::GnssLocation>
            location_cbq_;
    android::hardware::gnss::common::GnssCallbackEventQueue<
            std::vector<android::hardware::gnss::IGnssCallback::GnssSvInfo>>
            sv_info_list_cbq_;
};