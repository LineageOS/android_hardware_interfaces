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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android/hardware/gnss/IGnss.h>
#include <binder/IServiceManager.h>

#include <android/hardware/gnss/2.1/IGnss.h>
#include "GnssBatchingCallback.h"
#include "GnssCallbackAidl.h"
#include "GnssMeasurementCallbackAidl.h"
#include "v2_1/gnss_hal_test_template.h"

using IGnss_V2_1 = android::hardware::gnss::V2_1::IGnss;

using android::ProcessState;
using android::sp;
using android::String16;
using IGnssAidl = android::hardware::gnss::IGnss;
using android::hardware::gnss::BlocklistedSource;
using android::hardware::gnss::IGnssConfiguration;

// The main test class for GNSS HAL.
class GnssHalTest : public android::hardware::gnss::common::GnssHalTestTemplate<IGnss_V2_1> {
  public:
    GnssHalTest(){};
    ~GnssHalTest(){};

    struct ComparableBlocklistedSource {
        android::hardware::gnss::BlocklistedSource id;

        ComparableBlocklistedSource() {
            id.constellation = android::hardware::gnss::GnssConstellationType::UNKNOWN;
            id.svid = 0;
        }

        bool operator<(const ComparableBlocklistedSource& compare) const {
            return ((id.svid < compare.id.svid) || ((id.svid == compare.id.svid) &&
                                                    (id.constellation < compare.id.constellation)));
        }
    };

    struct SignalCounts {
        int observations;
        float max_cn0_dbhz;
    };

    virtual void SetUp() override;
    virtual void SetUpGnssCallback() override;
    virtual void TearDown() override;

    void CheckLocation(const android::hardware::gnss::GnssLocation& location,
                       const bool check_speed);
    void SetPositionMode(const int min_interval_msec, const bool low_power_mode);
    bool StartAndCheckFirstLocation(const int min_interval_msec, const bool low_power_mode);
    bool StartAndCheckFirstLocation(const int min_interval_msec, const bool low_power_mode,
                                    const bool start_sv_status, const bool start_nmea);
    void StopAndClearLocations();
    void StartAndCheckLocations(const int count);
    void StartAndCheckLocations(const int count, const bool start_sv_status, const bool start_nmea);

    android::hardware::gnss::GnssConstellationType startLocationAndGetNonGpsConstellation(
            const int locations_to_await, const int gnss_sv_info_list_timeout);
    std::list<std::vector<android::hardware::gnss::IGnssCallback::GnssSvInfo>> convertToAidl(
            const std::list<hidl_vec<android::hardware::gnss::V2_1::IGnssCallback::GnssSvInfo>>&
                    sv_info_list);
    android::hardware::gnss::BlocklistedSource FindStrongFrequentNonGpsSource(
            const std::list<hidl_vec<android::hardware::gnss::V2_1::IGnssCallback::GnssSvInfo>>
                    sv_info_list,
            const int min_observations);
    android::hardware::gnss::BlocklistedSource FindStrongFrequentNonGpsSource(
            const std::list<std::vector<android::hardware::gnss::IGnssCallback::GnssSvInfo>>
                    sv_info_list,
            const int min_observations);

    void checkGnssMeasurementClockFields(const android::hardware::gnss::GnssData& measurement);
    void checkGnssMeasurementFlags(const android::hardware::gnss::GnssMeasurement& measurement);
    void checkGnssMeasurementFields(const android::hardware::gnss::GnssMeasurement& measurement,
                                    const android::hardware::gnss::GnssData& data);
    void startMeasurementWithInterval(
            int intervalMillis,
            const sp<android::hardware::gnss::IGnssMeasurementInterface>& iMeasurement,
            sp<GnssMeasurementCallbackAidl>& callback);
    void collectMeasurementIntervals(const sp<GnssMeasurementCallbackAidl>& callback,
                                     const int numMeasurementEvents, const int timeoutSeconds,
                                     std::vector<int>& deltaMs);
    void assertMeanAndStdev(int intervalMillis, std::vector<int>& deltasMillis);

    sp<IGnssAidl> aidl_gnss_hal_;
    sp<GnssCallbackAidl> aidl_gnss_cb_;  // Primary callback interface
};
