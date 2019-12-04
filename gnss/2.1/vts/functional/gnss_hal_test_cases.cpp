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
#include "Utils.h"

#include <gtest/gtest.h>

using android::hardware::hidl_string;
using android::hardware::hidl_vec;

using android::hardware::gnss::common::Utils;

using IGnssMeasurement_2_1 = android::hardware::gnss::V2_1::IGnssMeasurement;
using IGnssMeasurement_2_0 = android::hardware::gnss::V2_0::IGnssMeasurement;
using IGnssMeasurement_1_1 = android::hardware::gnss::V1_1::IGnssMeasurement;
using IGnssMeasurement_1_0 = android::hardware::gnss::V1_0::IGnssMeasurement;

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
 * TestGnssMeasurementFields:
 * Sets a GnssMeasurementCallback, waits for a measurement, and verifies
 * 1. basebandCN0DbHz is valid
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
