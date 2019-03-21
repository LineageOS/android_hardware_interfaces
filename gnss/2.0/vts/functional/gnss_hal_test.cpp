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

#define LOG_TAG "GnssHalTest"

#include <gnss_hal_test.h>
#include <chrono>
#include "Utils.h"

using ::android::hardware::gnss::common::Utils;

// Implementations for the main test class for GNSS HAL
GnssHalTest::GnssHalTest()
    : info_called_count_(0),
      capabilities_called_count_(0),
      measurement_corrections_capabilities_called_count_(0),
      location_called_count_(0),
      name_called_count_(0),
      notify_count_(0) {}

void GnssHalTest::SetUp() {
    gnss_hal_ = ::testing::VtsHalHidlTargetTestBase::getService<IGnss>(
        GnssHidlEnvironment::Instance()->getServiceName<IGnss>());
    list_vec_gnss_sv_info_.clear();
    ASSERT_NE(gnss_hal_, nullptr);

    SetUpGnssCallback();
}

void GnssHalTest::TearDown() {
    // Reset counters
    info_called_count_ = 0;
    capabilities_called_count_ = 0;
    measurement_corrections_capabilities_called_count_ = 0;
    location_called_count_ = 0;
    name_called_count_ = 0;
    measurement_called_count_ = 0;

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

    auto result = gnss_hal_->setCallback_2_0(gnss_cb_);
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
    const auto result = gnss_hal_->stop();

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

    const auto result = gnss_hal_->setPositionMode_1_1(
            IGnss::GnssPositionMode::MS_BASED, IGnss::GnssPositionRecurrence::RECURRENCE_PERIODIC,
            min_interval_msec, kPreferredAccuracy, kPreferredTimeMsec, low_power_mode);

    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

bool GnssHalTest::StartAndCheckFirstLocation() {
    const auto result = gnss_hal_->start();

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

void GnssHalTest::CheckLocation(const GnssLocation_2_0& location, bool check_speed) {
    const bool check_more_accuracies = (info_called_count_ > 0 && last_info_.yearOfHw >= 2017);

    Utils::checkLocation(location.v1_0, check_speed, check_more_accuracies);
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

void GnssHalTest::notify() {
    {
        std::unique_lock<std::mutex> lock(mtx_);
        notify_count_++;
    }
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

std::cv_status GnssHalTest::waitForMeasurementCorrectionsCapabilities(int timeout_seconds) {
    std::unique_lock<std::mutex> lock(mtx_);
    auto status = std::cv_status::no_timeout;
    while (measurement_corrections_capabilities_called_count_ == 0) {
        status = cv_.wait_for(lock, std::chrono::seconds(timeout_seconds));
        if (status == std::cv_status::timeout) return status;
    }
    notify_count_--;
    return status;
}

Return<void> GnssHalTest::GnssCallback::gnssSetSystemInfoCb(
        const IGnssCallback_1_0::GnssSystemInfo& info) {
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

Return<void> GnssHalTest::GnssCallback::gnssSetCapabilitiesCb_2_0(uint32_t capabilities) {
    ALOGI("Capabilities (v2.0) received %d", capabilities);
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

Return<void> GnssHalTest::GnssCallback::gnssLocationCb(const GnssLocation_1_0& location) {
    ALOGI("Location received");
    GnssLocation_2_0 location_v2_0;
    location_v2_0.v1_0 = location;
    return gnssLocationCbImpl(location_v2_0);
}

Return<void> GnssHalTest::GnssCallback::gnssLocationCb_2_0(const GnssLocation_2_0& location) {
    ALOGI("Location (v2.0) received");
    return gnssLocationCbImpl(location);
}

Return<void> GnssHalTest::GnssCallback::gnssLocationCbImpl(const GnssLocation_2_0& location) {
    parent_.location_called_count_++;
    parent_.last_location_ = location;
    parent_.notify();
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssSvStatusCb(const IGnssCallback_1_0::GnssSvStatus&) {
    ALOGI("gnssSvStatusCb");

    return Void();
}

Return<void> GnssHalTest::GnssMeasurementCallback::gnssMeasurementCb_2_0(
    const IGnssMeasurementCallback_2_0::GnssData& data) {
    ALOGD("GnssMeasurement received. Size = %d", (int)data.measurements.size());
    parent_.measurement_called_count_++;
    parent_.last_measurement_ = data;
    parent_.notify();
    return Void();
}

Return<void> GnssHalTest::GnssMeasurementCorrectionsCallback::setCapabilitiesCb(
        uint32_t capabilities) {
    ALOGI("GnssMeasurementCorrectionsCallback capabilities received %d", capabilities);
    parent_.measurement_corrections_capabilities_called_count_++;
    parent_.last_measurement_corrections_capabilities_ = capabilities;
    parent_.notify();
    return Void();
}

Return<void> GnssHalTest::GnssCallback::gnssSvStatusCb_2_0(
        const hidl_vec<IGnssCallback_2_0::GnssSvInfo>& svInfoList) {
    ALOGI("gnssSvStatusCb_2_0. Size = %d", (int)svInfoList.size());
    parent_.list_vec_gnss_sv_info_.emplace_back(svInfoList);
    parent_.notify();
    return Void();
}
