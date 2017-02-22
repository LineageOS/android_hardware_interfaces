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

#define LOG_TAG "VtsHalGnssV1_0TargetTest"
#include <android/hardware/gnss/1.0/IGnss.h>
#include <android/log.h>

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

using android::hardware::Return;
using android::hardware::Void;

using android::hardware::gnss::V1_0::GnssLocation;
using android::hardware::gnss::V1_0::GnssLocationFlags;
using android::hardware::gnss::V1_0::IGnss;
using android::hardware::gnss::V1_0::IGnssCallback;
using android::sp;

#define TIMEOUT_SECONDS 5  // for basic commands/responses

// The main test class for GNSS HAL.
class GnssHalTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    /* TODO(b/35678469): Setup the init.rc for VTS such that there's a
     * single caller
     * to the GNSS HAL - as part of confirming that the info & capabilities
     * callbacks trigger.
     */

    gnss_hal_ = IGnss::getService("gnss");
    ASSERT_NE(gnss_hal_, nullptr);

    gnss_cb_ = new GnssCallback(*this);
    ASSERT_NE(gnss_cb_, nullptr);

    auto result = gnss_hal_->setCallback(gnss_cb_);
    if (!result.isOk()) {
      ALOGE("result of failed callback set %s", result.description().c_str());
    }

    ASSERT_TRUE(result.isOk());
    ASSERT_TRUE(result);

    /* TODO(b/35678469): Implement the capabilities & info (year) checks &
     * value store here.
     */
  }

  virtual void TearDown() override {
    if (gnss_hal_ != nullptr) {
      gnss_hal_->cleanup();
    }
  }

  /* Used as a mechanism to inform the test that a callback has occurred */
  inline void notify() {
    std::unique_lock<std::mutex> lock(mtx_);
    count++;
    cv_.notify_one();
  }

  /* Test code calls this function to wait for a callback */
  inline std::cv_status wait(int timeoutSeconds) {
    std::unique_lock<std::mutex> lock(mtx_);

    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (count == 0) {
      status = cv_.wait_until(lock, now + std::chrono::seconds(timeoutSeconds));
      if (status == std::cv_status::timeout) return status;
    }
    count--;
    return status;
  }

  /* Callback class for data & Event. */
  class GnssCallback : public IGnssCallback {
    GnssHalTest& parent_;

   public:
    GnssCallback(GnssHalTest& parent) : parent_(parent){};

    virtual ~GnssCallback() = default;

    // Dummy callback handlers
    Return<void> gnssStatusCb(
        const IGnssCallback::GnssStatusValue status) override {
      return Void();
    }
    Return<void> gnssSvStatusCb(
        const IGnssCallback::GnssSvStatus& svStatus) override {
      return Void();
    }
    Return<void> gnssNmeaCb(
        int64_t timestamp,
        const android::hardware::hidl_string& nmea) override {
      return Void();
    }
    Return<void> gnssAcquireWakelockCb() override { return Void(); }
    Return<void> gnssReleaseWakelockCb() override { return Void(); }
    Return<void> gnssRequestTimeCb() override { return Void(); }

    // Actual (test) callback handlers
    Return<void> gnssLocationCb(const GnssLocation& location) override {
      ALOGI("Location received");
      parent_.location_called_count_++;
      parent_.last_location_ = location;
      parent_.notify();
      return Void();
    }

    Return<void> gnssSetCapabilitesCb(uint32_t capabilities) override {
      ALOGI("Capabilities received %d", capabilities);
      parent_.capabilities_called_count_++;
      parent_.last_capabilities_ = capabilities;
      parent_.notify();
      return Void();
    }

    Return<void> gnssSetSystemInfoCb(
        const IGnssCallback::GnssSystemInfo& info) override {
      ALOGI("Info received, year %d", info.yearOfHw);
      parent_.info_called_count_++;
      parent_.last_info_ = info;
      parent_.notify();
      return Void();
    }
  };

  sp<IGnss> gnss_hal_;         // GNSS HAL to call into
  sp<IGnssCallback> gnss_cb_;  // Primary callback interface

  /* Count of calls to set the following items, and the latest item (used by
   * test.)
   */
  int capabilities_called_count_;
  uint32_t last_capabilities_;

  int location_called_count_;
  GnssLocation last_location_;

  int info_called_count_;
  IGnssCallback::GnssSystemInfo last_info_;

 private:
  std::mutex mtx_;
  std::condition_variable cv_;
  int count;
};

