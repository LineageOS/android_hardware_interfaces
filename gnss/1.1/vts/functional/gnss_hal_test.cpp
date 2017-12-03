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

#define LOG_TAG "VtsHalGnssV1_1TargetTest"
#include <log/log.h>

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
    gnss_hal_ = ::testing::VtsHalHidlTargetTestBase::getService<IGnss>();
    ASSERT_NE(gnss_hal_, nullptr);
}

void GnssHalTest::TearDown() {
    if (gnss_hal_ != nullptr) {
        gnss_hal_->cleanup();
    }
    if (notify_count_ > 0) {
        ALOGW("%d unprocessed callbacks discarded", notify_count_);
    }
}

void GnssHalTest::notify() {
    std::unique_lock<std::mutex> lock(mtx_);
    notify_count_++;
    cv_.notify_one();
}

std::cv_status GnssHalTest::wait(int timeoutSeconds) {
    std::unique_lock<std::mutex> lock(mtx_);

    auto status = std::cv_status::no_timeout;
    while (notify_count_ == 0) {
        status = cv_.wait_for(lock, std::chrono::seconds(timeoutSeconds));
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
