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

#include <android/hardware/tv/tuner/1.0/IDemux.h>
#include <android/hardware/tv/tuner/1.0/IDescrambler.h>
#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <android/hardware/tv/tuner/1.0/IDvrCallback.h>
#include <android/hardware/tv/tuner/1.0/IFilter.h>
#include <android/hardware/tv/tuner/1.0/IFilterCallback.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <binder/MemoryDealer.h>
#include <fmq/MessageQueue.h>
#include <fstream>
#include <iostream>
#include <map>

#include "FrontendTests.h"

using android::hardware::EventFlag;
using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::tv::tuner::V1_0::DataFormat;
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
using android::hardware::tv::tuner::V1_0::DvrSettings;
using android::hardware::tv::tuner::V1_0::DvrType;
using android::hardware::tv::tuner::V1_0::IDemux;
using android::hardware::tv::tuner::V1_0::IDescrambler;
using android::hardware::tv::tuner::V1_0::IDvr;
using android::hardware::tv::tuner::V1_0::IDvrCallback;
using android::hardware::tv::tuner::V1_0::IFilter;
using android::hardware::tv::tuner::V1_0::IFilterCallback;
using android::hardware::tv::tuner::V1_0::PlaybackSettings;
using android::hardware::tv::tuner::V1_0::PlaybackStatus;
using android::hardware::tv::tuner::V1_0::RecordSettings;
using android::hardware::tv::tuner::V1_0::RecordStatus;

using FilterMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using MQDesc = MQDescriptorSync<uint8_t>;

const uint32_t FMQ_SIZE_1M = 0x100000;
const uint32_t FMQ_SIZE_16M = 0x1000000;

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

struct PlaybackConf {
    string inputDataFile;
    PlaybackSettings setting;
};

static AssertionResult failure() {
    return ::testing::AssertionFailure();
}

static AssertionResult success() {
    return ::testing::AssertionSuccess();
}

namespace {

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

class DvrCallback : public IDvrCallback {
  public:
    virtual Return<void> onRecordStatus(DemuxFilterStatus status) override {
        ALOGW("[vts] record status %hhu", status);
        switch (status) {
            case DemuxFilterStatus::DATA_READY:
                break;
            case DemuxFilterStatus::LOW_WATER:
                break;
            case DemuxFilterStatus::HIGH_WATER:
            case DemuxFilterStatus::OVERFLOW:
                ALOGW("[vts] record overflow. Flushing");
                break;
        }
        return Void();
    }

    virtual Return<void> onPlaybackStatus(PlaybackStatus status) override {
        // android::Mutex::Autolock autoLock(mMsgLock);
        ALOGW("[vts] playback status %d", status);
        switch (status) {
            case PlaybackStatus::SPACE_EMPTY:
            case PlaybackStatus::SPACE_ALMOST_EMPTY:
                ALOGW("[vts] keep playback inputing %d", status);
                mKeepWritingPlaybackFMQ = true;
                break;
            case PlaybackStatus::SPACE_ALMOST_FULL:
            case PlaybackStatus::SPACE_FULL:
                ALOGW("[vts] stop playback inputing %d", status);
                mKeepWritingPlaybackFMQ = false;
                break;
        }
        return Void();
    }

    void testFilterDataOutput();
    void stopPlaybackThread();
    void testRecordOutput();
    void stopRecordThread();

    void startPlaybackInputThread(PlaybackConf playbackConf, MQDesc& playbackMQDescriptor);
    void startRecordOutputThread(RecordSettings recordSetting, MQDesc& recordMQDescriptor);
    static void* __threadLoopPlayback(void* threadArgs);
    static void* __threadLoopRecord(void* threadArgs);
    void playbackThreadLoop(PlaybackConf* playbackConf, bool* keepWritingPlaybackFMQ);
    void recordThreadLoop(RecordSettings* recordSetting, bool* keepWritingPlaybackFMQ);

    bool readRecordFMQ();

