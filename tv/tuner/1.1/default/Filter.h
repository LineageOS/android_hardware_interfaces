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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_1_FILTER_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_1_FILTER_H_

#include <android/hardware/tv/tuner/1.1/IFilter.h>
#include <android/hardware/tv/tuner/1.1/IFilterCallback.h>
#include <fmq/MessageQueue.h>
#include <inttypes.h>
#include <ion/ion.h>
#include <math.h>
#include <sys/stat.h>
#include <set>
#include "Demux.h"
#include "Dvr.h"
#include "Frontend.h"

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

using FilterMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
const uint32_t BUFFER_SIZE_16M = 0x1000000;

class Demux;
class Dvr;

class Filter : public V1_1::IFilter {
  public:
    Filter();

    Filter(DemuxFilterType type, uint64_t filterId, uint32_t bufferSize,
           const sp<IFilterCallback>& cb, sp<Demux> demux);

    ~Filter();

    virtual Return<void> getId64Bit(getId64Bit_cb _hidl_cb) override;

    virtual Return<void> getId(getId_cb _hidl_cb) override;

    virtual Return<Result> setDataSource(const sp<V1_0::IFilter>& filter) override;

    virtual Return<void> getQueueDesc(getQueueDesc_cb _hidl_cb) override;

    virtual Return<Result> configure(const DemuxFilterSettings& settings) override;

    virtual Return<Result> start() override;

    virtual Return<Result> stop() override;

    virtual Return<Result> flush() override;

    virtual Return<Result> releaseAvHandle(const hidl_handle& avMemory, uint64_t avDataId) override;

    virtual Return<Result> close() override;

    virtual Return<Result> configureIpCid(uint32_t ipCid) override;

    virtual Return<void> getAvSharedHandle(getAvSharedHandle_cb _hidl_cb) override;

    virtual Return<Result> configureAvStreamType(const V1_1::AvStreamType& avStreamType) override;

    virtual Return<Result> configureMonitorEvent(uint32_t monitorEventTypes) override;

    /**
     * To create a FilterMQ and its Event Flag.
     *
     * Return false is any of the above processes fails.
     */
    bool createFilterMQ();
    uint16_t getTpid();
    void updateFilterOutput(vector<uint8_t> data);
    void updateRecordOutput(vector<uint8_t> data);
    void updatePts(uint64_t pts);
    Result startFilterHandler();
    Result startRecordFilterHandler();
    void attachFilterToRecord(const sp<Dvr> dvr);
    void detachFilterFromRecord();
    void freeAvHandle();
    void freeSharedAvHandle();
    bool isMediaFilter() { return mIsMediaFilter; };
    bool isPcrFilter() { return mIsPcrFilter; };
    bool isRecordFilter() { return mIsRecordFilter; };

  private:
    // Tuner service
    sp<Demux> mDemux;
    // Dvr reference once the filter is attached to any
    sp<Dvr> mDvr = nullptr;
    /**
     * Filter callbacks used on filter events or FMQ status
     */
    sp<IFilterCallback> mCallback = nullptr;

    /**
     * V1_1 Filter callbacks used on filter events or FMQ status
     */
    sp<V1_1::IFilterCallback> mCallback_1_1 = nullptr;

    uint64_t mFilterId;
    uint32_t mCid = static_cast<uint32_t>(V1_1::Constant::INVALID_IP_FILTER_CONTEXT_ID);
    uint32_t mBufferSize;
    DemuxFilterType mType;
    bool mIsMediaFilter = false;
    bool mIsPcrFilter = false;
    bool mIsRecordFilter = false;
    DemuxFilterSettings mFilterSettings;

    uint16_t mTpid;
    sp<V1_0::IFilter> mDataSource;
    bool mIsDataSourceDemux = true;
    vector<uint8_t> mFilterOutput;
    vector<uint8_t> mRecordFilterOutput;
    uint64_t mPts = 0;
    unique_ptr<FilterMQ> mFilterMQ;
    bool mIsUsingFMQ = false;
    EventFlag* mFilterEventFlag;
    DemuxFilterEvent mFilterEvent;
    V1_1::DemuxFilterEventExt mFilterEventExt;

