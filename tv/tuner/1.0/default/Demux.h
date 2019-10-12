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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_0_DEMUX_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_0_DEMUX_H_

#include <android/hardware/tv/tuner/1.0/IDemux.h>
#include <fmq/MessageQueue.h>
#include <math.h>
#include <set>
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
using ::android::hardware::tv::tuner::V1_0::IDemuxCallback;
using ::android::hardware::tv::tuner::V1_0::Result;

using FilterMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

class Tuner;
class Frontend;

class Demux : public IDemux {
  public:
    Demux(uint32_t demuxId, sp<Tuner> tuner);

    ~Demux();

    virtual Return<Result> setFrontendDataSource(uint32_t frontendId) override;

    virtual Return<Result> close() override;

    virtual Return<void> addFilter(DemuxFilterType type, uint32_t bufferSize,
                                   const sp<IDemuxCallback>& cb, addFilter_cb _hidl_cb) override;

    virtual Return<void> getFilterQueueDesc(uint32_t filterId,
                                            getFilterQueueDesc_cb _hidl_cb) override;

    virtual Return<Result> configureFilter(uint32_t filterId,
                                           const DemuxFilterSettings& settings) override;

    virtual Return<Result> startFilter(uint32_t filterId) override;

    virtual Return<Result> stopFilter(uint32_t filterId) override;

    virtual Return<Result> flushFilter(uint32_t filterId) override;

    virtual Return<Result> removeFilter(uint32_t filterId) override;

    virtual Return<void> getAvSyncHwId(uint32_t filterId, getAvSyncHwId_cb _hidl_cb) override;

    virtual Return<void> getAvSyncTime(AvSyncHwId avSyncHwId, getAvSyncTime_cb _hidl_cb) override;

    virtual Return<Result> addInput(uint32_t bufferSize, const sp<IDemuxCallback>& cb) override;

    virtual Return<void> getInputQueueDesc(getInputQueueDesc_cb _hidl_cb) override;

    virtual Return<Result> configureInput(const DemuxInputSettings& settings) override;

    virtual Return<Result> startInput() override;

    virtual Return<Result> stopInput() override;

    virtual Return<Result> flushInput() override;

    virtual Return<Result> removeInput() override;

    virtual Return<Result> addOutput(uint32_t bufferSize, const sp<IDemuxCallback>& cb) override;

    virtual Return<void> getOutputQueueDesc(getOutputQueueDesc_cb _hidl_cb) override;

    virtual Return<Result> configureOutput(const DemuxOutputSettings& settings) override;

    virtual Return<Result> attachOutputFilter(uint32_t filterId) override;

    virtual Return<Result> detachOutputFilter(uint32_t filterId) override;

    virtual Return<Result> startOutput() override;

    virtual Return<Result> stopOutput() override;

    virtual Return<Result> flushOutput() override;

    virtual Return<Result> removeOutput() override;

    // Functions interacts with Tuner Service
    void stopBroadcastInput();

  private:
    // Tuner service
    sp<Tuner> mTunerService;

    // Frontend source
    sp<Frontend> mFrontend;
    string mFrontendSourceFile;

    // A struct that passes the arguments to a newly created filter thread
    struct ThreadArgs {
        Demux* user;
        uint32_t filterId;
    };

    /**
     * Filter handlers to handle the data filtering.
     * They are also responsible to write the filtered output into the filter FMQ
     * and update the filterEvent bound with the same filterId.
     */
    Result startSectionFilterHandler(uint32_t filterId);
    Result startPesFilterHandler(uint32_t filterId);
    Result startTsFilterHandler();
    Result startMediaFilterHandler(uint32_t filterId);
    Result startRecordFilterHandler(uint32_t filterId);
    Result startPcrFilterHandler();
    Result startFilterLoop(uint32_t filterId);
    Result startBroadcastInputLoop();

