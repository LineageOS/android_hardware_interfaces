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

#define LOG_TAG "GnssHalTest"

#include <gnss_hal_test.h>

#include <chrono>

// Implementations for the main test class for GNSS HAL
GnssHalTest::GnssHalTest()
    : info_called_count_(0),
      capabilities_called_count_(0),
      location_called_count_(0),
      name_called_count_(0),
      notify_count_(0) {}

void GnssHalTest::SetUp() {
    gnss_hal_ = ::testing::VtsHalHidlTargetTestBase::getService<IGnss>(
        GnssHidlEnvironment::Instance()->getServiceName<IGnss>());
    list_gnss_sv_status_.clear();
    ASSERT_NE(gnss_hal_, nullptr);

    SetUpGnssCallback();
}

void GnssHalTest::TearDown() {
    if (gnss_hal_ != nullptr) {
        gnss_hal_->cleanup();
    }
    if (notify_count_ > 0) {
        ALOGW("%d unprocessed callbacks discarded", notify_count_);
    }
}

void GnssHalTest::SetUpGnssCallback() {
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

void GnssHalTest::StopAndClearLocations() {
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

void GnssHalTest::SetPositionMode(const int min_interval_msec, const bool low_power_mode) {
    const int kPreferredAccuracy = 0;  // Ideally perfect (matches GnssLocationProvider)
    const int kPreferredTimeMsec = 0;  // Ideally immediate

    auto result = gnss_hal_->setPositionMode_1_1(
        IGnss::GnssPositionMode::MS_BASED, IGnss::GnssPositionRecurrence::RECURRENCE_PERIODIC,
        min_interval_msec, kPreferredAccuracy, kPreferredTimeMsec, low_power_mode);

    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

bool GnssHalTest::StartAndGetSingleLocation() {
    auto result = gnss_hal_->start();

    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    /*
     * GPS signals initially optional for this test, so don't expect fast fix,
     * or no timeout, unless signal is present
     */
    const int kFirstGnssLocationTimeoutSeconds = 15;

    wait(kFirstGnssLocationTimeoutSeconds);
    EXPECT_EQ(location_called_count_, 1);

    if (location_called_count_ > 0) {
        // don't require speed on first fix
        CheckLocation(last_location_, false);
        return true;
    }
    return false;
}

void GnssHalTest::CheckLocation(GnssLocation& location, bool check_speed) {
    bool check_more_accuracies = (info_called_count_ > 0 && last_info_.yearOfHw >= 2017);

    EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_LAT_LONG);
    EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_ALTITUDE);
    if (check_speed) {
        EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED);
    }
    EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_HORIZONTAL_ACCURACY);
    // New uncertainties available in O must be provided,
    // at least when paired with modern hardware (2017+)
    if (check_more_accuracies) {
        EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_VERTICAL_ACCURACY);
        if (check_speed) {
            EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED_ACCURACY);
            if (location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING) {
                EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING_ACCURACY);
            }
        }
    }
    EXPECT_GE(location.latitudeDegrees, -90.0);
    EXPECT_LE(location.latitudeDegrees, 90.0);
    EXPECT_GE(location.longitudeDegrees, -180.0);
    EXPECT_LE(location.longitudeDegrees, 180.0);
    EXPECT_GE(location.altitudeMeters, -1000.0);
    EXPECT_LE(location.altitudeMeters, 30000.0);
    if (check_speed) {
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

void GnssHalTest::StartAndCheckLocations(int count) {
    const int kMinIntervalMsec = 500;
    const int kLocationTimeoutSubsequentSec = 2;
    const bool kLowPowerMode = false;

    SetPositionMode(kMinIntervalMsec, kLowPowerMode);

    EXPECT_TRUE(StartAndGetSingleLocation());

    for (int i = 1; i < count; i++) {
        EXPECT_EQ(std::cv_status::no_timeout, wait(kLocationTimeoutSubsequentSec));
        EXPECT_EQ(location_called_count_, i + 1);
        // Don't cause confusion by checking details if no location yet
        if (location_called_count_ > 0) {
            // Should be more than 1 location by now, but if not, still don't check first fix speed
            CheckLocation(last_location_, location_called_count_ > 1);
        }
    }
}

void GnssHalTest::notify() {
    std::unique_lock<std::mutex> lock(mtx_);
    notify_count_++;
    cv_.notify_one();
}

std::cv_status GnssHalTest::wait(int timeout_seconds) {
    std::unique_lock<std::mutex> lock(mtx_);

    auto status = std::cv_status::no_timeout;
    while (notify_count_ == 0) {
        status = cv_.wait_for(lock, std::chrono::seconds(timeout_seconds));
        if (status == std::cv_status::timeout) return status;
    }
    notify_count_--;
    return status;
}

Return<void> GnssHalTest::GnssCallback::gnssSetSystemInfoCb(
    const IGnssCallback::GnssSystemInfo& info) {
    ALOGI("Info received, year %d", info.yearOfHw);
    parent_.info_called_count_++;
    parent_.last_info_ = info;
    parent_.notify();
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssSetCapabilitesCb(uint32_t capabilities) {
    ALOGI("Capabilities received %d", capabilities);
    parent_.capabilities_called_count_++;
    parent_.last_capabilities_ = capabilities;
    parent_.notify();
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssNameCb(const android::hardware::hidl_string& name) {
    ALOGI("Name received: %s", name.c_str());
    parent_.name_called_count_++;
    parent_.last_name_ = name;
    parent_.notify();
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssLocationCb(const GnssLocation& location) {
    ALOGI("Location received");
    parent_.location_called_count_++;
    parent_.last_location_ = location;
    parent_.notify();
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssSvStatusCb(
    const IGnssCallback::GnssSvStatus& svStatus) {
    ALOGI("GnssSvStatus received");
    parent_.list_gnss_sv_status_.emplace_back(svStatus);
    return Void();
}
