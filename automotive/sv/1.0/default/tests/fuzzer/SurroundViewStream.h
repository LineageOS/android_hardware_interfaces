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

#pragma once

#include <android/hardware/automotive/sv/1.0/ISurroundViewSession.h>
#include <android/hardware/automotive/sv/1.0/ISurroundViewStream.h>
#include <android/hardware/automotive/sv/1.0/types.h>

#include <thread>
#include <vector>

namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer {

using android::sp;
using android::hardware::Return;
using std::mutex;
using std::vector;

class SurroundViewStream : public ISurroundViewStream {
  public:
    SurroundViewStream(sp<ISurroundViewSession> session);

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
    SvFramesDesc mLastReceivedFrames;
    int mReceiveFramesCount;
    bool mAllFramesValid = true;
};
}  // namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer
