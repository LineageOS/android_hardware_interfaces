/*
 * Copyright 2022 The Android Open Source Project
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

#ifndef AUTOMOTIVE_EVS_VTS_FRAMEHANDLERULTRASONICS_H
#define AUTOMOTIVE_EVS_VTS_FRAMEHANDLERULTRASONICS_H

#include <aidl/android/hardware/automotive/evs/BnEvsUltrasonicsArrayStream.h>
#include <aidl/android/hardware/automotive/evs/IEvsUltrasonicsArray.h>

#include <vector>

class FrameHandlerUltrasonics
    : public ::aidl::android::hardware::automotive::evs::BnEvsUltrasonicsArrayStream {
  public:
    FrameHandlerUltrasonics(
            const std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsUltrasonicsArray>&
                    pArray);

    // Implementation for ::aidl::android::hardware::automotive::evs::IEvsUltrasonicsArrayStream
    ::ndk::ScopedAStatus notify(
            const ::aidl::android::hardware::automotive::evs::EvsEventDesc& event) override;
    ::ndk::ScopedAStatus deliverDataFrame(
            const ::aidl::android::hardware::automotive::evs::UltrasonicsDataFrameDesc& desc)
            override;

    bool checkEventReceived(
            const ::aidl::android::hardware::automotive::evs::EvsEventDesc& evsEvent);
    int getReceiveFramesCount();
    bool areAllFramesValid();

  private:
    std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsUltrasonicsArray>
            mEvsUltrasonicsArray;
    std::vector<::aidl::android::hardware::automotive::evs::EvsEventDesc> mReceivedEvents;
    int mReceiveFramesCount;
    bool mAllFramesValid = true;
};

#endif  // AUTOMOTIVE_EVS_VTS_FRAMEHANDLERULTRASONICS_H
