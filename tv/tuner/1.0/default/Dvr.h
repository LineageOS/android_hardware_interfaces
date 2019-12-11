/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_0_DVR_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_0_DVR_H_

#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <fmq/MessageQueue.h>
#include <math.h>
#include <set>
#include "Demux.h"
#include "Frontend.h"
#include "Tuner.h"

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

using ::android::hardware::EventFlag;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::MQDescriptorSync;
using ::android::hardware::tv::tuner::V1_0::IDemux;
using ::android::hardware::tv::tuner::V1_0::IDvrCallback;
using ::android::hardware::tv::tuner::V1_0::Result;

using DvrMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

class Demux;
class Filter;
class Frontend;
class Tuner;

class Dvr : public IDvr {
  public:
    Dvr();

    Dvr(DvrType type, uint32_t bufferSize, const sp<IDvrCallback>& cb, sp<Demux> demux);

    ~Dvr();

    virtual Return<void> getQueueDesc(getQueueDesc_cb _hidl_cb) override;

    virtual Return<Result> configure(const DvrSettings& settings) override;

    virtual Return<Result> attachFilter(const sp<IFilter>& filter) override;

    virtual Return<Result> detachFilter(const sp<IFilter>& filter) override;

    virtual Return<Result> start() override;

    virtual Return<Result> stop() override;

    virtual Return<Result> flush() override;

    virtual Return<Result> close() override;

    /**
     * To create a DvrMQ and its Event Flag.
     *
     * Return false is any of the above processes fails.
     */
    bool createDvrMQ();
    void sendBroadcastInputToDvrRecord(vector<uint8_t> byteBuffer);
    bool writeRecordFMQ(const std::vector<uint8_t>& data);

  private:
    // Demux service
    sp<Demux> mDemux;

    DvrType mType;
    uint32_t mBufferSize;
    sp<IDvrCallback> mCallback;
    std::map<uint32_t, sp<IFilter>> mFilters;

    void deleteEventFlag();
    bool readDataFromMQ();
    void maySendPlaybackStatusCallback();
    void maySendRecordStatusCallback();
    PlaybackStatus checkPlaybackStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                             uint32_t highThreshold, uint32_t lowThreshold);
    RecordStatus checkRecordStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                         uint32_t highThreshold, uint32_t lowThreshold);
    /**
     * A dispatcher to read and dispatch input data to all the started filters.
     * Each filter handler handles the data filtering/output writing/filterEvent updating.
     */
    bool readPlaybackFMQ();
    void startTpidFilter(vector<uint8_t> data);
    bool startFilterDispatcher();
    static void* __threadLoopPlayback(void* user);
    static void* __threadLoopRecord(void* user);
    void playbackThreadLoop();
    void recordThreadLoop();

    unique_ptr<DvrMQ> mDvrMQ;
    EventFlag* mDvrEventFlag;
    /**
     * Demux callbacks used on filter events or IO buffer status
     */
    bool mDvrConfigured = false;
    DvrSettings mDvrSettings;

    // Thread handlers
    pthread_t mDvrThread;
    pthread_t mBroadcastInputThread;

    // FMQ status local records
    PlaybackStatus mPlaybackStatus;
    RecordStatus mRecordStatus;
    /**
     * If a specific filter's writing loop is still running
     */
    bool mDvrThreadRunning;
    bool mBroadcastInputThreadRunning;
    bool mKeepFetchingDataFromFrontend;
    /**
     * Lock to protect writes to the FMQs
     */
    std::mutex mWriteLock;
    /**
     * Lock to protect writes to the input status
     */
    std::mutex mPlaybackStatusLock;
    std::mutex mRecordStatusLock;
    std::mutex mBroadcastInputThreadLock;
    std::mutex mDvrThreadLock;

    const bool DEBUG_DVR = false;

    // Booleans to check if recording is running.
    // Recording is ready when both of the following are set to true.
    bool mIsRecordStarted = false;
    bool mIsRecordFilterAttached = false;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_0_DVR_H_