/*
 * SetCallbackCapabilitiesCleanup:
 * Sets up the callback, awaits the capabilities, and calls cleanup
 *
 * Since this is just the basic operation of SetUp() and TearDown(),
 * the function definition is intentionally kept empty
 */
TEST_F(GnssHalTest, SetCallbackCapabilitiesCleanup) {}

void CheckLocation(GnssLocation& location) {
  EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_LAT_LONG);
  EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_ALTITUDE);
  EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED);
  EXPECT_TRUE(location.gnssLocationFlags &
              GnssLocationFlags::HAS_HORIZONTAL_ACCURACY);
  EXPECT_GE(location.latitudeDegrees, -90.0);
  EXPECT_LE(location.latitudeDegrees, 90.0);
  EXPECT_GE(location.longitudeDegrees, -180.0);
  EXPECT_LE(location.longitudeDegrees, 180.0);
  EXPECT_GE(location.altitudeMeters, -1000.0);
  EXPECT_LE(location.altitudeMeters, 30000.0);
  EXPECT_GE(location.speedMetersPerSec, 0.0);
  EXPECT_LE(location.speedMetersPerSec, 5.0);  // VTS tests are stationary.

  /*
   * Tolerating some especially high values for accuracy estimate, in case of
   * first fix with especially poor geoemtry (happens occasionally)
   */
  EXPECT_GT(location.horizontalAccuracyMeters, 0.0);
  EXPECT_LE(location.horizontalAccuracyMeters, 200.0);

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

  /* TODO(b/35678469): Check if the hardware year is 2017+, and if so,
   * that bearing, plus vertical, speed & bearing accuracy are present.
   * And allow bearing to be not present, only if associated with a speed of 0.0
   */
}

/*
 * GetLocation:
 * Turns on location, waits 45 second for at least 5 locations,
 * and checks them for reasonable validity.
 */
TEST_F(GnssHalTest, GetLocation) {
#define MIN_INTERVAL_MSEC 500
#define PREFERRED_ACCURACY 0   // Ideally perfect (matches GnssLocationProvider)
#define PREFERRED_TIME_MSEC 0  // Ideally immediate

#define LOCATION_TIMEOUT_FIRST_SEC 45
#define LOCATION_TIMEOUT_SUBSEQUENT_SEC 3
#define LOCATIONS_TO_CHECK 5

  auto result = gnss_hal_->setPositionMode(
      IGnss::GnssPositionMode::MS_BASED,
      IGnss::GnssPositionRecurrence::RECURRENCE_PERIODIC, MIN_INTERVAL_MSEC,
      PREFERRED_ACCURACY, PREFERRED_TIME_MSEC);

  ASSERT_TRUE(result.isOk());
  ASSERT_TRUE(result);

  result = gnss_hal_->start();

  ASSERT_TRUE(result.isOk());
  ASSERT_TRUE(result);

  EXPECT_EQ(std::cv_status::no_timeout, wait(LOCATION_TIMEOUT_FIRST_SEC));
  EXPECT_EQ(location_called_count_, 1);
  CheckLocation(last_location_);

  for (int i = 1; i < LOCATIONS_TO_CHECK; i++) {
    EXPECT_EQ(std::cv_status::no_timeout,
              wait(LOCATION_TIMEOUT_SUBSEQUENT_SEC));
    EXPECT_EQ(location_called_count_, i + 1);
    CheckLocation(last_location_);
  }

  result = gnss_hal_->stop();

  ASSERT_TRUE(result.isOk());
  ASSERT_TRUE(result);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}