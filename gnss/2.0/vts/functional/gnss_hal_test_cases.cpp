/*
 * Copyright (C) 2018 The Android Open Source Project
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
#include "Utils.h"

#include <gtest/gtest.h>

using android::hardware::hidl_string;
using android::hardware::hidl_vec;

using GnssConstellationType_2_0 = android::hardware::gnss::V2_0::GnssConstellationType;
using GnssConstellationType_1_0 = android::hardware::gnss::V1_0::GnssConstellationType;
using IGnssConfiguration_2_0 = android::hardware::gnss::V2_0::IGnssConfiguration;
using IGnssConfiguration_1_1 = android::hardware::gnss::V1_1::IGnssConfiguration;
using IAGnssRil_2_0 = android::hardware::gnss::V2_0::IAGnssRil;
using IGnssMeasurement_2_0 = android::hardware::gnss::V2_0::IGnssMeasurement;
using IGnssMeasurement_1_1 = android::hardware::gnss::V1_1::IGnssMeasurement;
using IGnssMeasurement_1_0 = android::hardware::gnss::V1_0::IGnssMeasurement;
using IAGnssRil_2_0 = android::hardware::gnss::V2_0::IAGnssRil;
using IAGnssRil_1_0 = android::hardware::gnss::V1_0::IAGnssRil;
using IAGnss_2_0 = android::hardware::gnss::V2_0::IAGnss;
using IAGnss_1_0 = android::hardware::gnss::V1_0::IAGnss;
using IAGnssCallback_2_0 = android::hardware::gnss::V2_0::IAGnssCallback;

using android::hardware::gnss::common::Utils;
using android::hardware::gnss::measurement_corrections::V1_0::IMeasurementCorrections;
using android::hardware::gnss::measurement_corrections::V1_0::MeasurementCorrections;
using android::hardware::gnss::V1_0::IGnssNi;
using android::hardware::gnss::V2_0::ElapsedRealtimeFlags;
using android::hardware::gnss::V2_0::IGnssCallback;
using android::hardware::gnss::visibility_control::V1_0::IGnssVisibilityControl;

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
    auto gnssMeasurement_2_0 = gnss_hal_->getExtensionGnssMeasurement_2_0();
    auto gnssMeasurement_1_1 = gnss_hal_->getExtensionGnssMeasurement_1_1();
    auto gnssMeasurement_1_0 = gnss_hal_->getExtensionGnssMeasurement();
    ASSERT_TRUE(gnssMeasurement_2_0.isOk() && gnssMeasurement_1_1.isOk() &&
                gnssMeasurement_1_0.isOk());
    sp<IGnssMeasurement_2_0> iGnssMeas_2_0 = gnssMeasurement_2_0;
    sp<IGnssMeasurement_1_1> iGnssMeas_1_1 = gnssMeasurement_1_1;
    sp<IGnssMeasurement_1_0> iGnssMeas_1_0 = gnssMeasurement_1_0;
    // At least one interface is non-null.
    int numNonNull = (int)(iGnssMeas_2_0 != nullptr) + (int)(iGnssMeas_1_1 != nullptr) +
                     (int)(iGnssMeas_1_0 != nullptr);
    ASSERT_TRUE(numNonNull >= 1);
}

/*
 * TestGnssConfigurationExtension:
 * Gets the GnssConfigurationExtension and verifies that it returns an actual extension by
 * calling a method.
 *
 * The GNSS HAL 2.0 implementation must support @2.0::IGnssConfiguration interface due to
 * the deprecation of some methods in @1.0::IGnssConfiguration interface.
 */
TEST_P(GnssHalTest, TestGnssConfigurationExtension) {
    auto gnssConfiguration = gnss_hal_->getExtensionGnssConfiguration_2_0();
    ASSERT_TRUE(gnssConfiguration.isOk());
    sp<IGnssConfiguration_2_0> iGnssConfiguration = gnssConfiguration;
    ASSERT_NE(iGnssConfiguration, nullptr);

    auto result = iGnssConfiguration->setEsExtensionSec(180);
    ASSERT_TRUE(result.isOk());
    // Expected result can be true or false depending on whether HAL implementation supports
    // detecting emergency sessions without involving the framework.
}

