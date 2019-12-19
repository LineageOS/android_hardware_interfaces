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

#define LOG_TAG "Tuner_hidl_hal_test"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/tv/tuner/1.0/IDemux.h>
#include <android/hardware/tv/tuner/1.0/IDescrambler.h>
#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <android/hardware/tv/tuner/1.0/IDvrCallback.h>
#include <android/hardware/tv/tuner/1.0/IFilter.h>
#include <android/hardware/tv/tuner/1.0/IFilterCallback.h>
#include <android/hardware/tv/tuner/1.0/IFrontend.h>
#include <android/hardware/tv/tuner/1.0/IFrontendCallback.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <binder/MemoryDealer.h>
#include <fmq/MessageQueue.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <fstream>
#include <iostream>
#include <map>

#define WAIT_TIMEOUT 3000000000
#define WAIT_TIMEOUT_data_ready 3000000000 * 4

using android::Condition;
using android::IMemory;
using android::IMemoryHeap;
using android::MemoryDealer;
using android::Mutex;
using android::sp;
using android::hardware::EventFlag;
using android::hardware::fromHeap;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::HidlMemory;
using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tv::tuner::V1_0::DataFormat;
using android::hardware::tv::tuner::V1_0::DemuxFilterEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
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
using android::hardware::tv::tuner::V1_0::FrontendAtscModulation;
using android::hardware::tv::tuner::V1_0::FrontendAtscSettings;
using android::hardware::tv::tuner::V1_0::FrontendDvbtSettings;
using android::hardware::tv::tuner::V1_0::FrontendEventType;
using android::hardware::tv::tuner::V1_0::FrontendId;
using android::hardware::tv::tuner::V1_0::FrontendInnerFec;
using android::hardware::tv::tuner::V1_0::FrontendScanMessage;
using android::hardware::tv::tuner::V1_0::FrontendScanMessageType;
using android::hardware::tv::tuner::V1_0::FrontendSettings;
using android::hardware::tv::tuner::V1_0::IDemux;
using android::hardware::tv::tuner::V1_0::IDescrambler;
using android::hardware::tv::tuner::V1_0::IDvr;
using android::hardware::tv::tuner::V1_0::IDvrCallback;
using android::hardware::tv::tuner::V1_0::IFilter;
using android::hardware::tv::tuner::V1_0::IFilterCallback;
using android::hardware::tv::tuner::V1_0::IFrontend;
using android::hardware::tv::tuner::V1_0::IFrontendCallback;
using android::hardware::tv::tuner::V1_0::ITuner;
using android::hardware::tv::tuner::V1_0::PlaybackSettings;
using android::hardware::tv::tuner::V1_0::PlaybackStatus;
using android::hardware::tv::tuner::V1_0::RecordSettings;
using android::hardware::tv::tuner::V1_0::RecordStatus;
using android::hardware::tv::tuner::V1_0::Result;

