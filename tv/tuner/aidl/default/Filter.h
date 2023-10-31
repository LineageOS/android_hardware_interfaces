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

#include <aidl/android/hardware/tv/tuner/BnFilter.h>
#include <aidl/android/hardware/tv/tuner/Constant.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterEvent.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterStatus.h>

#include <fmq/AidlMessageQueue.h>
#include <inttypes.h>
#include <ion/ion.h>
#include <math.h>
#include <sys/stat.h>
#include <atomic>
#include <condition_variable>
#include <set>
#include <thread>

#include "Demux.h"
#include "Dvr.h"
#include "Frontend.h"

using namespace std;

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

using ::aidl::android::hardware::common::NativeHandle;
using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::AidlMessageQueue;
using ::android::hardware::EventFlag;

using FilterMQ = AidlMessageQueue<int8_t, SynchronizedReadWrite>;

const uint32_t BUFFER_SIZE = 0x800000;  // 8 MB

class Demux;
class Dvr;

class FilterCallbackScheduler final {
  public:
    FilterCallbackScheduler(const std::shared_ptr<IFilterCallback>& cb);
    ~FilterCallbackScheduler();

    void onFilterEvent(DemuxFilterEvent&& event);
    void onFilterStatus(const DemuxFilterStatus& status);

    void setTimeDelayHint(int timeDelay);
    void setDataSizeDelayHint(int dataSizeDelay);

    bool hasCallbackRegistered() const;

    void flushEvents();

  private:
    void start();
    void stop();

    void threadLoop();
    void threadLoopOnce();

    // function needs to be called while holding mLock
    bool isDataSizeDelayConditionMetLocked();

    static int getDemuxFilterEventDataLength(const DemuxFilterEvent& event);

  private:
    std::shared_ptr<IFilterCallback> mCallback;
    std::thread mCallbackThread;
    std::atomic<bool> mIsRunning;

    // mLock protects mCallbackBuffer, mIsConditionMet, mCv, mDataLength,
    // mTimeDelayInMs, and mDataSizeDelayInBytes
    std::mutex mLock;
    std::vector<DemuxFilterEvent> mCallbackBuffer;
    bool mIsConditionMet;
    std::condition_variable mCv;
    int mDataLength;
    int mTimeDelayInMs;
    int mDataSizeDelayInBytes;
};

class Filter : public BnFilter {
    friend class FilterCallbackScheduler;

  public:
    Filter(DemuxFilterType type, int64_t filterId, uint32_t bufferSize,
           const std::shared_ptr<IFilterCallback>& cb, std::shared_ptr<Demux> demux);

    ~Filter();

    ::ndk::ScopedAStatus getQueueDesc(
            MQDescriptor<int8_t, SynchronizedReadWrite>* out_queue) override;
    ::ndk::ScopedAStatus close() override;
    ::ndk::ScopedAStatus configure(const DemuxFilterSettings& in_settings) override;
    ::ndk::ScopedAStatus configureAvStreamType(const AvStreamType& in_avStreamType) override;
    ::ndk::ScopedAStatus configureIpCid(int32_t in_ipCid) override;
    ::ndk::ScopedAStatus configureMonitorEvent(int32_t in_monitorEventTypes) override;
    ::ndk::ScopedAStatus start() override;
    ::ndk::ScopedAStatus stop() override;
    ::ndk::ScopedAStatus flush() override;
    ::ndk::ScopedAStatus getAvSharedHandle(NativeHandle* out_avMemory,
                                           int64_t* _aidl_return) override;
    ::ndk::ScopedAStatus getId(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getId64Bit(int64_t* _aidl_return) override;
    ::ndk::ScopedAStatus releaseAvHandle(const NativeHandle& in_avMemory,
                                         int64_t in_avDataId) override;
    ::ndk::ScopedAStatus setDataSource(const std::shared_ptr<IFilter>& in_filter) override;
    ::ndk::ScopedAStatus setDelayHint(const FilterDelayHint& in_hint) override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

    /**
     * To create a FilterMQ and its Event Flag.
     *
     * Return false is any of the above processes fails.
     */
    bool createFilterMQ();
    uint16_t getTpid();
    void updateFilterOutput(vector<int8_t>& data);
    void updateRecordOutput(vector<int8_t>& data);
    void updatePts(uint64_t pts);
    ::ndk::ScopedAStatus startFilterHandler();
    ::ndk::ScopedAStatus startRecordFilterHandler();
    void attachFilterToRecord(const std::shared_ptr<Dvr> dvr);
    void detachFilterFromRecord();
    void freeSharedAvHandle();
    bool isMediaFilter() { return mIsMediaFilter; };
    bool isPcrFilter() { return mIsPcrFilter; };
    bool isRecordFilter() { return mIsRecordFilter; };
    void setIptvDvrPlaybackStatus(PlaybackStatus newStatus) { mIptvDvrPlaybackStatus = newStatus; };

  private:
    // Demux service
    std::shared_ptr<Demux> mDemux;
    // Dvr reference once the filter is attached to any
    std::shared_ptr<Dvr> mDvr = nullptr;

    FilterCallbackScheduler mCallbackScheduler;

    int64_t mFilterId;
    int32_t mCid = static_cast<int32_t>(Constant::INVALID_IP_FILTER_CONTEXT_ID);
    uint32_t mBufferSize;
    DemuxFilterType mType;
    bool mIsMediaFilter = false;
    bool mIsPcrFilter = false;
    bool mIsRecordFilter = false;
    DemuxFilterSettings mFilterSettings;

    uint16_t mTpid;
    std::shared_ptr<IFilter> mDataSource;
    bool mIsDataSourceDemux = true;
    vector<int8_t> mFilterOutput;
    vector<int8_t> mRecordFilterOutput;
    int64_t mPts = 0;
    unique_ptr<FilterMQ> mFilterMQ;
    bool mIsUsingFMQ = false;
    EventFlag* mFilterEventsFlag;
    vector<DemuxFilterEvent> mFilterEvents;

    // Thread handlers
    std::thread mFilterThread;

    // FMQ status local records
    DemuxFilterStatus mFilterStatus;
    /**
     * If a specific filter's writing loop is still running
     */
    std::atomic<bool> mFilterThreadRunning;

    /**
     * How many times a filter should write
     * TODO make this dynamic/random/can take as a parameter
     */
    const uint16_t SECTION_WRITE_COUNT = 10;

    bool DEBUG_FILTER = false;

    /**
     * Filter handlers to handle the data filtering.
     * They are also responsible to write the filtered output into the filter FMQ
     * and update the filterEvent bound with the same filterId.
     */
    ::ndk::ScopedAStatus startSectionFilterHandler();
    ::ndk::ScopedAStatus startPesFilterHandler();
    ::ndk::ScopedAStatus startTsFilterHandler();
    ::ndk::ScopedAStatus startMediaFilterHandler();
    ::ndk::ScopedAStatus startPcrFilterHandler();
    ::ndk::ScopedAStatus startTemiFilterHandler();
    ::ndk::ScopedAStatus startFilterLoop();

    void deleteEventFlag();
    bool writeDataToFilterMQ(const std::vector<int8_t>& data);
    bool readDataFromMQ();
    bool writeSectionsAndCreateEvent(vector<int8_t>& data);
    void maySendFilterStatusCallback();
    DemuxFilterStatus checkFilterStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                              uint32_t highThreshold, uint32_t lowThreshold);
    /**
     * A dispatcher to read and dispatch input data to all the started filters.
     * Each filter handler handles the data filtering/output writing/filterEvent updating.
     */
    bool startFilterDispatcher();
    static void* __threadLoopFilter(void* user);
    void filterThreadLoop();