/*
 * TestGnssConfiguration_setSuplEs_Deprecation:
 * Calls setSuplEs and verifies that it returns false.
 */
TEST_P(GnssHalTest, TestGnssConfiguration_setSuplEs_Deprecation) {
    auto gnssConfiguration = gnss_hal_->getExtensionGnssConfiguration_2_0();
    ASSERT_TRUE(gnssConfiguration.isOk());
    sp<IGnssConfiguration_2_0> iGnssConfiguration = gnssConfiguration;
    ASSERT_NE(iGnssConfiguration, nullptr);

    auto result = iGnssConfiguration->setSuplEs(false);
    ASSERT_TRUE(result.isOk());
    EXPECT_FALSE(result);
}

/*
 * TestGnssConfiguration_setGpsLock_Deprecation:
 * Calls setGpsLock and verifies that it returns false.
 */
TEST_P(GnssHalTest, TestGnssConfiguration_setGpsLock_Deprecation) {
    auto gnssConfiguration = gnss_hal_->getExtensionGnssConfiguration_2_0();
    ASSERT_TRUE(gnssConfiguration.isOk());
    sp<IGnssConfiguration_2_0> iGnssConfiguration = gnssConfiguration;
    ASSERT_NE(iGnssConfiguration, nullptr);

    auto result = iGnssConfiguration->setGpsLock(0);
    ASSERT_TRUE(result.isOk());
    EXPECT_FALSE(result);
}

/*
 * TestAGnssRilExtension:
 * Gets the AGnssRilExtension and verifies that it returns an actual extension.
 *
 * If IAGnssRil interface is supported, then the GNSS HAL 2.0 implementation must support
 * @2.0::IAGnssRil interface due to the deprecation of framework network API methods needed
 * to support the @1.0::IAGnssRil interface.
 */
TEST_P(GnssHalTest, TestAGnssRilExtension) {
    auto agnssRil_2_0 = gnss_hal_->getExtensionAGnssRil_2_0();
    ASSERT_TRUE(agnssRil_2_0.isOk());
    sp<IAGnssRil_2_0> iAGnssRil_2_0 = agnssRil_2_0;
    if (iAGnssRil_2_0 == nullptr) {
        // Verify IAGnssRil 1.0 is not supported.
        auto agnssRil_1_0 = gnss_hal_->getExtensionAGnssRil();
        ASSERT_TRUE(agnssRil_1_0.isOk());
        sp<IAGnssRil_1_0> iAGnssRil_1_0 = agnssRil_1_0;
        ASSERT_EQ(iAGnssRil_1_0, nullptr);
    }
}

/*
 * TestAGnssRil_UpdateNetworkState_2_0:
 * 1. Updates GNSS HAL that a network has connected.
 * 2. Updates GNSS HAL that network has disconnected.
 */