namespace {

using FilterMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using MQDesc = MQDescriptorSync<uint8_t>;

const std::vector<uint8_t> goldenDataOutputBuffer{
        0x00, 0x00, 0x00, 0x01, 0x09, 0xf0, 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1e, 0xdb,
        0x01, 0x40, 0x16, 0xec, 0x04, 0x40, 0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 0x0f, 0x03,
        0xc5, 0x8b, 0xb8, 0x00, 0x00, 0x00, 0x01, 0x68, 0xca, 0x8c, 0xb2, 0x00, 0x00, 0x01, 0x06,
        0x05, 0xff, 0xff, 0x70, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8,
        0x20, 0xd9, 0x23, 0xee, 0xef, 0x78, 0x32, 0x36, 0x34, 0x20, 0x2d, 0x20, 0x63, 0x6f, 0x72,
        0x65, 0x20, 0x31, 0x34, 0x32, 0x20, 0x2d, 0x20, 0x48, 0x2e, 0x32, 0x36, 0x34, 0x2f, 0x4d,
        0x50, 0x45, 0x47, 0x2d, 0x34, 0x20, 0x41, 0x56, 0x43, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x63,
        0x20, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x6c, 0x65, 0x66, 0x74, 0x20, 0x32, 0x30, 0x30,
        0x33, 0x2d, 0x32, 0x30, 0x31, 0x34, 0x20, 0x2d, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
        0x2f, 0x77, 0x77, 0x77, 0x2e, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x6c, 0x61, 0x6e, 0x2e, 0x6f,
        0x72, 0x67, 0x2f, 0x78, 0x32, 0x36, 0x34, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x20, 0x2d, 0x20,
        0x6f, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x3a, 0x20, 0x63, 0x61, 0x62, 0x61, 0x63, 0x3d,
        0x30, 0x20, 0x72, 0x65, 0x66, 0x3d, 0x32, 0x20, 0x64, 0x65, 0x62, 0x6c, 0x6f, 0x63, 0x6b,
        0x3d, 0x31, 0x3a, 0x30, 0x3a, 0x30, 0x20, 0x61, 0x6e, 0x61, 0x6c, 0x79, 0x73, 0x65, 0x3d,
        0x30, 0x78, 0x31, 0x3a, 0x30, 0x78, 0x31, 0x31, 0x31, 0x20, 0x6d, 0x65, 0x3d, 0x68, 0x65,
        0x78, 0x20, 0x73, 0x75, 0x62, 0x6d, 0x65, 0x3d, 0x37, 0x20, 0x70, 0x73, 0x79, 0x3d, 0x31,
        0x20, 0x70, 0x73, 0x79, 0x5f, 0x72, 0x64, 0x3d, 0x31, 0x2e, 0x30, 0x30, 0x3a, 0x30, 0x2e,
        0x30, 0x30, 0x20, 0x6d, 0x69, 0x78, 0x65, 0x64, 0x5f, 0x72, 0x65, 0x66, 0x3d, 0x31, 0x20,
        0x6d, 0x65, 0x5f, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x3d, 0x31, 0x36, 0x20, 0x63, 0x68, 0x72,
        0x6f, 0x6d, 0x61, 0x5f, 0x6d, 0x65, 0x3d, 0x31, 0x20, 0x74, 0x72, 0x65, 0x6c, 0x6c, 0x69,
        0x73, 0x3d, 0x31, 0x20, 0x38, 0x78, 0x38, 0x64, 0x63, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x71,
        0x6d, 0x3d, 0x30, 0x20, 0x64, 0x65, 0x61, 0x64, 0x7a, 0x6f, 0x6e, 0x65, 0x3d, 0x32, 0x31,
        0x2c, 0x31, 0x31, 0x20, 0x66, 0x61, 0x73, 0x74, 0x5f, 0x70, 0x73, 0x6b, 0x69, 0x70, 0x3d,
        0x31, 0x20, 0x63, 0x68, 0x72, 0x6f, 0x6d, 0x61, 0x5f, 0x71, 0x70, 0x5f, 0x6f, 0x66, 0x66,
        0x73, 0x65, 0x74, 0x3d, 0x2d, 0x32, 0x20, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d,
        0x36, 0x30, 0x20, 0x6c, 0x6f, 0x6f, 0x6b, 0x61, 0x68, 0x65, 0x61, 0x64, 0x5f, 0x74, 0x68,
        0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x35, 0x20, 0x73, 0x6c, 0x69, 0x63, 0x65, 0x64, 0x5f,
        0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x30, 0x20, 0x6e, 0x72, 0x3d, 0x30, 0x20,
        0x64, 0x65, 0x63, 0x69, 0x6d, 0x61, 0x74, 0x65, 0x3d, 0x31, 0x20, 0x69, 0x6e, 0x74, 0x65,
        0x72, 0x6c, 0x61, 0x63, 0x65, 0x64, 0x3d, 0x30, 0x20, 0x62, 0x6c, 0x75, 0x72, 0x61, 0x79,
        0x5f, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74,
        0x72, 0x61, 0x69, 0x6e, 0x65, 0x64, 0x5f, 0x69, 0x6e, 0x74, 0x72, 0x61, 0x3d, 0x30, 0x20,
        0x62, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x73, 0x3d, 0x30, 0x20, 0x77, 0x65, 0x69, 0x67, 0x68,
        0x74, 0x70, 0x3d, 0x30, 0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x3d, 0x32, 0x35, 0x30,
        0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x5f, 0x6d, 0x69, 0x6e, 0x3d, 0x32, 0x35, 0x20,
        0x73, 0x63, 0x65, 0x6e, 0x65,
};

// const uint16_t FMQ_SIZE_4K = 0x1000;
const uint32_t FMQ_SIZE_1M = 0x100000;
const uint32_t FMQ_SIZE_16M = 0x1000000;

struct FilterConf {
    DemuxFilterType type;
    DemuxFilterSettings setting;
};

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

class FrontendCallback : public IFrontendCallback {
  public:
    virtual Return<void> onEvent(FrontendEventType frontendEventType) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mEventReceived = true;
        mEventType = frontendEventType;
        mMsgCondition.signal();
        return Void();
    }

    virtual Return<void> onScanMessage(FrontendScanMessageType /* type */,
                                       const FrontendScanMessage& /* message */) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mScanMessageReceived = true;
        mMsgCondition.signal();
        return Void();
    };

    void testOnEvent(sp<IFrontend>& frontend, FrontendSettings settings);
    void testOnDiseqcMessage(sp<IFrontend>& frontend, FrontendSettings settings);

  private:
    bool mEventReceived = false;
    bool mDiseqcMessageReceived = false;
    bool mScanMessageReceived = false;
    FrontendEventType mEventType;
    hidl_vec<uint8_t> mEventMessage;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
};

void FrontendCallback::testOnEvent(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);

    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "event not received within timeout";
            return;
        }
    }
}

void FrontendCallback::testOnDiseqcMessage(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);

    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mDiseqcMessageReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "diseqc message not received within timeout";
            return;
        }
    }
}

class FilterCallback : public IFilterCallback {
  public:
    virtual Return<void> onFilterEvent(const DemuxFilterEvent& filterEvent) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        // Temprarily we treat the first coming back filter data on the matching pid a success
        // once all of the MQ are cleared, means we got all the expected output
        mFilterIdToEvent = filterEvent;
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
    void setFilterEventType(FilterEventType type) { mFilterEventType = type; }

    void testFilterDataOutput();

    void startFilterEventThread(DemuxFilterEvent event);
    static void* __threadLoopFilter(void* threadArgs);
    void filterThreadLoop(DemuxFilterEvent& event);

    void updateFilterMQ(MQDesc& filterMQDescriptor);
    void updateGoldenOutputMap(string goldenOutputFile);
    bool readFilterEventData();

