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

#define LOG_TAG "GnssHalTest"

#include <gnss_hal_test.h>
#include <chrono>
#include "Utils.h"

#include <gtest/gtest.h>

using ::android::hardware::gnss::common::Utils;

// Implementations for the main test class for GNSS HAL
void GnssHalTest::SetUp() {
    gnss_hal_ = IGnss::getService(GetParam());
    ASSERT_NE(gnss_hal_, nullptr);

    SetUpGnssCallback();
}

void GnssHalTest::TearDown() {
    if (gnss_hal_ != nullptr) {
        gnss_hal_->cleanup();
        gnss_hal_ = nullptr;
    }

    // Set to nullptr to destruct the callback event queues and warn of any unprocessed events.
    gnss_cb_ = nullptr;
}

void GnssHalTest::SetUpGnssCallback() {
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

void GnssHalTest::StopAndClearLocations() {
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

void GnssHalTest::SetPositionMode(const int min_interval_msec, const bool low_power_mode) {
    const int kPreferredAccuracy = 0;  // Ideally perfect (matches GnssLocationProvider)
    const int kPreferredTimeMsec = 0;  // Ideally immediate

    const auto result = gnss_hal_->setPositionMode_1_1(
            IGnss::GnssPositionMode::MS_BASED, IGnss::GnssPositionRecurrence::RECURRENCE_PERIODIC,
            min_interval_msec, kPreferredAccuracy, kPreferredTimeMsec, low_power_mode);

    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

bool GnssHalTest::StartAndCheckFirstLocation() {
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

void GnssHalTest::CheckLocation(const GnssLocation_2_0& location, bool check_speed) {
    const bool check_more_accuracies =
            (gnss_cb_->info_cbq_.calledCount() > 0 && gnss_cb_->last_info_.yearOfHw >= 2017);

    Utils::checkLocation(location.v1_0, check_speed, check_more_accuracies);
}

void GnssHalTest::StartAndCheckLocations(int count) {
    const int kMinIntervalMsec = 500;
    const int kLocationTimeoutSubsequentSec = 2;
    const bool kLowPowerMode = false;

    SetPositionMode(kMinIntervalMsec, kLowPowerMode);

    EXPECT_TRUE(StartAndCheckFirstLocation());

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

GnssConstellationType GnssHalTest::startLocationAndGetNonGpsConstellation(
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
    GnssConstellationType constellation_to_blacklist = GnssConstellationType::UNKNOWN;
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        hidl_vec<IGnssCallback_2_1::GnssSvInfo> sv_info_vec;
        gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec, gnss_sv_info_list_timeout);
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            const auto& gnss_sv = sv_info_vec[iSv];
            if ((gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX) &&
                (gnss_sv.v2_0.constellation != GnssConstellationType::UNKNOWN) &&
                (gnss_sv.v2_0.constellation != GnssConstellationType::GPS)) {
                // found a non-GPS constellation
                constellation_to_blacklist = gnss_sv.v2_0.constellation;
                break;
            }
        }
        if (constellation_to_blacklist != GnssConstellationType::UNKNOWN) {
            break;
        }
    }

    if (constellation_to_blacklist == GnssConstellationType::UNKNOWN) {
        ALOGI("No non-GPS constellations found, constellation blacklist test less effective.");
        // Proceed functionally to blacklist something.
        constellation_to_blacklist = GnssConstellationType::GLONASS;
    }

    return constellation_to_blacklist;
}

GnssHalTest::GnssCallback::GnssCallback()
    : info_cbq_("system_info"),
      name_cbq_("name"),
      capabilities_cbq_("capabilities"),
      location_cbq_("location"),
      sv_info_list_cbq_("sv_info") {}

Return<void> GnssHalTest::GnssCallback::gnssSetSystemInfoCb(
        const IGnssCallback_1_0::GnssSystemInfo& info) {
    ALOGI("Info received, year %d", info.yearOfHw);
    info_cbq_.store(info);
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssSetCapabilitesCb(uint32_t capabilities) {
    ALOGI("Capabilities received %d", capabilities);
    capabilities_cbq_.store(capabilities);
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssSetCapabilitiesCb_2_0(uint32_t capabilities) {
    ALOGI("Capabilities (v2.0) received %d", capabilities);
    capabilities_cbq_.store(capabilities);
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssNameCb(const android::hardware::hidl_string& name) {
    ALOGI("Name received: %s", name.c_str());
    name_cbq_.store(name);
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssLocationCb(const GnssLocation_1_0& location) {
    ALOGI("Location received");
    GnssLocation_2_0 location_v2_0;
    location_v2_0.v1_0 = location;
    return gnssLocationCbImpl(location_v2_0);
}

Return<void> GnssHalTest::GnssCallback::gnssLocationCb_2_0(const GnssLocation_2_0& location) {
    ALOGI("Location (v2.0) received");
    return gnssLocationCbImpl(location);
}

Return<void> GnssHalTest::GnssCallback::gnssLocationCbImpl(const GnssLocation_2_0& location) {
    location_cbq_.store(location);
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssSvStatusCb(const IGnssCallback_1_0::GnssSvStatus&) {
    ALOGI("gnssSvStatusCb");
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssSvStatusCb_2_1(
        const hidl_vec<IGnssCallback_2_1::GnssSvInfo>& svInfoList) {
    ALOGI("gnssSvStatusCb_2_1. Size = %d", (int)svInfoList.size());
    sv_info_list_cbq_.store(svInfoList);
    return Void();
}

Return<void> GnssHalTest::GnssMeasurementCallback::gnssMeasurementCb_2_1(
        const IGnssMeasurementCallback_2_1::GnssData& data) {
    ALOGD("GnssMeasurement v2.1 received. Size = %d", (int)data.measurements.size());
    measurement_cbq_.store(data);
    return Void();
}

Return<void> GnssHalTest::GnssMeasurementCorrectionsCallback::setCapabilitiesCb(
        uint32_t capabilities) {
    ALOGI("GnssMeasurementCorrectionsCallback capabilities received %d", capabilities);
    capabilities_cbq_.store(capabilities);
    return Void();
}