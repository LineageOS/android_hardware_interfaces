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

#include <android/hardware/gnss/2.1/IGnss.h>
#include "GnssCallbackEventQueue.h"

#include <gtest/gtest.h>

using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;

using android::hardware::gnss::common::GnssCallbackEventQueue;
using android::hardware::gnss::measurement_corrections::V1_0::IMeasurementCorrectionsCallback;
using android::hardware::gnss::V1_0::GnssLocationFlags;
using android::hardware::gnss::V2_0::GnssConstellationType;

using GnssLocation_1_0 = android::hardware::gnss::V1_0::GnssLocation;
using GnssLocation_2_0 = android::hardware::gnss::V2_0::GnssLocation;

using IGnssCallback_1_0 = android::hardware::gnss::V1_0::IGnssCallback;
using IGnssCallback_2_0 = android::hardware::gnss::V2_0::IGnssCallback;
using IGnssCallback_2_1 = android::hardware::gnss::V2_1::IGnssCallback;

using IGnssMeasurementCallback_1_0 = android::hardware::gnss::V1_0::IGnssMeasurementCallback;
using IGnssMeasurementCallback_1_1 = android::hardware::gnss::V1_1::IGnssMeasurementCallback;
using IGnssMeasurementCallback_2_0 = android::hardware::gnss::V2_0::IGnssMeasurementCallback;
using IGnssMeasurementCallback_2_1 = android::hardware::gnss::V2_1::IGnssMeasurementCallback;

using android::sp;

#define TIMEOUT_SEC 2  // for basic commands/responses

namespace android::hardware::gnss::common {

/* Callback class for data & Event. */
class GnssCallback : public IGnssCallback_2_1 {
  public:
    IGnssCallback_1_0::GnssSystemInfo last_info_;
    android::hardware::hidl_string last_name_;
    uint32_t last_capabilities_;
    GnssLocation_2_0 last_location_;

    GnssCallbackEventQueue<IGnssCallback_1_0::GnssSystemInfo> info_cbq_;
    GnssCallbackEventQueue<android::hardware::hidl_string> name_cbq_;
    GnssCallbackEventQueue<uint32_t> capabilities_cbq_;
    GnssCallbackEventQueue<GnssLocation_2_0> location_cbq_;
    GnssCallbackEventQueue<hidl_vec<IGnssCallback_2_1::GnssSvInfo>> sv_info_list_cbq_;

    GnssCallback();
    virtual ~GnssCallback() = default;

    // Dummy callback handlers
    Return<void> gnssStatusCb(const IGnssCallback_1_0::GnssStatusValue /* status */) override {
        return Void();
    }
    Return<void> gnssNmeaCb(int64_t /* timestamp */,
                            const android::hardware::hidl_string& /* nmea */) override {
        return Void();
    }
    Return<void> gnssAcquireWakelockCb() override { return Void(); }
    Return<void> gnssReleaseWakelockCb() override { return Void(); }
    Return<void> gnssRequestLocationCb(bool /* independentFromGnss */) override { return Void(); }
    Return<void> gnssRequestTimeCb() override { return Void(); }
    // Actual (test) callback handlers
    Return<void> gnssNameCb(const android::hardware::hidl_string& name) override;
    Return<void> gnssLocationCb(const GnssLocation_1_0& location) override;
    Return<void> gnssSetCapabilitesCb(uint32_t capabilities) override;
    Return<void> gnssSetSystemInfoCb(const IGnssCallback_1_0::GnssSystemInfo& info) override;
    Return<void> gnssSvStatusCb(const IGnssCallback_1_0::GnssSvStatus& svStatus) override;

    // New in v2.0
    Return<void> gnssLocationCb_2_0(const GnssLocation_2_0& location) override;
    Return<void> gnssRequestLocationCb_2_0(bool /* independentFromGnss */,
                                           bool /* isUserEmergency */) override {
        return Void();
    }
    Return<void> gnssSetCapabilitiesCb_2_0(uint32_t capabilities) override;
    Return<void> gnssSvStatusCb_2_0(const hidl_vec<IGnssCallback_2_0::GnssSvInfo>&) override {
        return Void();
    }

    // New in v2.1
    Return<void> gnssSvStatusCb_2_1(
            const hidl_vec<IGnssCallback_2_1::GnssSvInfo>& svInfoList) override;
    Return<void> gnssSetCapabilitiesCb_2_1(uint32_t capabilities) override;

  private:
    Return<void> gnssLocationCbImpl(const GnssLocation_2_0& location);
};

}  // namespace android::hardware::gnss::common