  private:
    struct FilterThreadArgs {
        FilterCallback* user;
        DemuxFilterEvent event;
    };
    uint16_t mDataLength = 0;
    std::vector<uint8_t> mDataOutputBuffer;

    string mFilterIdToGoldenOutput;

    uint32_t mFilterId;
    FilterEventType mFilterEventType;
    std::unique_ptr<FilterMQ> mFilterIdToMQ;
    EventFlag* mFilterIdToMQEventFlag;
    DemuxFilterEvent mFilterIdToEvent;

    android::Mutex mMsgLock;
    android::Mutex mFilterOutputLock;
    android::Condition mMsgCondition;
    android::Condition mFilterOutputCondition;

    pthread_t mFilterThread;

    int mPidFilterOutputCount = 0;
};

void FilterCallback::startFilterEventThread(DemuxFilterEvent event) {
    struct FilterThreadArgs* threadArgs =
            (struct FilterThreadArgs*)malloc(sizeof(struct FilterThreadArgs));
    threadArgs->user = this;
    threadArgs->event = event;

    pthread_create(&mFilterThread, NULL, __threadLoopFilter, (void*)threadArgs);
    pthread_setname_np(mFilterThread, "test_playback_input_loop");
}

void FilterCallback::testFilterDataOutput() {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (mPidFilterOutputCount < 1) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "filter output matching pid does not output within timeout";
            return;
        }
    }
    mPidFilterOutputCount = 0;
    ALOGW("[vts] pass and stop");
}

void FilterCallback::updateFilterMQ(MQDesc& filterMQDescriptor) {
    mFilterIdToMQ = std::make_unique<FilterMQ>(filterMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mFilterIdToMQ);
    EXPECT_TRUE(EventFlag::createEventFlag(mFilterIdToMQ->getEventFlagWord(),
                                           &mFilterIdToMQEventFlag) == android::OK);
}

void FilterCallback::updateGoldenOutputMap(string goldenOutputFile) {
    mFilterIdToGoldenOutput = goldenOutputFile;
}

void* FilterCallback::__threadLoopFilter(void* threadArgs) {
    FilterCallback* const self =
            static_cast<FilterCallback*>(((struct FilterThreadArgs*)threadArgs)->user);
    self->filterThreadLoop(((struct FilterThreadArgs*)threadArgs)->event);
    return 0;
}

void FilterCallback::filterThreadLoop(DemuxFilterEvent& /* event */) {
    android::Mutex::Autolock autoLock(mFilterOutputLock);
    // Read from mFilterIdToMQ[event.filterId] per event and filter type

    // Assemble to filterOutput[filterId]

    // check if filterOutput[filterId] matches goldenOutput[filterId]

    // If match, remove filterId entry from MQ map

    // end thread
}

bool FilterCallback::readFilterEventData() {
    bool result = false;
    DemuxFilterEvent filterEvent = mFilterIdToEvent;
    ALOGW("[vts] reading from filter FMQ %d", mFilterId);
    // todo separate filter handlers
    for (int i = 0; i < filterEvent.events.size(); i++) {
        switch (mFilterEventType) {
            case FilterEventType::SECTION:
                mDataLength = filterEvent.events[i].section().dataLength;
                break;
            case FilterEventType::PES:
                mDataLength = filterEvent.events[i].pes().dataLength;
                break;
            case FilterEventType::MEDIA:
                break;
            case FilterEventType::RECORD:
                break;
            case FilterEventType::MMTPRECORD:
                break;
            case FilterEventType::DOWNLOAD:
                break;
            default:
                break;
        }
        // EXPECT_TRUE(mDataLength == goldenDataOutputBuffer.size()) << "buffer size does not
        // match";

        mDataOutputBuffer.resize(mDataLength);
        result = mFilterIdToMQ->read(mDataOutputBuffer.data(), mDataLength);
        EXPECT_TRUE(result) << "can't read from Filter MQ";

        /*for (int i = 0; i < mDataLength; i++) {
            EXPECT_TRUE(goldenDataOutputBuffer[i] == mDataOutputBuffer[i]) << "data does not match";
        }*/
    }
    mFilterIdToMQEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_CONSUMED));
    return result;
}

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

    std::map<uint32_t, std::unique_ptr<FilterMQ>> mFilterIdToMQ;
    std::unique_ptr<FilterMQ> mPlaybackMQ;
    std::unique_ptr<FilterMQ> mRecordMQ;
    std::map<uint32_t, EventFlag*> mFilterIdToMQEventFlag;
    std::map<uint32_t, DemuxFilterEvent> mFilterIdToEvent;

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

void DvrCallback::startPlaybackInputThread(PlaybackConf playbackConf,
                                           MQDesc& playbackMQDescriptor) {
    mPlaybackMQ = std::make_unique<FilterMQ>(playbackMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mPlaybackMQ);
    struct PlaybackThreadArgs* threadArgs =
            (struct PlaybackThreadArgs*)malloc(sizeof(struct PlaybackThreadArgs));
    threadArgs->user = this;
    threadArgs->playbackConf = &playbackConf;
    threadArgs->keepWritingPlaybackFMQ = &mKeepWritingPlaybackFMQ;

    pthread_create(&mPlaybackThread, NULL, __threadLoopPlayback, (void*)threadArgs);
    pthread_setname_np(mPlaybackThread, "test_playback_input_loop");
}

