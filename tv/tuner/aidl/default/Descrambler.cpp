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
#define LOG_TAG "android.hardware.tv.tuner-service.example-Descrambler"

#include <aidl/android/hardware/tv/tuner/IFrontendCallback.h>
#include <aidl/android/hardware/tv/tuner/Result.h>
#include <utils/Log.h>

#include "Descrambler.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

Descrambler::Descrambler() {}

Descrambler::~Descrambler() {}

::ndk::ScopedAStatus Descrambler::setDemuxSource(int32_t in_demuxId) {
    ALOGV("%s", __FUNCTION__);
    if (mDemuxSet) {
        ALOGW("[   WARN   ] Descrambler has already been set with a demux id %" PRIu32,
              mSourceDemuxId);
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }
    mDemuxSet = true;
    mSourceDemuxId = in_demuxId;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Descrambler::setKeyToken(const std::vector<uint8_t>& /* in_keyToken */) {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Descrambler::addPid(
        const DemuxPid& /* in_pid */,
        const std::shared_ptr<IFilter>& /* in_optionalSourceFilter */) {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Descrambler::removePid(
        const DemuxPid& /* in_pid */,
        const std::shared_ptr<IFilter>& /* in_optionalSourceFilter */) {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Descrambler::close() {
    ALOGV("%s", __FUNCTION__);
    mDemuxSet = false;

    return ::ndk::ScopedAStatus::ok();
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
