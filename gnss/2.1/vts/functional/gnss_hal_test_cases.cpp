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

#define LOG_TAG "GnssHalTestCases"

#include <gnss_hal_test.h>
#include <cmath>
#include "Utils.h"

#include <gtest/gtest.h>

using android::hardware::hidl_string;
using android::hardware::hidl_vec;

using android::hardware::gnss::common::Utils;

using IGnssMeasurement_2_1 = android::hardware::gnss::V2_1::IGnssMeasurement;
using IGnssMeasurement_2_0 = android::hardware::gnss::V2_0::IGnssMeasurement;
using IGnssMeasurement_1_1 = android::hardware::gnss::V1_1::IGnssMeasurement;
using IGnssMeasurement_1_0 = android::hardware::gnss::V1_0::IGnssMeasurement;

using IGnssConfiguration_2_1 = android::hardware::gnss::V2_1::IGnssConfiguration;
using IGnssConfiguration_2_0 = android::hardware::gnss::V2_0::IGnssConfiguration;
using IGnssConfiguration_1_1 = android::hardware::gnss::V1_1::IGnssConfiguration;
using IGnssConfiguration_1_0 = android::hardware::gnss::V1_0::IGnssConfiguration;

using android::hardware::gnss::V2_0::GnssConstellationType;
using android::hardware::gnss::V2_1::IGnssConfiguration;

using GnssMeasurementFlags = IGnssMeasurementCallback_2_1::GnssMeasurementFlags;
using IMeasurementCorrections_1_1 =
        android::hardware::gnss::measurement_corrections::V1_1::IMeasurementCorrections;

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_P(GnssHalTest, SetupTeardownCreateCleanup) {}

/*
 * TestGnssMeasurementExtension:
 * Gets the GnssMeasurementExtension and verifies that it returns an actual extension.
 */
TEST_P(GnssHalTest, TestGnssMeasurementExtension) {
    auto gnssMeasurement_2_1 = gnss_hal_->getExtensionGnssMeasurement_2_1();
    auto gnssMeasurement_2_0 = gnss_hal_->getExtensionGnssMeasurement_2_0();
    auto gnssMeasurement_1_1 = gnss_hal_->getExtensionGnssMeasurement_1_1();
    auto gnssMeasurement_1_0 = gnss_hal_->getExtensionGnssMeasurement();
    ASSERT_TRUE(gnssMeasurement_2_1.isOk() && gnssMeasurement_2_0.isOk() &&
                gnssMeasurement_1_1.isOk() && gnssMeasurement_1_0.isOk());
    sp<IGnssMeasurement_2_1> iGnssMeas_2_1 = gnssMeasurement_2_1;
    sp<IGnssMeasurement_2_0> iGnssMeas_2_0 = gnssMeasurement_2_0;
    sp<IGnssMeasurement_1_1> iGnssMeas_1_1 = gnssMeasurement_1_1;
    sp<IGnssMeasurement_1_0> iGnssMeas_1_0 = gnssMeasurement_1_0;
    // At least one interface is non-null.
    int numNonNull = (int)(iGnssMeas_2_1 != nullptr) + (int)(iGnssMeas_2_0 != nullptr) +
                     (int)(iGnssMeas_1_1 != nullptr) + (int)(iGnssMeas_1_0 != nullptr);
    ASSERT_TRUE(numNonNull >= 1);
}

/*
 * TestGnssConfigurationExtension:
 * Gets the GnssConfigurationExtension and verifies that it returns an actual extension.
 */
TEST_P(GnssHalTest, TestGnssConfigurationExtension) {
    auto gnssConfiguration_2_1 = gnss_hal_->getExtensionGnssConfiguration_2_1();
    auto gnssConfiguration_2_0 = gnss_hal_->getExtensionGnssConfiguration_2_0();
    auto gnssConfiguration_1_1 = gnss_hal_->getExtensionGnssConfiguration_1_1();
    auto gnssConfiguration_1_0 = gnss_hal_->getExtensionGnssConfiguration();
    ASSERT_TRUE(gnssConfiguration_2_1.isOk() && gnssConfiguration_2_0.isOk() &&
                gnssConfiguration_1_1.isOk() && gnssConfiguration_1_0.isOk());
    sp<IGnssConfiguration_2_1> iGnssConfig_2_1 = gnssConfiguration_2_1;
    sp<IGnssConfiguration_2_0> iGnssConfig_2_0 = gnssConfiguration_2_0;
    sp<IGnssConfiguration_1_1> iGnssConfig_1_1 = gnssConfiguration_1_1;
    sp<IGnssConfiguration_1_0> iGnssConfig_1_0 = gnssConfiguration_1_0;
    // At least one interface is non-null.
    int numNonNull = (int)(iGnssConfig_2_1 != nullptr) + (int)(iGnssConfig_2_0 != nullptr) +
                     (int)(iGnssConfig_1_1 != nullptr) + (int)(iGnssConfig_1_0 != nullptr);
    ASSERT_TRUE(numNonNull >= 1);
}

