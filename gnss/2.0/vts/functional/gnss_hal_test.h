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

#ifndef GNSS_HAL_TEST_H_
#define GNSS_HAL_TEST_H_

#include <android/hardware/gnss/2.0/IGnss.h>
#include "GnssCallbackEventQueue.h"

#include <gtest/gtest.h>

using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;

using android::hardware::gnss::common::GnssCallbackEventQueue;
using android::hardware::gnss::measurement_corrections::V1_0::IMeasurementCorrectionsCallback;
using android::hardware::gnss::V1_0::GnssLocationFlags;
using android::hardware::gnss::V2_0::IGnss;

using GnssLocation_1_0 = android::hardware::gnss::V1_0::GnssLocation;
using GnssLocation_2_0 = android::hardware::gnss::V2_0::GnssLocation;

using IGnssCallback_1_0 = android::hardware::gnss::V1_0::IGnssCallback;
using IGnssCallback_2_0 = android::hardware::gnss::V2_0::IGnssCallback;

using IGnssMeasurementCallback_1_0 = android::hardware::gnss::V1_0::IGnssMeasurementCallback;
using IGnssMeasurementCallback_1_1 = android::hardware::gnss::V1_1::IGnssMeasurementCallback;
using IGnssMeasurementCallback_2_0 = android::hardware::gnss::V2_0::IGnssMeasurementCallback;

using android::sp;

#define TIMEOUT_SEC 2  // for basic commands/responses

// The main test class for GNSS HAL.
class GnssHalTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override;

    virtual void TearDown() override;

    /* Callback class for data & Event. */
    class GnssCallback : public IGnssCallback_2_0 {
      public:
        IGnssCallback_1_0::GnssSystemInfo last_info_;
        android::hardware::hidl_string last_name_;
        uint32_t last_capabilities_;
        GnssLocation_2_0 last_location_;

        GnssCallbackEventQueue<IGnssCallback_1_0::GnssSystemInfo> info_cbq_;
        GnssCallbackEventQueue<android::hardware::hidl_string> name_cbq_;
        GnssCallbackEventQueue<uint32_t> capabilities_cbq_;
        GnssCallbackEventQueue<GnssLocation_2_0> location_cbq_;
        GnssCallbackEventQueue<hidl_vec<IGnssCallback_2_0::GnssSvInfo>> sv_info_list_cbq_;

        GnssCallback();
        virtual ~GnssCallback() = default;

        // Dummy callback handlers
        Return<void> gnssStatusCb(const IGnssCallback_1_0::GnssStatusValue /* status */) override {
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
        Return<void> gnssLocationCb(const GnssLocation_1_0& location) override;
        Return<void> gnssSetCapabilitesCb(uint32_t capabilities) override;
        Return<void> gnssSetSystemInfoCb(const IGnssCallback_1_0::GnssSystemInfo& info) override;
        Return<void> gnssSvStatusCb(const IGnssCallback_1_0::GnssSvStatus& svStatus) override;

        // New in v2.0
        Return<void> gnssLocationCb_2_0(const GnssLocation_2_0& location) override;
        Return<void> gnssRequestLocationCb_2_0(bool /* independentFromGnss */,
                                               bool /* isUserEmergency */) override {
            return Void();
        }
        Return<void> gnssSetCapabilitiesCb_2_0(uint32_t capabilities) override;
        Return<void> gnssSvStatusCb_2_0(
                const hidl_vec<IGnssCallback_2_0::GnssSvInfo>& svInfoList) override;

      private:
        Return<void> gnssLocationCbImpl(const GnssLocation_2_0& location);
    };

    /* Callback class for GnssMeasurement. */
    class GnssMeasurementCallback : public IGnssMeasurementCallback_2_0 {
      public:
        GnssCallbackEventQueue<IGnssMeasurementCallback_2_0::GnssData> measurement_cbq_;

        GnssMeasurementCallback() : measurement_cbq_("measurement"){};
        virtual ~GnssMeasurementCallback() = default;

        // Methods from V1_0::IGnssMeasurementCallback follow.
        Return<void> GnssMeasurementCb(const IGnssMeasurementCallback_1_0::GnssData&) override {
            return Void();
        }

        // Methods from V1_1::IGnssMeasurementCallback follow.
        Return<void> gnssMeasurementCb(const IGnssMeasurementCallback_1_1::GnssData&) override {
            return Void();
        }

        // Methods from V2_0::IGnssMeasurementCallback follow.
        Return<void> gnssMeasurementCb_2_0(const IGnssMeasurementCallback_2_0::GnssData&) override;
    };

    /* Callback class for GnssMeasurementCorrections. */
    class GnssMeasurementCorrectionsCallback : public IMeasurementCorrectionsCallback {
      public:
        uint32_t last_capabilities_;
        GnssCallbackEventQueue<uint32_t> capabilities_cbq_;

        GnssMeasurementCorrectionsCallback() : capabilities_cbq_("capabilities"){};
        virtual ~GnssMeasurementCorrectionsCallback() = default;

        // Methods from V1_0::IMeasurementCorrectionsCallback follow.
        Return<void> setCapabilitiesCb(uint32_t capabilities) override;
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
     *
     *   check_speed: true if speed related fields are also verified.
     */
    void CheckLocation(const GnssLocation_2_0& location, const bool check_speed);

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
     * IsGnssHalVersion_2_0:
     * returns  true if the GNSS HAL version is exactly 2.0.
     */
    bool IsGnssHalVersion_2_0() const;

    /*
     * SetPositionMode:
     * Helper function to set positioning mode and verify output
     */
    void SetPositionMode(const int min_interval_msec, const bool low_power_mode);

    sp<IGnss> gnss_hal_;         // GNSS HAL to call into
    sp<GnssCallback> gnss_cb_;   // Primary callback interface
};

#endif  // GNSS_HAL_TEST_H_
