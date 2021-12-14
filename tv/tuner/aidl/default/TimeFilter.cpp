/*
 * Copyright 2021 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "android.hardware.tv.tuner-service.example-TimeFilter"

#include <aidl/android/hardware/tv/tuner/Result.h>
#include <utils/Log.h>

#include "TimeFilter.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

TimeFilter::TimeFilter() {}

TimeFilter::TimeFilter(std::shared_ptr<Demux> demux) {
    mDemux = demux;
}

TimeFilter::~TimeFilter() {}

::ndk::ScopedAStatus TimeFilter::setTimeStamp(int64_t in_timeStamp) {
    ALOGV("%s", __FUNCTION__);
    if (in_timeStamp == INVALID_TIME_STAMP) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }
    mTimeStamp = in_timeStamp;
    mBeginTime = time(NULL);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TimeFilter::clearTimeStamp() {
    ALOGV("%s", __FUNCTION__);
    mTimeStamp = INVALID_TIME_STAMP;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TimeFilter::getTimeStamp(int64_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);
    if (mTimeStamp == INVALID_TIME_STAMP) {
        *_aidl_return = mTimeStamp;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    uint64_t currentTimeStamp = mTimeStamp + difftime(time(NULL), mBeginTime) * 900000;
    *_aidl_return = currentTimeStamp;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TimeFilter::getSourceTime(int64_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    *_aidl_return = 0;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TimeFilter::close() {
    ALOGV("%s", __FUNCTION__);
    mTimeStamp = INVALID_TIME_STAMP;

    return ::ndk::ScopedAStatus::ok();
}

binder_status_t TimeFilter::dump(int fd, const char** /* args */, uint32_t /* numArgs */) {
    dprintf(fd, "    TimeFilter:\n");
    dprintf(fd, "      mTimeStamp: %" PRIu64 "\n", mTimeStamp);
    return STATUS_OK;
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