void DvrCallback::stopPlaybackThread() {
    mPlaybackThreadRunning = false;
    mKeepWritingPlaybackFMQ = false;

    android::Mutex::Autolock autoLock(mPlaybackThreadLock);
}

void* DvrCallback::__threadLoopPlayback(void* threadArgs) {
    DvrCallback* const self =
            static_cast<DvrCallback*>(((struct PlaybackThreadArgs*)threadArgs)->user);
    self->playbackThreadLoop(((struct PlaybackThreadArgs*)threadArgs)->playbackConf,
                             ((struct PlaybackThreadArgs*)threadArgs)->keepWritingPlaybackFMQ);
    return 0;
}

void DvrCallback::playbackThreadLoop(PlaybackConf* playbackConf, bool* keepWritingPlaybackFMQ) {
    android::Mutex::Autolock autoLock(mPlaybackThreadLock);
    mPlaybackThreadRunning = true;

    // Create the EventFlag that is used to signal the HAL impl that data have been
    // written into the Playback FMQ
    EventFlag* playbackMQEventFlag;
    EXPECT_TRUE(EventFlag::createEventFlag(mPlaybackMQ->getEventFlagWord(), &playbackMQEventFlag) ==
                android::OK);

    // open the stream and get its length
    std::ifstream inputData(playbackConf->inputDataFile, std::ifstream::binary);
    int writeSize = playbackConf->setting.packetSize * 6;
    char* buffer = new char[writeSize];
    ALOGW("[vts] playback thread loop start %s", playbackConf->inputDataFile.c_str());
    if (!inputData.is_open()) {
        mPlaybackThreadRunning = false;
        ALOGW("[vts] Error %s", strerror(errno));
    }

    while (mPlaybackThreadRunning) {
        // move the stream pointer for packet size * 6 every read until the end
        while (*keepWritingPlaybackFMQ) {
            inputData.read(buffer, writeSize);
            if (!inputData) {
                int leftSize = inputData.gcount();
                if (leftSize == 0) {
                    mPlaybackThreadRunning = false;
                    break;
                }
                inputData.clear();
                inputData.read(buffer, leftSize);
                // Write the left over of the input data and quit the thread
                if (leftSize > 0) {
                    EXPECT_TRUE(mPlaybackMQ->write((unsigned char*)&buffer[0], leftSize));
                    playbackMQEventFlag->wake(
                            static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY));
                }
                mPlaybackThreadRunning = false;
                break;
            }
            // Write input FMQ and notify the Tuner Implementation
            EXPECT_TRUE(mPlaybackMQ->write((unsigned char*)&buffer[0], writeSize));
            playbackMQEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY));
            inputData.seekg(writeSize, inputData.cur);
            sleep(1);
        }
    }

    ALOGW("[vts] Playback thread end.");

    delete[] buffer;
    inputData.close();
}

void DvrCallback::testRecordOutput() {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (mDataOutputBuffer.empty()) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "record output matching pid does not output within timeout";
            return;
        }
    }
    stopRecordThread();
    ALOGW("[vts] record pass and stop");
}

void DvrCallback::startRecordOutputThread(RecordSettings recordSetting,
                                          MQDesc& recordMQDescriptor) {
    mRecordMQ = std::make_unique<FilterMQ>(recordMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mRecordMQ);
    struct RecordThreadArgs* threadArgs =
            (struct RecordThreadArgs*)malloc(sizeof(struct RecordThreadArgs));
    threadArgs->user = this;
    threadArgs->recordSetting = &recordSetting;
    threadArgs->keepReadingRecordFMQ = &mKeepReadingRecordFMQ;

    pthread_create(&mRecordThread, NULL, __threadLoopRecord, (void*)threadArgs);
    pthread_setname_np(mRecordThread, "test_record_input_loop");
}

void* DvrCallback::__threadLoopRecord(void* threadArgs) {
    DvrCallback* const self =
            static_cast<DvrCallback*>(((struct RecordThreadArgs*)threadArgs)->user);
    self->recordThreadLoop(((struct RecordThreadArgs*)threadArgs)->recordSetting,
                           ((struct RecordThreadArgs*)threadArgs)->keepReadingRecordFMQ);
    return 0;
}

void DvrCallback::recordThreadLoop(RecordSettings* /*recordSetting*/, bool* keepReadingRecordFMQ) {
    ALOGD("[vts] DvrCallback record threadLoop start.");
    android::Mutex::Autolock autoLock(mRecordThreadLock);
    mRecordThreadRunning = true;

    // Create the EventFlag that is used to signal the HAL impl that data have been
    // read from the Record FMQ
    EventFlag* recordMQEventFlag;
    EXPECT_TRUE(EventFlag::createEventFlag(mRecordMQ->getEventFlagWord(), &recordMQEventFlag) ==
                android::OK);

    while (mRecordThreadRunning) {
        while (*keepReadingRecordFMQ) {
            uint32_t efState = 0;
            android::status_t status = recordMQEventFlag->wait(
                    static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY), &efState, WAIT_TIMEOUT,
                    true /* retry on spurious wake */);
            if (status != android::OK) {
                ALOGD("[vts] wait for data ready on the record FMQ");
                continue;
            }
            // Our current implementation filter the data and write it into the filter FMQ
            // immediately after the DATA_READY from the VTS/framework
            if (!readRecordFMQ()) {
                ALOGD("[vts] record data failed to be filtered. Ending thread");
                mRecordThreadRunning = false;
                break;
            }
        }
    }

    mRecordThreadRunning = false;
    ALOGD("[vts] record thread ended.");
}

