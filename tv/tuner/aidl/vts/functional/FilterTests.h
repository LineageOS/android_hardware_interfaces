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

#include <aidl/android/hardware/tv/tuner/BnFilterCallback.h>
#include <aidl/android/hardware/tv/tuner/IDemux.h>
#include <aidl/android/hardware/tv/tuner/IFilter.h>
#include <aidl/android/hardware/tv/tuner/ITuner.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <future>
#include <map>
#include <unordered_map>

#include <fmq/AidlMessageQueue.h>

using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::AidlMessageQueue;
using ::android::Condition;
using ::android::Mutex;
using ::android::hardware::EventFlag;

using ::testing::AssertionResult;

using namespace aidl::android::hardware::tv::tuner;
using namespace std;

enum FilterEventType : uint8_t {
    UNDEFINED,
    SECTION,
    MEDIA,
    PES,
    RECORD,
    MMTPRECORD,
    DOWNLOAD,
    TEMI,
};

using FilterMQ = AidlMessageQueue<int8_t, SynchronizedReadWrite>;
using MQDesc = MQDescriptor<int8_t, SynchronizedReadWrite>;

#define WAIT_TIMEOUT 3000000000

class FilterCallback : public BnFilterCallback {
  public:
    /**
     * A FilterCallbackVerifier is used to test and verify filter callbacks.
     * The function should return true when a callback has been handled by this
     * filter verifier. This will cause the associated future to be unblocked.
     * If the function returns false, we continue to wait for future callbacks
     * (the future remains blocked).
     */
    using FilterCallbackVerifier = std::function<bool(const std::vector<DemuxFilterEvent>&)>;

    virtual ::ndk::ScopedAStatus onFilterEvent(const vector<DemuxFilterEvent>& events) override;

    std::future<void> verifyFilterCallback(FilterCallbackVerifier&& verifier);

    virtual ::ndk::ScopedAStatus onFilterStatus(const DemuxFilterStatus /*status*/) override {
        return ::ndk::ScopedAStatus::ok();
    }

    void setFilterId(int32_t filterId) { mFilterId = filterId; }
    void setFilterInterface(std::shared_ptr<IFilter> filter) { mFilter = filter; }
    void setSharedHandle(native_handle_t* sharedHandle) { mAvSharedHandle = sharedHandle; }
    void setMemSize(uint64_t size) { mAvSharedMemSize = size; }

    void testFilterDataOutput();
    void testFilterScramblingEvent();
    void testFilterIpCidEvent();
    void testStartIdAfterReconfigure();

    void readFilterEventsData(const vector<DemuxFilterEvent>& events);
    bool dumpAvData(const DemuxFilterMediaEvent& event);

  private:
    int32_t mFilterId;
    std::shared_ptr<IFilter> mFilter;

    std::vector<std::pair<FilterCallbackVerifier, std::promise<void>>> mFilterCallbackVerifiers;
    native_handle_t* mAvSharedHandle = nullptr;
    uint64_t mAvSharedMemSize = -1;

    android::Mutex mMsgLock;
    android::Mutex mFilterOutputLock;
    android::Condition mMsgCondition;

    int mPidFilterOutputCount = 0;
    int mScramblingStatusEvent = 0;
    int mIpCidEvent = 0;
    bool mStartIdReceived = false;
};

class FilterTests {
  public:
    void setService(std::shared_ptr<ITuner> tuner) { mService = tuner; }
    void setDemux(std::shared_ptr<IDemux> demux) { mDemux = demux; }
    std::shared_ptr<IFilter> getFilterById(int64_t filterId) { return mFilters[filterId]; }

    map<int64_t, std::shared_ptr<FilterCallback>> getFilterCallbacks() { return mFilterCallbacks; }

    AssertionResult openFilterInDemux(DemuxFilterType type, int32_t bufferSize);
    AssertionResult getNewlyOpenedFilterId_64bit(int64_t& filterId);
    AssertionResult getSharedAvMemoryHandle(int64_t filterId);
    AssertionResult releaseShareAvHandle(int64_t filterId);
    AssertionResult configFilter(DemuxFilterSettings setting, int64_t filterId);
    AssertionResult configAvFilterStreamType(AvStreamType type, int64_t filterId);
    AssertionResult configIpFilterCid(int32_t ipCid, int64_t filterId);
    AssertionResult configureMonitorEvent(int64_t filterId, int32_t monitorEventTypes);
    AssertionResult testMonitorEvent(uint64_t filterId, uint32_t monitorEventTypes);
    AssertionResult getFilterMQDescriptor(int64_t filterId, bool getMqDesc);
    AssertionResult startFilter(int64_t filterId);
    AssertionResult stopFilter(int64_t filterId);
    AssertionResult closeFilter(int64_t filterId);
    AssertionResult startIdTest(int64_t filterId);

    AssertionResult openTimeFilterInDemux();
    AssertionResult setTimeStamp(int64_t timeStamp);
    AssertionResult getTimeStamp();
    AssertionResult setFilterDataSource(int64_t sourceFilterId, int64_t sinkFilterId);
    AssertionResult setFilterDataSourceToDemux(int64_t filterId);
    AssertionResult clearTimeStamp();
    AssertionResult closeTimeFilter();

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    std::shared_ptr<ITuner> mService;
    std::shared_ptr<IFilter> mFilter;
    std::shared_ptr<IDemux> mDemux;
    std::shared_ptr<ITimeFilter> mTimeFilter;
    map<int64_t, std::shared_ptr<IFilter>> mFilters;
    map<int64_t, std::shared_ptr<FilterCallback>> mFilterCallbacks;

    std::shared_ptr<FilterCallback> mFilterCallback;
    MQDesc mFilterMQDescriptor;
    vector<int64_t> mUsedFilterIds;

    native_handle_t* mAvSharedHandle = nullptr;
    int64_t mFilterId = -1;
    int64_t mBeginTimeStamp;
};
