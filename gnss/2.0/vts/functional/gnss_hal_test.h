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

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

#include <condition_variable>
#include <list>
#include <mutex>

using android::hardware::Return;
using android::hardware::Void;

using android::hardware::gnss::V1_0::GnssLocation;

using android::hardware::gnss::V1_0::GnssLocationFlags;
using android::hardware::gnss::V1_1::IGnssCallback;
using android::hardware::gnss::V2_0::IGnss;

using android::sp;

#define TIMEOUT_SEC 2  // for basic commands/responses

// Test environment for GNSS HIDL HAL.
class GnssHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static GnssHidlEnvironment* Instance() {
        static GnssHidlEnvironment* instance = new GnssHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IGnss>(); }

   private:
    GnssHidlEnvironment() {}
};

// The main test class for GNSS HAL.
class GnssHalTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    GnssHalTest();

    virtual void SetUp() override;

    virtual void TearDown() override;

    /* Used as a mechanism to inform the test that a callback has occurred */
    void notify();

    /* Test code calls this function to wait for a callback */
    std::cv_status wait(int timeout_seconds);

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
    list<IGnssCallback::GnssSvStatus> list_gnss_sv_status_;

    int name_called_count_;
    android::hardware::hidl_string last_name_;

   private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int notify_count_;
};

#endif  // GNSS_HAL_TEST_H_