    int createAvIonFd(int size);
    uint8_t* getIonBuffer(int fd, int size);
    native_handle_t* createNativeHandle(int fd);
    ::ndk::ScopedAStatus createMediaFilterEventWithIon(vector<int8_t>& output);
    ::ndk::ScopedAStatus createIndependentMediaEvents(vector<int8_t>& output);
    ::ndk::ScopedAStatus createShareMemMediaEvents(vector<int8_t>& output);
    bool sameFile(int fd1, int fd2);

    void createMediaEvent(vector<DemuxFilterEvent>&, bool isAudioPresentation);
    void createTsRecordEvent(vector<DemuxFilterEvent>&);
    void createMmtpRecordEvent(vector<DemuxFilterEvent>&);
    void createSectionEvent(vector<DemuxFilterEvent>&);
    void createPesEvent(vector<DemuxFilterEvent>&);
    void createDownloadEvent(vector<DemuxFilterEvent>&);
    void createIpPayloadEvent(vector<DemuxFilterEvent>&);
    void createTemiEvent(vector<DemuxFilterEvent>&);
    void createMonitorEvent(vector<DemuxFilterEvent>&);
    void createRestartEvent(vector<DemuxFilterEvent>&);

    /**
     * Lock to protect writes to the FMQs
     */
    std::mutex mWriteLock;
    /**
     * Lock to protect writes to the filter event
     */
    // TODO make each filter separate event lock
    std::mutex mFilterEventsLock;
    /**
     * Lock to protect writes to the input status
     */
    std::mutex mFilterStatusLock;
    std::mutex mFilterOutputLock;
    std::mutex mRecordFilterOutputLock;

    // handle single Section filter
    uint32_t mSectionSizeLeft = 0;
    vector<int8_t> mSectionOutput;

    // temp handle single PES filter
    // TODO handle mulptiple Pes filters
    uint32_t mPesSizeLeft = 0;
    vector<int8_t> mPesOutput;

    // A map from data id to ion handle
    std::map<uint64_t, int> mDataId2Avfd;
    uint64_t mLastUsedDataId = 1;
    int mAvBufferCopyCount = 0;

    // Shared A/V memory handle
    native_handle_t* mSharedAvMemHandle = nullptr;
    bool mUsingSharedAvMem = false;
    int64_t mSharedAvMemOffset = 0;

    uint32_t mAudioStreamType;
    uint32_t mVideoStreamType;

    // Scrambling status to be monitored
    uint32_t mStatuses = 0;

    bool mConfigured = false;
    int mStartId = 0;
    uint8_t mScramblingStatusMonitored = 0;
    uint8_t mIpCidMonitored = 0;

    PlaybackStatus mIptvDvrPlaybackStatus;
    std::atomic<int> mFilterCount = 0;
};

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
