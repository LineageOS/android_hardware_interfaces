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

#ifndef SURROUND_VIEW_STREAM_HANDLER_H
#define SURROUND_VIEW_STREAM_HANDLER_H

#include <android/hardware/automotive/sv/1.0/types.h>
#include <android/hardware/automotive/sv/1.0/ISurroundViewStream.h>
#include <android/hardware/automotive/sv/1.0/ISurroundViewSession.h>

#include <thread>
#include <vector>

using std::vector;
using std::mutex;
using android::hardware::Return;
using android::sp;
using namespace ::android::hardware::automotive::sv::V1_0;

class SurroundViewServiceHandler : public ISurroundViewStream {
public:
    SurroundViewServiceHandler(sp<ISurroundViewSession> session);

    Return<void> notify(SvEvent svEvent) override;
    Return<void> receiveFrames(const SvFramesDesc& svFramesDesc) override;

    bool checkEventReceived(SvEvent svEvent);
    SvFramesDesc getLastReceivedFrames();
    int getReceiveFramesCount();
    bool areAllFramesValid();
    void setDoNotReturnFrames(bool flag);

private:
    mutex mLock;

    vector<SvEvent> mReceivedEvents;
    sp<ISurroundViewSession> mSession;
    SvFramesDesc mLastReceivedFrames; // only use timestampNs and sequenceId
    int mReceiveFramesCount; // TODO(haoxiangl): figure out a better name
    bool mAllFramesValid = true;
    bool mDoNotReturnFrames;
};

#endif //SURROUND_VIEW_STREAM_HANDLER_H