bool DvrCallback::readRecordFMQ() {
    android::Mutex::Autolock autoLock(mMsgLock);
    bool result = false;
    mDataOutputBuffer.clear();
    mDataOutputBuffer.resize(mRecordMQ->availableToRead());
    result = mRecordMQ->read(mDataOutputBuffer.data(), mRecordMQ->availableToRead());
    EXPECT_TRUE(result) << "can't read from Record MQ";
    mMsgCondition.signal();
    return result;
}

void DvrCallback::stopRecordThread() {
    mKeepReadingRecordFMQ = false;
    mRecordThreadRunning = false;
    android::Mutex::Autolock autoLock(mRecordThreadLock);
}

// Test environment for Tuner HIDL HAL.
class TunerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static TunerHidlEnvironment* Instance() {
        static TunerHidlEnvironment* instance = new TunerHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<ITuner>(); }
};

class TunerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  public:
    virtual void SetUp() override {
        mService = ::testing::VtsHalHidlTargetTestBase::getService<ITuner>(
                TunerHidlEnvironment::Instance()->getServiceName<ITuner>());
        ASSERT_NE(mService, nullptr);
    }

    sp<ITuner> mService;

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<IFrontend> mFrontend;
    sp<FrontendCallback> mFrontendCallback;
    sp<IDescrambler> mDescrambler;
    sp<IDemux> mDemux;
    sp<IDvr> mDvr;
    sp<IFilter> mFilter;
    std::map<uint32_t, sp<IFilter>> mFilters;
    std::map<uint32_t, sp<FilterCallback>> mFilterCallbacks;
    sp<FilterCallback> mFilterCallback;
    sp<DvrCallback> mDvrCallback;
    MQDesc mFilterMQDescriptor;
    MQDesc mPlaybackMQDescriptor;
    MQDesc mRecordMQDescriptor;
    vector<uint32_t> mUsedFilterIds;

    uint32_t mDemuxId;
    uint32_t mFilterId;

    pthread_t mPlaybackshread;
    bool mPlaybackThreadRunning;

    ::testing::AssertionResult createFrontend(int32_t frontendId);
    ::testing::AssertionResult tuneFrontend(int32_t frontendId);
    ::testing::AssertionResult stopTuneFrontend(int32_t frontendId);
    ::testing::AssertionResult closeFrontend(int32_t frontendId);
    ::testing::AssertionResult createDemux();
    ::testing::AssertionResult createDemuxWithFrontend(int32_t frontendId,
                                                       FrontendSettings settings);
    ::testing::AssertionResult getPlaybackMQDescriptor();
    ::testing::AssertionResult addPlaybackToDemux(PlaybackSettings setting);
    ::testing::AssertionResult getRecordMQDescriptor();
    ::testing::AssertionResult addRecordToDemux(RecordSettings setting);
    ::testing::AssertionResult addFilterToDemux(DemuxFilterType type, DemuxFilterSettings setting);
    ::testing::AssertionResult getFilterMQDescriptor();
    ::testing::AssertionResult closeDemux();
    ::testing::AssertionResult createDescrambler();
    ::testing::AssertionResult closeDescrambler();

    ::testing::AssertionResult playbackDataFlowTest(vector<FilterConf> filterConf,
                                                    PlaybackConf playbackConf,
                                                    vector<string> goldenOutputFiles);
    ::testing::AssertionResult recordDataFlowTest(vector<FilterConf> filterConf,
                                                  RecordSettings recordSetting,
                                                  vector<string> goldenOutputFiles);
    ::testing::AssertionResult broadcastDataFlowTest(vector<FilterConf> filterConf,
                                                     vector<string> goldenOutputFiles);
};

::testing::AssertionResult TunerHidlTest::createFrontend(int32_t frontendId) {
    Result status;

    mService->openFrontendById(frontendId, [&](Result result, const sp<IFrontend>& frontend) {
        mFrontend = frontend;
        status = result;
    });
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    mFrontendCallback = new FrontendCallback();
    auto callbackStatus = mFrontend->setCallback(mFrontendCallback);

    return ::testing::AssertionResult(callbackStatus.isOk());
}

::testing::AssertionResult TunerHidlTest::tuneFrontend(int32_t frontendId) {
    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Frontend Settings for testing
    FrontendSettings frontendSettings;
    FrontendAtscSettings frontendAtscSettings{
            .frequency = 0,
            .modulation = FrontendAtscModulation::UNDEFINED,
    };
    frontendSettings.atsc(frontendAtscSettings);
    mFrontendCallback->testOnEvent(mFrontend, frontendSettings);

    FrontendDvbtSettings frontendDvbtSettings{
            .frequency = 0,
    };
    frontendSettings.dvbt(frontendDvbtSettings);
    mFrontendCallback->testOnEvent(mFrontend, frontendSettings);

    return ::testing::AssertionResult(true);
}

