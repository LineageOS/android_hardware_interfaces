/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "SurroundViewStream.h"

namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer {

using std::lock_guard;

SurroundViewStream::SurroundViewStream(sp<ISurroundViewSession> pSession)
    : mSession(pSession), mReceiveFramesCount(0) {}

Return<void> SurroundViewStream::notify(SvEvent svEvent) {
    lock_guard<mutex> lock(mLock);
    switch (svEvent) {
        case SvEvent::STREAM_STARTED:
        case SvEvent::CONFIG_UPDATED:
        case SvEvent::STREAM_STOPPED:
        case SvEvent::FRAME_DROPPED:
        case SvEvent::TIMEOUT:
            mReceivedEvents.emplace_back(svEvent);
            break;
        default:
            break;
    }

    return android::hardware::Void();
}

Return<void> SurroundViewStream::receiveFrames(const SvFramesDesc& svFramesDesc) {
    lock_guard<mutex> lock(mLock);
    if ((mLastReceivedFrames.timestampNs >= svFramesDesc.timestampNs ||
         mLastReceivedFrames.sequenceId >= svFramesDesc.sequenceId) &&
        mReceiveFramesCount != 0) {
        // The incoming frames are with invalid timestamp or sequenceId
        mAllFramesValid = false;
    }

    for (int i = 0; i < svFramesDesc.svBuffers.size(); ++i) {
        if (svFramesDesc.svBuffers[i].hardwareBuffer.nativeHandle == nullptr) {
            mAllFramesValid = false;
            // The incoming frames are with invalid nativeHandle
            break;
        }
    }

    ++mReceiveFramesCount;

    // Store all the information except for the handle
    mLastReceivedFrames.timestampNs = svFramesDesc.timestampNs;
    mLastReceivedFrames.sequenceId = svFramesDesc.sequenceId;
    mLastReceivedFrames.svBuffers.resize(svFramesDesc.svBuffers.size());
    for (int i = 0; i < svFramesDesc.svBuffers.size(); ++i) {
        mLastReceivedFrames.svBuffers[i].viewId = svFramesDesc.svBuffers[i].viewId;
        mLastReceivedFrames.svBuffers[i].hardwareBuffer.description =
                svFramesDesc.svBuffers[i].hardwareBuffer.description;
    }

    return android::hardware::Void();
}
}  // namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer
