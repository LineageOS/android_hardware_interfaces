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

using android::hardware::hidl_vec;

using IGnssMeasurement_2_0 = android::hardware::gnss::V2_0::IGnssMeasurement;
using IGnssMeasurement_1_1 = android::hardware::gnss::V1_1::IGnssMeasurement;
using IAGnssRil_2_0 = android::hardware::gnss::V2_0::IAGnssRil;

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
    ASSERT_TRUE(gnssMeasurement_2_0.isOk() || gnssMeasurement_1_1.isOk());
    if (last_capabilities_ & IGnssCallback::Capabilities::MEASUREMENTS) {
        sp<IGnssMeasurement_2_0> iGnssMeas_2_0 = gnssMeasurement_2_0;
        sp<IGnssMeasurement_1_1> iGnssMeas_1_1 = gnssMeasurement_1_1;
        // Exactly one interface is non-null.
        ASSERT_TRUE((iGnssMeas_1_1 != nullptr) != (iGnssMeas_2_0 != nullptr));
    }
}

/*
 * TestAGnssRilExtension:
 * Gets the AGnssRilExtension and verifies that it returns an actual extension.
 *
 * The GNSS HAL 2.0 implementation must support @2.0::IAGnssRil interface due to the deprecation
 * of framework network API methods needed to support the @1::IAGnssRil interface.
 */
TEST_F(GnssHalTest, TestAGnssRilExtension) {
    auto agnssRil = gnss_hal_->getExtensionAGnssRil_2_0();
    ASSERT_TRUE(agnssRil.isOk());
    sp<IAGnssRil_2_0> iAGnssRil = agnssRil;
    ASSERT_NE(iAGnssRil, nullptr);
}

/*
 * TestAGnssRilUpdateNetworkState_2_0:
 * 1. Update GNSS HAL that a network has connected.
 * 2. Update GNSS HAL that network has disconnected.
 */
TEST_F(GnssHalTest, TestAGnssRilUpdateNetworkState_2_0) {
    auto agnssRil = gnss_hal_->getExtensionAGnssRil_2_0();
    ASSERT_TRUE(agnssRil.isOk());
    sp<IAGnssRil_2_0> iAGnssRil = agnssRil;
    ASSERT_NE(iAGnssRil, nullptr);

    // Update GNSS HAL that a network is connected.
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
