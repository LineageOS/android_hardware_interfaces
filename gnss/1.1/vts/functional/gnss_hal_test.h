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

#include <gtest/gtest.h>
#include "GnssCallbackEventQueue.h"

using android::hardware::Return;
using android::hardware::Void;

using android::hardware::gnss::V1_0::GnssLocation;

using android::hardware::gnss::common::GnssCallbackEventQueue;
using android::hardware::gnss::V1_0::GnssConstellationType;
using android::hardware::gnss::V1_0::GnssLocationFlags;
using android::hardware::gnss::V1_1::IGnss;
using android::hardware::gnss::V1_1::IGnssCallback;

using android::sp;

#define TIMEOUT_SEC 2  // for basic commands/responses

// The main test class for GNSS HAL.
class GnssHalTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override;

    virtual void TearDown() override;

    /* Used as a mechanism to inform the test that a callback has occurred */
    void notify();

    /* Test code calls this function to wait for a callback */
    std::cv_status wait(int timeout_seconds);

    /* Callback class for data & Event. */
    class GnssCallback : public IGnssCallback {
       public:
         IGnssCallback::GnssSystemInfo last_info_;
         android::hardware::hidl_string last_name_;
         uint32_t last_capabilities_;
         GnssLocation last_location_;

         GnssCallbackEventQueue<IGnssCallback::GnssSystemInfo> info_cbq_;
         GnssCallbackEventQueue<android::hardware::hidl_string> name_cbq_;
         GnssCallbackEventQueue<uint32_t> capabilities_cbq_;
         GnssCallbackEventQueue<GnssLocation> location_cbq_;
         GnssCallbackEventQueue<IGnssCallback::GnssSvStatus> sv_status_cbq_;

         GnssCallback();
         virtual ~GnssCallback() = default;

         // Dummy callback handlers
         Return<void> gnssStatusCb(const IGnssCallback::GnssStatusValue /* status */) override {
             return Void();
         }
         Return<void> gnssNmeaCb(int64_t /* timestamp */,
                                 const android::hardware::hidl_string& /* nmea */) override {
             return Void();
         }
         Return<void> gnssAcquireWakelockCb() override { return Void(); }
         Return<void> gnssReleaseWakelockCb() override { return Void(); }
         Return<void> gnssRequestLocationCb(bool /* independentFromGnss */) override {
             return Void();
         }
         Return<void> gnssRequestTimeCb() override { return Void(); }
         // Actual (test) callback handlers
         Return<void> gnssNameCb(const android::hardware::hidl_string& name) override;
         Return<void> gnssLocationCb(const GnssLocation& location) override;
         Return<void> gnssSetCapabilitesCb(uint32_t capabilities) override;
         Return<void> gnssSetSystemInfoCb(const IGnssCallback::GnssSystemInfo& info) override;
         Return<void> gnssSvStatusCb(const IGnssCallback::GnssSvStatus& svStatus) override;
    };

    /*
     * SetUpGnssCallback:
     *   Set GnssCallback and verify the result.
     */
    void SetUpGnssCallback();

    /*
     * StartAndCheckFirstLocation:
     *   Helper function to start location, and check the first one.
     *
     *   <p> Note this leaves the Location request active, to enable Stop call vs. other call
     *   reordering tests.
     *
     *   <p> if 'strict' is true, the test will fail if no location is generated.
     *
     * returns  true if a location was successfully generated
     */
    bool StartAndCheckFirstLocation(bool strict);

    /*
     * CheckLocation:
     *   Helper function to vet Location fields
     */
    void CheckLocation(GnssLocation& location, bool check_speed);

    /*
     * StartAndCheckLocations:
     *   Helper function to collect, and check a number of
     *   normal ~1Hz locations.
     *
     *   Note this leaves the Location request active, to enable Stop call vs. other call
     *   reordering tests.
     */
    void StartAndCheckLocations(int count);

    /*
     * StopAndClearLocations:
     * Helper function to stop locations, and clear any remaining notifications
     */
    void StopAndClearLocations();

    /*
     * SetPositionMode:
     * Helper function to set positioning mode and verify output
     */
    void SetPositionMode(const int min_interval_msec, const bool low_power_mode);

    /*
     * IsGnssHalVersion_1_1:
     * returns  true if the GNSS HAL version is exactly 1.1.
     */
    bool IsGnssHalVersion_1_1() const;

    /*
     * startLocationAndGetNonGpsConstellation:
     * 1. Start location
     * 2. Find and return first non-GPS constellation
     *
     * Note that location is not stopped in this method. The client should call
     * StopAndClearLocations() after the call.
     */
    GnssConstellationType startLocationAndGetNonGpsConstellation(
            const int locations_to_await, const int gnss_sv_info_list_timeout);

    sp<IGnss> gnss_hal_;         // GNSS HAL to call into
    sp<GnssCallback> gnss_cb_;   // Primary callback interface
};

#endif  // GNSS_HAL_TEST_H_
