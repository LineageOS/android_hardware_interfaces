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

#define LOG_TAG "GnssHalTest"

#include "gnss_hal_test.h"
#include <hidl/ServiceManagement.h>
#include "Utils.h"

using android::hardware::gnss::GnssConstellationType;
using android::hardware::gnss::GnssLocation;
using android::hardware::gnss::IGnss;
using android::hardware::gnss::IGnssCallback;
using android::hardware::gnss::common::Utils;
using GnssConstellationTypeV2_0 = android::hardware::gnss::V2_0::GnssConstellationType;

void GnssHalTest::SetUp() {
    // Get AIDL handle
    aidl_gnss_hal_ = android::waitForDeclaredService<IGnssAidl>(String16(GetParam().c_str()));
    ASSERT_NE(aidl_gnss_hal_, nullptr);
    ALOGD("AIDL Interface Version = %d", aidl_gnss_hal_->getInterfaceVersion());

    if (aidl_gnss_hal_->getInterfaceVersion() == 1) {
        const auto& hidlInstanceNames = android::hardware::getAllHalInstanceNames(
                android::hardware::gnss::V2_1::IGnss::descriptor);
        gnss_hal_ = IGnss_V2_1::getService(hidlInstanceNames[0]);
        ASSERT_NE(gnss_hal_, nullptr);
    }

    SetUpGnssCallback();
}

void GnssHalTest::SetUpGnssCallback() {
    aidl_gnss_cb_ = new GnssCallbackAidl();
    ASSERT_NE(aidl_gnss_cb_, nullptr);

    auto status = aidl_gnss_hal_->setCallback(aidl_gnss_cb_);
    if (!status.isOk()) {
        ALOGE("Failed to setCallback");
    }
    ASSERT_TRUE(status.isOk());

    /*
     * Capabilities callback should trigger.
     */
    EXPECT_TRUE(aidl_gnss_cb_->capabilities_cbq_.retrieve(aidl_gnss_cb_->last_capabilities_,
                                                          TIMEOUT_SEC));
    EXPECT_EQ(aidl_gnss_cb_->capabilities_cbq_.calledCount(), 1);

    if (aidl_gnss_hal_->getInterfaceVersion() == 1) {
        // Invoke the super method.
        GnssHalTestTemplate<IGnss_V2_1>::SetUpGnssCallback();
    }
}

void GnssHalTest::CheckLocation(const GnssLocation& location, bool check_speed) {
    Utils::checkLocation(location, check_speed, /* check_more_accuracies= */ true);
}

void GnssHalTest::SetPositionMode(const int min_interval_msec, const bool low_power_mode) {
    if (aidl_gnss_hal_->getInterfaceVersion() == 1) {
        // Invoke the super method.
        return GnssHalTestTemplate<IGnss_V2_1>::SetPositionMode(min_interval_msec, low_power_mode);
    }

    const int kPreferredAccuracy = 0;  // Ideally perfect (matches GnssLocationProvider)
    const int kPreferredTimeMsec = 0;  // Ideally immediate

    IGnss::PositionModeOptions options;
    options.mode = IGnss::GnssPositionMode::MS_BASED;
    options.recurrence = IGnss::GnssPositionRecurrence::RECURRENCE_PERIODIC;
    options.minIntervalMs = min_interval_msec;
    options.preferredAccuracyMeters = kPreferredAccuracy;
    options.preferredTimeMs = kPreferredTimeMsec;
    options.lowPowerMode = low_power_mode;
    auto status = aidl_gnss_hal_->setPositionMode(options);

    ASSERT_TRUE(status.isOk());
}

bool GnssHalTest::StartAndCheckFirstLocation(const int min_interval_msec,
                                             const bool low_power_mode) {
    if (aidl_gnss_hal_->getInterfaceVersion() == 1) {
        // Invoke the super method.
        return GnssHalTestTemplate<IGnss_V2_1>::StartAndCheckFirstLocation(min_interval_msec,
                                                                           low_power_mode);
    }

    SetPositionMode(min_interval_msec, low_power_mode);
    auto status = aidl_gnss_hal_->start();
    EXPECT_TRUE(status.isOk());

    status = aidl_gnss_hal_->startSvStatus();
    EXPECT_TRUE(status.isOk());

    /*
     * GnssLocationProvider support of AGPS SUPL & XtraDownloader is not available in VTS,
     * so allow time to demodulate ephemeris over the air.
     */
    const int kFirstGnssLocationTimeoutSeconds = 75;

    EXPECT_TRUE(aidl_gnss_cb_->location_cbq_.retrieve(aidl_gnss_cb_->last_location_,
                                                      kFirstGnssLocationTimeoutSeconds));
    int locationCalledCount = aidl_gnss_cb_->location_cbq_.calledCount();
    EXPECT_EQ(locationCalledCount, 1);

    if (locationCalledCount > 0) {
        // don't require speed on first fix
        CheckLocation(aidl_gnss_cb_->last_location_, false);
        return true;
    }
    return false;
}