TEST_P(GnssHalTest, TestAGnssRil_UpdateNetworkState_2_0) {
    auto agnssRil = gnss_hal_->getExtensionAGnssRil_2_0();
    ASSERT_TRUE(agnssRil.isOk());
    sp<IAGnssRil_2_0> iAGnssRil = agnssRil;
    if (iAGnssRil == nullptr) {
        return;
    }

    // Update GNSS HAL that a network has connected.
    IAGnssRil_2_0::NetworkAttributes networkAttributes = {
        .networkHandle = static_cast<uint64_t>(7700664333),
        .isConnected = true,
        .capabilities = static_cast<uint16_t>(IAGnssRil_2_0::NetworkCapability::NOT_ROAMING),
        .apn = "dummy-apn"};
    auto result = iAGnssRil->updateNetworkState_2_0(networkAttributes);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    // Update GNSS HAL that network has disconnected.
    networkAttributes.isConnected = false;
    result = iAGnssRil->updateNetworkState_2_0(networkAttributes);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

/*
 * TestGnssMeasurementFields:
 * Sets a GnssMeasurementCallback, waits for a measurement, and verifies
 * 1. codeType is valid,
 * 2. constellation is valid.
 * 3. state is valid.
 */
TEST_P(GnssHalTest, TestGnssMeasurementFields) {
    if (!IsGnssHalVersion_2_0()) {
        ALOGI("Test GnssMeasurementFields skipped. GNSS HAL version is greater than 2.0.");
        return;
    }
    const int kFirstGnssMeasurementTimeoutSeconds = 10;

    auto gnssMeasurement = gnss_hal_->getExtensionGnssMeasurement_2_0();
    if (!gnssMeasurement.isOk()) {
        return;
    }

    sp<IGnssMeasurement_2_0> iGnssMeasurement = gnssMeasurement;
    if (iGnssMeasurement == nullptr) {
        return;
    }

    sp<GnssMeasurementCallback> callback = new GnssMeasurementCallback();
    auto result = iGnssMeasurement->setCallback_2_0(callback, /* enableFullTracking= */ true);
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result, IGnssMeasurement_1_0::GnssMeasurementStatus::SUCCESS);

    IGnssMeasurementCallback_2_0::GnssData lastMeasurement;
    ASSERT_TRUE(callback->measurement_cbq_.retrieve(lastMeasurement,
                                                    kFirstGnssMeasurementTimeoutSeconds));
    EXPECT_EQ(callback->measurement_cbq_.calledCount(), 1);
    ASSERT_TRUE(lastMeasurement.measurements.size() > 0);
    for (auto measurement : lastMeasurement.measurements) {
        // Verify CodeType is valid.
        ASSERT_NE(measurement.codeType, "");

        // Verify ConstellationType is valid.
        ASSERT_TRUE(static_cast<uint8_t>(measurement.constellation) >=
                            static_cast<uint8_t>(GnssConstellationType_2_0::UNKNOWN) &&
                    static_cast<uint8_t>(measurement.constellation) <=
                            static_cast<uint8_t>(GnssConstellationType_2_0::IRNSS));

        // Verify State is valid.
        ASSERT_TRUE(
                static_cast<uint32_t>(measurement.state) >=
                        static_cast<uint32_t>(IGnssMeasurementCallback_2_0::GnssMeasurementState::
                                                      STATE_UNKNOWN) &&
                static_cast<uint32_t>(measurement.state) <=
                        static_cast<uint32_t>(IGnssMeasurementCallback_2_0::GnssMeasurementState::
                                                      STATE_2ND_CODE_LOCK));
    }

    iGnssMeasurement->close();
}

/*
 * TestAGnssExtension:
 * Gets the AGnssExtension and verifies that it returns an actual extension.
 *
 * If IAGnss interface is supported, then the GNSS HAL 2.0 implementation must support
 * @2.0::IAGnss interface due to the deprecation of framework network API methods needed
 * to support the @1.0::IAGnss interface.
 */
