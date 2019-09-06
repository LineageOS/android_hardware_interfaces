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

#define LOG_TAG "android.hardware.tv.tuner@1.0-Tuner"

#include "Tuner.h"
#include <android/hardware/tv/tuner/1.0/IFrontendCallback.h>
#include <utils/Log.h>
#include "Demux.h"
#include "Descrambler.h"
#include "Frontend.h"

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tv::tuner::V1_0::DemuxId;

Tuner::Tuner() {
    // Static Frontends array to maintain local frontends information
    // Array index matches their FrontendId in the default impl
    mFrontendSize = 8;
    mFrontends.resize(mFrontendSize);
    mFrontends[0] = new Frontend();
    mFrontends[1] = new Frontend(FrontendType::ATSC, 1);
    mFrontends[2] = new Frontend(FrontendType::DVBC, 2);
    mFrontends[3] = new Frontend(FrontendType::DVBS, 3);
    mFrontends[4] = new Frontend(FrontendType::DVBT, 4);
    mFrontends[5] = new Frontend(FrontendType::ISDBT, 5);
    mFrontends[6] = new Frontend(FrontendType::ANALOG, 6);
    mFrontends[7] = new Frontend(FrontendType::ATSC, 7);
}

Tuner::~Tuner() {}

Return<void> Tuner::getFrontendIds(getFrontendIds_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    vector<FrontendId> frontendIds;
    frontendIds.resize(mFrontendSize);
    for (int i = 0; i < mFrontendSize; i++) {
        frontendIds[i] = mFrontends[i]->getFrontendId();
    }

    _hidl_cb(Result::SUCCESS, frontendIds);
    return Void();
}

Return<void> Tuner::openFrontendById(uint32_t frontendId, openFrontendById_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (frontendId >= mFrontendSize || frontendId < 0) {
        ALOGW("[   WARN   ] Frontend with id %d isn't available", frontendId);
        _hidl_cb(Result::UNAVAILABLE, nullptr);
        return Void();
    }

    _hidl_cb(Result::SUCCESS, mFrontends[frontendId]);
    return Void();
}

Return<void> Tuner::openDemux(openDemux_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    DemuxId demuxId = mLastUsedId + 1;
    mLastUsedId += 1;
    sp<IDemux> demux = new Demux(demuxId);

    _hidl_cb(Result::SUCCESS, demuxId, demux);
    return Void();
}

Return<void> Tuner::openDescrambler(openDescrambler_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    sp<IDescrambler> descrambler = new Descrambler();

    _hidl_cb(Result::SUCCESS, descrambler);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