void GnssHalTest::StopAndClearLocations() {
    ALOGD("StopAndClearLocations");
    if (aidl_gnss_hal_->getInterfaceVersion() == 1) {
        // Invoke the super method.
        return GnssHalTestTemplate<IGnss_V2_1>::StopAndClearLocations();
    }
    auto status = aidl_gnss_hal_->stopSvStatus();
    EXPECT_TRUE(status.isOk());

    status = aidl_gnss_hal_->stop();
    EXPECT_TRUE(status.isOk());

    /*
     * Clear notify/waiting counter, allowing up till the timeout after
     * the last reply for final startup messages to arrive (esp. system
     * info.)
     */
    while (aidl_gnss_cb_->location_cbq_.retrieve(aidl_gnss_cb_->last_location_, TIMEOUT_SEC)) {
    }
    aidl_gnss_cb_->location_cbq_.reset();
}

void GnssHalTest::StartAndCheckLocations(int count) {
    if (aidl_gnss_hal_->getInterfaceVersion() == 1) {
        // Invoke the super method.
        return GnssHalTestTemplate<IGnss_V2_1>::StartAndCheckLocations(count);
    }
    const int kMinIntervalMsec = 500;
    const int kLocationTimeoutSubsequentSec = 2;
    const bool kLowPowerMode = false;

    EXPECT_TRUE(StartAndCheckFirstLocation(kMinIntervalMsec, kLowPowerMode));

    for (int i = 1; i < count; i++) {
        EXPECT_TRUE(aidl_gnss_cb_->location_cbq_.retrieve(aidl_gnss_cb_->last_location_,
                                                          kLocationTimeoutSubsequentSec));
        int locationCalledCount = aidl_gnss_cb_->location_cbq_.calledCount();
        EXPECT_EQ(locationCalledCount, i + 1);
        // Don't cause confusion by checking details if no location yet
        if (locationCalledCount > 0) {
            // Should be more than 1 location by now, but if not, still don't check first fix speed
            CheckLocation(aidl_gnss_cb_->last_location_, locationCalledCount > 1);
        }
    }
}

std::list<std::vector<IGnssCallback::GnssSvInfo>> GnssHalTest::convertToAidl(
        const std::list<hidl_vec<IGnssCallback_2_1::GnssSvInfo>>& sv_info_list) {
    std::list<std::vector<IGnssCallback::GnssSvInfo>> aidl_sv_info_list;
    for (const auto& sv_info_vec : sv_info_list) {
        std::vector<IGnssCallback::GnssSvInfo> aidl_sv_info_vec;
        for (const auto& sv_info : sv_info_vec) {
            IGnssCallback::GnssSvInfo aidl_sv_info;
            aidl_sv_info.svid = sv_info.v2_0.v1_0.svid;
            aidl_sv_info.constellation =
                    static_cast<GnssConstellationType>(sv_info.v2_0.constellation);
            aidl_sv_info.cN0Dbhz = sv_info.v2_0.v1_0.cN0Dbhz;
            aidl_sv_info.basebandCN0DbHz = sv_info.basebandCN0DbHz;
            aidl_sv_info.elevationDegrees = sv_info.v2_0.v1_0.elevationDegrees;
            aidl_sv_info.azimuthDegrees = sv_info.v2_0.v1_0.azimuthDegrees;
            aidl_sv_info.carrierFrequencyHz = (int64_t)sv_info.v2_0.v1_0.carrierFrequencyHz;
            aidl_sv_info.svFlag = (int)sv_info.v2_0.v1_0.svFlag;
            aidl_sv_info_vec.push_back(aidl_sv_info);
        }
        aidl_sv_info_list.push_back(aidl_sv_info_vec);
    }
    return aidl_sv_info_list;
}

/*
 * FindStrongFrequentNonGpsSource:
 *
 * Search through a GnssSvStatus list for the strongest non-GPS satellite observed enough times
 *
 * returns the strongest source,
 *         or a source with constellation == UNKNOWN if none are found sufficient times
 */
BlocklistedSource GnssHalTest::FindStrongFrequentNonGpsSource(
        const std::list<hidl_vec<IGnssCallback_2_1::GnssSvInfo>> sv_info_list,
        const int min_observations) {
    return FindStrongFrequentNonGpsSource(convertToAidl(sv_info_list), min_observations);
}

