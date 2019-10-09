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

#define LOG_TAG "android.hardware.tv.tuner@1.0-Frontend"

#include "Frontend.h"
#include <android/hardware/tv/tuner/1.0/IFrontendCallback.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

Frontend::Frontend(FrontendType type, FrontendId id, sp<Tuner> tuner) {
    mType = type;
    mId = id;
    mTunerService = tuner;
    // Init callback to nullptr
    mCallback = nullptr;
}

Frontend::~Frontend() {}

Return<Result> Frontend::close() {
    ALOGV("%s", __FUNCTION__);
    // Reset callback
    mCallback = nullptr;

    return Result::SUCCESS;
}

Return<Result> Frontend::setCallback(const sp<IFrontendCallback>& callback) {
    ALOGV("%s", __FUNCTION__);
    if (callback == nullptr) {
        ALOGW("[   WARN   ] Set Frontend callback with nullptr");
        return Result::INVALID_ARGUMENT;
    }

    mCallback = callback;
    return Result::SUCCESS;
}

Return<Result> Frontend::tune(const FrontendSettings& /* settings */) {
    ALOGV("%s", __FUNCTION__);
    if (mCallback == nullptr) {
        ALOGW("[   WARN   ] Frontend callback is not set when tune");
        return Result::INVALID_STATE;
    }

    // TODO dynamically allocate file to the source file
    mSourceStreamFile = FRONTEND_STREAM_FILE;

    mCallback->onEvent(FrontendEventType::LOCKED);
    return Result::SUCCESS;
}

Return<Result> Frontend::stopTune() {
    ALOGV("%s", __FUNCTION__);

    mTunerService->frontendStopTune(mId);

    return Result::SUCCESS;
}

Return<Result> Frontend::scan(const FrontendSettings& /* settings */, FrontendScanType /* type */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Frontend::stopScan() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<void> Frontend::getStatus(const hidl_vec<FrontendStatusType>& /* statusTypes */,
                                 getStatus_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    vector<FrontendStatus> statuses;
    _hidl_cb(Result::SUCCESS, statuses);

    return Void();
}

Return<Result> Frontend::setLna(bool /* bEnable */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Frontend::setLnb(uint32_t /* lnb */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

FrontendType Frontend::getFrontendType() {
    return mType;
}

FrontendId Frontend::getFrontendId() {
    return mId;
}

string Frontend::getSourceFile() {
    return mSourceStreamFile;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