/*
 * TestGnssMeasurementFields:
 * Sets a GnssMeasurementCallback, waits for a measurement, and verifies
 * 1. basebandCN0DbHz is valid
 * 2. ISB fields are valid if HAS_INTER_SIGNAL_BIAS is true.
 */
TEST_P(GnssHalTest, TestGnssMeasurementFields) {
    const int kFirstGnssMeasurementTimeoutSeconds = 10;

    auto gnssMeasurement = gnss_hal_->getExtensionGnssMeasurement_2_1();
    ASSERT_TRUE(gnssMeasurement.isOk());

    // Skip test if GnssMeasurement v2.1 is not supported
    sp<IGnssMeasurement_2_1> iGnssMeasurement = gnssMeasurement;
    if (iGnssMeasurement == nullptr) {
        return;
    }

    sp<GnssMeasurementCallback> callback = new GnssMeasurementCallback();
    auto result = iGnssMeasurement->setCallback_2_1(callback, /* enableFullTracking= */ true);
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result, IGnssMeasurement_1_0::GnssMeasurementStatus::SUCCESS);

    IGnssMeasurementCallback_2_1::GnssData lastMeasurement;
    ASSERT_TRUE(callback->measurement_cbq_.retrieve(lastMeasurement,
                                                    kFirstGnssMeasurementTimeoutSeconds));
    EXPECT_EQ(callback->measurement_cbq_.calledCount(), 1);
    ASSERT_TRUE(lastMeasurement.measurements.size() > 0);
    for (auto measurement : lastMeasurement.measurements) {
        // Verify basebandCn0DbHz is valid.
        ASSERT_TRUE(measurement.basebandCN0DbHz > 0.0 && measurement.basebandCN0DbHz <= 65.0);

        if (((uint32_t)(measurement.flags & GnssMeasurementFlags::HAS_RECEIVER_ISB) > 0) &&
            ((uint32_t)(measurement.flags & GnssMeasurementFlags::HAS_RECEIVER_ISB_UNCERTAINTY) >
             0) &&
            ((uint32_t)(measurement.flags & GnssMeasurementFlags::HAS_SATELLITE_ISB) > 0) &&
            ((uint32_t)(measurement.flags & GnssMeasurementFlags::HAS_SATELLITE_ISB_UNCERTAINTY) >
             0)) {
            GnssConstellationType referenceConstellation =
                    lastMeasurement.clock.referenceSignalTypeForIsb.constellation;
            double carrierFrequencyHz =
                    lastMeasurement.clock.referenceSignalTypeForIsb.carrierFrequencyHz;
            std::string codeType = lastMeasurement.clock.referenceSignalTypeForIsb.codeType;

            ASSERT_TRUE(referenceConstellation >= GnssConstellationType::UNKNOWN &&
                        referenceConstellation >= GnssConstellationType::IRNSS);
            ASSERT_TRUE(carrierFrequencyHz > 0);
            ASSERT_TRUE(codeType != "");

            ASSERT_TRUE(std::abs(measurement.receiverInterSignalBiasNs) < 1.0e6);
            ASSERT_TRUE(measurement.receiverInterSignalBiasUncertaintyNs >= 0);
            ASSERT_TRUE(std::abs(measurement.satelliteInterSignalBiasNs) < 1.0e6);
            ASSERT_TRUE(measurement.satelliteInterSignalBiasUncertaintyNs >= 0);
        }
    }

    iGnssMeasurement->close();
}

/*
 * TestGnssSvInfoFields:
 * Gets 1 location and a GnssSvInfo, and verifies
 * 1. basebandCN0DbHz is valid.
 */
