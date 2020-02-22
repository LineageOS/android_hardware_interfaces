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

#include "FrameHandlerUltrasonics.h"

#include <android-base/logging.h>
#include <hidlmemory/mapping.h>
#include <android/hidl/memory/1.0/IMemory.h>

using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::Return;
using ::android::sp;

using ::android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArrayStream;
using ::android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArray;
using ::android::hardware::automotive::evs::V1_1::UltrasonicsDataFrameDesc;
using ::android::hardware::automotive::evs::V1_1::EvsEventDesc;
using ::android::hardware::automotive::evs::V1_1::EvsEventType;

FrameHandlerUltrasonics::FrameHandlerUltrasonics(sp<IEvsUltrasonicsArray> pEvsUltrasonicsArray) :
    mEvsUltrasonicsArray(pEvsUltrasonicsArray), mReceiveFramesCount(0) {
    // Nothing but member initialization
}

Return<void> FrameHandlerUltrasonics::notify(const EvsEventDesc& evsEvent) {
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

    return android::hardware::Void();
}

// Struct used by SerializeWaveformData().
struct WaveformData {
    uint8_t receiverId;
    std::vector<std::pair<float, float>> readings;
};

// De-serializes shared memory to vector of WaveformData.
// TODO(b/149950362): Add a common library for serialiazing and deserializing waveform data.
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

bool DataFrameValidator(const UltrasonicsDataFrameDesc& dataFrameDesc) {

    if (dataFrameDesc.receiversIdList.size() != dataFrameDesc.receiversReadingsCountList.size()) {
        LOG(ERROR) << "Size mismatch of receiversIdList and receiversReadingsCountList";
        return false;
    }

    if(!dataFrameDesc.waveformsData.valid()) {
        LOG(ERROR) << "Data frame does not valid hidl memory";
        return false;
    }

    // Check total bytes from dataFrameDesc are within the shared memory size.
    int totalWaveformDataBytesSize = 0;
    for (int i = 0; i < dataFrameDesc.receiversIdList.size(); i++) {
        totalWaveformDataBytesSize = 1 + (4 * 2 * dataFrameDesc.receiversReadingsCountList[i]);
    }
    if (totalWaveformDataBytesSize > dataFrameDesc.waveformsData.size()) {
        LOG(ERROR) << "Total waveform data bytes in desc exceed shared memory size";
        return false;
    }

    sp<IMemory> pIMemory = mapMemory(dataFrameDesc.waveformsData);
    if(pIMemory.get() == nullptr) {
        LOG(ERROR) << "Failed to map hidl memory";
        return false;
    }

    uint8_t* pData = (uint8_t*)((void*)pIMemory->getPointer());
    if(pData == nullptr) {
        LOG(ERROR) << "Failed getPointer from mapped shared memory";
        return false;
    }

    const std::vector<WaveformData> waveformDataList = DeSerializeWaveformData(
            dataFrameDesc.receiversReadingsCountList, pData);

    // Verify the waveforms data.
    for(int i = 0; i < waveformDataList.size(); i++) {
        if (waveformDataList[i].receiverId != dataFrameDesc.receiversIdList[i]) {
            LOG(ERROR) << "Receiver Id mismatch";
            return false;
        }
        for(auto& reading : waveformDataList[i].readings) {
            if (reading.second < 0.0f || reading.second > 1.0f) {
                LOG(ERROR) << "Resonance reading is not in range [0, 1]";
                return false;
            }
        }
    }
    return true;
}

Return<void> FrameHandlerUltrasonics::deliverDataFrame(
        const UltrasonicsDataFrameDesc& dataFrameDesc) {
    LOG(DEBUG) << "FrameHandlerUltrasonics::receiveFrames";

    mReceiveFramesCount++;
    mLastReceivedFrames = dataFrameDesc;

    if(!DataFrameValidator(dataFrameDesc)) {
        mAllFramesValid = false;
    }

    // Send done with data frame.
    mEvsUltrasonicsArray->doneWithDataFrame(dataFrameDesc);

    return android::hardware::Void();
}

bool FrameHandlerUltrasonics::checkEventReceived(EvsEventDesc evsEvent) {
    LOG(DEBUG) << "FrameHandlerUltrasonics::checkEventReceived";
    int size = mReceivedEvents.size(); // work around
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
