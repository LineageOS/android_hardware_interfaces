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

class Demux : public IDemux {
  public:
    Demux(uint32_t demuxId);

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

  private:
    virtual ~Demux();
    /**
     * To create a FilterMQ with the the next available Filter ID.
     * Creating Event Flag at the same time.
     * Add the successfully created/saved FilterMQ into the local list.
     *
     * Return false is any of the above processes fails.
     */
    bool createAndSaveMQ(uint32_t bufferSize, uint32_t filterId);
    void deleteEventFlag();
    bool writeDataToFilterMQ(const std::vector<uint8_t>& data, uint32_t filterId);
    Result startSectionFilterHandler(DemuxFilterEvent event);
    Result startPesFilterHandler(DemuxFilterEvent& event);
    Result startTsFilterHandler();
    Result startMediaFilterHandler(DemuxFilterEvent& event);
    Result startRecordFilterHandler(DemuxFilterEvent& event);
    Result startPcrFilterHandler();
    bool writeSectionsAndCreateEvent(DemuxFilterEvent& event, uint32_t sectionNum);
    void filterThreadLoop(DemuxFilterEvent* event);
    static void* __threadLoop(void* data);

    uint32_t mDemuxId;
    uint32_t mSourceFrontendId;
    /**
     * Record the last used filer id. Initial value is -1.
     * Filter Id starts with 0.
     */
    uint32_t mLastUsedFilterId = -1;
    /**
     * A list of created FilterMQ ptrs.
     * The array number is the filter ID.
     */
    vector<unique_ptr<FilterMQ>> mFilterMQs;
    vector<DemuxFilterType> mFilterTypes;
    vector<EventFlag*> mFilterEventFlags;
    /**
     * Demux callbacks used on filter events or IO buffer status
     */
    vector<sp<IDemuxCallback>> mDemuxCallbacks;
    /**
     * How many times a specific filter has written since started
     */
    vector<uint16_t> mFilterWriteCount;
    pthread_t mThreadId = 0;
    /**
     * If a specific filter's writing loop is still running
     */
    vector<bool> mThreadRunning;
    /**
     * Lock to protect writes to the FMQs
     */
    std::mutex mWriteLock;
    /**
     * How many times a filter should write
     * TODO make this dynamic/random/can take as a parameter
     */
    const uint16_t SECTION_WRITE_COUNT = 10;
    // A struct that passes the arguments to a newly created filter thread
    struct ThreadArgs {
        Demux* user;
        DemuxFilterEvent* event;
    };
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_0_DEMUX_H_
