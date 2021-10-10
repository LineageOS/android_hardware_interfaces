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

#include "FrameHandlerUltrasonics.h"

#include <aidl/android/hardware/automotive/evs/EvsEventDesc.h>
#include <aidl/android/hardware/automotive/evs/EvsEventType.h>
#include <aidl/android/hardware/automotive/evs/IEvsUltrasonicsArray.h>
#include <aidl/android/hardware/automotive/evs/UltrasonicsDataFrameDesc.h>
#include <android-base/logging.h>

using ::aidl::android::hardware::automotive::evs::EvsEventDesc;
using ::aidl::android::hardware::automotive::evs::EvsEventType;
using ::aidl::android::hardware::automotive::evs::IEvsUltrasonicsArray;
using ::aidl::android::hardware::automotive::evs::UltrasonicsDataFrameDesc;
using ::ndk::ScopedAStatus;

namespace {

// Struct used by SerializeWaveformData().
struct WaveformData {
    uint8_t receiverId;
    std::vector<std::pair<float, float>> readings;
};

}  // namespace

FrameHandlerUltrasonics::FrameHandlerUltrasonics(
        const std::shared_ptr<IEvsUltrasonicsArray>& pArray)
    : mEvsUltrasonicsArray(pArray), mReceiveFramesCount(0) {
    // Nothing but member initialization
}

ScopedAStatus FrameHandlerUltrasonics::notify(const EvsEventDesc& evsEvent) {
    switch (evsEvent.aType) {
        case EvsEventType::STREAM_STARTED:
        case EvsEventType::STREAM_STOPPED:
        case EvsEventType::FRAME_DROPPED:
        case EvsEventType::TIMEOUT:
            mReceivedEvents.emplace_back(evsEvent);
            break;
        default:
            LOG(ERROR) << "Received unexpected event";
    }

    return ScopedAStatus::ok();
}

// De-serializes shared memory to vector of WaveformData.
// TODO(b/149950362): Add a common library for serializing and deserializing waveform data.
std::vector<WaveformData> DeSerializeWaveformData(std::vector<uint32_t> recvReadingsCountList,
                                                  uint8_t* pData) {
    std::vector<WaveformData> waveformDataList(recvReadingsCountList.size());

    for (int i = 0; i < waveformDataList.size(); i++) {
        // Set Id
        memcpy(&waveformDataList[i].receiverId, pData, sizeof(uint8_t));
        pData += sizeof(uint8_t);

        waveformDataList[i].readings.resize(recvReadingsCountList[i]);

        for (auto& reading : waveformDataList[i].readings) {
            // Set the time of flight.
            memcpy(&reading.first, pData, sizeof(float));
            pData += sizeof(float);

            // Set the resonance.
            memcpy(&reading.second, pData, sizeof(float));
            pData += sizeof(float);
        }
    }
    return waveformDataList;
}

bool DataFrameValidator(const UltrasonicsDataFrameDesc& /*dataFrameDesc*/) {
    // TODO(b/214026378): implement a method to validate an ultrasonics data frame
    (void)DeSerializeWaveformData;
    return true;
}

ScopedAStatus FrameHandlerUltrasonics::deliverDataFrame(
        const UltrasonicsDataFrameDesc& dataFrameDesc) {
    LOG(DEBUG) << "FrameHandlerUltrasonics::receiveFrames";

    mReceiveFramesCount++;

    if (!DataFrameValidator(dataFrameDesc)) {
        mAllFramesValid = false;
    }

    // Send done with data frame.
    mEvsUltrasonicsArray->doneWithDataFrame(dataFrameDesc);
    return ScopedAStatus::ok();
}

bool FrameHandlerUltrasonics::checkEventReceived(const EvsEventDesc& evsEvent) {
    LOG(DEBUG) << "FrameHandlerUltrasonics::checkEventReceived";
    int size = mReceivedEvents.size();  // work around
    LOG(DEBUG) << "Received event number: " << size;
    auto iter = find(mReceivedEvents.begin(), mReceivedEvents.end(), evsEvent);
    return iter != mReceivedEvents.end();
}

int FrameHandlerUltrasonics::getReceiveFramesCount() {
    return mReceiveFramesCount;
}

bool FrameHandlerUltrasonics::areAllFramesValid() {
    return mAllFramesValid;
}