TEST_P(GnssHalTest, TestGnssSvInfoFields) {
    gnss_cb_->location_cbq_.reset();
    StartAndCheckFirstLocation();
    int location_called_count = gnss_cb_->location_cbq_.calledCount();

    // Tolerate 1 less sv status to handle edge cases in reporting.
    int sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size, 0);
    ALOGD("Observed %d GnssSvStatus, while awaiting one location (%d received)",
          sv_info_list_cbq_size, location_called_count);

    hidl_vec<IGnssCallback_2_1::GnssSvInfo> last_sv_info_list;
    ASSERT_TRUE(gnss_cb_->sv_info_list_cbq_.retrieve(last_sv_info_list, 1));

    bool nonZeroCn0Found = false;
    for (auto sv_info : last_sv_info_list) {
        ASSERT_TRUE(sv_info.basebandCN0DbHz >= 0.0 && sv_info.basebandCN0DbHz <= 65.0);
        if (sv_info.basebandCN0DbHz > 0.0) {
            nonZeroCn0Found = true;
        }
    }
    // Assert at least one value is non-zero. Zero is ok in status as it's possibly
    // reporting a searched but not found satellite.
    ASSERT_TRUE(nonZeroCn0Found);
    StopAndClearLocations();
}

/*
 * FindStrongFrequentNonGpsSource:
 *
 * Search through a GnssSvStatus list for the strongest non-GPS satellite observed enough times
 *
 * returns the strongest source,
 *         or a source with constellation == UNKNOWN if none are found sufficient times
 * TODO(skz): create a template for this to reduce code duplication of v2.1 and v2.0 since both
 * are using vectors.
 */
IGnssConfiguration::BlacklistedSource FindStrongFrequentNonGpsSource(
        const std::list<hidl_vec<IGnssCallback_2_1::GnssSvInfo>> sv_info_list,
        const int min_observations) {
    struct ComparableBlacklistedSource {
        IGnssConfiguration::BlacklistedSource id;

        ComparableBlacklistedSource() {
            id.constellation = GnssConstellationType::UNKNOWN;
            id.svid = 0;
        }

        bool operator<(const ComparableBlacklistedSource& compare) const {
            return ((id.svid < compare.id.svid) || ((id.svid == compare.id.svid) &&
                                                    (id.constellation < compare.id.constellation)));
        }
    };

    struct SignalCounts {
        int observations;
        float max_cn0_dbhz;
    };

    std::map<ComparableBlacklistedSource, SignalCounts> mapSignals;

    for (const auto& sv_info_vec : sv_info_list) {
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            const auto& gnss_sv = sv_info_vec[iSv];
            if ((gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX) &&
                (gnss_sv.v2_0.constellation != GnssConstellationType::GPS)) {
                ComparableBlacklistedSource source;
                source.id.svid = gnss_sv.v2_0.v1_0.svid;
                source.id.constellation = gnss_sv.v2_0.constellation;

                const auto& itSignal = mapSignals.find(source);
                if (itSignal == mapSignals.end()) {
                    SignalCounts counts;
                    counts.observations = 1;
                    counts.max_cn0_dbhz = gnss_sv.v2_0.v1_0.cN0Dbhz;
                    mapSignals.insert(
                            std::pair<ComparableBlacklistedSource, SignalCounts>(source, counts));
                } else {
                    itSignal->second.observations++;
                    if (itSignal->second.max_cn0_dbhz < gnss_sv.v2_0.v1_0.cN0Dbhz) {
                        itSignal->second.max_cn0_dbhz = gnss_sv.v2_0.v1_0.cN0Dbhz;
                    }
                }
            }
        }
    }

    float max_cn0_dbhz_with_sufficient_count = 0.;
    int total_observation_count = 0;
    int blacklisted_source_count_observation = 0;

    ComparableBlacklistedSource source_to_blacklist;  // initializes to zero = UNKNOWN constellation
    for (auto const& pairSignal : mapSignals) {
        total_observation_count += pairSignal.second.observations;
        if ((pairSignal.second.observations >= min_observations) &&
            (pairSignal.second.max_cn0_dbhz > max_cn0_dbhz_with_sufficient_count)) {
            source_to_blacklist = pairSignal.first;
            blacklisted_source_count_observation = pairSignal.second.observations;
            max_cn0_dbhz_with_sufficient_count = pairSignal.second.max_cn0_dbhz;
        }
    }
    ALOGD("Among %d observations, chose svid %d, constellation %d, "
          "with %d observations at %.1f max CNo",
          total_observation_count, source_to_blacklist.id.svid,
          (int)source_to_blacklist.id.constellation, blacklisted_source_count_observation,
          max_cn0_dbhz_with_sufficient_count);

    return source_to_blacklist.id;
}

