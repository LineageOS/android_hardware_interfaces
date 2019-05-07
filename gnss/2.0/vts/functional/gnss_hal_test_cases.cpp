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

#include <VtsHalHidlTargetTestBase.h>
#include <gnss_hal_test.h>
#include "Utils.h"

using android::hardware::hidl_string;
using android::hardware::hidl_vec;

using IGnssConfiguration_2_0 = android::hardware::gnss::V2_0::IGnssConfiguration;
using IAGnssRil_2_0 = android::hardware::gnss::V2_0::IAGnssRil;
using IGnssMeasurement_2_0 = android::hardware::gnss::V2_0::IGnssMeasurement;
using IGnssMeasurement_1_1 = android::hardware::gnss::V1_1::IGnssMeasurement;
using IGnssMeasurement_1_0 = android::hardware::gnss::V1_0::IGnssMeasurement;
using IAGnssRil_2_0 = android::hardware::gnss::V2_0::IAGnssRil;
using IAGnssRil_1_0 = android::hardware::gnss::V1_0::IAGnssRil;
using IAGnss_2_0 = android::hardware::gnss::V2_0::IAGnss;
using IAGnss_1_0 = android::hardware::gnss::V1_0::IAGnss;
using IAGnssCallback_2_0 = android::hardware::gnss::V2_0::IAGnssCallback;
using IGnssBatching_V1_0 = android::hardware::gnss::V1_0::IGnssBatching;
using IGnssBatching_V2_0 = android::hardware::gnss::V2_0::IGnssBatching;

using android::hardware::gnss::common::Utils;
using android::hardware::gnss::measurement_corrections::V1_0::IMeasurementCorrections;
using android::hardware::gnss::measurement_corrections::V1_0::MeasurementCorrections;
using android::hardware::gnss::V1_0::IGnssNi;
using android::hardware::gnss::V2_0::ElapsedRealtimeFlags;
using android::hardware::gnss::V2_0::GnssConstellationType;
using android::hardware::gnss::V2_0::IGnssCallback;
using android::hardware::gnss::visibility_control::V1_0::IGnssVisibilityControl;

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_F(GnssHalTest, SetupTeardownCreateCleanup) {}

/*
 * TestGnssMeasurementExtension:
 * Gets the GnssMeasurementExtension and verifies that it returns an actual extension.
 */
TEST_F(GnssHalTest, TestGnssMeasurementExtension) {
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
TEST_F(GnssHalTest, TestGnssConfigurationExtension) {
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
TEST_F(GnssHalTest, TestGnssConfiguration_setSuplEs_Deprecation) {
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
TEST_F(GnssHalTest, TestGnssConfiguration_setGpsLock_Deprecation) {
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
TEST_F(GnssHalTest, TestAGnssRilExtension) {
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
TEST_F(GnssHalTest, TestAGnssRil_UpdateNetworkState_2_0) {
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
TEST_F(GnssHalTest, TestGnssMeasurementFields) {
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
                            static_cast<uint8_t>(GnssConstellationType::UNKNOWN) &&
                    static_cast<uint8_t>(measurement.constellation) <=
                            static_cast<uint8_t>(GnssConstellationType::IRNSS));

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
TEST_F(GnssHalTest, TestAGnssExtension) {
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
TEST_F(GnssHalTest, TestGnssNiExtension_Deprecation) {
    // Verify IGnssNi 1.0 is not supported.
    auto gnssNi = gnss_hal_->getExtensionGnssNi();
    ASSERT_TRUE(!gnssNi.isOk() || ((sp<IGnssNi>)gnssNi) == nullptr);
}

/*
 * TestGnssVisibilityControlExtension:
 * Gets the GnssVisibilityControlExtension and if it is not null, verifies that it supports
 * the gnss.visibility_control@1.0::IGnssVisibilityControl interface by invoking a method.
 */
TEST_F(GnssHalTest, TestGnssVisibilityControlExtension) {
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
TEST_F(GnssHalTest, TestGnssMeasurementCorrectionsCapabilities) {
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
TEST_F(GnssHalTest, TestGnssMeasurementCorrections) {
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
TEST_F(GnssHalTest, TestGnssDataElapsedRealtimeFlags) {
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

TEST_F(GnssHalTest, TestGnssLocationElapsedRealtime) {
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
TEST_F(GnssHalTest, TestInjectBestLocation_2_0) {
    StartAndCheckFirstLocation();
    gnss_hal_->injectBestLocation_2_0(gnss_cb_->last_location_);
    StopAndClearLocations();
}

/*
 * TestGnssBatchingExtension:
 * Gets the @2.0::IGnssBatching extension and verifies that it doesn't return an error. Support
 * for this interface is optional.
 */
TEST_F(GnssHalTest, TestGnssBatchingExtension) {
    auto gnssBatching_2_0 = gnss_hal_->getExtensionGnssBatching_2_0();
    ASSERT_TRUE(gnssBatching_2_0.isOk());
}
