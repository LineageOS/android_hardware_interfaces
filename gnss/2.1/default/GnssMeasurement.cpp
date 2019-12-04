/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "GnssMeasurement"

#include "GnssMeasurement.h"
#include <log/log.h>
#include "Utils.h"

namespace android {
namespace hardware {
namespace gnss {

using common::Utils;

namespace V2_1 {
namespace implementation {

sp<V2_1::IGnssMeasurementCallback> GnssMeasurement::sCallback = nullptr;

GnssMeasurement::GnssMeasurement() : mMinIntervalMillis(1000) {}

GnssMeasurement::~GnssMeasurement() {
    stop();
}

// Methods from V1_0::IGnssMeasurement follow.
Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> GnssMeasurement::setCallback(
        const sp<V1_0::IGnssMeasurementCallback>&) {
    // TODO implement
    return V1_0::IGnssMeasurement::GnssMeasurementStatus{};
}

Return<void> GnssMeasurement::close() {
    ALOGD("close");
    std::unique_lock<std::mutex> lock(mMutex);
    stop();
    sCallback = nullptr;
    return Void();
}

// Methods from V1_1::IGnssMeasurement follow.
Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> GnssMeasurement::setCallback_1_1(
        const sp<V1_1::IGnssMeasurementCallback>&, bool) {
    // TODO implement
    return V1_0::IGnssMeasurement::GnssMeasurementStatus{};
}

// Methods from V2_0::IGnssMeasurement follow.
Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> GnssMeasurement::setCallback_2_0(
        const sp<V2_0::IGnssMeasurementCallback>&, bool) {
    // TODO implement
    return V1_0::IGnssMeasurement::GnssMeasurementStatus{};
}

// Methods from V2_1::IGnssMeasurement follow.
Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> GnssMeasurement::setCallback_2_1(
        const sp<V2_1::IGnssMeasurementCallback>& callback, bool) {
    ALOGD("setCallback_2_1");
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = callback;

    if (mIsActive) {
        ALOGW("GnssMeasurement callback already set. Resetting the callback...");
        stop();
    }
    start();

    return V1_0::IGnssMeasurement::GnssMeasurementStatus::SUCCESS;
}

void GnssMeasurement::start() {
    ALOGD("start");
    mIsActive = true;
    mThread = std::thread([this]() {
        while (mIsActive == true) {
            auto measurement = Utils::getMockMeasurementV2_1();
            this->reportMeasurement(measurement);

            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMillis));
        }
    });
}

void GnssMeasurement::stop() {
    ALOGD("stop");
    mIsActive = false;
    if (mThread.joinable()) {
        mThread.join();
    }
}

void GnssMeasurement::reportMeasurement(const GnssDataV2_1& data) {
    ALOGD("reportMeasurement()");
    std::unique_lock<std::mutex> lock(mMutex);
    if (sCallback == nullptr) {
        ALOGE("%s: GnssMeasurement::sCallback is null.", __func__);
        return;
    }
    auto ret = sCallback->gnssMeasurementCb_2_1(data);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