/*
 * BlacklistIndividualSatellites:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for common satellites (strongest and one other.)
 * 2a & b) Turns off location, and blacklists common satellites.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use those satellites.
 * 4a & b) Turns off location, and send in empty blacklist.
 * 5a) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does re-use at least the previously strongest satellite
 * 5b) Retry a few times, in case GNSS search strategy takes a while to reacquire even the
 * formerly strongest satellite
 */
TEST_P(GnssHalTest, BlacklistIndividualSatellites) {
    const int kLocationsToAwait = 3;
    const int kRetriesToUnBlacklist = 10;

    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(kLocationsToAwait);
    int location_called_count = gnss_cb_->location_cbq_.calledCount();

    // Tolerate 1 less sv status to handle edge cases in reporting.
    int sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, kLocationsToAwait);
    ALOGD("Observed %d GnssSvInfo, while awaiting %d Locations (%d received)",
          sv_info_list_cbq_size, kLocationsToAwait, location_called_count);

    /*
     * Identify strongest SV seen at least kLocationsToAwait -1 times
     * Why -1?  To avoid test flakiness in case of (plausible) slight flakiness in strongest signal
     * observability (one epoch RF null)
     */

    const int kGnssSvInfoListTimeout = 2;
    std::list<hidl_vec<IGnssCallback_2_1::GnssSvInfo>> sv_info_vec_list;
    int count = gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec_list, sv_info_list_cbq_size,
                                                     kGnssSvInfoListTimeout);

    ASSERT_EQ(count, sv_info_list_cbq_size);

    IGnssConfiguration::BlacklistedSource source_to_blacklist =
            FindStrongFrequentNonGpsSource(sv_info_vec_list, kLocationsToAwait - 1);

    if (source_to_blacklist.constellation == GnssConstellationType::UNKNOWN) {
        // Cannot find a non-GPS satellite. Let the test pass.
        ALOGD("Cannot find a non-GPS satellite. Letting the test pass.");
        return;
    }

    // Stop locations, blacklist the common SV
    StopAndClearLocations();

    auto gnss_configuration_hal_return = gnss_hal_->getExtensionGnssConfiguration_2_1();
    ASSERT_TRUE(gnss_configuration_hal_return.isOk());
    sp<IGnssConfiguration> gnss_configuration_hal = gnss_configuration_hal_return;
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<IGnssConfiguration::BlacklistedSource> sources;
    sources.resize(1);
    sources[0] = source_to_blacklist;

    auto result = gnss_configuration_hal->setBlacklist_2_1(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    // retry and ensure satellite not used
    gnss_cb_->sv_info_list_cbq_.reset();

    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(kLocationsToAwait);

    // early exit if test is being run with insufficient signal
    location_called_count = gnss_cb_->location_cbq_.calledCount();
    if (location_called_count == 0) {
        ALOGE("0 Gnss locations received - ensure sufficient signal and retry");
    }
    ASSERT_TRUE(location_called_count > 0);

    // Tolerate 1 less sv status to handle edge cases in reporting.
    sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, kLocationsToAwait);
    ALOGD("Observed %d GnssSvInfo, while awaiting %d Locations (%d received)",
          sv_info_list_cbq_size, kLocationsToAwait, location_called_count);
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        hidl_vec<IGnssCallback_2_1::GnssSvInfo> sv_info_vec;
        gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec, kGnssSvInfoListTimeout);
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            const auto& gnss_sv = sv_info_vec[iSv];
            EXPECT_FALSE((gnss_sv.v2_0.v1_0.svid == source_to_blacklist.svid) &&
                         (gnss_sv.v2_0.constellation == source_to_blacklist.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clear blacklist and restart - this time updating the blacklist while location is still on
    sources.resize(0);

    result = gnss_configuration_hal->setBlacklist_2_1(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    bool strongest_sv_is_reobserved = false;
    // do several loops awaiting a few locations, allowing non-immediate reacquisition strategies
    int unblacklist_loops_remaining = kRetriesToUnBlacklist;
    while (!strongest_sv_is_reobserved && (unblacklist_loops_remaining-- > 0)) {
        StopAndClearLocations();
        gnss_cb_->sv_info_list_cbq_.reset();

        gnss_cb_->location_cbq_.reset();
        StartAndCheckLocations(kLocationsToAwait);

        // early exit loop if test is being run with insufficient signal
        location_called_count = gnss_cb_->location_cbq_.calledCount();
        if (location_called_count == 0) {
            ALOGE("0 Gnss locations received - ensure sufficient signal and retry");
        }
        ASSERT_TRUE(location_called_count > 0);

        // Tolerate 1 less sv status to handle edge cases in reporting.
        sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
        EXPECT_GE(sv_info_list_cbq_size + 1, kLocationsToAwait);
        ALOGD("Clear blacklist, observed %d GnssSvInfo, while awaiting %d Locations"
              ", tries remaining %d",
              sv_info_list_cbq_size, kLocationsToAwait, unblacklist_loops_remaining);

        for (int i = 0; i < sv_info_list_cbq_size; ++i) {
            hidl_vec<IGnssCallback_2_1::GnssSvInfo> sv_info_vec;
            gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec, kGnssSvInfoListTimeout);
            for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
                const auto& gnss_sv = sv_info_vec[iSv];
                if ((gnss_sv.v2_0.v1_0.svid == source_to_blacklist.svid) &&
                    (gnss_sv.v2_0.constellation == source_to_blacklist.constellation) &&
                    (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX)) {
                    strongest_sv_is_reobserved = true;
                    break;
                }
            }
            if (strongest_sv_is_reobserved) break;
        }
    }
    EXPECT_TRUE(strongest_sv_is_reobserved);
    StopAndClearLocations();
}

