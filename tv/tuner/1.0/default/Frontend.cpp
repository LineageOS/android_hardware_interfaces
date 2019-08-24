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

Frontend::Frontend() {
    // Init callback to nullptr
    mCallback = nullptr;
}

Frontend::Frontend(FrontendType type, FrontendId id) {
    mType = type;
    mId = id;
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

    mCallback->onEvent(FrontendEventType::NO_SIGNAL);
    return Result::SUCCESS;
}

Return<Result> Frontend::stopTune() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

FrontendType Frontend::getFrontendType() {
    return mType;
}

FrontendId Frontend::getFrontendId() {
    return mId;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
