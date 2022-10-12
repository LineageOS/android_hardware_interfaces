/*
 * Copyright 2021 The Android Open Source Project
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

#include <aidl/android/hardware/tv/tuner/BnDvr.h>
#include <aidl/android/hardware/tv/tuner/RecordStatus.h>

#include <fmq/AidlMessageQueue.h>
#include <math.h>
#include <atomic>
#include <set>
#include <thread>
#include "Demux.h"
#include "Frontend.h"
#include "Tuner.h"

using namespace std;

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::AidlMessageQueue;
using ::android::hardware::EventFlag;

using DvrMQ = AidlMessageQueue<int8_t, SynchronizedReadWrite>;

struct MediaEsMetaData {
    bool isAudio;
    int startIndex;
    int len;
    int pts;
};

class Demux;
class Filter;
class Frontend;
class Tuner;

class Dvr : public BnDvr {
  public:
    Dvr(DvrType type, uint32_t bufferSize, const std::shared_ptr<IDvrCallback>& cb,
        std::shared_ptr<Demux> demux);
    ~Dvr();

    ::ndk::ScopedAStatus getQueueDesc(
            MQDescriptor<int8_t, SynchronizedReadWrite>* out_queue) override;
    ::ndk::ScopedAStatus configure(const DvrSettings& in_settings) override;
    ::ndk::ScopedAStatus attachFilter(const std::shared_ptr<IFilter>& in_filter) override;
    ::ndk::ScopedAStatus detachFilter(const std::shared_ptr<IFilter>& in_filter) override;
    ::ndk::ScopedAStatus start() override;
    ::ndk::ScopedAStatus stop() override;
    ::ndk::ScopedAStatus flush() override;
    ::ndk::ScopedAStatus close() override;
    ::ndk::ScopedAStatus setStatusCheckIntervalHint(int64_t in_milliseconds) override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

    /**
     * To create a DvrMQ and its Event Flag.
     *
     * Return false is any of the above processes fails.
     */
    bool createDvrMQ();
    bool writeRecordFMQ(const std::vector<int8_t>& data);
    bool addPlaybackFilter(int64_t filterId, std::shared_ptr<IFilter> filter);
    bool removePlaybackFilter(int64_t filterId);
    bool readPlaybackFMQ(bool isVirtualFrontend, bool isRecording);
    bool processEsDataOnPlayback(bool isVirtualFrontend, bool isRecording);
    bool startFilterDispatcher(bool isVirtualFrontend, bool isRecording);
    EventFlag* getDvrEventFlag();
    DvrSettings getSettings() { return mDvrSettings; }

  private:
    // Demux service
    std::shared_ptr<Demux> mDemux;

    DvrType mType;
    uint32_t mBufferSize;
    std::shared_ptr<IDvrCallback> mCallback;
    std::map<int64_t, std::shared_ptr<IFilter>> mFilters;

    void deleteEventFlag();
    bool readDataFromMQ();
    void getMetaDataValue(int& index, int8_t* dataOutputBuffer, int& value);
    void maySendPlaybackStatusCallback();
    void maySendRecordStatusCallback();
    PlaybackStatus checkPlaybackStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                             int64_t highThreshold, int64_t lowThreshold);
    RecordStatus checkRecordStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                         int64_t highThreshold, int64_t lowThreshold);
    /**
     * A dispatcher to read and dispatch input data to all the started filters.
     * Each filter handler handles the data filtering/output writing/filterEvent updating.
     */
    void startTpidFilter(vector<int8_t> data);
    void playbackThreadLoop();

    unique_ptr<DvrMQ> mDvrMQ;
    EventFlag* mDvrEventFlag;
    /**
     * Demux callbacks used on filter events or IO buffer status
     */
    bool mDvrConfigured = false;
    DvrSettings mDvrSettings;

    // Thread handlers
    std::thread mDvrThread;

    // FMQ status local records
    PlaybackStatus mPlaybackStatus;
    RecordStatus mRecordStatus;
    /**
     * If a specific filter's writing loop is still running
     */
    std::atomic<bool> mDvrThreadRunning;

    /**
     * Lock to protect writes to the FMQs
     */
    std::mutex mWriteLock;
    /**
     * Lock to protect writes to the input status
     */
    std::mutex mPlaybackStatusLock;
    std::mutex mRecordStatusLock;

    const bool DEBUG_DVR = false;
};

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
