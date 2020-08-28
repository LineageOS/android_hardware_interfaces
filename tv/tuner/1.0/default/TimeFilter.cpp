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

#define LOG_TAG "android.hardware.tv.tuner@1.0-TimeFilter"

#include "TimeFilter.h"
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

TimeFilter::TimeFilter() {}

TimeFilter::TimeFilter(sp<Demux> demux) {
    mDemux = demux;
}

TimeFilter::~TimeFilter() {}

Return<Result> TimeFilter::setTimeStamp(uint64_t timeStamp) {
    ALOGV("%s", __FUNCTION__);
    if (timeStamp == INVALID_TIME_STAMP) {
        return Result::INVALID_ARGUMENT;
    }
    mTimeStamp = timeStamp;
    mBeginTime = time(NULL);

    return Result::SUCCESS;
}

Return<Result> TimeFilter::clearTimeStamp() {
    ALOGV("%s", __FUNCTION__);
    mTimeStamp = INVALID_TIME_STAMP;

    return Result::SUCCESS;
}

Return<void> TimeFilter::getTimeStamp(getTimeStamp_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);
    if (mTimeStamp == INVALID_TIME_STAMP) {
        _hidl_cb(Result::INVALID_STATE, mTimeStamp);
    }

    uint64_t currentTimeStamp = mTimeStamp + difftime(time(NULL), mBeginTime) * 900000;
    _hidl_cb(Result::SUCCESS, currentTimeStamp);
    return Void();
}

Return<void> TimeFilter::getSourceTime(getSourceTime_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    uint64_t time = 0;

    _hidl_cb(Result::SUCCESS, time);
    return Void();
}

Return<Result> TimeFilter::close() {
    ALOGV("%s", __FUNCTION__);
    mTimeStamp = INVALID_TIME_STAMP;

    return Result::SUCCESS;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android