  private:
    struct PlaybackThreadArgs {
        DvrCallback* user;
        PlaybackConf* playbackConf;
        bool* keepWritingPlaybackFMQ;
    };
    struct RecordThreadArgs {
        DvrCallback* user;
        RecordSettings* recordSetting;
        bool* keepReadingRecordFMQ;
    };
    uint16_t mDataLength = 0;
    std::vector<uint8_t> mDataOutputBuffer;

    std::map<uint32_t, std::unique_ptr<FilterMQ>> mFilterMQ;
    std::unique_ptr<FilterMQ> mPlaybackMQ;
    std::unique_ptr<FilterMQ> mRecordMQ;
    std::map<uint32_t, EventFlag*> mFilterMQEventFlag;

    android::Mutex mMsgLock;
    android::Mutex mPlaybackThreadLock;
    android::Mutex mRecordThreadLock;
    android::Condition mMsgCondition;

    bool mKeepWritingPlaybackFMQ = true;
    bool mKeepReadingRecordFMQ = true;
    bool mPlaybackThreadRunning;
    bool mRecordThreadRunning;
    pthread_t mPlaybackThread;
    pthread_t mRecordThread;

    int mPidFilterOutputCount = 0;
};

class TunerFrontendHidlTest : public testing::TestWithParam<std::string> {
  public:
    sp<ITuner> mService;
    FrontendTests mFrontendTests;

    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initFrontendConfig();
        initFrontendScanConfig();

        mFrontendTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }
};

class TunerHidlTest : public testing::TestWithParam<std::string> {
  public:
    sp<ITuner> mService;
    FrontendTests mFrontendTests;

    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initFrontendConfig();
        initFrontendScanConfig();
        initFilterConfig();

        mFrontendTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<IDescrambler> mDescrambler;

    sp<IDemux> mDemux;
    sp<IDvr> mDvr;
    sp<IFilter> mFilter;
    std::map<uint32_t, sp<IFilter>> mFilters;
    std::map<uint32_t, sp<FilterCallback>> mFilterCallbacks;

    sp<FilterCallback> mFilterCallback;
    sp<DvrCallback> mDvrCallback;
    MQDesc mFilterMQDescriptor;
    MQDesc mDvrMQDescriptor;
    MQDesc mRecordMQDescriptor;
    vector<uint32_t> mUsedFilterIds;
    hidl_vec<FrontendId> mFeIds;

    uint32_t mDemuxId;
    uint32_t mFilterId = -1;

    pthread_t mPlaybackshread;
    bool mPlaybackThreadRunning;

    AssertionResult openDemux();
    AssertionResult setDemuxFrontendDataSource(uint32_t frontendId);
    AssertionResult closeDemux();

    AssertionResult openDvrInDemux(DvrType type);
    AssertionResult configDvr(DvrSettings setting);
    AssertionResult getDvrMQDescriptor();

    AssertionResult openFilterInDemux(DemuxFilterType type);
    AssertionResult getNewlyOpenedFilterId(uint32_t& filterId);
    AssertionResult configFilter(DemuxFilterSettings setting, uint32_t filterId);
    AssertionResult getFilterMQDescriptor(uint32_t filterId);
    AssertionResult startFilter(uint32_t filterId);
    AssertionResult stopFilter(uint32_t filterId);
    AssertionResult closeFilter(uint32_t filterId);

    AssertionResult createDescrambler();
    AssertionResult closeDescrambler();

    AssertionResult playbackDataFlowTest(vector<FilterConfig> filterConf, PlaybackConf playbackConf,
                                         vector<string> goldenOutputFiles);
    AssertionResult recordDataFlowTest(vector<FilterConfig> filterConf,
                                       RecordSettings recordSetting,
                                       vector<string> goldenOutputFiles);
    AssertionResult broadcastDataFlowTest(vector<string> goldenOutputFiles);

    void broadcastSingleFilterTest(FilterConfig filterConf, FrontendConfig frontendConf);

    FilterEventType getFilterEventType(DemuxFilterType type);
};
}  // namespace