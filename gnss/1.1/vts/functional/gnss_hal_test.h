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

using android::sp;

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
        Return<void> gnssLocationCb(const GnssLocation& /* location */) override { return Void(); }
        Return<void> gnssSetCapabilitesCb(uint32_t /* capabilities */) override { return Void(); }
        // Actual (test) callback handlers
        Return<void> gnssSetSystemInfoCb(const IGnssCallback::GnssSystemInfo& info) override;
        Return<void> gnssNameCb(const android::hardware::hidl_string& name) override;
    };

    sp<IGnss> gnss_hal_;         // GNSS HAL to call into
    sp<IGnssCallback> gnss_cb_;  // Primary callback interface

    /* Count of calls to set the following items, and the latest item (used by
     * test.)
     */
    int info_called_count_;
    IGnssCallback::GnssSystemInfo last_info_;

    int name_called_count_;
    android::hardware::hidl_string last_name_;

   private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int notify_count_;
};

#endif  // GNSS_HAL_TEST_H_
