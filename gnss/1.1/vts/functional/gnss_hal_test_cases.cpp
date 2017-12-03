/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <gnss_hal_test.h>

#include <VtsHalHidlTargetTestBase.h>

using android::hardware::gnss::V1_1::IGnssMeasurement;

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_F(GnssHalTest, SetupTeardownCreateCleanup) {}

/*
 * SetCallbackResponses:
 * Sets up the callback, awaits the capability, info & name
 */
TEST_F(GnssHalTest, SetCallbackResponses) {
    gnss_cb_ = new GnssCallback(*this);
    ASSERT_NE(gnss_cb_, nullptr);

    auto result = gnss_hal_->setCallback_1_1(gnss_cb_);
    if (!result.isOk()) {
        ALOGE("result of failed setCallback %s", result.description().c_str());
    }

    ASSERT_TRUE(result.isOk());
    ASSERT_TRUE(result);

    /*
     * All capabilities, name and systemInfo callbacks should trigger
     */
    EXPECT_EQ(std::cv_status::no_timeout, wait(TIMEOUT_SEC));
    EXPECT_EQ(std::cv_status::no_timeout, wait(TIMEOUT_SEC));
    EXPECT_EQ(std::cv_status::no_timeout, wait(TIMEOUT_SEC));

    EXPECT_EQ(capabilities_called_count_, 1);
    EXPECT_EQ(info_called_count_, 1);
    EXPECT_EQ(name_called_count_, 1);
}

/*
 * TestGnssMeasurementCallback:
 * Gets the GnssMeasurementExtension and verify that it returns an actual extension.
 */
TEST_F(GnssHalTest, TestGnssMeasurementCallback) {
    auto gnssMeasurement = gnss_hal_->getExtensionGnssMeasurement_1_1();
    ASSERT_TRUE(gnssMeasurement.isOk());
    if (last_capabilities_ & IGnssCallback::Capabilities::MEASUREMENTS) {
        sp<IGnssMeasurement> iGnssMeas = gnssMeasurement;
        EXPECT_NE(iGnssMeas, nullptr);
    }
}

/*
 * GetLocation:
 * Turns on location, waits for at least 5 locations allowing max of LOCATION_TIMEOUT_SUBSEQUENT_SEC
 * between one location and the next. Also ensure that MIN_INTERVAL_MSEC is respected by waiting
 * NO_LOCATION_PERIOD_SEC and verfiy that no location is received. Also perform validity checks on
 * each received location.
 */
TEST_F(GnssHalTest, GetLocationLowPower) {
#define MIN_INTERVAL_MSEC 5000
#define PREFERRED_ACCURACY 0   // Ideally perfect (matches GnssLocationProvider)
#define PREFERRED_TIME_MSEC 0  // Ideally immediate

#define LOCATION_TIMEOUT_SUBSEQUENT_SEC (MIN_INTERVAL_MSEC + 1000) / 1000
#define NO_LOCATION_PERIOD_SEC 2
#define LOCATIONS_TO_CHECK 5
#define LOW_POWER_MODE true

    bool checkMoreAccuracies = (info_called_count_ > 0 && last_info_.yearOfHw >= 2017);

    auto result = gnss_hal_->setPositionMode_1_1(
        IGnss::GnssPositionMode::MS_BASED, IGnss::GnssPositionRecurrence::RECURRENCE_PERIODIC,
        MIN_INTERVAL_MSEC, PREFERRED_ACCURACY, PREFERRED_TIME_MSEC, LOW_POWER_MODE);

    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    EXPECT_TRUE(StartAndGetSingleLocation(checkMoreAccuracies));

    for (int i = 1; i < LOCATIONS_TO_CHECK; i++) {
        // Verify that MIN_INTERVAL_MSEC is respected by waiting NO_LOCATION_PERIOD_SEC and
        // ensure that no location is received yet
        wait(NO_LOCATION_PERIOD_SEC);
        EXPECT_EQ(location_called_count_, i);
        EXPECT_EQ(std::cv_status::no_timeout,
                  wait(LOCATION_TIMEOUT_SUBSEQUENT_SEC - NO_LOCATION_PERIOD_SEC));
        EXPECT_EQ(location_called_count_, i + 1);
        CheckLocation(last_location_, checkMoreAccuracies, true);
    }

    StopAndClearLocations();
}