TEST_P(GnssHalTest, TestAGnssExtension) {
    auto agnss_2_0 = gnss_hal_->getExtensionAGnss_2_0();
    ASSERT_TRUE(agnss_2_0.isOk());
    sp<IAGnss_2_0> iAGnss_2_0 = agnss_2_0;
    if (iAGnss_2_0 == nullptr) {
        // Verify IAGnss 1.0 is not supported.
        auto agnss_1_0 = gnss_hal_->getExtensionAGnss();
        ASSERT_TRUE(agnss_1_0.isOk());
        sp<IAGnss_1_0> iAGnss_1_0 = agnss_1_0;
        ASSERT_EQ(iAGnss_1_0, nullptr);
        return;
    }

    // Set SUPL server host/port
    auto result =
            iAGnss_2_0->setServer(IAGnssCallback_2_0::AGnssType::SUPL, "supl.google.com", 7275);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

/*
 * TestGnssNiExtension_Deprecation:
 * Gets the @1.0::IGnssNi extension and verifies that it is a nullptr.
 */
TEST_P(GnssHalTest, TestGnssNiExtension_Deprecation) {
    // Verify IGnssNi 1.0 is not supported.
    auto gnssNi = gnss_hal_->getExtensionGnssNi();
    ASSERT_TRUE(!gnssNi.isOk() || ((sp<IGnssNi>)gnssNi) == nullptr);
}

/*
 * TestGnssVisibilityControlExtension:
 * Gets the GnssVisibilityControlExtension and if it is not null, verifies that it supports
 * the gnss.visibility_control@1.0::IGnssVisibilityControl interface by invoking a method.
 */
TEST_P(GnssHalTest, TestGnssVisibilityControlExtension) {
    auto gnssVisibilityControl = gnss_hal_->getExtensionVisibilityControl();
    ASSERT_TRUE(gnssVisibilityControl.isOk());
    sp<IGnssVisibilityControl> iGnssVisibilityControl = gnssVisibilityControl;
    if (iGnssVisibilityControl == nullptr) {
        return;
    }

    // Set non-framework proxy apps.
    hidl_vec<hidl_string> proxyApps{"com.example.ims", "com.example.mdt"};
    auto result = iGnssVisibilityControl->enableNfwLocationAccess(proxyApps);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

/*
 * TestGnssMeasurementCorrectionsCapabilities:
 * If measurement corrections capability is supported, verifies that the measurement corrections
 * capabilities are reported and the mandatory LOS_SATS or the EXCESS_PATH_LENGTH
 * capability flag is set.
 */
TEST_P(GnssHalTest, TestGnssMeasurementCorrectionsCapabilities) {
    if (!IsGnssHalVersion_2_0()) {
        ALOGI("Test GnssMeasurementCorrectionsCapabilities skipped. GNSS HAL version is greater "
              "than 2.0.");
        return;
    }

    if (!(gnss_cb_->last_capabilities_ & IGnssCallback::Capabilities::MEASUREMENT_CORRECTIONS)) {
        return;
    }

    auto measurementCorrections = gnss_hal_->getExtensionMeasurementCorrections();
    ASSERT_TRUE(measurementCorrections.isOk());
    sp<IMeasurementCorrections> iMeasurementCorrections = measurementCorrections;
    ASSERT_NE(iMeasurementCorrections, nullptr);

    // Setup measurement corrections callback.
    sp<GnssMeasurementCorrectionsCallback> callback = new GnssMeasurementCorrectionsCallback();
    iMeasurementCorrections->setCallback(callback);

    const int kMeasurementCorrectionsCapabilitiesTimeoutSeconds = 5;
    callback->capabilities_cbq_.retrieve(callback->last_capabilities_,
                                         kMeasurementCorrectionsCapabilitiesTimeoutSeconds);
    ASSERT_TRUE(callback->capabilities_cbq_.calledCount() > 0);
    using Capabilities = IMeasurementCorrectionsCallback::Capabilities;
    ASSERT_TRUE((callback->last_capabilities_ &
                 (Capabilities::LOS_SATS | Capabilities::EXCESS_PATH_LENGTH)) != 0);
}

/*
 * TestGnssMeasurementCorrections:
 * If measurement corrections capability is supported, verifies that it supports the
 * gnss.measurement_corrections@1.0::IMeasurementCorrections interface by invoking a method.
 */
TEST_P(GnssHalTest, TestGnssMeasurementCorrections) {
    if (!(gnss_cb_->last_capabilities_ & IGnssCallback::Capabilities::MEASUREMENT_CORRECTIONS)) {
        return;
    }

    // Verify IMeasurementCorrections is supported.
    auto measurementCorrections = gnss_hal_->getExtensionMeasurementCorrections();
    ASSERT_TRUE(measurementCorrections.isOk());
    sp<IMeasurementCorrections> iMeasurementCorrections = measurementCorrections;
    ASSERT_NE(iMeasurementCorrections, nullptr);

    sp<GnssMeasurementCorrectionsCallback> callback = new GnssMeasurementCorrectionsCallback();
    iMeasurementCorrections->setCallback(callback);

    const int kMeasurementCorrectionsCapabilitiesTimeoutSeconds = 5;
    callback->capabilities_cbq_.retrieve(callback->last_capabilities_,
                                         kMeasurementCorrectionsCapabilitiesTimeoutSeconds);
    ASSERT_TRUE(callback->capabilities_cbq_.calledCount() > 0);

    // Set a mock MeasurementCorrections.
    auto result = iMeasurementCorrections->setCorrections(Utils::getMockMeasurementCorrections());
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

/*
 * TestGnssDataElapsedRealtimeFlags:
 * Sets a GnssMeasurementCallback, waits for a GnssData object, and verifies the flags in member
 * elapsedRealitme are valid.
 */
TEST_P(GnssHalTest, TestGnssDataElapsedRealtimeFlags) {
    const int kFirstGnssMeasurementTimeoutSeconds = 10;

    auto gnssMeasurement = gnss_hal_->getExtensionGnssMeasurement_2_0();
    if (!gnssMeasurement.isOk()) {
        return;
    }

    sp<IGnssMeasurement_2_0> iGnssMeasurement = gnssMeasurement;
    if (iGnssMeasurement == nullptr) {
        return;
    }

    sp<GnssMeasurementCallback> callback = new GnssMeasurementCallback();
    auto result = iGnssMeasurement->setCallback_2_0(callback, /* enableFullTracking= */ true);
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result, IGnssMeasurement_1_0::GnssMeasurementStatus::SUCCESS);

    IGnssMeasurementCallback_2_0::GnssData lastMeasurement;
    ASSERT_TRUE(callback->measurement_cbq_.retrieve(lastMeasurement,
                                                    kFirstGnssMeasurementTimeoutSeconds));
    EXPECT_EQ(callback->measurement_cbq_.calledCount(), 1);

    ASSERT_TRUE((int)lastMeasurement.elapsedRealtime.flags <=
                (int)(ElapsedRealtimeFlags::HAS_TIMESTAMP_NS |
                      ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS));

    // We expect a non-zero timestamp when set.
    if (lastMeasurement.elapsedRealtime.flags & ElapsedRealtimeFlags::HAS_TIMESTAMP_NS) {
        ASSERT_TRUE(lastMeasurement.elapsedRealtime.timestampNs != 0);
    }

    iGnssMeasurement->close();
}

TEST_P(GnssHalTest, TestGnssLocationElapsedRealtime) {
    StartAndCheckFirstLocation();

    ASSERT_TRUE((int)gnss_cb_->last_location_.elapsedRealtime.flags <=
                (int)(ElapsedRealtimeFlags::HAS_TIMESTAMP_NS |
                      ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS));

    // We expect a non-zero timestamp when set.
    if (gnss_cb_->last_location_.elapsedRealtime.flags & ElapsedRealtimeFlags::HAS_TIMESTAMP_NS) {
        ASSERT_TRUE(gnss_cb_->last_location_.elapsedRealtime.timestampNs != 0);
    }

    StopAndClearLocations();
}

// This test only verify that injectBestLocation_2_0 does not crash.
TEST_P(GnssHalTest, TestInjectBestLocation_2_0) {
    StartAndCheckFirstLocation();
    gnss_hal_->injectBestLocation_2_0(gnss_cb_->last_location_);
    StopAndClearLocations();
}

/*
 * TestGnssBatchingExtension:
 * Gets the @2.0::IGnssBatching extension and verifies that it doesn't return an error. Support
 * for this interface is optional.
 */
TEST_P(GnssHalTest, TestGnssBatchingExtension) {
    auto gnssBatching_2_0 = gnss_hal_->getExtensionGnssBatching_2_0();
    ASSERT_TRUE(gnssBatching_2_0.isOk());
}

/*
 * GetLocationLowPower:
 * Turns on location, waits for at least 5 locations allowing max of LOCATION_TIMEOUT_SUBSEQUENT_SEC
 * between one location and the next. Also ensure that MIN_INTERVAL_MSEC is respected by waiting
 * NO_LOCATION_PERIOD_SEC and verfiy that no location is received. Also perform validity checks on
 * each received location.
 */
TEST_P(GnssHalTest, GetLocationLowPower) {
    if (!(gnss_cb_->last_capabilities_ & IGnssCallback::Capabilities::LOW_POWER_MODE)) {
        ALOGI("Test GetLocationLowPower skipped. LOW_POWER_MODE capability not supported.");
        return;
    }

    const int kMinIntervalMsec = 5000;
    const int kLocationTimeoutSubsequentSec = (kMinIntervalMsec / 1000) * 2;
    const int kNoLocationPeriodSec = (kMinIntervalMsec / 1000) / 2;
    const int kLocationsToCheck = 5;
    const bool kLowPowerMode = true;

    // Warmup period - VTS doesn't have AGPS access via GnssLocationProvider
    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(kLocationsToCheck);
    StopAndClearLocations();
    gnss_cb_->location_cbq_.reset();

    // Start of Low Power Mode test
    SetPositionMode(kMinIntervalMsec, kLowPowerMode);

    // Don't expect true - as without AGPS access
    if (!StartAndCheckFirstLocation()) {
        ALOGW("GetLocationLowPower test - no first low power location received.");
    }

    for (int i = 1; i < kLocationsToCheck; i++) {
        // Verify that kMinIntervalMsec is respected by waiting kNoLocationPeriodSec and
        // ensure that no location is received yet

        gnss_cb_->location_cbq_.retrieve(gnss_cb_->last_location_, kNoLocationPeriodSec);
        const int location_called_count = gnss_cb_->location_cbq_.calledCount();

        // Tolerate (ignore) one extra location right after the first one
        // to handle startup edge case scheduling limitations in some implementations
        if ((i == 1) && (location_called_count == 2)) {
            CheckLocation(gnss_cb_->last_location_, true);
            continue;  // restart the quiet wait period after this too-fast location
        }
        EXPECT_LE(location_called_count, i);
        if (location_called_count != i) {
            ALOGW("GetLocationLowPower test - too many locations received. %d vs. %d expected ",
                  location_called_count, i);
        }

        if (!gnss_cb_->location_cbq_.retrieve(
                    gnss_cb_->last_location_,
                    kLocationTimeoutSubsequentSec - kNoLocationPeriodSec)) {
            ALOGW("GetLocationLowPower test - timeout awaiting location %d", i);
        } else {
            CheckLocation(gnss_cb_->last_location_, true);
        }
    }

    StopAndClearLocations();
}

/*
 * MapConstellationType:
 * Given a GnssConstellationType_2_0 type constellation, maps to its equivalent
 * GnssConstellationType_1_0 type constellation. For constellations that do not have
 * an equivalent value, maps to GnssConstellationType_1_0::UNKNOWN
 */
GnssConstellationType_1_0 MapConstellationType(GnssConstellationType_2_0 constellation) {
    switch (constellation) {
        case GnssConstellationType_2_0::GPS:
            return GnssConstellationType_1_0::GPS;
        case GnssConstellationType_2_0::SBAS:
            return GnssConstellationType_1_0::SBAS;
        case GnssConstellationType_2_0::GLONASS:
            return GnssConstellationType_1_0::GLONASS;
        case GnssConstellationType_2_0::QZSS:
            return GnssConstellationType_1_0::QZSS;
        case GnssConstellationType_2_0::BEIDOU:
            return GnssConstellationType_1_0::BEIDOU;
        case GnssConstellationType_2_0::GALILEO:
            return GnssConstellationType_1_0::GALILEO;
        default:
            return GnssConstellationType_1_0::UNKNOWN;
    }
}

/*
 * FindStrongFrequentNonGpsSource:
 *
 * Search through a GnssSvStatus list for the strongest non-GPS satellite observed enough times
 *
 * returns the strongest source,
 *         or a source with constellation == UNKNOWN if none are found sufficient times
 */
IGnssConfiguration_1_1::BlacklistedSource FindStrongFrequentNonGpsSource(
        const std::list<hidl_vec<IGnssCallback_2_0::GnssSvInfo>>& sv_info_lists,
        const int min_observations) {
    struct ComparableBlacklistedSource {
        IGnssConfiguration_1_1::BlacklistedSource id;

        ComparableBlacklistedSource() {
            id.constellation = GnssConstellationType_1_0::UNKNOWN;
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

    for (const auto& sv_info_list : sv_info_lists) {
        for (IGnssCallback_2_0::GnssSvInfo sv_info : sv_info_list) {
            if ((sv_info.v1_0.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX) &&
                (sv_info.constellation != GnssConstellationType_2_0::IRNSS) &&
                (sv_info.constellation != GnssConstellationType_2_0::GPS)) {
                ComparableBlacklistedSource source;
                source.id.svid = sv_info.v1_0.svid;
                source.id.constellation = MapConstellationType(sv_info.constellation);

                const auto& itSignal = mapSignals.find(source);
                if (itSignal == mapSignals.end()) {
                    SignalCounts counts;
                    counts.observations = 1;
                    counts.max_cn0_dbhz = sv_info.v1_0.cN0Dbhz;
                    mapSignals.insert(
                            std::pair<ComparableBlacklistedSource, SignalCounts>(source, counts));
                } else {
                    itSignal->second.observations++;
                    if (itSignal->second.max_cn0_dbhz < sv_info.v1_0.cN0Dbhz) {
                        itSignal->second.max_cn0_dbhz = sv_info.v1_0.cN0Dbhz;
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
    if (!IsGnssHalVersion_2_0()) {
        ALOGI("Test BlacklistIndividualSatellites skipped. GNSS HAL version is greater than 2.0.");
        return;
    }

    if (!(gnss_cb_->last_capabilities_ & IGnssCallback::Capabilities::SATELLITE_BLACKLIST)) {
        ALOGI("Test BlacklistIndividualSatellites skipped. SATELLITE_BLACKLIST capability"
              " not supported.");
        return;
    }

    const int kLocationsToAwait = 3;
    const int kRetriesToUnBlacklist = 10;

    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(kLocationsToAwait);
    int location_called_count = gnss_cb_->location_cbq_.calledCount();

    // Tolerate 1 less sv status to handle edge cases in reporting.
    int sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, kLocationsToAwait);
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations (%d received)",
          sv_info_list_cbq_size, kLocationsToAwait, location_called_count);

    /*
     * Identify strongest SV seen at least kLocationsToAwait -1 times
     * Why -1?  To avoid test flakiness in case of (plausible) slight flakiness in strongest signal
     * observability (one epoch RF null)
     */

    const int kGnssSvStatusTimeout = 2;
    std::list<hidl_vec<IGnssCallback_2_0::GnssSvInfo>> sv_info_lists;
    int count = gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_lists, sv_info_list_cbq_size,
                                                     kGnssSvStatusTimeout);
    ASSERT_EQ(count, sv_info_list_cbq_size);

    IGnssConfiguration_1_1::BlacklistedSource source_to_blacklist =
            FindStrongFrequentNonGpsSource(sv_info_lists, kLocationsToAwait - 1);

    if (source_to_blacklist.constellation == GnssConstellationType_1_0::UNKNOWN) {
        // Cannot find a non-GPS satellite. Let the test pass.
        return;
    }

    // Stop locations, blacklist the common SV
    StopAndClearLocations();

    auto gnss_configuration_hal_return = gnss_hal_->getExtensionGnssConfiguration_1_1();
    ASSERT_TRUE(gnss_configuration_hal_return.isOk());
    sp<IGnssConfiguration_1_1> gnss_configuration_hal = gnss_configuration_hal_return;
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<IGnssConfiguration_1_1::BlacklistedSource> sources;
    sources.resize(1);
    sources[0] = source_to_blacklist;

    auto result = gnss_configuration_hal->setBlacklist(sources);
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
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations (%d received)",
          sv_info_list_cbq_size, kLocationsToAwait, location_called_count);
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        hidl_vec<IGnssCallback_2_0::GnssSvInfo> sv_info_list;
        gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_list, kGnssSvStatusTimeout);
        for (IGnssCallback_2_0::GnssSvInfo sv_info : sv_info_list) {
            auto constellation = MapConstellationType(sv_info.constellation);
            EXPECT_FALSE((sv_info.v1_0.svid == source_to_blacklist.svid) &&
                         (constellation == source_to_blacklist.constellation) &&
                         (sv_info.v1_0.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clear blacklist and restart - this time updating the blacklist while location is still on
    sources.resize(0);

    result = gnss_configuration_hal->setBlacklist(sources);
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
        ALOGD("Clear blacklist, observed %d GnssSvStatus, while awaiting %d Locations"
              ", tries remaining %d",
              sv_info_list_cbq_size, kLocationsToAwait, unblacklist_loops_remaining);

        for (int i = 0; i < sv_info_list_cbq_size; ++i) {
            hidl_vec<IGnssCallback_2_0::GnssSvInfo> sv_info_list;
            gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_list, kGnssSvStatusTimeout);
            for (IGnssCallback_2_0::GnssSvInfo sv_info : sv_info_list) {
                auto constellation = MapConstellationType(sv_info.constellation);
                if ((sv_info.v1_0.svid == source_to_blacklist.svid) &&
                    (constellation == source_to_blacklist.constellation) &&
                    (sv_info.v1_0.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX)) {
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
 * BlacklistConstellation:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for any non-GPS constellations.
 * 2a & b) Turns off location, and blacklist first non-GPS constellations.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use any constellation but GPS.
 * 4a & b) Clean up by turning off location, and send in empty blacklist.
 */
TEST_P(GnssHalTest, BlacklistConstellation) {
    if (!IsGnssHalVersion_2_0()) {
        ALOGI("Test BlacklistConstellation skipped. GNSS HAL version is greater than 2.0.");
        return;
    }

    if (!(gnss_cb_->last_capabilities_ & IGnssCallback::Capabilities::SATELLITE_BLACKLIST)) {
        ALOGI("Test BlacklistConstellation skipped. SATELLITE_BLACKLIST capability not supported.");
        return;
    }

    const int kLocationsToAwait = 3;

    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(kLocationsToAwait);
    const int location_called_count = gnss_cb_->location_cbq_.calledCount();

    // Tolerate 1 less sv status to handle edge cases in reporting.
    int sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, kLocationsToAwait);
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations (%d received)",
          sv_info_list_cbq_size, kLocationsToAwait, location_called_count);

    // Find first non-GPS constellation to blacklist. Exclude IRNSS in GnssConstellationType_2_0
    // as blacklisting of this constellation is not supported in gnss@2.0.
    const int kGnssSvStatusTimeout = 2;
    GnssConstellationType_1_0 constellation_to_blacklist = GnssConstellationType_1_0::UNKNOWN;
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        hidl_vec<IGnssCallback_2_0::GnssSvInfo> sv_info_list;
        gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_list, kGnssSvStatusTimeout);
        for (IGnssCallback_2_0::GnssSvInfo sv_info : sv_info_list) {
            if ((sv_info.v1_0.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX) &&
                (sv_info.constellation != GnssConstellationType_2_0::UNKNOWN) &&
                (sv_info.constellation != GnssConstellationType_2_0::IRNSS) &&
                (sv_info.constellation != GnssConstellationType_2_0::GPS)) {
                // found a non-GPS V1_0 constellation
                constellation_to_blacklist = MapConstellationType(sv_info.constellation);
                break;
            }
        }
        if (constellation_to_blacklist != GnssConstellationType_1_0::UNKNOWN) {
            break;
        }
    }

    if (constellation_to_blacklist == GnssConstellationType_1_0::UNKNOWN) {
        ALOGI("No non-GPS constellations found, constellation blacklist test less effective.");
        // Proceed functionally to blacklist something.
        constellation_to_blacklist = GnssConstellationType_1_0::GLONASS;
    }
    IGnssConfiguration_1_1::BlacklistedSource source_to_blacklist;
    source_to_blacklist.constellation = constellation_to_blacklist;
    source_to_blacklist.svid = 0;  // documented wildcard for all satellites in this constellation

    auto gnss_configuration_hal_return = gnss_hal_->getExtensionGnssConfiguration_1_1();
    ASSERT_TRUE(gnss_configuration_hal_return.isOk());
    sp<IGnssConfiguration_1_1> gnss_configuration_hal = gnss_configuration_hal_return;
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<IGnssConfiguration_1_1::BlacklistedSource> sources;
    sources.resize(1);
    sources[0] = source_to_blacklist;

    auto result = gnss_configuration_hal->setBlacklist(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    // retry and ensure constellation not used
    gnss_cb_->sv_info_list_cbq_.reset();

    gnss_cb_->location_cbq_.reset();
    StartAndCheckLocations(kLocationsToAwait);

    // Tolerate 1 less sv status to handle edge cases in reporting.
    sv_info_list_cbq_size = gnss_cb_->sv_info_list_cbq_.size();
    EXPECT_GE(sv_info_list_cbq_size + 1, kLocationsToAwait);
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations", sv_info_list_cbq_size,
          kLocationsToAwait);
    for (int i = 0; i < sv_info_list_cbq_size; ++i) {
        hidl_vec<IGnssCallback_2_0::GnssSvInfo> sv_info_list;
        gnss_cb_->sv_info_list_cbq_.retrieve(sv_info_list, kGnssSvStatusTimeout);
        for (IGnssCallback_2_0::GnssSvInfo sv_info : sv_info_list) {
            auto constellation = MapConstellationType(sv_info.constellation);
            EXPECT_FALSE((constellation == source_to_blacklist.constellation) &&
                         (sv_info.v1_0.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clean up
    StopAndClearLocations();
    sources.resize(0);
    result = gnss_configuration_hal->setBlacklist(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}