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

using android::hardware::hidl_string;
using android::hardware::hidl_vec;

using IGnssConfiguration_2_0 = android::hardware::gnss::V2_0::IGnssConfiguration;
using IAGnssRil_2_0 = android::hardware::gnss::V2_0::IAGnssRil;
using IGnssMeasurement_2_0 = android::hardware::gnss::V2_0::IGnssMeasurement;
using IGnssMeasurement_1_1 = android::hardware::gnss::V1_1::IGnssMeasurement;
using IGnssMeasurement_1_0 = android::hardware::gnss::V1_0::IGnssMeasurement;
using IAGnssRil_2_0 = android::hardware::gnss::V2_0::IAGnssRil;
using IAGnss_2_0 = android::hardware::gnss::V2_0::IAGnss;
using IAGnss_1_0 = android::hardware::gnss::V1_0::IAGnss;
using IAGnssCallback_2_0 = android::hardware::gnss::V2_0::IAGnssCallback;

using android::hardware::gnss::V1_0::IGnssNi;
using android::hardware::gnss::V2_0::ElapsedRealtimeFlags;
using android::hardware::gnss::visibility_control::V1_0::IGnssVisibilityControl;

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_F(GnssHalTest, SetupTeardownCreateCleanup) {}

/*
 * TestGnssMeasurementCallback:
 * Gets the GnssMeasurementExtension and verify that it returns an actual extension.
 */