    /**
     * To create a FilterMQ with the the next available Filter ID.
     * Creating Event Flag at the same time.
     * Add the successfully created/saved FilterMQ into the local list.
     *
     * Return false is any of the above processes fails.
     */
    bool createFilterMQ(uint32_t bufferSize, uint32_t filterId);
    bool createMQ(FilterMQ* queue, EventFlag* eventFlag, uint32_t bufferSize);
    void deleteEventFlag();
    bool writeDataToFilterMQ(const std::vector<uint8_t>& data, uint32_t filterId);
    bool readDataFromMQ();
    bool writeSectionsAndCreateEvent(uint32_t filterId, vector<uint8_t> data);
    void maySendInputStatusCallback();
    void maySendFilterStatusCallback(uint32_t filterId);
    DemuxInputStatus checkInputStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                            uint32_t highThreshold, uint32_t lowThreshold);
    DemuxFilterStatus checkFilterStatusChange(uint32_t filterId, uint32_t availableToWrite,
                                              uint32_t availableToRead, uint32_t highThreshold,
                                              uint32_t lowThreshold);
    /**
     * A dispatcher to read and dispatch input data to all the started filters.
     * Each filter handler handles the data filtering/output writing/filterEvent updating.
     */
    bool readInputFMQ();
    void startTsFilter(vector<uint8_t> data);
    bool startFilterDispatcher();
    static void* __threadLoopFilter(void* data);
    static void* __threadLoopInput(void* user);
    static void* __threadLoopBroadcast(void* user);
    void filterThreadLoop(uint32_t filterId);
    void inputThreadLoop();
    void broadcastInputThreadLoop();

    uint32_t mDemuxId;
    /**
     * Record the last used filter id. Initial value is -1.
     * Filter Id starts with 0.
     */
    uint32_t mLastUsedFilterId = -1;
    /**
     * Record all the used filter Ids.
     * Any removed filter id should be removed from this set.
     */
    set<uint32_t> mUsedFilterIds;
    /**
     * Record all the unused filter Ids within mLastUsedFilterId.
     * Removed filter Id should be added into this set.
     * When this set is not empty, ids here should be allocated first
     * and added into usedFilterIds.
     */
    set<uint32_t> mUnusedFilterIds;
    /**
     * A list of created FilterMQ ptrs.
     * The array number is the filter ID.
     */
    vector<uint16_t> mFilterPids;
    vector<vector<uint8_t>> mFilterOutputs;
    vector<unique_ptr<FilterMQ>> mFilterMQs;
    vector<EventFlag*> mFilterEventFlags;
    vector<DemuxFilterEvent> mFilterEvents;
    unique_ptr<FilterMQ> mInputMQ;
    unique_ptr<FilterMQ> mOutputMQ;
    EventFlag* mInputEventFlag;
    EventFlag* mOutputEventFlag;
    /**
     * Demux callbacks used on filter events or IO buffer status
     */
    vector<sp<IDemuxCallback>> mFilterCallbacks;
    sp<IDemuxCallback> mInputCallback;
    sp<IDemuxCallback> mOutputCallback;
    bool mInputConfigured = false;
    bool mOutputConfigured = false;
    DemuxInputSettings mInputSettings;
    DemuxOutputSettings mOutputSettings;

    // Thread handlers
    pthread_t mInputThread;
    pthread_t mOutputThread;
    pthread_t mBroadcastInputThread;
    vector<pthread_t> mFilterThreads;

    // FMQ status local records
    DemuxInputStatus mIntputStatus;
    vector<DemuxFilterStatus> mFilterStatus;
    /**
     * If a specific filter's writing loop is still running
     */
    vector<bool> mFilterThreadRunning;
    bool mInputThreadRunning;
    bool mBroadcastInputThreadRunning;
    bool mKeepFetchingDataFromFrontend;
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
    std::mutex mInputStatusLock;
    std::mutex mFilterStatusLock;
    std::mutex mBroadcastInputThreadLock;
    std::mutex mFilterThreadLock;
    std::mutex mInputThreadLock;
    /**
     * How many times a filter should write
     * TODO make this dynamic/random/can take as a parameter
     */
    const uint16_t SECTION_WRITE_COUNT = 10;

    // temp handle single PES filter
    // TODO handle mulptiple Pes filters
    int mPesSizeLeft = 0;
    vector<uint8_t> mPesOutput;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_0_DEMUX_H_