::testing::AssertionResult TunerHidlTest::stopTuneFrontend(int32_t frontendId) {
    Result status;
    if (!mFrontend && createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mFrontend->stopTune();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::closeFrontend(int32_t frontendId) {
    Result status;
    if (!mFrontend && createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mFrontend->close();
    mFrontend = nullptr;
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDemux() {
    Result status;

    mService->openDemux([&](Result result, uint32_t demuxId, const sp<IDemux>& demux) {
        mDemux = demux;
        mDemuxId = demuxId;
        status = result;
    });
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDemuxWithFrontend(int32_t frontendId,
                                                                  FrontendSettings settings) {
    Result status;

    if (!mDemux && createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    if (!mFrontend && createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    mFrontendCallback->testOnEvent(mFrontend, settings);

    status = mDemux->setFrontendDataSource(frontendId);

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::closeDemux() {
    Result status;
    if (!mDemux && createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDemux->close();
    mDemux = nullptr;
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDescrambler() {
    Result status;

    mService->openDescrambler([&](Result result, const sp<IDescrambler>& descrambler) {
        mDescrambler = descrambler;
        status = result;
    });
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    if (!mDemux && createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDescrambler->setDemuxSource(mDemuxId);
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    // Test if demux source can be set more than once.
    status = mDescrambler->setDemuxSource(mDemuxId);
    return ::testing::AssertionResult(status == Result::INVALID_STATE);
}

::testing::AssertionResult TunerHidlTest::closeDescrambler() {
    Result status;
    if (!mDescrambler && createDescrambler() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDescrambler->close();
    mDescrambler = nullptr;
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::addPlaybackToDemux(PlaybackSettings setting) {
    Result status;

    if (!mDemux && createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Create dvr callback
    mDvrCallback = new DvrCallback();

    // Add playback input to the local demux
    mDemux->openDvr(DvrType::PLAYBACK, FMQ_SIZE_1M, mDvrCallback,
                    [&](Result result, const sp<IDvr>& dvr) {
                        mDvr = dvr;
                        status = result;
                    });

    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    DvrSettings dvrSetting;
    dvrSetting.playback(setting);
    status = mDvr->configure(dvrSetting);

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::getPlaybackMQDescriptor() {
    Result status;

    if ((!mDemux && createDemux() == ::testing::AssertionFailure()) || !mDvr) {
        return ::testing::AssertionFailure();
    }

    mDvr->getQueueDesc([&](Result result, const MQDesc& dvrMQDesc) {
        mPlaybackMQDescriptor = dvrMQDesc;
        status = result;
    });

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::addRecordToDemux(RecordSettings setting) {
    Result status;

    if (!mDemux && createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Create dvr callback
    mDvrCallback = new DvrCallback();

    // Add playback input to the local demux
    mDemux->openDvr(DvrType::RECORD, FMQ_SIZE_1M, mDvrCallback,
                    [&](Result result, const sp<IDvr>& dvr) {
                        mDvr = dvr;
                        status = result;
                    });

    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    DvrSettings dvrSetting;
    dvrSetting.record(setting);
    status = mDvr->configure(dvrSetting);

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::getRecordMQDescriptor() {
    Result status;

    if ((!mDemux && createDemux() == ::testing::AssertionFailure()) || !mDvr) {
        return ::testing::AssertionFailure();
    }

    mDvr->getQueueDesc([&](Result result, const MQDesc& dvrMQDesc) {
        mRecordMQDescriptor = dvrMQDesc;
        status = result;
    });

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::addFilterToDemux(DemuxFilterType type,
                                                           DemuxFilterSettings setting) {
    Result status;

    if (!mDemux && createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Create demux callback
    mFilterCallback = new FilterCallback();

    // Add filter to the local demux
    mDemux->openFilter(type, FMQ_SIZE_16M, mFilterCallback,
                       [&](Result result, const sp<IFilter>& filter) {
                           mFilter = filter;
                           status = result;
                       });

    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    mFilter->getId([&](Result result, uint32_t filterId) {
        mFilterId = filterId;
        status = result;
    });

    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    mFilterCallback->setFilterId(mFilterId);

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
    mFilterCallback->setFilterEventType(eventType);

    // Configure the filter
    status = mFilter->configure(setting);

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::getFilterMQDescriptor() {
    Result status;

    if (!mDemux || !mFilter) {
        return ::testing::AssertionFailure();
    }

    mFilter->getQueueDesc([&](Result result, const MQDesc& filterMQDesc) {
        mFilterMQDescriptor = filterMQDesc;
        status = result;
    });

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::playbackDataFlowTest(
        vector<FilterConf> filterConf, PlaybackConf playbackConf,
        vector<string> /*goldenOutputFiles*/) {
    Result status;
    int filterIdsSize;
    // Filter Configuration Module
    for (int i = 0; i < filterConf.size(); i++) {
        if (addFilterToDemux(filterConf[i].type, filterConf[i].setting) ==
                    ::testing::AssertionFailure() ||
            // TODO use a map to save the FMQs/EvenFlags and pass to callback
            getFilterMQDescriptor() == ::testing::AssertionFailure()) {
            return ::testing::AssertionFailure();
        }
        filterIdsSize = mUsedFilterIds.size();
        mUsedFilterIds.resize(filterIdsSize + 1);
        mUsedFilterIds[filterIdsSize] = mFilterId;
        mFilters[mFilterId] = mFilter;
        mFilterCallbacks[mFilterId] = mFilterCallback;
        mFilterCallback->updateFilterMQ(mFilterMQDescriptor);
        // mDemuxCallback->updateGoldenOutputMap(goldenOutputFiles[i]);
        status = mFilter->start();
        if (status != Result::SUCCESS) {
            return ::testing::AssertionFailure();
        }
    }

    // Playback Input Module
    PlaybackSettings playbackSetting = playbackConf.setting;
    if (addPlaybackToDemux(playbackSetting) == ::testing::AssertionFailure() ||
        getPlaybackMQDescriptor() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mDvr->attachFilter(mFilters[mUsedFilterIds[i]]) != Result::SUCCESS) {
            return ::testing::AssertionFailure();
        }
    }
    mDvrCallback->startPlaybackInputThread(playbackConf, mPlaybackMQDescriptor);
    status = mDvr->start();
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    // Data Verify Module
    std::map<uint32_t, sp<FilterCallback>>::iterator it;
    for (it = mFilterCallbacks.begin(); it != mFilterCallbacks.end(); it++) {
        it->second->testFilterDataOutput();
    }
    mDvrCallback->stopPlaybackThread();

    // Clean Up Module
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mFilters[mUsedFilterIds[i]]->stop() != Result::SUCCESS) {
            return ::testing::AssertionFailure();
        }
    }
    if (mDvr->stop() != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }
    mUsedFilterIds.clear();
    mFilterCallbacks.clear();
    mFilters.clear();
    return closeDemux();
}

::testing::AssertionResult TunerHidlTest::broadcastDataFlowTest(
        vector<FilterConf> filterConf, vector<string> /*goldenOutputFiles*/) {
    Result status;
    hidl_vec<FrontendId> feIds;

    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return ::testing::AssertionFailure();
    }

    FrontendDvbtSettings dvbt{
            .frequency = 1000,
    };
    FrontendSettings settings;
    settings.dvbt(dvbt);

    if (createDemuxWithFrontend(feIds[0], settings) != ::testing::AssertionSuccess()) {
        return ::testing::AssertionFailure();
    }

    int filterIdsSize;
    // Filter Configuration Module
    for (int i = 0; i < filterConf.size(); i++) {
        if (addFilterToDemux(filterConf[i].type, filterConf[i].setting) ==
                    ::testing::AssertionFailure() ||
            // TODO use a map to save the FMQs/EvenFlags and pass to callback
            getFilterMQDescriptor() == ::testing::AssertionFailure()) {
            return ::testing::AssertionFailure();
        }
        filterIdsSize = mUsedFilterIds.size();
        mUsedFilterIds.resize(filterIdsSize + 1);
        mUsedFilterIds[filterIdsSize] = mFilterId;
        mFilters[mFilterId] = mFilter;
        mFilterCallbacks[mFilterId] = mFilterCallback;
        mFilterCallback->updateFilterMQ(mFilterMQDescriptor);
        status = mFilter->start();
        if (status != Result::SUCCESS) {
            return ::testing::AssertionFailure();
        }
    }

    // Data Verify Module
    std::map<uint32_t, sp<FilterCallback>>::iterator it;
    for (it = mFilterCallbacks.begin(); it != mFilterCallbacks.end(); it++) {
        it->second->testFilterDataOutput();
    }

    // Clean Up Module
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mFilters[mUsedFilterIds[i]]->stop() != Result::SUCCESS) {
            return ::testing::AssertionFailure();
        }
    }
    if (mFrontend->stopTune() != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }
    mUsedFilterIds.clear();
    mFilterCallbacks.clear();
    mFilters.clear();
    return closeDemux();
}

::testing::AssertionResult TunerHidlTest::recordDataFlowTest(vector<FilterConf> filterConf,
                                                             RecordSettings recordSetting,
                                                             vector<string> /*goldenOutputFiles*/) {
    Result status;
    hidl_vec<FrontendId> feIds;

    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return ::testing::AssertionFailure();
    }

    FrontendDvbtSettings dvbt{
            .frequency = 1000,
    };
    FrontendSettings settings;
    settings.dvbt(dvbt);

    int filterIdsSize;
    // Filter Configuration Module
    for (int i = 0; i < filterConf.size(); i++) {
        if (addFilterToDemux(filterConf[i].type, filterConf[i].setting) ==
                    ::testing::AssertionFailure() ||
            // TODO use a map to save the FMQs/EvenFlags and pass to callback
            getFilterMQDescriptor() == ::testing::AssertionFailure()) {
            return ::testing::AssertionFailure();
        }
        filterIdsSize = mUsedFilterIds.size();
        mUsedFilterIds.resize(filterIdsSize + 1);
        mUsedFilterIds[filterIdsSize] = mFilterId;
        mFilters[mFilterId] = mFilter;
    }

    // Record Config Module
    if (addRecordToDemux(recordSetting) == ::testing::AssertionFailure() ||
        getRecordMQDescriptor() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mDvr->attachFilter(mFilters[mUsedFilterIds[i]]) != Result::SUCCESS) {
            return ::testing::AssertionFailure();
        }
    }

    mDvrCallback->startRecordOutputThread(recordSetting, mRecordMQDescriptor);
    status = mDvr->start();
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    if (createDemuxWithFrontend(feIds[0], settings) != ::testing::AssertionSuccess()) {
        return ::testing::AssertionFailure();
    }

    // Data Verify Module
    mDvrCallback->testRecordOutput();

    // Clean Up Module
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mFilters[mUsedFilterIds[i]]->stop() != Result::SUCCESS) {
            return ::testing::AssertionFailure();
        }
    }
    if (mFrontend->stopTune() != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }
    mUsedFilterIds.clear();
    mFilterCallbacks.clear();
    mFilters.clear();
    return closeDemux();
}

/*
 * API STATUS TESTS
 */
TEST_F(TunerHidlTest, CreateFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Create Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(createFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, TuneFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Tune Frontends and check callback onEvent");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(tuneFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, StopTuneFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("stopTune Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(stopTuneFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, CloseFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Close Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(closeFrontend(feIds[i]));
    }
}

/*TEST_F(TunerHidlTest, CreateDemuxWithFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Create Demux with Frontend");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    FrontendDvbtSettings dvbt{
        .frequency = 1000,
    };
    FrontendSettings settings;
    settings.dvbt(dvbt);

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(createDemuxWithFrontend(feIds[i], settings));
        mFrontend->stopTune();
    }
}*/

TEST_F(TunerHidlTest, CreateDemux) {
    description("Create Demux");
    ASSERT_TRUE(createDemux());
}

TEST_F(TunerHidlTest, CloseDemux) {
    description("Close Demux");
    ASSERT_TRUE(closeDemux());
}

TEST_F(TunerHidlTest, CreateDescrambler) {
    description("Create Descrambler");
    ASSERT_TRUE(createDescrambler());
}

TEST_F(TunerHidlTest, CloseDescrambler) {
    description("Close Descrambler");
    ASSERT_TRUE(closeDescrambler());
}

/*
 * DATA FLOW TESTS
 */
TEST_F(TunerHidlTest, PlaybackDataFlowWithSectionFilterTest) {
    description("Feed ts data from playback and configure pes filter to get output");

    // todo modulize the filter conf parser
    vector<FilterConf> filterConf;
    filterConf.resize(1);

    DemuxFilterSettings filterSetting;
    DemuxTsFilterSettings tsFilterSetting{
            .tpid = 18,
    };
    DemuxFilterSectionSettings sectionFilterSetting;
    tsFilterSetting.filterSettings.section(sectionFilterSetting);
    filterSetting.ts(tsFilterSetting);

    DemuxFilterType type{
            .mainType = DemuxFilterMainType::TS,
    };
    type.subType.tsFilterType(DemuxTsFilterType::SECTION);
    FilterConf sectionFilterConf{
            .type = type,
            .setting = filterSetting,
    };
    filterConf[0] = sectionFilterConf;

    PlaybackSettings playbackSetting{
            .statusMask = 0xf,
            .lowThreshold = 0x1000,
            .highThreshold = 0x07fff,
            .dataFormat = DataFormat::TS,
            .packetSize = 188,
    };

    PlaybackConf playbackConf{
            .inputDataFile = "/vendor/etc/test1.ts",
            .setting = playbackSetting,
    };

    vector<string> goldenOutputFiles;

    ASSERT_TRUE(playbackDataFlowTest(filterConf, playbackConf, goldenOutputFiles));
}

TEST_F(TunerHidlTest, BroadcastDataFlowWithPesFilterTest) {
    description("Feed ts data from frontend and test with PES filter");

    // todo modulize the filter conf parser
    vector<FilterConf> filterConf;
    filterConf.resize(1);

    DemuxFilterSettings filterSetting;
    DemuxTsFilterSettings tsFilterSetting{
            .tpid = 119,
    };
    DemuxFilterPesDataSettings pesFilterSetting;
    tsFilterSetting.filterSettings.pesData(pesFilterSetting);
    filterSetting.ts(tsFilterSetting);

    DemuxFilterType type{
            .mainType = DemuxFilterMainType::TS,
    };
    type.subType.tsFilterType(DemuxTsFilterType::PES);
    FilterConf pesFilterConf{
            .type = type,
            .setting = filterSetting,
    };
    filterConf[0] = pesFilterConf;

    vector<string> goldenOutputFiles;

    ASSERT_TRUE(broadcastDataFlowTest(filterConf, goldenOutputFiles));
}

TEST_F(TunerHidlTest, RecordDataFlowWithTsRecordFilterTest) {
    description("Feed ts data from frontend to recording and test with ts record filter");

    // todo modulize the filter conf parser
    vector<FilterConf> filterConf;
    filterConf.resize(1);

    DemuxFilterSettings filterSetting;
    DemuxTsFilterSettings tsFilterSetting{
            .tpid = 119,
    };
    DemuxFilterRecordSettings recordFilterSetting;
    tsFilterSetting.filterSettings.record(recordFilterSetting);
    filterSetting.ts(tsFilterSetting);

    DemuxFilterType type{
            .mainType = DemuxFilterMainType::TS,
    };
    type.subType.tsFilterType(DemuxTsFilterType::RECORD);
    FilterConf recordFilterConf{
            .type = type,
            .setting = filterSetting,
    };
    filterConf[0] = recordFilterConf;

    RecordSettings recordSetting{
            .statusMask = 0xf,
            .lowThreshold = 0x1000,
            .highThreshold = 0x07fff,
            .dataFormat = DataFormat::TS,
            .packetSize = 188,
    };

    vector<string> goldenOutputFiles;

    ASSERT_TRUE(recordDataFlowTest(filterConf, recordSetting, goldenOutputFiles));
}

}  // namespace

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(TunerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    TunerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
