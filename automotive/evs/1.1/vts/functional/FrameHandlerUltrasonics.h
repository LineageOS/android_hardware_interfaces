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

#ifndef FRAME_HANDLER_ULTRASONICS_H
#define FRAME_HANDLER_ULTRASONICS_H

#include <android/hardware/automotive/evs/1.1/types.h>
#include <android/hardware/automotive/evs/1.1/IEvsUltrasonicsArrayStream.h>
#include <android/hardware/automotive/evs/1.1/IEvsUltrasonicsArray.h>

#include <vector>

class FrameHandlerUltrasonics : public
        android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArrayStream {
public:
    FrameHandlerUltrasonics(
            android::sp<android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArray>
            pEvsUltrasonicsArray);

    // Implementation for ::android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArrayStream
    android::hardware::Return<void> notify(
            const android::hardware::automotive::evs::V1_1::EvsEventDesc& evsEvent) override;
    android::hardware::Return<void> deliverDataFrame(
            const android::hardware::automotive::evs::V1_1::UltrasonicsDataFrameDesc&
             dataFrameDesc) override;

    bool checkEventReceived(android::hardware::automotive::evs::V1_1::EvsEventDesc evsEvent);
    int getReceiveFramesCount();
    bool areAllFramesValid();

private:
    android::sp<android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArray>
            mEvsUltrasonicsArray;
    android::hardware::automotive::evs::V1_1::UltrasonicsDataFrameDesc mLastReceivedFrames;
    std::vector<android::hardware::automotive::evs::V1_1::EvsEventDesc> mReceivedEvents;
    int mReceiveFramesCount;
    bool mAllFramesValid = true;
};

#endif //FRAME_HANDLER_ULTRASONICS_H
