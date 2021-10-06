/*
 * Copyright 2020 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.tuner@1.1-Descrambler"

#include <android/hardware/tv/tuner/1.0/IFrontendCallback.h>
#include <utils/Log.h>

#include "Descrambler.h"

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

Descrambler::Descrambler() {}

Descrambler::~Descrambler() {}

Return<Result> Descrambler::setDemuxSource(uint32_t demuxId) {
    ALOGV("%s", __FUNCTION__);
    if (mDemuxSet) {
        ALOGW("[   WARN   ] Descrambler has already been set with a demux id %" PRIu32,
              mSourceDemuxId);
        return Result::INVALID_STATE;
    }
    mDemuxSet = true;
    mSourceDemuxId = static_cast<uint32_t>(demuxId);

    return Result::SUCCESS;
}

Return<Result> Descrambler::setKeyToken(const hidl_vec<uint8_t>& /* keyToken */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Descrambler::addPid(const DemuxPid& /* pid */,
                                   const sp<IFilter>& /* optionalSourceFilter */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Descrambler::removePid(const DemuxPid& /* pid */,
                                      const sp<IFilter>& /* optionalSourceFilter */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Descrambler::close() {
    ALOGV("%s", __FUNCTION__);
    mDemuxSet = false;

    return Result::SUCCESS;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
