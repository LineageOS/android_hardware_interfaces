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

#define LOG_TAG "GnssAntennaInfo"

#include "GnssAntennaInfo.h"
#include "Utils.h"

#include <log/log.h>

using ::android::hardware::gnss::common::Utils;

namespace android {
namespace hardware {
namespace gnss {
namespace V2_1 {
namespace implementation {

sp<IGnssAntennaInfoCallback> GnssAntennaInfo::sCallback = nullptr;

GnssAntennaInfo::GnssAntennaInfo() : mMinIntervalMillis(1000) {}

GnssAntennaInfo::~GnssAntennaInfo() {
    stop();
}

// Methods from ::android::hardware::gnss::V2_1::IGnssAntennaInfo follow.
Return<GnssAntennaInfo::GnssAntennaInfoStatus> GnssAntennaInfo::setCallback(
        const sp<IGnssAntennaInfoCallback>& callback) {
    ALOGD("setCallback");
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = callback;

    if (mIsActive) {
        ALOGW("GnssAntennaInfo callback already set. Resetting the callback...");
        stop();
    }
    start();

    return GnssAntennaInfoStatus::SUCCESS;
}

Return<void> GnssAntennaInfo::close() {
    ALOGD("close");
    stop();
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = nullptr;
    return Void();
}

// Private methods
void GnssAntennaInfo::start() {
    ALOGD("start");
    mIsActive = true;
    mThread = std::thread([this]() {
        while (mIsActive == true) {
            if (sCallback != nullptr) {
                auto antennaInfos = Utils::getMockAntennaInfos();
                this->reportAntennaInfo(antennaInfos);
            }

            /** For mock implementation this is good. On real device, we should only report
                antennaInfo at start and when there is a configuration change. **/
            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMillis));
        }
    });
}

void GnssAntennaInfo::stop() {
    ALOGD("stop");
    mIsActive = false;
    if (mThread.joinable()) {
        mThread.join();
    }
}

void GnssAntennaInfo::reportAntennaInfo(
        const hidl_vec<IGnssAntennaInfoCallback::GnssAntennaInfo>& antennaInfo) const {
    std::unique_lock<std::mutex> lock(mMutex);

    if (sCallback == nullptr) {
        ALOGE("%s: No non-null callback", __func__);
        return;
    }

    auto ret = sCallback->gnssAntennaInfoCb(antennaInfo);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
    }
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android