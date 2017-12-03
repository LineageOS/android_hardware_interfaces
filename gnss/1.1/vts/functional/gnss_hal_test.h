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

#ifndef GNSS_HAL_TEST_H_
#define GNSS_HAL_TEST_H_

#include <android/hardware/gnss/1.1/IGnss.h>

#include <VtsHalHidlTargetTestBase.h>

#include <condition_variable>
#include <mutex>

using android::hardware::Return;
using android::hardware::Void;

using android::hardware::gnss::V1_0::GnssLocation;

using android::hardware::gnss::V1_1::IGnss;
using android::hardware::gnss::V1_1::IGnssCallback;
using android::hardware::gnss::V1_0::GnssLocationFlags;
using android::sp;
#define TIMEOUT_SEC 2  // for basic commands/responses

// The main test class for GNSS HAL.
class GnssHalTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    GnssHalTest();

    virtual void SetUp() override;

    virtual void TearDown() override;

    /* Used as a mechanism to inform the test that a callback has occurred */
    void notify();

    /* Test code calls this function to wait for a callback */
    std::cv_status wait(int timeoutSeconds);

    /* Callback class for data & Event. */
    class GnssCallback : public IGnssCallback {
       public:
        GnssHalTest& parent_;

        GnssCallback(GnssHalTest& parent) : parent_(parent){};

        virtual ~GnssCallback() = default;

        // Dummy callback handlers
        Return<void> gnssStatusCb(const IGnssCallback::GnssStatusValue /* status */) override {
            return Void();
        }
        Return<void> gnssSvStatusCb(const IGnssCallback::GnssSvStatus& /* svStatus */) override {
            return Void();
        }
        Return<void> gnssNmeaCb(int64_t /* timestamp */,
                                const android::hardware::hidl_string& /* nmea */) override {
            return Void();
        }
        Return<void> gnssAcquireWakelockCb() override { return Void(); }
        Return<void> gnssReleaseWakelockCb() override { return Void(); }
        Return<void> gnssRequestTimeCb() override { return Void(); }
        // Actual (test) callback handlers
        Return<void> gnssLocationCb(const GnssLocation& location) override;
        Return<void> gnssSetCapabilitesCb(uint32_t capabilities) override;
        Return<void> gnssSetSystemInfoCb(const IGnssCallback::GnssSystemInfo& info) override;
        Return<void> gnssNameCb(const android::hardware::hidl_string& name) override;
    };

    /*
     * StartAndGetSingleLocation:
     * Helper function to get one Location and check fields
     *
     * returns  true if a location was successfully generated
     */
    bool StartAndGetSingleLocation(bool checkAccuracies) {
        auto result = gnss_hal_->start();

        EXPECT_TRUE(result.isOk());
        EXPECT_TRUE(result);

        /*
         * GPS signals initially optional for this test, so don't expect fast fix,
         * or no timeout, unless signal is present
         */
        int firstGnssLocationTimeoutSeconds = 15;

        wait(firstGnssLocationTimeoutSeconds);
        EXPECT_EQ(location_called_count_, 1);

        if (location_called_count_ > 0) {
            // don't require speed on first fix
            CheckLocation(last_location_, checkAccuracies, false);
            return true;
        }
        return false;
    }

    /*
     * CheckLocation:
     *   Helper function to vet Location fields when calling setPositionMode_1_1()
     */
    void CheckLocation(GnssLocation& location, bool checkAccuracies, bool checkSpeed) {
        EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_LAT_LONG);
        EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_ALTITUDE);
        if (checkSpeed) {
            EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED);
        }
        EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_HORIZONTAL_ACCURACY);
        // New uncertainties available in O must be provided,
        // at least when paired with modern hardware (2017+)
        if (checkAccuracies) {
            EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_VERTICAL_ACCURACY);
            if (checkSpeed) {
                EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED_ACCURACY);
                if (location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING) {
                    EXPECT_TRUE(location.gnssLocationFlags &
                                GnssLocationFlags::HAS_BEARING_ACCURACY);
                }
            }
        }
        EXPECT_GE(location.latitudeDegrees, -90.0);
        EXPECT_LE(location.latitudeDegrees, 90.0);
        EXPECT_GE(location.longitudeDegrees, -180.0);
        EXPECT_LE(location.longitudeDegrees, 180.0);
        EXPECT_GE(location.altitudeMeters, -1000.0);
        EXPECT_LE(location.altitudeMeters, 30000.0);
        if (checkSpeed) {
            EXPECT_GE(location.speedMetersPerSec, 0.0);
            EXPECT_LE(location.speedMetersPerSec, 5.0);  // VTS tests are stationary.

            // Non-zero speeds must be reported with an associated bearing
            if (location.speedMetersPerSec > 0.0) {
                EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING);
            }
        }

        /*
         * Tolerating some especially high values for accuracy estimate, in case of
         * first fix with especially poor geometry (happens occasionally)
         */
        EXPECT_GT(location.horizontalAccuracyMeters, 0.0);
        EXPECT_LE(location.horizontalAccuracyMeters, 250.0);

        /*
         * Some devices may define bearing as -180 to +180, others as 0 to 360.
         * Both are okay & understandable.
         */
        if (location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING) {
            EXPECT_GE(location.bearingDegrees, -180.0);
            EXPECT_LE(location.bearingDegrees, 360.0);
        }
        if (location.gnssLocationFlags & GnssLocationFlags::HAS_VERTICAL_ACCURACY) {
            EXPECT_GT(location.verticalAccuracyMeters, 0.0);
            EXPECT_LE(location.verticalAccuracyMeters, 500.0);
        }
        if (location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED_ACCURACY) {
            EXPECT_GT(location.speedAccuracyMetersPerSecond, 0.0);
            EXPECT_LE(location.speedAccuracyMetersPerSecond, 50.0);
        }
        if (location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING_ACCURACY) {
            EXPECT_GT(location.bearingAccuracyDegrees, 0.0);
            EXPECT_LE(location.bearingAccuracyDegrees, 360.0);
        }

        // Check timestamp > 1.48e12 (47 years in msec - 1970->2017+)
        EXPECT_GT(location.timestamp, 1.48e12);
    }

    /*
     * StopAndClearLocations:
     * Helper function to stop locations
     *
     * returns  true if a location was successfully generated
     */
    void StopAndClearLocations() {
        auto result = gnss_hal_->stop();

        EXPECT_TRUE(result.isOk());
        EXPECT_TRUE(result);

        /*
         * Clear notify/waiting counter, allowing up till the timeout after
         * the last reply for final startup messages to arrive (esp. system
         * info.)
         */
        while (wait(TIMEOUT_SEC) == std::cv_status::no_timeout) {
        }
    }

    sp<IGnss> gnss_hal_;         // GNSS HAL to call into
    sp<IGnssCallback> gnss_cb_;  // Primary callback interface

    /* Count of calls to set the following items, and the latest item (used by
     * test.)
     */
    int info_called_count_;
    IGnssCallback::GnssSystemInfo last_info_;
    uint32_t last_capabilities_;
    int capabilities_called_count_;
    int location_called_count_;
    GnssLocation last_location_;

    int name_called_count_;
    android::hardware::hidl_string last_name_;

   private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int notify_count_;
};

#endif  // GNSS_HAL_TEST_H_
