/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "GnssMeasIfaceAidl"

#include "GnssMeasurementInterface.h"
#include <aidl/android/hardware/gnss/BnGnss.h>
#include <log/log.h>
#include "Utils.h"

namespace aidl::android::hardware::gnss {

using Utils = ::android::hardware::gnss::common::Utils;

std::shared_ptr<IGnssMeasurementCallback> GnssMeasurementInterface::sCallback = nullptr;

GnssMeasurementInterface::GnssMeasurementInterface() : mMinIntervalMillis(1000) {}

GnssMeasurementInterface::~GnssMeasurementInterface() {
    stop();
}

ndk::ScopedAStatus GnssMeasurementInterface::setCallback(
        const std::shared_ptr<IGnssMeasurementCallback>& callback, const bool enableFullTracking,
        const bool enableCorrVecOutputs) {
    ALOGD("setCallback: enableFullTracking: %d enableCorrVecOutputs: %d", (int)enableFullTracking,
          (int)enableCorrVecOutputs);
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = callback;

    if (mIsActive) {
        ALOGW("GnssMeasurement callback already set. Resetting the callback...");
        stop();
    }
    start(enableCorrVecOutputs);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus GnssMeasurementInterface::close() {
    ALOGD("close");
    stop();
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = nullptr;
    return ndk::ScopedAStatus::ok();
}

void GnssMeasurementInterface::start(const bool enableCorrVecOutputs) {
    ALOGD("start");
    mIsActive = true;
    mThread = std::thread([this, enableCorrVecOutputs]() {
        while (mIsActive == true) {
            auto measurement = Utils::getMockMeasurement(enableCorrVecOutputs);
            this->reportMeasurement(measurement);

            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMillis));
        }
    });
    mThread.detach();
}

void GnssMeasurementInterface::stop() {
    ALOGD("stop");
    mIsActive = false;
}

void GnssMeasurementInterface::reportMeasurement(const GnssData& data) {
    ALOGD("reportMeasurement()");
    std::shared_ptr<IGnssMeasurementCallback> callbackCopy;
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (sCallback == nullptr) {
            ALOGE("%s: GnssMeasurement::sCallback is null.", __func__);
            return;
        }
        callbackCopy = sCallback;
    }
    callbackCopy->gnssMeasurementCb(data);
}

}  // namespace aidl::android::hardware::gnss
