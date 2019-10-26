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
#include "Dvr.h"
#include "Filter.h"
#include "Frontend.h"
#include "TimeFilter.h"
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
using ::android::hardware::tv::tuner::V1_0::IFilterCallback;
using ::android::hardware::tv::tuner::V1_0::Result;

using FilterMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

class Dvr;
class Filter;
class Frontend;
class TimeFilter;
class Tuner;

class Demux : public IDemux {
  public:
    Demux(uint32_t demuxId, sp<Tuner> tuner);

    ~Demux();

    virtual Return<Result> setFrontendDataSource(uint32_t frontendId) override;

    virtual Return<void> openFilter(const DemuxFilterType& type, uint32_t bufferSize,
                                    const sp<IFilterCallback>& cb, openFilter_cb _hidl_cb) override;

    virtual Return<void> openTimeFilter(openTimeFilter_cb _hidl_cb) override;

    virtual Return<void> getAvSyncHwId(const sp<IFilter>& filter,
                                       getAvSyncHwId_cb _hidl_cb) override;

    virtual Return<void> getAvSyncTime(AvSyncHwId avSyncHwId, getAvSyncTime_cb _hidl_cb) override;

    virtual Return<Result> close() override;

    virtual Return<void> openDvr(DvrType type, uint32_t bufferSize, const sp<IDvrCallback>& cb,
                                 openDvr_cb _hidl_cb) override;

    // Functions interacts with Tuner Service
    void stopBroadcastInput();
    Result removeFilter(uint32_t filterId);
    Result startFilterHandler(uint32_t filterId);
    void updateFilterOutput(uint16_t filterId, vector<uint8_t> data);
    uint16_t getFilterTpid(uint32_t filterId);

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

    Result startBroadcastInputLoop();
    static void* __threadLoopBroadcast(void* user);
    void broadcastInputThreadLoop();

    /**
     * To create a FilterMQ with the the next available Filter ID.
     * Creating Event Flag at the same time.
     * Add the successfully created/saved FilterMQ into the local list.
     *
     * Return false is any of the above processes fails.
     */
    void deleteEventFlag();
    bool readDataFromMQ();
    /**
     * A dispatcher to read and dispatch input data to all the started filters.
     * Each filter handler handles the data filtering/output writing/filterEvent updating.
     */
    bool startFilterDispatcher();
    void startTsFilter(vector<uint8_t> data);

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
    std::map<uint32_t, sp<Filter>> mFilters;

    // Thread handlers
    pthread_t mBroadcastInputThread;
    /**
     * If a specific filter's writing loop is still running
     */
    bool mBroadcastInputThreadRunning;
    bool mKeepFetchingDataFromFrontend;
    /**
     * Lock to protect writes to the FMQs
     */
    std::mutex mWriteLock;
    /**
     * Lock to protect writes to the input status
     */
    std::mutex mBroadcastInputThreadLock;

    // temp handle single PES filter
    // TODO handle mulptiple Pes filters
    int mPesSizeLeft = 0;
    vector<uint8_t> mPesOutput;

    const bool DEBUG_FILTER = false;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_0_DEMUX_H_
