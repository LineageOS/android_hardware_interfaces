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

#include <aidl/android/hardware/tv/tuner/BnDemux.h>
#include <aidl/android/hardware/tv/tuner/BnDvrCallback.h>

#include <fmq/AidlMessageQueue.h>
#include <math.h>
#include <atomic>
#include <set>
#include <thread>

#include "Dvr.h"
#include "Filter.h"
#include "Frontend.h"
#include "TimeFilter.h"
#include "Timer.h"
#include "Tuner.h"
#include "dtv_plugin.h"

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

using FilterMQ = AidlMessageQueue<int8_t, SynchronizedReadWrite>;
using AidlMQ = AidlMessageQueue<int8_t, SynchronizedReadWrite>;
using AidlMQDesc = MQDescriptor<int8_t, SynchronizedReadWrite>;

class Dvr;
class Filter;
class Frontend;
class TimeFilter;
class Tuner;

class DvrPlaybackCallback : public BnDvrCallback {
  public:
    virtual ::ndk::ScopedAStatus onPlaybackStatus(PlaybackStatus status) override {
        ALOGD("demux.h: playback status %d", status);
        return ndk::ScopedAStatus::ok();
    }

    virtual ::ndk::ScopedAStatus onRecordStatus(RecordStatus status) override {
        ALOGD("Record Status %hhd", status);
        return ndk::ScopedAStatus::ok();
    }
};

class Demux : public BnDemux {
  public:
    Demux(int32_t demuxId, uint32_t filterTypes);
    ~Demux();

    ::ndk::ScopedAStatus setFrontendDataSource(int32_t in_frontendId) override;
    ::ndk::ScopedAStatus openFilter(const DemuxFilterType& in_type, int32_t in_bufferSize,
                                    const std::shared_ptr<IFilterCallback>& in_cb,
                                    std::shared_ptr<IFilter>* _aidl_return) override;
    ::ndk::ScopedAStatus openTimeFilter(std::shared_ptr<ITimeFilter>* _aidl_return) override;
    ::ndk::ScopedAStatus getAvSyncHwId(const std::shared_ptr<IFilter>& in_filter,
                                       int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getAvSyncTime(int32_t in_avSyncHwId, int64_t* _aidl_return) override;
    ::ndk::ScopedAStatus close() override;
    ::ndk::ScopedAStatus openDvr(DvrType in_type, int32_t in_bufferSize,
                                 const std::shared_ptr<IDvrCallback>& in_cb,
                                 std::shared_ptr<IDvr>* _aidl_return) override;
    ::ndk::ScopedAStatus connectCiCam(int32_t in_ciCamId) override;
    ::ndk::ScopedAStatus disconnectCiCam() override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

    // Functions interacts with Tuner Service
    void stopFrontendInput();
    ::ndk::ScopedAStatus removeFilter(int64_t filterId);
    bool attachRecordFilter(int64_t filterId);
    bool detachRecordFilter(int64_t filterId);
    ::ndk::ScopedAStatus startFilterHandler(int64_t filterId);
    void updateFilterOutput(int64_t filterId, vector<int8_t> data);
    void updateMediaFilterOutput(int64_t filterId, vector<int8_t> data, uint64_t pts);
    uint16_t getFilterTpid(int64_t filterId);
    void setIsRecording(bool isRecording);
    bool isRecording();
    void startFrontendInputLoop();
    void readIptvThreadLoop(dtv_plugin* interface, dtv_streamer* streamer, size_t size,
                            int timeout_ms, int buffer_timeout);

    /**
     * A dispatcher to read and dispatch input data to all the started filters.
     * Each filter handler handles the data filtering/output writing/filterEvent updating.
     * Note that recording filters are not included.
     */
    bool startBroadcastFilterDispatcher();
    void startBroadcastTsFilter(vector<int8_t> data);

    void sendFrontendInputToRecord(vector<int8_t> data);
    void sendFrontendInputToRecord(vector<int8_t> data, uint16_t pid, uint64_t pts);
    bool startRecordFilterDispatcher();

    void getDemuxInfo(DemuxInfo* demuxInfo);
    int32_t getDemuxId();
    bool isInUse();
    void setInUse(bool inUse);
    void setTunerService(std::shared_ptr<Tuner> tuner);

    /**
     * Setter for IPTV Reading thread
     */
    void setIptvThreadRunning(bool isIptvThreadRunning);

  private:
    // Tuner service
    std::shared_ptr<Tuner> mTuner;

    // Frontend source
    std::shared_ptr<Frontend> mFrontend;

    // A struct that passes the arguments to a newly created filter thread
    struct ThreadArgs {
        Demux* user;
        int64_t filterId;
    };

    static void* __threadLoopFrontend(void* user);
    void frontendInputThreadLoop();

    /**
     * To create a FilterMQ with the next available Filter ID.
     * Creating Event Flag at the same time.
     * Add the successfully created/saved FilterMQ into the local list.
     *
     * Return false is any of the above processes fails.
     */
    void deleteEventFlag();
    bool readDataFromMQ();

    int32_t mDemuxId = -1;
    int32_t mCiCamId;
    set<int64_t> mPcrFilterIds;
    /**
     * Record the last used filter id. Initial value is -1.
     * Filter Id starts with 0.
     */
    int64_t mLastUsedFilterId = -1;
    /**
     * Record all the used playback filter Ids.
     * Any removed filter id should be removed from this set.
     */
    set<int64_t> mPlaybackFilterIds;
    /**
     * Record all the attached record filter Ids.
     * Any removed filter id should be removed from this set.
     */
    set<int64_t> mRecordFilterIds;
    /**
     * A list of created Filter sp.
     * The array number is the filter ID.
     */
    std::map<int64_t, std::shared_ptr<Filter>> mFilters;

    /**
     * Local reference to the opened Timer Filter instance.
     */
    std::shared_ptr<TimeFilter> mTimeFilter;

    /**
     * Local reference to the opened DVR object.
     */
    std::shared_ptr<Dvr> mDvrPlayback;
    std::shared_ptr<Dvr> mDvrRecord;

    // Thread handlers
    std::thread mFrontendInputThread;
    std::thread mDemuxIptvReadThread;

    // track whether the DVR FMQ for IPTV Playback is full
    bool mIsIptvDvrFMQFull = false;

    /**
     * If a specific filter's writing loop is still running
     */
    std::atomic<bool> mFrontendInputThreadRunning;
    std::atomic<bool> mKeepFetchingDataFromFrontend;

    /**
     * Controls IPTV reading thread status
     */
    bool mIsIptvReadThreadRunning;
    std::mutex mIsIptvThreadRunningMutex;
    std::condition_variable mIsIptvThreadRunningCv;

    /**
     * If the dvr recording is running.
     */
    bool mIsRecording = false;
    /**
     * Lock to protect writes to the FMQs
     */
    std::mutex mWriteLock;

    // temp handle single PES filter
    // TODO handle mulptiple Pes filters
    int mPesSizeLeft = 0;
    vector<uint8_t> mPesOutput;

    const bool DEBUG_DEMUX = false;

    int32_t mFilterTypes;
    bool mInUse = false;
};

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
