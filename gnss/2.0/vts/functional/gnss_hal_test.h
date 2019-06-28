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
#include <deque>
#include <list>
#include <mutex>

using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;

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
    virtual void SetUp() override;

    virtual void TearDown() override;

    /* Producer/consumer queue for storing/retrieving callback events from GNSS HAL */
    template <class T>
    class CallbackQueue {
      public:
        CallbackQueue(const std::string& name) : name_(name), called_count_(0){};
        ~CallbackQueue() { reset(); }

        /* Adds callback event to the end of the queue. */
        void store(const T& event);

        /*
         * Removes the callack event at the front of the queue, stores it in event parameter
         * and returns true. Returns false on timeout and event is not populated.
         */
        bool retrieve(T& event, int timeout_seconds);

        /*
         * Removes parameter count number of callack events at the front of the queue, stores
         * them in event_list parameter and returns the number of events retrieved. Waits up to
         * timeout_seconds to retrieve each event. If timeout occurs, it returns the number of
         * items retrieved which will be less than count.
         */
        int retrieve(list<T>& event_list, int count, int timeout_seconds);

        /* Returns the number of events pending to be retrieved from the callback event queue. */
        int size() const;

        /* Returns the number of callback events received since last reset(). */
        int calledCount() const;

        /* Clears the callback event queue and resets the calledCount() to 0. */
        void reset();

      private:
        CallbackQueue(const CallbackQueue&) = delete;
        CallbackQueue& operator=(const CallbackQueue&) = delete;

        std::string name_;
        int called_count_;
        mutable std::recursive_mutex mtx_;
        std::condition_variable_any cv_;
        std::deque<T> events_;
    };

    /* Callback class for data & Event. */
    class GnssCallback : public IGnssCallback_2_0 {
      public:
        IGnssCallback_1_0::GnssSystemInfo last_info_;
        android::hardware::hidl_string last_name_;
        uint32_t last_capabilities_;
        GnssLocation_2_0 last_location_;

        CallbackQueue<IGnssCallback_1_0::GnssSystemInfo> info_cbq_;
        CallbackQueue<android::hardware::hidl_string> name_cbq_;
        CallbackQueue<uint32_t> capabilities_cbq_;
        CallbackQueue<GnssLocation_2_0> location_cbq_;
        CallbackQueue<hidl_vec<IGnssCallback_2_0::GnssSvInfo>> sv_info_list_cbq_;

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
        CallbackQueue<IGnssMeasurementCallback_2_0::GnssData> measurement_cbq_;

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
        CallbackQueue<uint32_t> capabilities_cbq_;

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
     * returns  true if a location was successfully generated
     */
    bool StartAndCheckFirstLocation();

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
     * SetPositionMode:
     * Helper function to set positioning mode and verify output
     */
    void SetPositionMode(const int min_interval_msec, const bool low_power_mode);

    sp<IGnss> gnss_hal_;         // GNSS HAL to call into
    sp<GnssCallback> gnss_cb_;   // Primary callback interface
};

template <class T>
void GnssHalTest::CallbackQueue<T>::store(const T& event) {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    events_.push_back(event);
    ++called_count_;
    lock.unlock();
    cv_.notify_all();
}

template <class T>
bool GnssHalTest::CallbackQueue<T>::retrieve(T& event, int timeout_seconds) {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    cv_.wait_for(lock, std::chrono::seconds(timeout_seconds), [&] { return !events_.empty(); });
    if (events_.empty()) {
        return false;
    }
    event = events_.front();
    events_.pop_front();
    return true;
}

template <class T>
int GnssHalTest::CallbackQueue<T>::retrieve(list<T>& event_list, int count, int timeout_seconds) {
    for (int i = 0; i < count; ++i) {
        T event;
        if (!retrieve(event, timeout_seconds)) {
            return i;
        }
        event_list.push_back(event);
    }

    return count;
}

template <class T>
int GnssHalTest::CallbackQueue<T>::size() const {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    return events_.size();
}

template <class T>
int GnssHalTest::CallbackQueue<T>::calledCount() const {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    return called_count_;
}

template <class T>
void GnssHalTest::CallbackQueue<T>::reset() {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    if (!events_.empty()) {
        ALOGW("%u unprocessed events discarded in callback queue %s", (unsigned int)events_.size(),
              name_.c_str());
    }
    events_.clear();
    called_count_ = 0;
}

#endif  // GNSS_HAL_TEST_H_
