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

#define LOG_TAG "GnssHalTestCases"

#include <android/hardware/gnss/IGnss.h>
#include <android/hardware/gnss/IGnssMeasurementCallback.h>
#include <android/hardware/gnss/IGnssMeasurementInterface.h>
#include <android/hardware/gnss/IGnssPowerIndication.h>
#include <android/hardware/gnss/IGnssPsds.h>
#include "GnssMeasurementCallbackAidl.h"
#include "GnssPowerIndicationCallback.h"
#include "gnss_hal_test.h"

using android::sp;
using android::hardware::gnss::BlocklistedSource;
using android::hardware::gnss::ElapsedRealtime;
using android::hardware::gnss::GnssClock;
using android::hardware::gnss::GnssData;
using android::hardware::gnss::GnssMeasurement;
using android::hardware::gnss::GnssPowerStats;
using android::hardware::gnss::IGnss;
using android::hardware::gnss::IGnssConfiguration;
using android::hardware::gnss::IGnssMeasurementCallback;
using android::hardware::gnss::IGnssMeasurementInterface;
using android::hardware::gnss::IGnssPowerIndication;
using android::hardware::gnss::IGnssPsds;
using android::hardware::gnss::PsdsType;
using android::hardware::gnss::SatellitePvt;

using GnssConstellationTypeAidl = android::hardware::gnss::GnssConstellationType;

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_P(GnssHalTest, SetupTeardownCreateCleanup) {}

/*
 * TestPsdsExtension:
 * 1. Gets the PsdsExtension
 * 2. Injects empty PSDS data and verifies that it returns an error.
 */
TEST_P(GnssHalTest, TestPsdsExtension) {
    sp<IGnssPsds> iGnssPsds;
    auto status = aidl_gnss_hal_->getExtensionPsds(&iGnssPsds);
    if (status.isOk() && iGnssPsds != nullptr) {
        status = iGnssPsds->injectPsdsData(PsdsType::LONG_TERM, std::vector<uint8_t>());
        ASSERT_FALSE(status.isOk());
    }
}

void CheckSatellitePvt(const SatellitePvt& satellitePvt) {
    const double kMaxOrbitRadiusMeters = 43000000.0;
    const double kMaxVelocityMps = 4000.0;
    // The below values are determined using GPS ICD Table 20-1
    const double kMinHardwareCodeBiasMeters = -17.869;
    const double kMaxHardwareCodeBiasMeters = 17.729;
    const double kMaxTimeCorrelationMeters = 3e6;
    const double kMaxSatClkDriftMps = 1.117;

    ASSERT_TRUE(satellitePvt.flags & SatellitePvt::HAS_POSITION_VELOCITY_CLOCK_INFO ||
                satellitePvt.flags & SatellitePvt::HAS_IONO ||
                satellitePvt.flags & SatellitePvt::HAS_TROPO);
    if (satellitePvt.flags & SatellitePvt::HAS_POSITION_VELOCITY_CLOCK_INFO) {
        ALOGD("Found HAS_POSITION_VELOCITY_CLOCK_INFO");
        ASSERT_TRUE(satellitePvt.satPosEcef.posXMeters >= -kMaxOrbitRadiusMeters &&
                    satellitePvt.satPosEcef.posXMeters <= kMaxOrbitRadiusMeters);
        ASSERT_TRUE(satellitePvt.satPosEcef.posYMeters >= -kMaxOrbitRadiusMeters &&
                    satellitePvt.satPosEcef.posYMeters <= kMaxOrbitRadiusMeters);
        ASSERT_TRUE(satellitePvt.satPosEcef.posZMeters >= -kMaxOrbitRadiusMeters &&
                    satellitePvt.satPosEcef.posZMeters <= kMaxOrbitRadiusMeters);
        ASSERT_TRUE(satellitePvt.satPosEcef.ureMeters > 0);
        ASSERT_TRUE(satellitePvt.satVelEcef.velXMps >= -kMaxVelocityMps &&
                    satellitePvt.satVelEcef.velXMps <= kMaxVelocityMps);
        ASSERT_TRUE(satellitePvt.satVelEcef.velYMps >= -kMaxVelocityMps &&
                    satellitePvt.satVelEcef.velYMps <= kMaxVelocityMps);
        ASSERT_TRUE(satellitePvt.satVelEcef.velZMps >= -kMaxVelocityMps &&
                    satellitePvt.satVelEcef.velZMps <= kMaxVelocityMps);
        ASSERT_TRUE(satellitePvt.satVelEcef.ureRateMps > 0);
        ASSERT_TRUE(
                satellitePvt.satClockInfo.satHardwareCodeBiasMeters > kMinHardwareCodeBiasMeters &&
                satellitePvt.satClockInfo.satHardwareCodeBiasMeters < kMaxHardwareCodeBiasMeters);
        ASSERT_TRUE(satellitePvt.satClockInfo.satTimeCorrectionMeters >
                            -kMaxTimeCorrelationMeters &&
                    satellitePvt.satClockInfo.satTimeCorrectionMeters < kMaxTimeCorrelationMeters);
        ASSERT_TRUE(satellitePvt.satClockInfo.satClkDriftMps > -kMaxSatClkDriftMps &&
                    satellitePvt.satClockInfo.satClkDriftMps < kMaxSatClkDriftMps);
    }
    if (satellitePvt.flags & SatellitePvt::HAS_IONO) {
        ALOGD("Found HAS_IONO");
        ASSERT_TRUE(satellitePvt.ionoDelayMeters > 0 && satellitePvt.ionoDelayMeters < 100);
    }
    if (satellitePvt.flags & SatellitePvt::HAS_TROPO) {
        ALOGD("Found HAS_TROPO");
        ASSERT_TRUE(satellitePvt.tropoDelayMeters > 0 && satellitePvt.tropoDelayMeters < 100);
    }
}