    // Thread handlers
    pthread_t mFilterThread;

    // FMQ status local records
    DemuxFilterStatus mFilterStatus;
    /**
     * If a specific filter's writing loop is still running
     */
    bool mFilterThreadRunning;
    bool mKeepFetchingDataFromFrontend;

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
    Result startSectionFilterHandler();
    Result startPesFilterHandler();
    Result startTsFilterHandler();
    Result startMediaFilterHandler();
    Result startPcrFilterHandler();
    Result startTemiFilterHandler();
    Result startFilterLoop();

    void deleteEventFlag();
    bool writeDataToFilterMQ(const std::vector<uint8_t>& data);
    bool readDataFromMQ();
    bool writeSectionsAndCreateEvent(vector<uint8_t> data);
    void maySendFilterStatusCallback();
    DemuxFilterStatus checkFilterStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                              uint32_t highThreshold, uint32_t lowThreshold);
    /**
     * A dispatcher to read and dispatch input data to all the started filters.
     * Each filter handler handles the data filtering/output writing/filterEvent updating.
     */
    void startTsFilter(vector<uint8_t> data);
    bool startFilterDispatcher();
    static void* __threadLoopFilter(void* user);
    void filterThreadLoop();

    int createAvIonFd(int size);
    uint8_t* getIonBuffer(int fd, int size);
    native_handle_t* createNativeHandle(int fd);
    Result createMediaFilterEventWithIon(vector<uint8_t> output);
    Result createIndependentMediaEvents(vector<uint8_t> output);
    Result createShareMemMediaEvents(vector<uint8_t> output);
    bool sameFile(int fd1, int fd2);

    DemuxFilterEvent createMediaEvent();
    DemuxFilterEvent createTsRecordEvent();
    V1_1::DemuxFilterEventExt createTsRecordEventExt();
    DemuxFilterEvent createMmtpRecordEvent();
    V1_1::DemuxFilterEventExt createMmtpRecordEventExt();
    DemuxFilterEvent createSectionEvent();
    DemuxFilterEvent createPesEvent();
    DemuxFilterEvent createDownloadEvent();
    DemuxFilterEvent createIpPayloadEvent();
    DemuxFilterEvent createTemiEvent();
    V1_1::DemuxFilterEventExt createMonitorEvent();
    V1_1::DemuxFilterEventExt createRestartEvent();
    /**
     * Lock to protect writes to the FMQs
     */
    std::mutex mWriteLock;
    /**
     * Lock to protect writes to the filter event
     */
    // TODO make each filter separate event lock
    std::mutex mFilterEventLock;
    /**
     * Lock to protect writes to the input status
     */
    std::mutex mFilterStatusLock;
    std::mutex mFilterThreadLock;
    std::mutex mFilterOutputLock;
    std::mutex mRecordFilterOutputLock;

    // temp handle single PES filter
    // TODO handle mulptiple Pes filters
    int mPesSizeLeft = 0;
    vector<uint8_t> mPesOutput;

    // A map from data id to ion handle
    std::map<uint64_t, int> mDataId2Avfd;
    uint64_t mLastUsedDataId = 1;
    int mAvBufferCopyCount = 0;

    // Shared A/V memory handle
    hidl_handle mSharedAvMemHandle;
    bool mUsingSharedAvMem = false;
    uint32_t mSharedAvMemOffset = 0;

    uint32_t mAudioStreamType;
    uint32_t mVideoStreamType;

    // Scrambling status to be monitored
    uint32_t mStatuses = 0;

    bool mConfigured = false;
    int mStartId = 0;
    uint8_t mScramblingStatusMonitored = 0;
    uint8_t mIpCidMonitored = 0;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_1_FILTER_H_
