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
    // Reset counters
    info_called_count_ = 0;
    capabilities_called_count_ = 0;
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

Return<void> GnssHalTest::GnssMeasurementCallback::gnssMeasurementCb_2_0(
    const IGnssMeasurementCallback_2_0::GnssData& data) {
    ALOGD("GnssMeasurement received. Size = %d", (int)data.measurements.size());
    parent_.measurement_called_count_++;
    parent_.last_measurement_ = data;
    parent_.notify();
    return Void();
}