void CheckGnssMeasurementClockFields(const GnssData& measurement) {
    ASSERT_TRUE(measurement.elapsedRealtime.flags >= 0 &&
                measurement.elapsedRealtime.flags <= (ElapsedRealtime::HAS_TIMESTAMP_NS |
                                                      ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS));
    if (measurement.elapsedRealtime.flags & ElapsedRealtime::HAS_TIMESTAMP_NS) {
        ASSERT_TRUE(measurement.elapsedRealtime.timestampNs > 0);
    }
    if (measurement.elapsedRealtime.flags & ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS) {
        ASSERT_TRUE(measurement.elapsedRealtime.timeUncertaintyNs > 0);
    }
    ASSERT_TRUE(measurement.clock.gnssClockFlags >= 0 &&
                measurement.clock.gnssClockFlags <=
                        (GnssClock::HAS_LEAP_SECOND | GnssClock::HAS_TIME_UNCERTAINTY |
                         GnssClock::HAS_FULL_BIAS | GnssClock::HAS_BIAS |
                         GnssClock::HAS_BIAS_UNCERTAINTY | GnssClock::HAS_DRIFT |
                         GnssClock::HAS_DRIFT_UNCERTAINTY));
}

void CheckGnssMeasurementFlags(const GnssMeasurement& measurement) {
    ASSERT_TRUE(measurement.flags >= 0 &&
                measurement.flags <=
                        (GnssMeasurement::HAS_SNR | GnssMeasurement::HAS_CARRIER_FREQUENCY |
                         GnssMeasurement::HAS_CARRIER_CYCLES | GnssMeasurement::HAS_CARRIER_PHASE |
                         GnssMeasurement::HAS_CARRIER_PHASE_UNCERTAINTY |
                         GnssMeasurement::HAS_AUTOMATIC_GAIN_CONTROL |
                         GnssMeasurement::HAS_FULL_ISB | GnssMeasurement::HAS_FULL_ISB_UNCERTAINTY |
                         GnssMeasurement::HAS_SATELLITE_ISB |
                         GnssMeasurement::HAS_SATELLITE_ISB_UNCERTAINTY |
                         GnssMeasurement::HAS_SATELLITE_PVT |
                         GnssMeasurement::HAS_CORRELATION_VECTOR));
}

/*
 * TestGnssMeasurementExtensionAndSatellitePvt:
 * 1. Gets the GnssMeasurementExtension and verifies that it returns a non-null extension.
 * 2. Sets a GnssMeasurementCallback, waits for a measurement, and verifies mandatory fields are
 *    valid.
 * 3. If SatellitePvt is supported, waits for a measurement with SatellitePvt, and verifies the
 *    fields are valid.
 */
