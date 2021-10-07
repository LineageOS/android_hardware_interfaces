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

#include "GnssCallbackEventQueue.h"
#include "Utils.h"
#include "v2_1/GnssCallback.h"

#include <gtest/gtest.h>
#include <chrono>

#define TIMEOUT_SEC 2  // for basic commands/responses

namespace android::hardware::gnss::common {

// The main test class for GNSS HAL.
template <class T_IGnss>
class GnssHalTestTemplate : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override;

    virtual void TearDown() override;

    /* Callback class for GnssMeasurement. */
    class GnssMeasurementCallback : public V2_1::IGnssMeasurementCallback {
      public:
        GnssCallbackEventQueue<V2_1::IGnssMeasurementCallback::GnssData> measurement_cbq_;

        GnssMeasurementCallback() : measurement_cbq_("measurement"){};
        virtual ~GnssMeasurementCallback() = default;

        // Methods from V1_0::IGnssMeasurementCallback follow.
        Return<void> GnssMeasurementCb(const V1_0::IGnssMeasurementCallback::GnssData&) override {
            return Void();
        }

        // Methods from V1_1::IGnssMeasurementCallback follow.
        Return<void> gnssMeasurementCb(const V1_1::IGnssMeasurementCallback::GnssData&) override {
            return Void();
        }

        // Methods from V2_0::IGnssMeasurementCallback follow.
        Return<void> gnssMeasurementCb_2_0(
                const V2_0::IGnssMeasurementCallback::GnssData&) override {
            return Void();
        }

        // Methods from V2_1::IGnssMeasurementCallback follow.
        Return<void> gnssMeasurementCb_2_1(
                const V2_1::IGnssMeasurementCallback::GnssData&) override;
    };

    /* Callback class for GnssMeasurementCorrections. */
    class GnssMeasurementCorrectionsCallback
        : public measurement_corrections::V1_0::IMeasurementCorrectionsCallback {
      public:
        uint32_t last_capabilities_;
        GnssCallbackEventQueue<uint32_t> capabilities_cbq_;

        GnssMeasurementCorrectionsCallback() : capabilities_cbq_("capabilities"){};
        virtual ~GnssMeasurementCorrectionsCallback() = default;

        // Methods from V1_0::IMeasurementCorrectionsCallback follow.
        Return<void> setCapabilitiesCb(uint32_t capabilities) override;
    };

    /* Callback class for GnssAntennaInfo. */
    class GnssAntennaInfoCallback : public V2_1::IGnssAntennaInfoCallback {
      public:
        GnssCallbackEventQueue<hidl_vec<V2_1::IGnssAntennaInfoCallback::GnssAntennaInfo>>
                antenna_info_cbq_;

        GnssAntennaInfoCallback() : antenna_info_cbq_("info"){};
        virtual ~GnssAntennaInfoCallback() = default;

        // Methods from V2_1::GnssAntennaInfoCallback follow.
        Return<void> gnssAntennaInfoCb(
                const hidl_vec<V2_1::IGnssAntennaInfoCallback::GnssAntennaInfo>& gnssAntennaInfos);
    };

    /*
     * SetUpGnssCallback:
     *   Set GnssCallback and verify the result.
     */
    virtual void SetUpGnssCallback();

    /*
     * StartAndCheckFirstLocation:
     *   Helper function to start location, and check the first one.
     *
     *   <p> Note this leaves the Location request active, to enable Stop call vs. other call
     *   reordering tests.
     *
     * returns  true if a location was successfully generated
     */
    bool StartAndCheckFirstLocation(const int min_interval_msec, const bool low_power_mode);

    /*
     * CheckLocation:
     *   Helper function to vet Location fields
     *
     *   check_speed: true if speed related fields are also verified.
     */
    void CheckLocation(const V2_0::GnssLocation& location, const bool check_speed);

    /*
     * StartAndCheckLocations:
     *   Helper function to collect, and check a number of
     *   normal ~1Hz locations.
     *
     *   Note this leaves the Location request active, to enable Stop call vs. other call
     *   reordering tests.
     */
    void StartAndCheckLocations(int count);

    /*
     * StopAndClearLocations:
     * Helper function to stop locations, and clear any remaining notifications
     */
    void StopAndClearLocations();

    /*
     * SetPositionMode:
     * Helper function to set positioning mode and verify output
     */
    void SetPositionMode(const int min_interval_msec, const bool low_power_mode);

    /*
     * startLocationAndGetNonGpsConstellation:
     * 1. Start location
     * 2. Find and return first non-GPS constellation
     *
     * Note that location is not stopped in this method. The client should call
     * StopAndClearLocations() after the call.
     */
    V2_0::GnssConstellationType startLocationAndGetNonGpsConstellation(
            const int locations_to_await, const int gnss_sv_info_list_timeout);

