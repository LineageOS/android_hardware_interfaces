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

#include <android-base/logging.h>
#include <android/hardware/tv/tuner/1.0/IDemux.h>
#include <android/hardware/tv/tuner/1.0/IFilter.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <android/hardware/tv/tuner/1.1/IFilter.h>
#include <android/hardware/tv/tuner/1.1/IFilterCallback.h>
#include <android/hardware/tv/tuner/1.1/ITuner.h>
#include <android/hardware/tv/tuner/1.1/types.h>
#include <fmq/MessageQueue.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
#include <inttypes.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <map>

using android::Condition;
using android::Mutex;
using android::sp;
using android::hardware::EventFlag;
using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tv::tuner::V1_0::DemuxFilterEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
using android::hardware::tv::tuner::V1_0::DemuxFilterMediaEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterStatus;
using android::hardware::tv::tuner::V1_0::DemuxFilterType;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterType;
using android::hardware::tv::tuner::V1_0::IDemux;
using android::hardware::tv::tuner::V1_0::IFilter;
using android::hardware::tv::tuner::V1_0::Result;
using android::hardware::tv::tuner::V1_1::AvStreamType;
using android::hardware::tv::tuner::V1_1::DemuxFilterEventExt;
using android::hardware::tv::tuner::V1_1::DemuxFilterMonitorEvent;
using android::hardware::tv::tuner::V1_1::DemuxFilterMonitorEventType;
using android::hardware::tv::tuner::V1_1::IFilterCallback;
using android::hardware::tv::tuner::V1_1::ITuner;

using ::testing::AssertionResult;

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

using FilterMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using MQDesc = MQDescriptorSync<uint8_t>;

#define WAIT_TIMEOUT 3000000000

class FilterCallback : public IFilterCallback {
  public:
    virtual Return<void> onFilterEvent_1_1(const DemuxFilterEvent& filterEvent,
                                           const DemuxFilterEventExt& filterEventExt) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        // Temprarily we treat the first coming back filter data on the matching pid a success
        // once all of the MQ are cleared, means we got all the expected output
        mFilterEvent = filterEvent;
        mFilterEventExt = filterEventExt;
        readFilterEventData();
        mPidFilterOutputCount++;
        mMsgCondition.signal();
        return Void();
    }

    virtual Return<void> onFilterEvent(
            const android::hardware::tv::tuner::V1_0::DemuxFilterEvent& filterEvent) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        // Temprarily we treat the first coming back filter data on the matching pid a success
        // once all of the MQ are cleared, means we got all the expected output
        mFilterEvent = filterEvent;
        readFilterEventData();
        mPidFilterOutputCount++;
        mMsgCondition.signal();
        return Void();
    }

    virtual Return<void> onFilterStatus(const DemuxFilterStatus /*status*/) override {
        return Void();
    }

    void setFilterId(uint32_t filterId) { mFilterId = filterId; }
    void setFilterInterface(sp<IFilter> filter) { mFilter = filter; }
    void setFilterEventType(FilterEventType type) { mFilterEventType = type; }
    void setSharedHandle(hidl_handle sharedHandle) { mAvSharedHandle = sharedHandle; }
    void setMemSize(uint64_t size) { mAvSharedMemSize = size; }

    void testFilterDataOutput();
    void testFilterScramblingEvent();
    void testFilterIpCidEvent();
    void testStartIdAfterReconfigure();

    void readFilterEventData();
    bool dumpAvData(DemuxFilterMediaEvent event);

  private:
    uint32_t mFilterId;
    sp<IFilter> mFilter;
    FilterEventType mFilterEventType;
    DemuxFilterEvent mFilterEvent;
    DemuxFilterEventExt mFilterEventExt;

    hidl_handle mAvSharedHandle = NULL;
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
    void setService(sp<ITuner> tuner) { mService = tuner; }
    void setDemux(sp<IDemux> demux) { mDemux = demux; }
    sp<IFilter> getFilterById(uint64_t filterId) { return mFilters[filterId]; }

    map<uint64_t, sp<FilterCallback>> getFilterCallbacks() { return mFilterCallbacks; }

    AssertionResult openFilterInDemux(DemuxFilterType type, uint32_t bufferSize);
    AssertionResult getNewlyOpenedFilterId_64bit(uint64_t& filterId);
    AssertionResult getSharedAvMemoryHandle(uint64_t filterId);
    AssertionResult releaseShareAvHandle(uint64_t filterId);
    AssertionResult configFilter(DemuxFilterSettings setting, uint64_t filterId);
    AssertionResult configAvFilterStreamType(AvStreamType type, uint64_t filterId);
    AssertionResult configIpFilterCid(uint32_t ipCid, uint64_t filterId);
    AssertionResult configureMonitorEvent(uint64_t filterId, uint32_t monitorEventTypes);
    AssertionResult getFilterMQDescriptor(uint64_t filterId, bool getMqDesc);
    AssertionResult startFilter(uint64_t filterId);
    AssertionResult stopFilter(uint64_t filterId);
    AssertionResult closeFilter(uint64_t filterId);
    AssertionResult startIdTest(uint64_t filterId);

    FilterEventType getFilterEventType(DemuxFilterType type) {
        FilterEventType eventType = FilterEventType::UNDEFINED;
        switch (type.mainType) {
            case DemuxFilterMainType::TS:
                switch (type.subType.tsFilterType()) {
                    case DemuxTsFilterType::UNDEFINED:
                        break;
                    case DemuxTsFilterType::SECTION:
                        eventType = FilterEventType::SECTION;
                        break;
                    case DemuxTsFilterType::PES:
                        eventType = FilterEventType::PES;
                        break;
                    case DemuxTsFilterType::TS:
                        break;
                    case DemuxTsFilterType::AUDIO:
                    case DemuxTsFilterType::VIDEO:
                        eventType = FilterEventType::MEDIA;
                        break;
                    case DemuxTsFilterType::PCR:
                        break;
                    case DemuxTsFilterType::RECORD:
                        eventType = FilterEventType::RECORD;
                        break;
                    case DemuxTsFilterType::TEMI:
                        eventType = FilterEventType::TEMI;
                        break;
                }
                break;
            case DemuxFilterMainType::MMTP:
                /*mmtpSettings*/
                break;
            case DemuxFilterMainType::IP:
                /*ipSettings*/
                break;
            case DemuxFilterMainType::TLV:
                /*tlvSettings*/
                break;
            case DemuxFilterMainType::ALP:
                /*alpSettings*/
                break;
            default:
                break;
        }
        return eventType;
    }

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    sp<ITuner> mService;
    sp<IFilter> mFilter;
    sp<IDemux> mDemux;
    map<uint64_t, sp<IFilter>> mFilters;
    map<uint64_t, sp<FilterCallback>> mFilterCallbacks;

    sp<FilterCallback> mFilterCallback;
    MQDesc mFilterMQDescriptor;
    vector<uint64_t> mUsedFilterIds;

    hidl_handle mAvSharedHandle = NULL;

    uint64_t mFilterId = -1;
};
