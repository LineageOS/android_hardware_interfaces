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

#include "SurroundViewStreamHandler.h"

#include <utils/Log.h>

using std::lock_guard;

SurroundViewServiceHandler::SurroundViewServiceHandler(sp<ISurroundViewSession> pSession) :
    mSession(pSession),
    mReceiveFramesCount(0),
    mDoNotReturnFrames(false) {
    // Nothing but member initialization
}

Return<void> SurroundViewServiceHandler::notify(SvEvent svEvent) {
    ALOGD("SurroundViewServiceHandler::notify %d", svEvent);

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
            ALOGI("[SurroundViewLog] Received other event");
    }

    return android::hardware::Void();
}

Return<void> SurroundViewServiceHandler::receiveFrames(const SvFramesDesc& svFramesDesc) {
    ALOGD("SurroundViewServiceHandler::receiveFrames");

    lock_guard<mutex> lock(mLock);
    unsigned long timestampNs = svFramesDesc.timestampNs;
    unsigned sequenceId = svFramesDesc.sequenceId;
    ALOGD("receiveFrames count: %d", mReceiveFramesCount);
    ALOGD("timestampNs: %lu, sequenceId: %u", timestampNs, sequenceId);
    if (mReceiveFramesCount != 0
        && (mLastReceivedFrames.timestampNs >= svFramesDesc.timestampNs
            || mLastReceivedFrames.sequenceId >= svFramesDesc.sequenceId)) {
        mAllFramesValid = false;
        ALOGD("The incoming frames are with invalid timestamp or sequenceId!");
    }

    for (int i=0; i<svFramesDesc.svBuffers.size(); i++) {
        if (svFramesDesc.svBuffers[i].hardwareBuffer.nativeHandle == nullptr) {
            mAllFramesValid = false;
            ALOGD("The incoming frames are with invalid nativeHandle!");
            break;
        }
    }

    mReceiveFramesCount++;

    // Store all the information except for the handle
    mLastReceivedFrames.timestampNs = svFramesDesc.timestampNs;
    mLastReceivedFrames.sequenceId = svFramesDesc.sequenceId;
    mLastReceivedFrames.svBuffers.resize(svFramesDesc.svBuffers.size());
    for (int i=0; i<svFramesDesc.svBuffers.size(); i++) {
        mLastReceivedFrames.svBuffers[i].viewId = svFramesDesc.svBuffers[i].viewId;
        mLastReceivedFrames.svBuffers[i].hardwareBuffer.description =
            svFramesDesc.svBuffers[i].hardwareBuffer.description;
    }

    if (!mDoNotReturnFrames) {
        mSession->doneWithFrames(svFramesDesc);
    }

    return android::hardware::Void();
}

bool SurroundViewServiceHandler::checkEventReceived(SvEvent svEvent) {
    ALOGD("SurroundViewServiceHandler::checkEventReceived");
    int size = mReceivedEvents.size(); // work around
    ALOGD("Received event number: %d", size);
    auto iter = find(mReceivedEvents.begin(), mReceivedEvents.end(), svEvent);
    return iter != mReceivedEvents.end();
}

SvFramesDesc SurroundViewServiceHandler::getLastReceivedFrames() {
    return mLastReceivedFrames;
}

int SurroundViewServiceHandler::getReceiveFramesCount() {
    return mReceiveFramesCount;
}

bool SurroundViewServiceHandler::areAllFramesValid() {
    return mAllFramesValid;
}

void SurroundViewServiceHandler::setDoNotReturnFrames(bool flag) {
    mDoNotReturnFrames = flag;
}