TEST_P(GnssHalTest, TestGnssMeasurementExtensionAndSatellitePvt) {
    const bool kIsSatellitePvtSupported =
            aidl_gnss_cb_->last_capabilities_ & (int)GnssCallbackAidl::CAPABILITY_SATELLITE_PVT;
    ALOGD("SatellitePvt supported: %s", kIsSatellitePvtSupported ? "true" : "false");
    const int kFirstGnssMeasurementTimeoutSeconds = 10;
    const int kNumMeasurementEvents = 75;

    sp<IGnssMeasurementInterface> iGnssMeasurement;
    auto status = aidl_gnss_hal_->getExtensionGnssMeasurement(&iGnssMeasurement);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(iGnssMeasurement != nullptr);

    auto callback = sp<GnssMeasurementCallbackAidl>::make();
    status = iGnssMeasurement->setCallback(callback, /* enableFullTracking= */ true,
                                           /* enableCorrVecOutputs */ false);
    ASSERT_TRUE(status.isOk());

    bool satellitePvtFound = false;
    for (int i = 0; i < kNumMeasurementEvents; i++) {
        if (i > 0 && (!kIsSatellitePvtSupported || satellitePvtFound)) {
            break;
        }
        GnssData lastMeasurement;
        ASSERT_TRUE(callback->gnss_data_cbq_.retrieve(lastMeasurement,
                                                      kFirstGnssMeasurementTimeoutSeconds));
        EXPECT_EQ(callback->gnss_data_cbq_.calledCount(), i + 1);
        ASSERT_TRUE(lastMeasurement.measurements.size() > 0);

        // Validity check GnssData fields
        CheckGnssMeasurementClockFields(lastMeasurement);

        for (const auto& measurement : lastMeasurement.measurements) {
            CheckGnssMeasurementFlags(measurement);
            if (measurement.flags & GnssMeasurement::HAS_SATELLITE_PVT &&
                kIsSatellitePvtSupported == true) {
                ALOGD("Found a measurement with SatellitePvt");
                satellitePvtFound = true;
                CheckSatellitePvt(measurement.satellitePvt);
            }
        }
    }
    if (kIsSatellitePvtSupported) {
        ASSERT_TRUE(satellitePvtFound);
    }

    status = iGnssMeasurement->close();
    ASSERT_TRUE(status.isOk());
}

/*
 * TestCorrelationVector:
 * 1. Gets the GnssMeasurementExtension and verifies that it returns a non-null extension.
 * 2. Sets a GnssMeasurementCallback, waits for GnssMeasurements with CorrelationVector, and
 *    verifies fields are valid.
 */
TEST_P(GnssHalTest, TestCorrelationVector) {
    const bool kIsCorrelationVectorSupported = aidl_gnss_cb_->last_capabilities_ &
                                               (int)GnssCallbackAidl::CAPABILITY_CORRELATION_VECTOR;
    const int kNumMeasurementEvents = 75;
    // Pass the test if CorrelationVector is not supported
    if (!kIsCorrelationVectorSupported) {
        return;
    }

    const int kFirstGnssMeasurementTimeoutSeconds = 10;
    sp<IGnssMeasurementInterface> iGnssMeasurement;
    auto status = aidl_gnss_hal_->getExtensionGnssMeasurement(&iGnssMeasurement);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(iGnssMeasurement != nullptr);

    auto callback = sp<GnssMeasurementCallbackAidl>::make();
    status =
            iGnssMeasurement->setCallback(callback, /* enableFullTracking= */ true,
                                          /* enableCorrVecOutputs */ kIsCorrelationVectorSupported);
    ASSERT_TRUE(status.isOk());

    bool correlationVectorFound = false;
    for (int i = 0; i < kNumMeasurementEvents; i++) {
        // Pass the test if at least one CorrelationVector has been found.
        if (correlationVectorFound) {
            break;
        }
        GnssData lastMeasurement;
        ASSERT_TRUE(callback->gnss_data_cbq_.retrieve(lastMeasurement,
                                                      kFirstGnssMeasurementTimeoutSeconds));
        EXPECT_EQ(callback->gnss_data_cbq_.calledCount(), i + 1);
        ASSERT_TRUE(lastMeasurement.measurements.size() > 0);

        // Validity check GnssData fields
        CheckGnssMeasurementClockFields(lastMeasurement);

        for (const auto& measurement : lastMeasurement.measurements) {
            CheckGnssMeasurementFlags(measurement);
            if (measurement.flags & GnssMeasurement::HAS_CORRELATION_VECTOR) {
                correlationVectorFound = true;
                ASSERT_TRUE(measurement.correlationVectors.size() > 0);
                for (const auto& correlationVector : measurement.correlationVectors) {
                    ASSERT_GE(correlationVector.frequencyOffsetMps, 0);
                    ASSERT_GT(correlationVector.samplingWidthM, 0);
                    ASSERT_TRUE(correlationVector.magnitude.size() > 0);
                    for (const auto& magnitude : correlationVector.magnitude) {
                        ASSERT_TRUE(magnitude >= -32768 && magnitude <= 32767);
                    }
                }
            }
        }
    }
    ASSERT_TRUE(correlationVectorFound);

    status = iGnssMeasurement->close();
    ASSERT_TRUE(status.isOk());
}

/*
 * TestGnssPowerIndication
 * 1. Gets the GnssPowerIndicationExtension.
 * 2. Sets a GnssPowerIndicationCallback.
 * 3. Requests and verifies the 1st GnssPowerStats is received.
 * 4. Gets a location.
 * 5. Requests the 2nd GnssPowerStats, and verifies it has larger values than the 1st one.
 */
TEST_P(GnssHalTest, TestGnssPowerIndication) {
    // Set up gnssPowerIndication and callback
    sp<IGnssPowerIndication> iGnssPowerIndication;
    auto status = aidl_gnss_hal_->getExtensionGnssPowerIndication(&iGnssPowerIndication);
    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(iGnssPowerIndication != nullptr);

    auto gnssPowerIndicationCallback = sp<GnssPowerIndicationCallback>::make();
    status = iGnssPowerIndication->setCallback(gnssPowerIndicationCallback);
    ASSERT_TRUE(status.isOk());

    const int kTimeoutSec = 2;
    EXPECT_TRUE(gnssPowerIndicationCallback->capabilities_cbq_.retrieve(
            gnssPowerIndicationCallback->last_capabilities_, kTimeoutSec));

    EXPECT_EQ(gnssPowerIndicationCallback->capabilities_cbq_.calledCount(), 1);

    // Request and verify a GnssPowerStats is received
    gnssPowerIndicationCallback->gnss_power_stats_cbq_.reset();
    iGnssPowerIndication->requestGnssPowerStats();

    EXPECT_TRUE(gnssPowerIndicationCallback->gnss_power_stats_cbq_.retrieve(
            gnssPowerIndicationCallback->last_gnss_power_stats_, kTimeoutSec));
    EXPECT_EQ(gnssPowerIndicationCallback->gnss_power_stats_cbq_.calledCount(), 1);
    auto powerStats1 = gnssPowerIndicationCallback->last_gnss_power_stats_;

    // Get a location and request another GnssPowerStats
    gnss_cb_->location_cbq_.reset();
    StartAndCheckFirstLocation(/* min_interval_msec= */ 1000, /* low_power_mode= */ false);

    // Request and verify the 2nd GnssPowerStats has larger values than the 1st one
    iGnssPowerIndication->requestGnssPowerStats();

    EXPECT_TRUE(gnssPowerIndicationCallback->gnss_power_stats_cbq_.retrieve(
            gnssPowerIndicationCallback->last_gnss_power_stats_, kTimeoutSec));
    EXPECT_EQ(gnssPowerIndicationCallback->gnss_power_stats_cbq_.calledCount(), 2);

    auto powerStats2 = gnssPowerIndicationCallback->last_gnss_power_stats_;

    if ((gnssPowerIndicationCallback->last_capabilities_ &
         (int)GnssPowerIndicationCallback::CAPABILITY_TOTAL)) {
        // Elapsed realtime must increase
        EXPECT_GT(powerStats2.elapsedRealtime.timestampNs, powerStats1.elapsedRealtime.timestampNs);

        // Total energy must increase
        EXPECT_GT(powerStats2.totalEnergyMilliJoule, powerStats1.totalEnergyMilliJoule);
    }

    // At least oone of singleband and multiband acquisition energy must increase
    bool singlebandAcqEnergyIncreased = powerStats2.singlebandAcquisitionModeEnergyMilliJoule >
                                        powerStats1.singlebandAcquisitionModeEnergyMilliJoule;
    bool multibandAcqEnergyIncreased = powerStats2.multibandAcquisitionModeEnergyMilliJoule >
                                       powerStats1.multibandAcquisitionModeEnergyMilliJoule;

    if ((gnssPowerIndicationCallback->last_capabilities_ &
         (int)GnssPowerIndicationCallback::CAPABILITY_SINGLEBAND_ACQUISITION) ||
        (gnssPowerIndicationCallback->last_capabilities_ &
         (int)GnssPowerIndicationCallback::CAPABILITY_MULTIBAND_ACQUISITION)) {
        EXPECT_TRUE(singlebandAcqEnergyIncreased || multibandAcqEnergyIncreased);
    }

    // At least one of singleband and multiband tracking energy must increase
    bool singlebandTrackingEnergyIncreased = powerStats2.singlebandTrackingModeEnergyMilliJoule >
                                             powerStats1.singlebandTrackingModeEnergyMilliJoule;
    bool multibandTrackingEnergyIncreased = powerStats2.multibandTrackingModeEnergyMilliJoule >
                                            powerStats1.multibandTrackingModeEnergyMilliJoule;
    if ((gnssPowerIndicationCallback->last_capabilities_ &
         (int)GnssPowerIndicationCallback::CAPABILITY_SINGLEBAND_TRACKING) ||
        (gnssPowerIndicationCallback->last_capabilities_ &
         (int)GnssPowerIndicationCallback::CAPABILITY_MULTIBAND_TRACKING)) {
        EXPECT_TRUE(singlebandTrackingEnergyIncreased || multibandTrackingEnergyIncreased);
    }

    // Clean up
    StopAndClearLocations();
}