    sp<T_IGnss> gnss_hal_;      // GNSS HAL to call into
    sp<GnssCallback> gnss_cb_;  // Primary callback interface
};

using ::android::hardware::gnss::common::Utils;

// Implementations for the main test class for GNSS HAL
template <class T_IGnss>
void GnssHalTestTemplate<T_IGnss>::SetUp() {
    gnss_hal_ = T_IGnss::getService(GetParam());
    ASSERT_NE(gnss_hal_, nullptr);

    SetUpGnssCallback();
}

template <class T_IGnss>
void GnssHalTestTemplate<T_IGnss>::TearDown() {
    if (gnss_hal_ != nullptr) {
        gnss_hal_->cleanup();
        gnss_hal_ = nullptr;
    }

    // Set to nullptr to destruct the callback event queues and warn of any unprocessed events.
    gnss_cb_ = nullptr;
}

template <class T_IGnss>
void GnssHalTestTemplate<T_IGnss>::SetUpGnssCallback() {
    gnss_cb_ = new GnssCallback();
    ASSERT_NE(gnss_cb_, nullptr);

    auto result = gnss_hal_->setCallback_2_1(gnss_cb_);
    if (!result.isOk()) {
        ALOGE("result of failed setCallback %s", result.description().c_str());
    }

    ASSERT_TRUE(result.isOk());
    ASSERT_TRUE(result);

    /*
     * All capabilities, name and systemInfo callbacks should trigger
     */
    EXPECT_TRUE(gnss_cb_->capabilities_cbq_.retrieve(gnss_cb_->last_capabilities_, TIMEOUT_SEC));
    EXPECT_TRUE(gnss_cb_->info_cbq_.retrieve(gnss_cb_->last_info_, TIMEOUT_SEC));
    EXPECT_TRUE(gnss_cb_->name_cbq_.retrieve(gnss_cb_->last_name_, TIMEOUT_SEC));

    EXPECT_EQ(gnss_cb_->capabilities_cbq_.calledCount(), 1);
    EXPECT_EQ(gnss_cb_->info_cbq_.calledCount(), 1);
    EXPECT_EQ(gnss_cb_->name_cbq_.calledCount(), 1);
}

template <class T_IGnss>
void GnssHalTestTemplate<T_IGnss>::StopAndClearLocations() {
    const auto result = gnss_hal_->stop();

    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    /*
     * Clear notify/waiting counter, allowing up till the timeout after
     * the last reply for final startup messages to arrive (esp. system
     * info.)
     */
    while (gnss_cb_->location_cbq_.retrieve(gnss_cb_->last_location_, TIMEOUT_SEC)) {
    }
    gnss_cb_->location_cbq_.reset();
}

