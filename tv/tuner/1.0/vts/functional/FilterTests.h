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
#include <android/hardware/tv/tuner/1.0/IFilter.h>
#include <android/hardware/tv/tuner/1.0/IFilterCallback.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <fmq/MessageQueue.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
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
using android::hardware::tv::tuner::V1_0::DemuxFilterPesDataSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterPesEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterRecordSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterSectionEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterSectionSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterStatus;
using android::hardware::tv::tuner::V1_0::DemuxFilterType;
using android::hardware::tv::tuner::V1_0::DemuxQueueNotifyBits;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterType;
using android::hardware::tv::tuner::V1_0::IDemux;
using android::hardware::tv::tuner::V1_0::IFilter;
using android::hardware::tv::tuner::V1_0::IFilterCallback;
using android::hardware::tv::tuner::V1_0::ITuner;
using android::hardware::tv::tuner::V1_0::Result;

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

const uint32_t FMQ_SIZE_1M = 0x100000;
const uint32_t FMQ_SIZE_16M = 0x1000000;

#define WAIT_TIMEOUT 3000000000

class FilterCallback : public IFilterCallback {
  public:
    virtual Return<void> onFilterEvent(const DemuxFilterEvent& filterEvent) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        // Temprarily we treat the first coming back filter data on the matching pid a success
        // once all of the MQ are cleared, means we got all the expected output
        mFilterEvent = filterEvent;
        readFilterEventData();
        mPidFilterOutputCount++;
        // mFilterIdToMQ.erase(filterEvent.filterId);

        // startFilterEventThread(filterEvent);
        mMsgCondition.signal();
        return Void();
    }

    virtual Return<void> onFilterStatus(const DemuxFilterStatus /*status*/) override {
        return Void();
    }

    void setFilterId(uint32_t filterId) { mFilterId = filterId; }
    void setFilterInterface(sp<IFilter> filter) { mFilter = filter; }
    void setFilterEventType(FilterEventType type) { mFilterEventType = type; }

    void testFilterDataOutput();

    void startFilterEventThread(DemuxFilterEvent event);
    static void* __threadLoopFilter(void* threadArgs);
    void filterThreadLoop(DemuxFilterEvent& event);

    void updateFilterMQ(MQDesc& filterMQDescriptor);
    void updateGoldenOutputMap(string goldenOutputFile);
    bool readFilterEventData();
    bool dumpAvData(DemuxFilterMediaEvent event);

  private:
    struct FilterThreadArgs {
        FilterCallback* user;
        DemuxFilterEvent event;
    };
    uint16_t mDataLength = 0;
    std::vector<uint8_t> mDataOutputBuffer;

    string mFilterIdToGoldenOutput;

    uint32_t mFilterId;
    sp<IFilter> mFilter;
    FilterEventType mFilterEventType;
    std::unique_ptr<FilterMQ> mFilterMQ;
    EventFlag* mFilterMQEventFlag;
    DemuxFilterEvent mFilterEvent;

    android::Mutex mMsgLock;
    android::Mutex mFilterOutputLock;
    android::Condition mMsgCondition;
    android::Condition mFilterOutputCondition;

    pthread_t mFilterThread;

    int mPidFilterOutputCount = 0;
};

class FilterTests {
  public:
    void setService(sp<ITuner> tuner) { mService = tuner; }
    void setDemux(sp<IDemux> demux) { mDemux = demux; }

    std::map<uint32_t, sp<FilterCallback>> getFilterCallbacks() { return mFilterCallbacks; }

    AssertionResult openFilterInDemux(DemuxFilterType type);
    AssertionResult getNewlyOpenedFilterId(uint32_t& filterId);
    AssertionResult configFilter(DemuxFilterSettings setting, uint32_t filterId);
    AssertionResult getFilterMQDescriptor(uint32_t filterId);
    AssertionResult startFilter(uint32_t filterId);
    AssertionResult stopFilter(uint32_t filterId);
    AssertionResult closeFilter(uint32_t filterId);

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
    std::map<uint32_t, sp<IFilter>> mFilters;
    std::map<uint32_t, sp<FilterCallback>> mFilterCallbacks;

    sp<FilterCallback> mFilterCallback;
    MQDesc mFilterMQDescriptor;
    vector<uint32_t> mUsedFilterIds;

    uint32_t mFilterId = -1;
};