/*
 * FindStrongFrequentNonGpsSource:
 *
 * Search through a GnssSvStatus list for the strongest non-GPS satellite observed enough times
 *
 * returns the strongest source,
 *         or a source with constellation == UNKNOWN if none are found sufficient times
 */
BlocklistedSource FindStrongFrequentNonGpsSource(
        const std::list<hidl_vec<IGnssCallback_2_1::GnssSvInfo>> sv_info_list,
        const int min_observations) {
    struct ComparableBlocklistedSource {
        BlocklistedSource id;

        ComparableBlocklistedSource() {
            id.constellation = GnssConstellationTypeAidl::UNKNOWN;
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

    std::map<ComparableBlocklistedSource, SignalCounts> mapSignals;

    for (const auto& sv_info_vec : sv_info_list) {
        for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
            const auto& gnss_sv = sv_info_vec[iSv];
            if ((gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX) &&
                (gnss_sv.v2_0.constellation != GnssConstellationType::GPS)) {
                ComparableBlocklistedSource source;
                source.id.svid = gnss_sv.v2_0.v1_0.svid;
                source.id.constellation =
                        static_cast<GnssConstellationTypeAidl>(gnss_sv.v2_0.constellation);

                const auto& itSignal = mapSignals.find(source);
                if (itSignal == mapSignals.end()) {
                    SignalCounts counts;
                    counts.observations = 1;
                    counts.max_cn0_dbhz = gnss_sv.v2_0.v1_0.cN0Dbhz;
                    mapSignals.insert(
                            std::pair<ComparableBlocklistedSource, SignalCounts>(source, counts));
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

/*
 * BlocklistIndividualSatellites:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for common satellites (strongest and one other.)
 * 2a & b) Turns off location, and blocklists common satellites.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use those satellites.
 * 4a & b) Turns off location, and send in empty blocklist.
 * 5a) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does re-use at least the previously strongest satellite
 * 5b) Retry a few times, in case GNSS search strategy takes a while to reacquire even the
 * formerly strongest satellite
 */
TEST_P(GnssHalTest, BlocklistIndividualSatellites) {
    if (!(aidl_gnss_cb_->last_capabilities_ &
          (int)GnssCallbackAidl::CAPABILITY_SATELLITE_BLOCKLIST)) {
        ALOGI("Test BlocklistIndividualSatellites skipped. SATELLITE_BLOCKLIST capability not "
              "supported.");
        return;
    }

    const int kLocationsToAwait = 3;
    const int kRetriesToUnBlocklist = 10;

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

    BlocklistedSource source_to_blocklist =
            FindStrongFrequentNonGpsSource(sv_info_vec_list, kLocationsToAwait - 1);

    if (source_to_blocklist.constellation == GnssConstellationTypeAidl::UNKNOWN) {
        // Cannot find a non-GPS satellite. Let the test pass.
        ALOGD("Cannot find a non-GPS satellite. Letting the test pass.");
        return;
    }

    // Stop locations, blocklist the common SV
    StopAndClearLocations();

    sp<IGnssConfiguration> gnss_configuration_hal;
    auto status = aidl_gnss_hal_->getExtensionGnssConfiguration(&gnss_configuration_hal);
    ASSERT_TRUE(status.isOk());
    ASSERT_NE(gnss_configuration_hal, nullptr);

    std::vector<BlocklistedSource> sources;
    sources.resize(1);
    sources[0] = source_to_blocklist;

    status = gnss_configuration_hal->setBlocklist(sources);
    ASSERT_TRUE(status.isOk());

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
            EXPECT_FALSE((gnss_sv.v2_0.v1_0.svid == source_to_blocklist.svid) &&
                         (static_cast<GnssConstellationTypeAidl>(gnss_sv.v2_0.constellation) ==
                          source_to_blocklist.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clear blocklist and restart - this time updating the blocklist while location is still on
    sources.resize(0);

    status = gnss_configuration_hal->setBlocklist(sources);
    ASSERT_TRUE(status.isOk());

    bool strongest_sv_is_reobserved = false;
    // do several loops awaiting a few locations, allowing non-immediate reacquisition strategies
    int unblocklist_loops_remaining = kRetriesToUnBlocklist;
    while (!strongest_sv_is_reobserved && (unblocklist_loops_remaining-- > 0)) {
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
        ALOGD("Clear blocklist, observed %d GnssSvInfo, while awaiting %d Locations"
              ", tries remaining %d",
              sv_info_list_cbq_size, kLocationsToAwait, unblocklist_loops_remaining);

        for (int i = 0; i < sv_info_list_cbq_size; ++i) {
            hidl_vec<IGnssCallback_2_1::GnssSvInfo> sv_info_vec;
            gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_vec, kGnssSvInfoListTimeout);
            for (uint32_t iSv = 0; iSv < sv_info_vec.size(); iSv++) {
                const auto& gnss_sv = sv_info_vec[iSv];
                if ((gnss_sv.v2_0.v1_0.svid == source_to_blocklist.svid) &&
                    (static_cast<GnssConstellationTypeAidl>(gnss_sv.v2_0.constellation) ==
                     source_to_blocklist.constellation) &&
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
 * BlocklistConstellationLocationOff:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for any non-GPS constellations.
 * 2a & b) Turns off location, and blocklist first non-GPS constellations.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use any constellation but GPS.
 * 4a & b) Clean up by turning off location, and send in empty blocklist.
 */
TEST_P(GnssHalTest, BlocklistConstellationLocationOff) {
    if (!(aidl_gnss_cb_->last_capabilities_ &
          (int)GnssCallbackAidl::CAPABILITY_SATELLITE_BLOCKLIST)) {
        ALOGI("Test BlocklistConstellationLocationOff skipped. SATELLITE_BLOCKLIST capability not "
              "supported.");
        return;
    }

    const int kLocationsToAwait = 3;
    const int kGnssSvInfoListTimeout = 2;

    // Find first non-GPS constellation to blocklist
    GnssConstellationTypeAidl constellation_to_blocklist = static_cast<GnssConstellationTypeAidl>(
            startLocationAndGetNonGpsConstellation(kLocationsToAwait, kGnssSvInfoListTimeout));

    // Turns off location
    StopAndClearLocations();

    BlocklistedSource source_to_blocklist_1;
    source_to_blocklist_1.constellation = constellation_to_blocklist;
    source_to_blocklist_1.svid = 0;  // documented wildcard for all satellites in this constellation

    // IRNSS was added in 2.0. Always attempt to blocklist IRNSS to verify that the new enum is
    // supported.
    BlocklistedSource source_to_blocklist_2;
    source_to_blocklist_2.constellation = GnssConstellationTypeAidl::IRNSS;
    source_to_blocklist_2.svid = 0;  // documented wildcard for all satellites in this constellation

    sp<IGnssConfiguration> gnss_configuration_hal;
    auto status = aidl_gnss_hal_->getExtensionGnssConfiguration(&gnss_configuration_hal);
    ASSERT_TRUE(status.isOk());
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<BlocklistedSource> sources;
    sources.resize(2);
    sources[0] = source_to_blocklist_1;
    sources[1] = source_to_blocklist_2;

    status = gnss_configuration_hal->setBlocklist(sources);
    ASSERT_TRUE(status.isOk());

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
            EXPECT_FALSE((static_cast<GnssConstellationTypeAidl>(gnss_sv.v2_0.constellation) ==
                          source_to_blocklist_1.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
            EXPECT_FALSE((static_cast<GnssConstellationTypeAidl>(gnss_sv.v2_0.constellation) ==
                          source_to_blocklist_2.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clean up
    StopAndClearLocations();
    sources.resize(0);
    status = gnss_configuration_hal->setBlocklist(sources);
    ASSERT_TRUE(status.isOk());
}

/*
 * BlocklistConstellationLocationOn:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for any non-GPS constellations.
 * 2a & b) Blocklist first non-GPS constellation, and turn off location.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use any constellation but GPS.
 * 4a & b) Clean up by turning off location, and send in empty blocklist.
 */
TEST_P(GnssHalTest, BlocklistConstellationLocationOn) {
    if (!(aidl_gnss_cb_->last_capabilities_ &
          (int)GnssCallbackAidl::CAPABILITY_SATELLITE_BLOCKLIST)) {
        ALOGI("Test BlocklistConstellationLocationOn skipped. SATELLITE_BLOCKLIST capability not "
              "supported.");
        return;
    }

    const int kLocationsToAwait = 3;
    const int kGnssSvInfoListTimeout = 2;

    // Find first non-GPS constellation to blocklist
    GnssConstellationTypeAidl constellation_to_blocklist = static_cast<GnssConstellationTypeAidl>(
            startLocationAndGetNonGpsConstellation(kLocationsToAwait, kGnssSvInfoListTimeout));

    BlocklistedSource source_to_blocklist_1;
    source_to_blocklist_1.constellation = constellation_to_blocklist;
    source_to_blocklist_1.svid = 0;  // documented wildcard for all satellites in this constellation

    // IRNSS was added in 2.0. Always attempt to blocklist IRNSS to verify that the new enum is
    // supported.
    BlocklistedSource source_to_blocklist_2;
    source_to_blocklist_2.constellation = GnssConstellationTypeAidl::IRNSS;
    source_to_blocklist_2.svid = 0;  // documented wildcard for all satellites in this constellation

    sp<IGnssConfiguration> gnss_configuration_hal;
    auto status = aidl_gnss_hal_->getExtensionGnssConfiguration(&gnss_configuration_hal);
    ASSERT_TRUE(status.isOk());
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<BlocklistedSource> sources;
    sources.resize(2);
    sources[0] = source_to_blocklist_1;
    sources[1] = source_to_blocklist_2;

    status = gnss_configuration_hal->setBlocklist(sources);
    ASSERT_TRUE(status.isOk());

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
            EXPECT_FALSE((static_cast<GnssConstellationTypeAidl>(gnss_sv.v2_0.constellation) ==
                          source_to_blocklist_1.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
            EXPECT_FALSE((static_cast<GnssConstellationTypeAidl>(gnss_sv.v2_0.constellation) ==
                          source_to_blocklist_2.constellation) &&
                         (gnss_sv.v2_0.v1_0.svFlag & IGnssCallback_1_0::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clean up
    StopAndClearLocations();
    sources.resize(0);
    status = gnss_configuration_hal->setBlocklist(sources);
    ASSERT_TRUE(status.isOk());
}