template <class T_IGnss>
void GnssHalTestTemplate<T_IGnss>::SetPositionMode(const int min_interval_msec,
                                                   const bool low_power_mode) {
    const int kPreferredAccuracy = 0;  // Ideally perfect (matches GnssLocationProvider)
    const int kPreferredTimeMsec = 0;  // Ideally immediate

    const auto result = gnss_hal_->setPositionMode_1_1(
            T_IGnss::GnssPositionMode::MS_BASED,
            T_IGnss::GnssPositionRecurrence::RECURRENCE_PERIODIC, min_interval_msec,
            kPreferredAccuracy, kPreferredTimeMsec, low_power_mode);

    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

template <class T_IGnss>
bool GnssHalTestTemplate<T_IGnss>::StartAndCheckFirstLocation(const int min_interval_msec,
                                                              const bool low_power_mode) {
    SetPositionMode(min_interval_msec, low_power_mode);
    const auto result = gnss_hal_->start();

    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    /*
     * GnssLocationProvider support of AGPS SUPL & XtraDownloader is not available in VTS,
     * so allow time to demodulate ephemeris over the air.
     */
    const int kFirstGnssLocationTimeoutSeconds = 75;

    EXPECT_TRUE(gnss_cb_->location_cbq_.retrieve(gnss_cb_->last_location_,
                                                 kFirstGnssLocationTimeoutSeconds));
    int locationCalledCount = gnss_cb_->location_cbq_.calledCount();
    EXPECT_EQ(locationCalledCount, 1);

    if (locationCalledCount > 0) {
        // don't require speed on first fix
        CheckLocation(gnss_cb_->last_location_, false);
        return true;
    }
    return false;
}

template <class T_IGnss>
void GnssHalTestTemplate<T_IGnss>::CheckLocation(const V2_0::GnssLocation& location,
                                                 bool check_speed) {
    const bool check_more_accuracies =
            (gnss_cb_->info_cbq_.calledCount() > 0 && gnss_cb_->last_info_.yearOfHw >= 2017);

    Utils::checkLocation(location.v1_0, check_speed, check_more_accuracies);
}

template <class T_IGnss>
void GnssHalTestTemplate<T_IGnss>::StartAndCheckLocations(int count) {
    const int kMinIntervalMsec = 500;
    const int kLocationTimeoutSubsequentSec = 2;
    const bool kLowPowerMode = false;

    EXPECT_TRUE(StartAndCheckFirstLocation(kMinIntervalMsec, kLowPowerMode));

    for (int i = 1; i < count; i++) {
        EXPECT_TRUE(gnss_cb_->location_cbq_.retrieve(gnss_cb_->last_location_,
                                                     kLocationTimeoutSubsequentSec));
        int locationCalledCount = gnss_cb_->location_cbq_.calledCount();
        EXPECT_EQ(locationCalledCount, i + 1);
        // Don't cause confusion by checking details if no location yet
        if (locationCalledCount > 0) {
            // Should be more than 1 location by now, but if not, still don't check first fix speed
            CheckLocation(gnss_cb_->last_location_, locationCalledCount > 1);
        }
    }
}

template <class T_IGnss>
V2_0::GnssConstellationType GnssHalTestTemplate<T_IGnss>::startLocationAndGetNonGpsConstellation(
        const int locations_to_await, const int gnss_sv_info_list_timeout) {
    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(locations_to_await);
    const int location_called_count = gnss_cb_->location_cbq_.calledCount();

    // Tolerate 1 less sv status to handle edge cases in reporting.
    int sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, locations_to_await);
    ALOGD("Observed %d GnssSvInfo, while awaiting %d Locations (%d received)",
          sv_info_list_cbq_size, locations_to_await, location_called_count);

    // Find first non-GPS constellation to blacklist
    V2_0::GnssConstellationType constellation_to_blacklist = V2_0::GnssConstellationType::UNKNOWN;
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        hidl_vec<V2_1::IGnssCallback::GnssSvInfo> sv_info_vec;
        gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec, gnss_sv_info_list_timeout);
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            const auto& gnss_sv = sv_info_vec[iSv];
            if ((gnss_sv.v2_0.v1_0.svFlag & V1_0::IGnssCallback::GnssSvFlags::USED_IN_FIX) &&
                (gnss_sv.v2_0.constellation != V2_0::GnssConstellationType::UNKNOWN) &&
                (gnss_sv.v2_0.constellation != V2_0::GnssConstellationType::GPS)) {
                // found a non-GPS constellation
                constellation_to_blacklist = gnss_sv.v2_0.constellation;
                break;
            }
        }
        if (constellation_to_blacklist != V2_0::GnssConstellationType::UNKNOWN) {
            break;
        }
    }

    if (constellation_to_blacklist == V2_0::GnssConstellationType::UNKNOWN) {
        ALOGI("No non-GPS constellations found, constellation blacklist test less effective.");
        // Proceed functionally to blacklist something.
        constellation_to_blacklist = V2_0::GnssConstellationType::GLONASS;
    }

    return constellation_to_blacklist;
}

template <class T_IGnss>
Return<void> GnssHalTestTemplate<T_IGnss>::GnssMeasurementCallback::gnssMeasurementCb_2_1(
        const V2_1::IGnssMeasurementCallback::GnssData& data) {
    ALOGD("GnssMeasurement v2.1 received. Size = %d", (int)data.measurements.size());
    measurement_cbq_.store(data);
    return Void();
}

template <class T_IGnss>
Return<void> GnssHalTestTemplate<T_IGnss>::GnssMeasurementCorrectionsCallback::setCapabilitiesCb(
        uint32_t capabilities) {
    ALOGI("GnssMeasurementCorrectionsCallback capabilities received %d", capabilities);
    capabilities_cbq_.store(capabilities);
    return Void();
}

template <class T_IGnss>
Return<void> GnssHalTestTemplate<T_IGnss>::GnssAntennaInfoCallback::gnssAntennaInfoCb(
        const hidl_vec<V2_1::IGnssAntennaInfoCallback::GnssAntennaInfo>& gnssAntennaInfos) {
    ALOGD("GnssAntennaInfo v2.1 received. Size = %d", (int)gnssAntennaInfos.size());
    antenna_info_cbq_.store(gnssAntennaInfos);
    return Void();
}

}  // namespace android::hardware::gnss::common
