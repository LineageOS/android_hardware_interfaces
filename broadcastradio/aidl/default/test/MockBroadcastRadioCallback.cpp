/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "MockBroadcastRadioCallback.h"

#include <android-base/logging.h>

namespace aidl::android::hardware::broadcastradio {

namespace {
using std::vector;
}

MockBroadcastRadioCallback::MockBroadcastRadioCallback() {
    mAntennaConnectionState = true;
}

ScopedAStatus MockBroadcastRadioCallback::onTuneFailed(Result result,
                                                       const ProgramSelector& selector) {
    LOG(DEBUG) << "onTuneFailed with result with " << selector.toString().c_str();
    if (result != Result::CANCELED) {
        std::lock_guard<std::mutex> lk(mLock);
        tunerFailed = true;
    }
    return ndk::ScopedAStatus::ok();
}

ScopedAStatus MockBroadcastRadioCallback::onCurrentProgramInfoChanged(const ProgramInfo& info) {
    LOG(DEBUG) << "onCurrentProgramInfoChanged with " << info.toString().c_str();
    {
        std::lock_guard<std::mutex> lk(mLock);
        mCurrentProgramInfo = info;
    }

    mOnCurrentProgramInfoChangedFlag.notify();
    return ndk::ScopedAStatus::ok();
}

ScopedAStatus MockBroadcastRadioCallback::onProgramListUpdated(const ProgramListChunk& chunk) {
    {
        std::lock_guard<std::mutex> lk(mLock);
        updateProgramList(chunk, &mProgramList);
    }

    if (chunk.complete) {
        mOnProgramListReadyFlag.notify();
    }

    return ndk::ScopedAStatus::ok();
}

ScopedAStatus MockBroadcastRadioCallback::onParametersUpdated(
        [[maybe_unused]] const vector<VendorKeyValue>& parameters) {
    return ndk::ScopedAStatus::ok();
}

ScopedAStatus MockBroadcastRadioCallback::onAntennaStateChange(bool connected) {
    if (!connected) {
        std::lock_guard<std::mutex> lk(mLock);
        mAntennaConnectionState = false;
    }
    return ndk::ScopedAStatus::ok();
}

ScopedAStatus MockBroadcastRadioCallback::onConfigFlagUpdated([[maybe_unused]] ConfigFlag in_flag,
                                                              [[maybe_unused]] bool in_value) {
    return ndk::ScopedAStatus::ok();
}

bool MockBroadcastRadioCallback::waitOnCurrentProgramInfoChangedCallback() {
    return mOnCurrentProgramInfoChangedFlag.wait();
}

bool MockBroadcastRadioCallback::waitProgramReady() {
    return mOnProgramListReadyFlag.wait();
}

void MockBroadcastRadioCallback::reset() {
    mOnCurrentProgramInfoChangedFlag.reset();
    mOnProgramListReadyFlag.reset();
}

bool MockBroadcastRadioCallback::isTunerFailed() {
    std::lock_guard<std::mutex> lk(mLock);
    return tunerFailed;
}

ProgramInfo MockBroadcastRadioCallback::getCurrentProgramInfo() {
    std::lock_guard<std::mutex> lk(mLock);
    return mCurrentProgramInfo;
}

utils::ProgramInfoSet MockBroadcastRadioCallback::getProgramList() {
    std::lock_guard<std::mutex> lk(mLock);
    return mProgramList;
}

}  // namespace aidl::android::hardware::broadcastradio
