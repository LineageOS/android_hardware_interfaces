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

#include <android/hidl/manager/1.2/IServiceManager.h>
#include <hidl/ServiceManagement.h>

#include <gnss_hal_test.h>
#include <chrono>
#include "Utils.h"

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;

using ::android::hardware::gnss::common::Utils;

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
    location_called_count_ = 0;
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

bool GnssHalTest::StartAndCheckFirstLocation() {
    auto result = gnss_hal_->start();

    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    /*
     * GnssLocationProvider support of AGPS SUPL & XtraDownloader is not available in VTS,
     * so allow time to demodulate ephemeris over the air.
     */
    const int kFirstGnssLocationTimeoutSeconds = 75;

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

    Utils::checkLocation(location, check_speed, check_more_accuracies);
}

void GnssHalTest::StartAndCheckLocations(int count) {
    const int kMinIntervalMsec = 500;
    const int kLocationTimeoutSubsequentSec = 2;
    const bool kLowPowerMode = false;

    SetPositionMode(kMinIntervalMsec, kLowPowerMode);

    EXPECT_TRUE(StartAndCheckFirstLocation());

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

bool GnssHalTest::IsGnssHalVersion_1_1() const {
    using ::android::hidl::manager::V1_2::IServiceManager;
    sp<IServiceManager> manager = ::android::hardware::defaultServiceManager1_2();

    bool hasGnssHalVersion_1_1 = false;
    manager->listManifestByInterface(
            "android.hardware.gnss@1.1::IGnss",
            [&hasGnssHalVersion_1_1](const hidl_vec<hidl_string>& registered) {
                ASSERT_EQ(1, registered.size());
                hasGnssHalVersion_1_1 = true;
            });

    bool hasGnssHalVersion_2_0 = false;
    manager->listManifestByInterface(
            "android.hardware.gnss@2.0::IGnss",
            [&hasGnssHalVersion_2_0](const hidl_vec<hidl_string>& registered) {
                hasGnssHalVersion_2_0 = registered.size() != 0;
            });

    return hasGnssHalVersion_1_1 && !hasGnssHalVersion_2_0;
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
