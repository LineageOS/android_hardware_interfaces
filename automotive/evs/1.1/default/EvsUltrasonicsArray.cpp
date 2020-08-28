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

#include "EvsUltrasonicsArray.h"

#include <android-base/logging.h>
#include <hidlmemory/mapping.h>
#include <log/log.h>
#include <time.h>
#include <utils/SystemClock.h>
#include <utils/Timers.h>

namespace android {
namespace hardware {
namespace automotive {
namespace evs {
namespace V1_1 {
namespace implementation {

// Arbitrary limit on number of data frames allowed to be allocated
// Safeguards against unreasonable resource consumption and provides a testable limit
const unsigned int kMaximumDataFramesInFlight = 100;

const uint32_t kMaxReadingsPerSensor = 5;
const uint32_t kMaxReceiversCount = 3;

const unsigned int kSharedMemoryMaxSize =
        kMaxReadingsPerSensor * kMaxReceiversCount * 2 * sizeof(float);

// Target frame rate in frames per second.
const int kTargetFrameRate = 10;

namespace {

void fillDummyArrayDesc(UltrasonicsArrayDesc& arrayDesc) {
    arrayDesc.maxReadingsPerSensorCount = kMaxReadingsPerSensor;
    arrayDesc.maxReceiversCount = kMaxReceiversCount;

    const int kSensorCount = 3;
    const float kMaxRange = 4000;                // 4 metres.
    const float kAngleOfMeasurement = 0.261799;  // 15 degrees.

    std::vector<UltrasonicSensor> sensors(kSensorCount);

    // Sensor pointing forward on left side of front bumper.
    sensors[0].maxRange = kMaxRange;
    sensors[0].angleOfMeasurement = kAngleOfMeasurement;
    sensors[0].pose = {{1, 0, 0, 0}, {-1000, 2000, 200}};

    // Sensor pointing forward on center of front bumper.
    sensors[1].maxRange = kMaxRange;
    sensors[1].angleOfMeasurement = kAngleOfMeasurement;
    sensors[1].pose = {{1, 0, 0, 0}, {0, 2000, 200}};

    // Sensor pointing forward on right side of front bumper.
    sensors[2].maxRange = kMaxRange;
    sensors[2].angleOfMeasurement = kAngleOfMeasurement;
    sensors[2].pose = {{1, 0, 0, 0}, {1000, 2000, 200}};

    arrayDesc.sensors = sensors;
}

// Struct used by SerializeWaveformData().
struct WaveformData {
    uint8_t receiverId;
    std::vector<std::pair<float, float>> readings;
};

// Serializes data provided in waveformDataList to a shared memory data pointer.
// TODO(b/149950362): Add a common library for serialiazing and deserializing waveform data.
void SerializeWaveformData(const std::vector<WaveformData>& waveformDataList, uint8_t* pData) {
    for (auto& waveformData : waveformDataList) {
        // Set Id
        memcpy(pData, &waveformData.receiverId, sizeof(uint8_t));
        pData += sizeof(uint8_t);

        for (auto& reading : waveformData.readings) {
            // Set the time of flight.
            memcpy(pData, &reading.first, sizeof(float));
            pData += sizeof(float);

            // Set the resonance.
            memcpy(pData, &reading.second, sizeof(float));
            pData += sizeof(float);
        }
    }
}

// Fills dataFrameDesc with dummy data.
bool fillDummyDataFrame(UltrasonicsDataFrameDesc& dataFrameDesc, sp<IMemory> pIMemory) {
    dataFrameDesc.timestampNs = elapsedRealtimeNano();

    const std::vector<uint8_t> transmittersIdList = {0};
    dataFrameDesc.transmittersIdList = transmittersIdList;

    const std::vector<uint8_t> recvIdList = {0, 1, 2};
    dataFrameDesc.receiversIdList = recvIdList;

    const std::vector<uint32_t> receiversReadingsCountList = {2, 2, 4};
    dataFrameDesc.receiversReadingsCountList = receiversReadingsCountList;

    const std::vector<WaveformData> waveformDataList = {
            {recvIdList[0], { {1000, 0.1f}, {2000, 0.8f} }},
            {recvIdList[1], { {1000, 0.1f}, {2000, 1.0f} }},
            {recvIdList[2], { {1000, 0.1f}, {2000, 0.2f}, {4000, 0.2f}, {5000, 0.1f} }}
    };

    if (pIMemory.get() == nullptr) {
        return false;
    }

    uint8_t* pData = (uint8_t*)((void*)pIMemory->getPointer());

    pIMemory->update();
    SerializeWaveformData(waveformDataList, pData);
    pIMemory->commit();

    return true;
}

}  // namespace

EvsUltrasonicsArray::EvsUltrasonicsArray(const char* deviceName)
    : mFramesAllowed(0), mFramesInUse(0), mStreamState(STOPPED) {
    LOG(DEBUG) << "EvsUltrasonicsArray instantiated";

    // Set up dummy data for description.
    mArrayDesc.ultrasonicsArrayId = deviceName;
    fillDummyArrayDesc(mArrayDesc);

    // Assign allocator.
    mShmemAllocator = IAllocator::getService("ashmem");
    if (mShmemAllocator.get() == nullptr) {
        LOG(ERROR) << "SurroundViewHidlTest getService ashmem failed";
    }
}

sp<EvsUltrasonicsArray> EvsUltrasonicsArray::Create(const char* deviceName) {
    return sp<EvsUltrasonicsArray>(new EvsUltrasonicsArray(deviceName));
}

EvsUltrasonicsArray::~EvsUltrasonicsArray() {
    LOG(DEBUG) << "EvsUltrasonicsArray being destroyed";
    forceShutdown();
}

// This gets called if another caller "steals" ownership of the ultrasonic array.
void EvsUltrasonicsArray::forceShutdown() {
    LOG(DEBUG) << "EvsUltrasonicsArray forceShutdown";

    // Make sure our output stream is cleaned up
    // (It really should be already)
    stopStream();

    // Claim the lock while we work on internal state
    std::lock_guard<std::mutex> lock(mAccessLock);

    // Drop all the data frames we've been using
    for (auto&& dataFrame : mDataFrames) {
        if (dataFrame.inUse) {
            LOG(ERROR) << "Error - releasing data frame despite remote ownership";
        }
        dataFrame.sharedMemory.clear();
    }
    mDataFrames.clear();

    // Put this object into an unrecoverable error state since somebody else
    // is going to own the underlying ultrasonic array now
    mStreamState = DEAD;
}

UltrasonicsArrayDesc EvsUltrasonicsArray::GetDummyArrayDesc(const char* deviceName) {
    UltrasonicsArrayDesc ultrasonicsArrayDesc;
    ultrasonicsArrayDesc.ultrasonicsArrayId = deviceName;
    fillDummyArrayDesc(ultrasonicsArrayDesc);
    return ultrasonicsArrayDesc;
}

Return<void> EvsUltrasonicsArray::getUltrasonicArrayInfo(getUltrasonicArrayInfo_cb _get_info_cb) {
    LOG(DEBUG) << "EvsUltrasonicsArray getUltrasonicsArrayInfo";

    // Return the description for the get info callback.
    _get_info_cb(mArrayDesc);

    return Void();
}

Return<EvsResult> EvsUltrasonicsArray::setMaxFramesInFlight(uint32_t bufferCount) {
    LOG(DEBUG) << "EvsUltrasonicsArray setMaxFramesInFlight";

    // Lock mutex for performing changes to available frames.
    std::lock_guard<std::mutex> lock(mAccessLock);

    // We cannot function without at least one buffer to send data.
    if (bufferCount < 1) {
        LOG(ERROR) << "Ignoring setMaxFramesInFlight with less than one buffer requested";
        return EvsResult::INVALID_ARG;
    }

    // Update our internal state of buffer count.
    if (setAvailableFrames_Locked(bufferCount)) {
        return EvsResult::OK;
    } else {
        return EvsResult::BUFFER_NOT_AVAILABLE;
    }

    return EvsResult::OK;
}

Return<void> EvsUltrasonicsArray::doneWithDataFrame(const UltrasonicsDataFrameDesc& dataFrameDesc) {
    LOG(DEBUG) << "EvsUltrasonicsArray doneWithFrame";

    std::lock_guard<std::mutex> lock(mAccessLock);

    if (dataFrameDesc.dataFrameId >= mDataFrames.size()) {
        LOG(ERROR) << "ignoring doneWithFrame called with invalid dataFrameId "
                   << dataFrameDesc.dataFrameId << "(max is " << mDataFrames.size() - 1 << ")";
        return Void();
    }

    if (!mDataFrames[dataFrameDesc.dataFrameId].inUse) {
        LOG(ERROR) << "ignoring doneWithFrame called on frame " << dataFrameDesc.dataFrameId
                   << "which is already free";
        return Void();
    }

    // Mark the frame as available
    mDataFrames[dataFrameDesc.dataFrameId].inUse = false;
    mFramesInUse--;

    // If this frame's index is high in the array, try to move it down
    // to improve locality after mFramesAllowed has been reduced.
    if (dataFrameDesc.dataFrameId >= mFramesAllowed) {
        // Find an empty slot lower in the array (which should always exist in this case)
        for (auto&& dataFrame : mDataFrames) {
            if (!dataFrame.sharedMemory.IsValid()) {
                dataFrame.sharedMemory = mDataFrames[dataFrameDesc.dataFrameId].sharedMemory;
                mDataFrames[dataFrameDesc.dataFrameId].sharedMemory.clear();
                return Void();
            }
        }
    }

    return Void();
}

Return<EvsResult> EvsUltrasonicsArray::startStream(
        const ::android::sp<IEvsUltrasonicsArrayStream>& stream) {
    LOG(DEBUG) << "EvsUltrasonicsArray startStream";

    std::lock_guard<std::mutex> lock(mAccessLock);

    if (mStreamState != STOPPED) {
        LOG(ERROR) << "ignoring startStream call when a stream is already running.";
        return EvsResult::STREAM_ALREADY_RUNNING;
    }

    // If the client never indicated otherwise, configure ourselves for a single streaming buffer
    if (mFramesAllowed < 1) {
        if (!setAvailableFrames_Locked(1)) {
            LOG(ERROR)
                    << "Failed to start stream because we couldn't get shared memory data buffer";
            return EvsResult::BUFFER_NOT_AVAILABLE;
        }
    }

    // Record the user's callback for use when we have a frame ready
    mStream = stream;

    // Start the frame generation thread
    mStreamState = RUNNING;
    mCaptureThread = std::thread([this]() { generateDataFrames(); });

    return EvsResult::OK;
}

Return<void> EvsUltrasonicsArray::stopStream() {
    LOG(DEBUG) << "EvsUltrasonicsArray stopStream";

    bool streamStateStopping = false;
    {
        std::lock_guard<std::mutex> lock(mAccessLock);
        if (mStreamState == RUNNING) {
            // Tell the GenerateFrames loop we want it to stop
            mStreamState = STOPPING;
            streamStateStopping = true;
        }
    }

    if (streamStateStopping) {
        // Block outside the mutex until the "stop" flag has been acknowledged
        // We won't send any more frames, but the client might still get some already in flight
        LOG(DEBUG) << "Waiting for stream thread to end...";
        mCaptureThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(mAccessLock);
        mStreamState = STOPPED;
        mStream = nullptr;
        LOG(DEBUG) << "Stream marked STOPPED.";
    }

    return Void();
}

bool EvsUltrasonicsArray::setAvailableFrames_Locked(unsigned bufferCount) {
    if (bufferCount < 1) {
        LOG(ERROR) << "Ignoring request to set buffer count to zero";
        return false;
    }
    if (bufferCount > kMaximumDataFramesInFlight) {
        LOG(ERROR) << "Rejecting buffer request in excess of internal limit";
        return false;
    }

    // Is an increase required?
    if (mFramesAllowed < bufferCount) {
        // An increase is required
        unsigned needed = bufferCount - mFramesAllowed;
        LOG(INFO) << "Number of data frame buffers to add: " << needed;

        unsigned added = increaseAvailableFrames_Locked(needed);
        if (added != needed) {
            // If we didn't add all the frames we needed, then roll back to the previous state
            LOG(ERROR) << "Rolling back to previous frame queue size";
            decreaseAvailableFrames_Locked(added);
            return false;
        }
    } else if (mFramesAllowed > bufferCount) {
        // A decrease is required
        unsigned framesToRelease = mFramesAllowed - bufferCount;
        LOG(INFO) << "Number of data frame buffers to reduce: " << framesToRelease;

        unsigned released = decreaseAvailableFrames_Locked(framesToRelease);
        if (released != framesToRelease) {
            // This shouldn't happen with a properly behaving client because the client
            // should only make this call after returning sufficient outstanding buffers
            // to allow a clean resize.
            LOG(ERROR) << "Buffer queue shrink failed -- too many buffers currently in use?";
        }
    }

    return true;
}

EvsUltrasonicsArray::SharedMemory EvsUltrasonicsArray::allocateAndMapSharedMemory() {
    SharedMemory sharedMemory;

    // Check shared memory allocator is valid.
    if (mShmemAllocator.get() == nullptr) {
        LOG(ERROR) << "Shared memory allocator not initialized.";
        return SharedMemory();
    }

    // Allocate memory.
    bool allocateSuccess = false;
    Return<void> result = mShmemAllocator->allocate(kSharedMemoryMaxSize,
                                                    [&](bool success, const hidl_memory& hidlMem) {
                                                        if (!success) {
                                                            return;
                                                        }
                                                        allocateSuccess = success;
                                                        sharedMemory.hidlMemory = hidlMem;
                                                    });

    // Check result of allocated memory.
    if (!result.isOk() || !allocateSuccess) {
        LOG(ERROR) << "Shared memory allocation failed.";
        return SharedMemory();
    }

    // Map shared memory.
    sharedMemory.pIMemory = mapMemory(sharedMemory.hidlMemory);
    if (sharedMemory.pIMemory.get() == nullptr) {
        LOG(ERROR) << "Shared memory mapping failed.";
        return SharedMemory();
    }

    // Return success.
    return sharedMemory;
}

unsigned EvsUltrasonicsArray::increaseAvailableFrames_Locked(unsigned numToAdd) {
    unsigned added = 0;

    while (added < numToAdd) {
        SharedMemory sharedMemory = allocateAndMapSharedMemory();

        // If allocate and map fails, break.
        if (!sharedMemory.IsValid()) {
            break;
        }

        // Find a place to store the new buffer
        bool stored = false;
        for (auto&& dataFrame : mDataFrames) {
            if (!dataFrame.sharedMemory.IsValid()) {
                // Use this existing entry
                dataFrame.sharedMemory = sharedMemory;
                dataFrame.inUse = false;
                stored = true;
                break;
            }
        }

        if (!stored) {
            // Add a BufferRecord wrapping this handle to our set of available buffers
            mDataFrames.emplace_back(sharedMemory);
        }

        mFramesAllowed++;
        added++;
    }

    return added;
}

unsigned EvsUltrasonicsArray::decreaseAvailableFrames_Locked(unsigned numToRemove) {
    unsigned removed = 0;

    for (auto&& dataFrame : mDataFrames) {
        // Is this record not in use, but holding a buffer that we can free?
        if (!dataFrame.inUse && dataFrame.sharedMemory.IsValid()) {
            // Release buffer and update the record so we can recognize it as "empty"
            dataFrame.sharedMemory.clear();

            mFramesAllowed--;
            removed++;

            if (removed == numToRemove) {
                break;
            }
        }
    }

    return removed;
}

// This is the asynchronous data frame generation thread that runs in parallel with the
// main serving thread. There is one for each active ultrasonic array instance.
void EvsUltrasonicsArray::generateDataFrames() {
    LOG(DEBUG) << "Data frame generation loop started";

    unsigned idx = 0;

    while (true) {
        bool timeForFrame = false;

        nsecs_t startTime = elapsedRealtimeNano();

        // Lock scope for updating shared state
        {
            std::lock_guard<std::mutex> lock(mAccessLock);

            if (mStreamState != RUNNING) {
                // Break out of our main thread loop
                break;
            }

            // Are we allowed to issue another buffer?
            if (mFramesInUse >= mFramesAllowed) {
                // Can't do anything right now -- skip this frame
                LOG(WARNING) << "Skipped a frame because too many are in flight";
            } else {
                // Identify an available buffer to fill
                for (idx = 0; idx < mDataFrames.size(); idx++) {
                    if (!mDataFrames[idx].inUse && mDataFrames[idx].sharedMemory.IsValid()) {
                        // Found an available record, so stop looking
                        break;
                    }
                }
                if (idx >= mDataFrames.size()) {
                    // This shouldn't happen since we already checked mFramesInUse vs mFramesAllowed
                    LOG(ERROR) << "Failed to find an available buffer slot";
                } else {
                    // We're going to make the frame busy
                    mDataFrames[idx].inUse = true;
                    mFramesInUse++;
                    timeForFrame = true;
                }
            }
        }

        if (timeForFrame) {
            // Assemble the buffer description we'll transmit below
            UltrasonicsDataFrameDesc dummyDataFrameDesc;
            dummyDataFrameDesc.dataFrameId = idx;
            dummyDataFrameDesc.waveformsData = mDataFrames[idx].sharedMemory.hidlMemory;

            // Fill dummy waveform data.
            fillDummyDataFrame(dummyDataFrameDesc, mDataFrames[idx].sharedMemory.pIMemory);

            // Issue the (asynchronous) callback to the client -- can't be holding the lock
            auto result = mStream->deliverDataFrame(dummyDataFrameDesc);
            if (result.isOk()) {
                LOG(DEBUG) << "Delivered data frame id: " << dummyDataFrameDesc.dataFrameId;
            } else {
                // This can happen if the client dies and is likely unrecoverable.
                // To avoid consuming resources generating failing calls, we stop sending
                // frames.  Note, however, that the stream remains in the "STREAMING" state
                // until cleaned up on the main thread.
                LOG(ERROR) << "Frame delivery call failed in the transport layer.";

                // Since we didn't actually deliver it, mark the frame as available
                std::lock_guard<std::mutex> lock(mAccessLock);
                mDataFrames[idx].inUse = false;
                mFramesInUse--;

                break;
            }
        }

        // Sleep to generate frames at kTargetFrameRate.
        static const nsecs_t kTargetFrameTimeUs = 1000 * 1000 / kTargetFrameRate;
        const nsecs_t now = elapsedRealtimeNano();
        const nsecs_t workTimeUs = (now - startTime) / 1000;
        const nsecs_t sleepDurationUs = kTargetFrameTimeUs - workTimeUs;
        if (sleepDurationUs > 0) {
            usleep(sleepDurationUs);
        }
    }

    // If we've been asked to stop, send an event to signal the actual end of stream
    EvsEventDesc event;
    event.aType = EvsEventType::STREAM_STOPPED;
    auto result = mStream->notify(event);
    if (!result.isOk()) {
        LOG(ERROR) << "Error delivering end of stream marker";
    }
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace evs
}  // namespace automotive
}  // namespace hardware
}  // namespace android