TEST_F(GnssHalTest, TestGnssMeasurementCallback) {
    auto gnssMeasurement_2_0 = gnss_hal_->getExtensionGnssMeasurement_2_0();
    auto gnssMeasurement_1_1 = gnss_hal_->getExtensionGnssMeasurement_1_1();
    auto gnssMeasurement_1_0 = gnss_hal_->getExtensionGnssMeasurement();
    ASSERT_TRUE(gnssMeasurement_2_0.isOk() || gnssMeasurement_1_1.isOk() ||
                gnssMeasurement_1_0.isOk());
    if (last_capabilities_ & IGnssCallback::Capabilities::MEASUREMENTS) {
        sp<IGnssMeasurement_2_0> iGnssMeas_2_0 = gnssMeasurement_2_0;
        sp<IGnssMeasurement_1_1> iGnssMeas_1_1 = gnssMeasurement_1_1;
        sp<IGnssMeasurement_1_0> iGnssMeas_1_0 = gnssMeasurement_1_0;
        // At least one interface is non-null.
        int numNonNull = (int)(iGnssMeas_2_0 != nullptr) + (int)(iGnssMeas_1_1 != nullptr) +
                         (int)(iGnssMeas_1_0 != nullptr);
        ASSERT_TRUE(numNonNull >= 1);
    }
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
 * The GNSS HAL 2.0 implementation must support @2.0::IAGnssRil interface due to the deprecation
 * of framework network API methods needed to support the @1.0::IAGnssRil interface.
 *
 * TODO (b/121287858): Enforce gnss@2.0 HAL package is supported on devices launched with Q or later
 */
TEST_F(GnssHalTest, TestAGnssRilExtension) {
    auto agnssRil = gnss_hal_->getExtensionAGnssRil_2_0();
    ASSERT_TRUE(agnssRil.isOk());
    sp<IAGnssRil_2_0> iAGnssRil = agnssRil;
    ASSERT_NE(iAGnssRil, nullptr);
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
    ASSERT_NE(iAGnssRil, nullptr);

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
 * TestGnssMeasurementCodeType:
 * Sets a GnssMeasurementCallback, waits for a measurement, and verifies the codeType is valid.
 */
TEST_F(GnssHalTest, TestGnssMeasurementCodeType) {
    const int kFirstGnssMeasurementTimeoutSeconds = 10;

    auto gnssMeasurement = gnss_hal_->getExtensionGnssMeasurement_2_0();
    if (!gnssMeasurement.isOk()) {
        return;
    }

    sp<IGnssMeasurement_2_0> iGnssMeasurement = gnssMeasurement;
    if (iGnssMeasurement == nullptr) {
        return;
    }

    sp<IGnssMeasurementCallback_2_0> callback = new GnssMeasurementCallback(*this);

    auto result = iGnssMeasurement->setCallback_2_0(callback, /* enableFullTracking= */ true);
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result, IGnssMeasurement_1_0::GnssMeasurementStatus::SUCCESS);

    wait(kFirstGnssMeasurementTimeoutSeconds);
    EXPECT_EQ(measurement_called_count_, 1);
    ASSERT_TRUE(last_measurement_.measurements.size() > 0);
    for (auto measurement : last_measurement_.measurements) {
        ASSERT_TRUE(
                ((int)measurement.codeType >=
                                (int)IGnssMeasurementCallback_2_0::GnssMeasurementCodeType::A &&
                        (int)measurement.codeType <=
                                (int)IGnssMeasurementCallback_2_0::GnssMeasurementCodeType::N) ||
                (int)measurement.codeType ==
                        (int)IGnssMeasurementCallback_2_0::GnssMeasurementCodeType::OTHER);
        if ((int)measurement.codeType ==
                (int)IGnssMeasurementCallback_2_0::GnssMeasurementCodeType::OTHER) {
            ASSERT_NE(measurement.otherCodeTypeName, "");
        }
    }

    iGnssMeasurement->close();
}

/*
 * TestAGnssExtension:
 * Gets the AGnssExtension and verifies that it supports @2.0::IAGnss interface by invoking
 * a method.
 *
 * The GNSS HAL 2.0 implementation must support @2.0::IAGnss interface due to the deprecation
 * of framework network API methods needed to support the @1.0::IAGnss interface.
 *
 * TODO (b/121287858): Enforce gnss@2.0 HAL package is supported on devices launched with Q or later
 */
TEST_F(GnssHalTest, TestAGnssExtension) {
    // Verify IAGnss 2.0 is supported.
    auto agnss = gnss_hal_->getExtensionAGnss_2_0();
    ASSERT_TRUE(agnss.isOk());
    sp<IAGnss_2_0> iAGnss = agnss;
    ASSERT_NE(iAGnss, nullptr);

    // Set SUPL server host/port
    auto result = iAGnss->setServer(IAGnssCallback_2_0::AGnssType::SUPL, "supl.google.com", 7275);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

/*
 * TestAGnssExtension_1_0_Deprecation:
 * Gets the @1.0::IAGnss extension and verifies that it is a nullptr.
 *
 * TODO (b/121287858): Enforce gnss@2.0 HAL package is supported on devices launched with Q or later
 */
TEST_F(GnssHalTest, TestAGnssExtension_1_0_Deprecation) {
    // Verify IAGnss 1.0 is not supported.
    auto agnss_1_0 = gnss_hal_->getExtensionAGnss();
    ASSERT_TRUE(!agnss_1_0.isOk() || ((sp<IAGnss_1_0>)agnss_1_0) == nullptr);
}

/*
 * TestGnssNiExtension_Deprecation:
 * Gets the @1.0::IGnssNi extension and verifies that it is a nullptr.
 *
 * TODO (b/121287858): Enforce gnss@2.0 HAL package is supported on devices launched with Q or later
 */
TEST_F(GnssHalTest, TestGnssNiExtension_Deprecation) {
    // Verify IGnssNi 1.0 is not supported.
    auto gnssNi = gnss_hal_->getExtensionGnssNi();
    ASSERT_TRUE(!gnssNi.isOk() || ((sp<IGnssNi>)gnssNi) == nullptr);
}

/*
 * TestGnssVisibilityControlExtension:
 * Gets the GnssVisibilityControlExtension and verifies that it supports the
 * gnss.visibility_control@1.0::IGnssVisibilityControl interface by invoking a method.
 *
 * The GNSS HAL 2.0 implementation must support gnss.visibility_control@1.0::IGnssVisibilityControl.
 *
 * TODO (b/121287858): Enforce gnss@2.0 HAL package is supported on devices launched with Q or later
 */
TEST_F(GnssHalTest, TestGnssVisibilityControlExtension) {
    // Verify IGnssVisibilityControl is supported.
    auto gnssVisibilityControl = gnss_hal_->getExtensionVisibilityControl();
    ASSERT_TRUE(gnssVisibilityControl.isOk());
    sp<IGnssVisibilityControl> iGnssVisibilityControl = gnssVisibilityControl;
    ASSERT_NE(iGnssVisibilityControl, nullptr);

    // Set non-framework proxy apps.
    hidl_vec<hidl_string> proxyApps{"ims.example.com", "mdt.example.com"};
    auto result = iGnssVisibilityControl->enableNfwLocationAccess(proxyApps);
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

    sp<IGnssMeasurementCallback_2_0> callback = new GnssMeasurementCallback(*this);

    auto result = iGnssMeasurement->setCallback_2_0(callback, /* enableFullTracking= */ true);
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result, IGnssMeasurement_1_0::GnssMeasurementStatus::SUCCESS);

    wait(kFirstGnssMeasurementTimeoutSeconds);
    EXPECT_EQ(measurement_called_count_, 1);

    ASSERT_TRUE((int)last_measurement_.elapsedRealtime.flags >= 0 &&
                (int)last_measurement_.elapsedRealtime.flags <=
                        (int)ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS);

    // We expect a non-zero timestamp when set.
    if (last_measurement_.elapsedRealtime.flags & ElapsedRealtimeFlags::HAS_TIMESTAMP_NS) {
        ASSERT_TRUE(last_measurement_.elapsedRealtime.timestampNs != 0);
    }

    iGnssMeasurement->close();
}

TEST_F(GnssHalTest, TestGnssLocationElapsedRealtime) {
    StartAndCheckFirstLocation();

    ASSERT_TRUE((int)last_location_.elapsedRealtime.flags >= 0 &&
                (int)last_location_.elapsedRealtime.flags <=
                        (int)ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS);

    // We expect a non-zero timestamp when set.
    if (last_location_.elapsedRealtime.flags & ElapsedRealtimeFlags::HAS_TIMESTAMP_NS) {
        ASSERT_TRUE(last_location_.elapsedRealtime.timestampNs != 0);
    }

    StopAndClearLocations();
}

// This test only verify that injectBestLocation_2_0 does not crash.
TEST_F(GnssHalTest, TestInjectBestLocation_2_0) {
    StartAndCheckFirstLocation();
    gnss_hal_->injectBestLocation_2_0(last_location_);
    StopAndClearLocations();
}