/*
 * BlacklistConstellationLocationOff:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for any non-GPS constellations.
 * 2a & b) Turns off location, and blacklist first non-GPS constellations.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use any constellation but GPS.
 * 4a & b) Clean up by turning off location, and send in empty blacklist.
 */
TEST_P(GnssHalTest, BlacklistConstellationLocationOff) {
    const int kLocationsToAwait = 3;
    const int kGnssSvInfoListTimeout = 2;

    // Find first non-GPS constellation to blacklist
    GnssConstellationType constellation_to_blacklist =
            startLocationAndGetNonGpsConstellation(kLocationsToAwait, kGnssSvInfoListTimeout);

    // Turns off location
    StopAndClearLocations();

    IGnssConfiguration::BlacklistedSource source_to_blacklist_1;
    source_to_blacklist_1.constellation = constellation_to_blacklist;
    source_to_blacklist_1.svid = 0;  // documented wildcard for all satellites in this constellation

    // IRNSS was added in 2.0. Always attempt to blacklist IRNSS to verify that the new enum is
    // supported.
    IGnssConfiguration::BlacklistedSource source_to_blacklist_2;
    source_to_blacklist_2.constellation = GnssConstellationType::IRNSS;
    source_to_blacklist_2.svid = 0;  // documented wildcard for all satellites in this constellation

    auto gnss_configuration_hal_return = gnss_hal_->getExtensionGnssConfiguration_2_1();
    ASSERT_TRUE(gnss_configuration_hal_return.isOk());
    sp<IGnssConfiguration> gnss_configuration_hal = gnss_configuration_hal_return;
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<IGnssConfiguration::BlacklistedSource> sources;
    sources.resize(2);
    sources[0] = source_to_blacklist_1;
    sources[1] = source_to_blacklist_2;

    auto result = gnss_configuration_hal->setBlacklist_2_1(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    // retry and ensure constellation not used
    gnss_cb_->sv_info_list_cbq_.reset();

    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(kLocationsToAwait);

    // Tolerate 1 less sv status to handle edge cases in reporting.
    int sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, kLocationsToAwait);
    ALOGD("Observed %d GnssSvInfo, while awaiting %d Locations", sv_info_list_cbq_size,
          kLocationsToAwait);
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        hidl_vec<IGnssCallback_2_1::GnssSvInfo> sv_info_vec;
        gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec, kGnssSvInfoListTimeout);
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            const auto& gnss_sv = sv_info_vec[iSv];
            EXPECT_FALSE((gnss_sv.v2_0.constellation == source_to_blacklist_1.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
            EXPECT_FALSE((gnss_sv.v2_0.constellation == source_to_blacklist_2.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clean up
    StopAndClearLocations();
    sources.resize(0);
    result = gnss_configuration_hal->setBlacklist_2_1(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

/*
 * BlacklistConstellationLocationOn:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for any non-GPS constellations.
 * 2a & b) Blacklist first non-GPS constellation, and turn off location.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use any constellation but GPS.
 * 4a & b) Clean up by turning off location, and send in empty blacklist.
 */
TEST_P(GnssHalTest, BlacklistConstellationLocationOn) {
    const int kLocationsToAwait = 3;
    const int kGnssSvInfoListTimeout = 2;

    // Find first non-GPS constellation to blacklist
    GnssConstellationType constellation_to_blacklist =
            startLocationAndGetNonGpsConstellation(kLocationsToAwait, kGnssSvInfoListTimeout);

    IGnssConfiguration::BlacklistedSource source_to_blacklist_1;
    source_to_blacklist_1.constellation = constellation_to_blacklist;
    source_to_blacklist_1.svid = 0;  // documented wildcard for all satellites in this constellation

    // IRNSS was added in 2.0. Always attempt to blacklist IRNSS to verify that the new enum is
    // supported.
    IGnssConfiguration::BlacklistedSource source_to_blacklist_2;
    source_to_blacklist_2.constellation = GnssConstellationType::IRNSS;
    source_to_blacklist_2.svid = 0;  // documented wildcard for all satellites in this constellation

    auto gnss_configuration_hal_return = gnss_hal_->getExtensionGnssConfiguration_2_1();
    ASSERT_TRUE(gnss_configuration_hal_return.isOk());
    sp<IGnssConfiguration> gnss_configuration_hal = gnss_configuration_hal_return;
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<IGnssConfiguration::BlacklistedSource> sources;
    sources.resize(2);
    sources[0] = source_to_blacklist_1;
    sources[1] = source_to_blacklist_2;

    auto result = gnss_configuration_hal->setBlacklist_2_1(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    // Turns off location
    StopAndClearLocations();

    // retry and ensure constellation not used
    gnss_cb_->sv_info_list_cbq_.reset();

    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(kLocationsToAwait);

    // Tolerate 1 less sv status to handle edge cases in reporting.
    int sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, kLocationsToAwait);
    ALOGD("Observed %d GnssSvInfo, while awaiting %d Locations", sv_info_list_cbq_size,
          kLocationsToAwait);
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        hidl_vec<IGnssCallback_2_1::GnssSvInfo> sv_info_vec;
        gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec, kGnssSvInfoListTimeout);
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            const auto& gnss_sv = sv_info_vec[iSv];
            EXPECT_FALSE((gnss_sv.v2_0.constellation == source_to_blacklist_1.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
            EXPECT_FALSE((gnss_sv.v2_0.constellation == source_to_blacklist_2.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clean up
    StopAndClearLocations();
    sources.resize(0);
    result = gnss_configuration_hal->setBlacklist_2_1(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

/*
 * TestGnssMeasurementCorrections:
 * If measurement corrections capability is supported, verifies that it supports the
 * gnss.measurement_corrections@1.1::IMeasurementCorrections interface by invoking a method.
 */
TEST_P(GnssHalTest, TestGnssMeasurementCorrections) {
    if (!(gnss_cb_->last_capabilities_ &
          IGnssCallback_2_1::Capabilities::MEASUREMENT_CORRECTIONS)) {
        return;
    }

    // Verify IMeasurementCorrections is supported.
    auto measurementCorrections = gnss_hal_->getExtensionMeasurementCorrections_1_1();
    ASSERT_TRUE(measurementCorrections.isOk());
    sp<IMeasurementCorrections_1_1> iMeasurementCorrections = measurementCorrections;
    ASSERT_NE(iMeasurementCorrections, nullptr);

    sp<GnssMeasurementCorrectionsCallback> callback = new GnssMeasurementCorrectionsCallback();
    iMeasurementCorrections->setCallback(callback);

    const int kMeasurementCorrectionsCapabilitiesTimeoutSeconds = 5;
    callback->capabilities_cbq_.retrieve(callback->last_capabilities_,
                                         kMeasurementCorrectionsCapabilitiesTimeoutSeconds);
    ASSERT_TRUE(callback->capabilities_cbq_.calledCount() > 0);

    // Set a mock MeasurementCorrections.
    auto result =
            iMeasurementCorrections->setCorrections_1_1(Utils::getMockMeasurementCorrections_1_1());
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}