BlocklistedSource GnssHalTest::FindStrongFrequentNonGpsSource(
        const std::list<std::vector<IGnssCallback::GnssSvInfo>> sv_info_list,
        const int min_observations) {
    std::map<ComparableBlocklistedSource, SignalCounts> mapSignals;

    for (const auto& sv_info_vec : sv_info_list) {
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            const auto& gnss_sv = sv_info_vec[iSv];
            if ((gnss_sv.svFlag & (int)IGnssCallback::GnssSvFlags::USED_IN_FIX) &&
                (gnss_sv.constellation != GnssConstellationType::GPS)) {
                ComparableBlocklistedSource source;
                source.id.svid = gnss_sv.svid;
                source.id.constellation = gnss_sv.constellation;

                const auto& itSignal = mapSignals.find(source);
                if (itSignal == mapSignals.end()) {
                    SignalCounts counts;
                    counts.observations = 1;
                    counts.max_cn0_dbhz = gnss_sv.cN0Dbhz;
                    mapSignals.insert(
                            std::pair<ComparableBlocklistedSource, SignalCounts>(source, counts));
                } else {
                    itSignal->second.observations++;
                    if (itSignal->second.max_cn0_dbhz < gnss_sv.cN0Dbhz) {
                        itSignal->second.max_cn0_dbhz = gnss_sv.cN0Dbhz;
                    }
                }
            }
        }
    }

    float max_cn0_dbhz_with_sufficient_count = 0.;
    int total_observation_count = 0;
    int blocklisted_source_count_observation = 0;

    ComparableBlocklistedSource source_to_blocklist;  // initializes to zero = UNKNOWN constellation
    for (auto const& pairSignal : mapSignals) {
        total_observation_count += pairSignal.second.observations;
        if ((pairSignal.second.observations >= min_observations) &&
            (pairSignal.second.max_cn0_dbhz > max_cn0_dbhz_with_sufficient_count)) {
            source_to_blocklist = pairSignal.first;
            blocklisted_source_count_observation = pairSignal.second.observations;
            max_cn0_dbhz_with_sufficient_count = pairSignal.second.max_cn0_dbhz;
        }
    }
    ALOGD("Among %d observations, chose svid %d, constellation %d, "
          "with %d observations at %.1f max CNo",
          total_observation_count, source_to_blocklist.id.svid,
          (int)source_to_blocklist.id.constellation, blocklisted_source_count_observation,
          max_cn0_dbhz_with_sufficient_count);

    return source_to_blocklist.id;
}

GnssConstellationType GnssHalTest::startLocationAndGetNonGpsConstellation(
        const int locations_to_await, const int gnss_sv_info_list_timeout) {
    if (aidl_gnss_hal_->getInterfaceVersion() == 1) {
        return static_cast<GnssConstellationType>(
                GnssHalTestTemplate<IGnss_V2_1>::startLocationAndGetNonGpsConstellation(
                        locations_to_await, gnss_sv_info_list_timeout));
    }
    aidl_gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(locations_to_await);
    const int location_called_count = aidl_gnss_cb_->location_cbq_.calledCount();

    // Tolerate 1 less sv status to handle edge cases in reporting.
    int sv_info_list_cbq_size = aidl_gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, locations_to_await);
    ALOGD("Observed %d GnssSvInfo, while awaiting %d Locations (%d received)",
          sv_info_list_cbq_size, locations_to_await, location_called_count);

    // Find first non-GPS constellation to blocklist
    GnssConstellationType constellation_to_blocklist = GnssConstellationType::UNKNOWN;
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        std::vector<IGnssCallback::GnssSvInfo> sv_info_vec;
        aidl_gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec, gnss_sv_info_list_timeout);
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            auto& gnss_sv = sv_info_vec[iSv];
            if ((gnss_sv.svFlag & (uint32_t)IGnssCallback::GnssSvFlags::USED_IN_FIX) &&
                (gnss_sv.constellation != GnssConstellationType::UNKNOWN) &&
                (gnss_sv.constellation != GnssConstellationType::GPS)) {
                // found a non-GPS constellation
                constellation_to_blocklist = gnss_sv.constellation;
                break;
            }
        }
        if (constellation_to_blocklist != GnssConstellationType::UNKNOWN) {
            break;
        }
    }

    if (constellation_to_blocklist == GnssConstellationType::UNKNOWN) {
        ALOGI("No non-GPS constellations found, constellation blocklist test less effective.");
        // Proceed functionally to blocklist something.
        constellation_to_blocklist = GnssConstellationType::GLONASS;
    }

    return constellation_to_blocklist;
}
