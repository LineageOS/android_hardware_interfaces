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
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <fstream>
#include <iostream>
#include <map>

#include "VtsHalTvTunerV1_0TestConfigurations.h"

#define WAIT_TIMEOUT 3000000000

using android::Condition;
using android::IMemory;
using android::IMemoryHeap;
using android::MemoryDealer;
using android::Mutex;
using android::sp;
using android::hardware::EventFlag;
using android::hardware::fromHeap;
using android::hardware::hidl_handle;
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
using android::hardware::tv::tuner::V1_0::FrontendAtscModulation;
using android::hardware::tv::tuner::V1_0::FrontendAtscSettings;
using android::hardware::tv::tuner::V1_0::FrontendDvbtSettings;
using android::hardware::tv::tuner::V1_0::FrontendEventType;
using android::hardware::tv::tuner::V1_0::FrontendId;
using android::hardware::tv::tuner::V1_0::FrontendInfo;
using android::hardware::tv::tuner::V1_0::FrontendInnerFec;
using android::hardware::tv::tuner::V1_0::FrontendScanMessage;
using android::hardware::tv::tuner::V1_0::FrontendScanMessageType;
using android::hardware::tv::tuner::V1_0::FrontendScanType;
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

using ::testing::AssertionResult;

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

/******************************** Start FrontendCallback **********************************/
class FrontendCallback : public IFrontendCallback {
  public:
    virtual Return<void> onEvent(FrontendEventType frontendEventType) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        ALOGD("[vts] frontend event received. Type: %d", frontendEventType);
        mEventReceived = true;
        mMsgCondition.signal();
        switch (frontendEventType) {
            case FrontendEventType::LOCKED:
                mLockMsgReceived = true;
                mLockMsgCondition.signal();
                return Void();
            default:
                // do nothing
                return Void();
        }
    }

    virtual Return<void> onScanMessage(FrontendScanMessageType type,
                                       const FrontendScanMessage& message) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        while (!mScanMsgProcessed) {
            mMsgCondition.wait(mMsgLock);
        }
        ALOGD("[vts] frontend scan message. Type: %d", type);
        mScanMessageReceived = true;
        mScanMsgProcessed = false;
        mScanMessageType = type;
        mScanMessage = message;
        mMsgCondition.signal();
        return Void();
    }

    void tuneTestOnEventReceive(sp<IFrontend>& frontend, FrontendSettings settings);
    void tuneTestOnLock(sp<IFrontend>& frontend, FrontendSettings settings);
    void scanTest(sp<IFrontend>& frontend, FrontendConfig config, FrontendScanType type);

    // Helper methods
    uint32_t getTargetFrequency(FrontendSettings settings, FrontendType type);
    void resetBlindScanStartingFrequency(FrontendConfig config, uint32_t resetingFreq);

  private:
    bool mEventReceived = false;
    bool mScanMessageReceived = false;
    bool mLockMsgReceived = false;
    bool mScanMsgProcessed = true;
    FrontendScanMessageType mScanMessageType;
    FrontendScanMessage mScanMessage;
    hidl_vec<uint8_t> mEventMessage;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
    android::Condition mLockMsgCondition;
};

void FrontendCallback::tuneTestOnEventReceive(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);
    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "Event not received within timeout";
            mLockMsgReceived = false;
            return;
        }
    }
    mEventReceived = false;
}

void FrontendCallback::tuneTestOnLock(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);
    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mLockMsgReceived) {
        if (-ETIMEDOUT == mLockMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "Event LOCKED not received within timeout";
            mLockMsgReceived = false;
            return;
        }
    }
    mLockMsgReceived = false;
}

void FrontendCallback::scanTest(sp<IFrontend>& frontend, FrontendConfig config,
                                FrontendScanType type) {
    uint32_t targetFrequency = getTargetFrequency(config.settings, config.type);
    if (type == FrontendScanType::SCAN_BLIND) {
        // reset the frequency in the scan configuration to test blind scan. The settings param of
        // passed in means the real input config on the transponder connected to the DUT.
        // We want the blind the test to start from lower frequency than this to check the blind
        // scan implementation.
        resetBlindScanStartingFrequency(config, targetFrequency - 100);
    }

    Result result = frontend->scan(config.settings, type);
    EXPECT_TRUE(result == Result::SUCCESS);

    bool scanMsgLockedReceived = false;
    bool targetFrequencyReceived = false;

    android::Mutex::Autolock autoLock(mMsgLock);
wait:
    while (!mScanMessageReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "Scan message not received within timeout";
            mScanMessageReceived = false;
            mScanMsgProcessed = true;
            return;
        }
    }

    if (mScanMessageType != FrontendScanMessageType::END) {
        if (mScanMessageType == FrontendScanMessageType::LOCKED) {
            scanMsgLockedReceived = true;
            Result result = frontend->scan(config.settings, type);
            EXPECT_TRUE(result == Result::SUCCESS);
        }

        if (mScanMessageType == FrontendScanMessageType::FREQUENCY) {
            targetFrequencyReceived = mScanMessage.frequencies().size() > 0 &&
                                      mScanMessage.frequencies()[0] == targetFrequency;
        }

        if (mScanMessageType == FrontendScanMessageType::PROGRESS_PERCENT) {
            ALOGD("[vts] Scan in progress...[%d%%]", mScanMessage.progressPercent());
        }

        mScanMessageReceived = false;
        mScanMsgProcessed = true;
        mMsgCondition.signal();
        goto wait;
    }

    EXPECT_TRUE(scanMsgLockedReceived) << "Scan message LOCKED not received before END";
    EXPECT_TRUE(targetFrequencyReceived) << "frequency not received before LOCKED on blindScan";
    mScanMessageReceived = false;
    mScanMsgProcessed = true;
}

uint32_t FrontendCallback::getTargetFrequency(FrontendSettings settings, FrontendType type) {
    switch (type) {
        case FrontendType::ANALOG:
            return settings.analog().frequency;
        case FrontendType::ATSC:
            return settings.atsc().frequency;
        case FrontendType::ATSC3:
            return settings.atsc3().frequency;
        case FrontendType::DVBC:
            return settings.dvbc().frequency;
        case FrontendType::DVBS:
            return settings.dvbs().frequency;
        case FrontendType::DVBT:
            return settings.dvbt().frequency;
        case FrontendType::ISDBS:
            return settings.isdbs().frequency;
        case FrontendType::ISDBS3:
            return settings.isdbs3().frequency;
        case FrontendType::ISDBT:
            return settings.isdbt().frequency;
        default:
            return 0;
    }
}

void FrontendCallback::resetBlindScanStartingFrequency(FrontendConfig config,
                                                       uint32_t resetingFreq) {
    switch (config.type) {
        case FrontendType::ANALOG:
            config.settings.analog().frequency = resetingFreq;
            break;
        case FrontendType::ATSC:
            config.settings.atsc().frequency = resetingFreq;
            break;
        case FrontendType::ATSC3:
            config.settings.atsc3().frequency = resetingFreq;
            break;
        case FrontendType::DVBC:
            config.settings.dvbc().frequency = resetingFreq;
            break;
        case FrontendType::DVBS:
            config.settings.dvbs().frequency = resetingFreq;
            break;
        case FrontendType::DVBT:
            config.settings.dvbt().frequency = resetingFreq;
            break;
        case FrontendType::ISDBS:
            config.settings.isdbs().frequency = resetingFreq;
            break;
        case FrontendType::ISDBS3:
            config.settings.isdbs3().frequency = resetingFreq;
            break;
        case FrontendType::ISDBT:
            config.settings.isdbt().frequency = resetingFreq;
            break;
        default:
            // do nothing
            return;
    }
}
/******************************** End FrontendCallback **********************************/

/******************************** Start FilterCallback **********************************/
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
    mFilterMQ = std::make_unique<FilterMQ>(filterMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mFilterMQ);
    EXPECT_TRUE(EventFlag::createEventFlag(mFilterMQ->getEventFlagWord(), &mFilterMQEventFlag) ==
                android::OK);
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
    // Read from mFilterMQ[event.filterId] per event and filter type

    // Assemble to filterOutput[filterId]

    // check if filterOutput[filterId] matches goldenOutput[filterId]

    // If match, remove filterId entry from MQ map

    // end thread
}

bool FilterCallback::readFilterEventData() {
    bool result = false;
    DemuxFilterEvent filterEvent = mFilterEvent;
    ALOGW("[vts] reading from filter FMQ or buffer %d", mFilterId);
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
                return dumpAvData(filterEvent.events[i].media());
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
        result = mFilterMQ->read(mDataOutputBuffer.data(), mDataLength);
        EXPECT_TRUE(result) << "can't read from Filter MQ";

        /*for (int i = 0; i < mDataLength; i++) {
            EXPECT_TRUE(goldenDataOutputBuffer[i] == mDataOutputBuffer[i]) << "data does not match";
        }*/
    }
    mFilterMQEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_CONSUMED));
    return result;
}

bool FilterCallback::dumpAvData(DemuxFilterMediaEvent event) {
    uint32_t length = event.dataLength;
    uint64_t dataId = event.avDataId;
    // read data from buffer pointed by a handle
    hidl_handle handle = event.avMemory;

    int av_fd = handle.getNativeHandle()->data[0];
    uint8_t* buffer = static_cast<uint8_t*>(
            mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, av_fd, 0 /*offset*/));
    if (buffer == MAP_FAILED) {
        ALOGE("[vts] fail to allocate av buffer, errno=%d", errno);
        return false;
    }
    uint8_t output[length + 1];
    memcpy(output, buffer, length);
    // print buffer and check with golden output.
    EXPECT_TRUE(mFilter->releaseAvHandle(handle, dataId) == Result::SUCCESS);
    return true;
}
/******************************** End FilterCallback **********************************/

/******************************** Start DvrCallback **********************************/
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
/********************************** End DvrCallback ************************************/

/***************************** Start Test Implementation ******************************/
class TunerHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initFrontendConfig();
        initFrontendScanConfig();
        initFilterConfig();
    }

    sp<ITuner> mService;

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<IFrontend> mFrontend;
    FrontendInfo mFrontendInfo;
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
    MQDesc mDvrMQDescriptor;
    MQDesc mRecordMQDescriptor;
    vector<uint32_t> mUsedFilterIds;
    hidl_vec<FrontendId> mFeIds;

    uint32_t mDemuxId;
    uint32_t mFilterId = -1;

    pthread_t mPlaybackshread;
    bool mPlaybackThreadRunning;

    AssertionResult getFrontendIds();
    AssertionResult getFrontendInfo(uint32_t frontendId);
    AssertionResult openFrontend(uint32_t frontendId);
    AssertionResult setFrontendCallback();
    AssertionResult scanFrontend(FrontendConfig config, FrontendScanType type);
    AssertionResult stopScanFrontend();
    AssertionResult tuneFrontend(FrontendConfig config);
    AssertionResult stopTuneFrontend();
    AssertionResult closeFrontend();

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

    FilterEventType getFilterEventType(DemuxFilterType type);
};

/*========================== Start Frontend APIs Tests Implementation ==========================*/
AssertionResult TunerHidlTest::getFrontendIds() {
    Result status;
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        mFeIds = frontendIds;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::getFrontendInfo(uint32_t frontendId) {
    Result status;
    mService->getFrontendInfo(frontendId, [&](Result result, const FrontendInfo& frontendInfo) {
        mFrontendInfo = frontendInfo;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::openFrontend(uint32_t frontendId) {
    Result status;
    mService->openFrontendById(frontendId, [&](Result result, const sp<IFrontend>& frontend) {
        mFrontend = frontend;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::setFrontendCallback() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontend first.";
    mFrontendCallback = new FrontendCallback();
    auto callbackStatus = mFrontend->setCallback(mFrontendCallback);
    return AssertionResult(callbackStatus.isOk());
}

AssertionResult TunerHidlTest::scanFrontend(FrontendConfig config, FrontendScanType type) {
    EXPECT_TRUE(mFrontendCallback)
            << "test with openFrontend/setFrontendCallback/getFrontendInfo first.";

    EXPECT_TRUE(mFrontendInfo.type == config.type)
            << "FrontendConfig does not match the frontend info of the given id.";

    mFrontendCallback->scanTest(mFrontend, config, type);
    return AssertionResult(true);
}

AssertionResult TunerHidlTest::stopScanFrontend() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontend first.";
    Result status;
    status = mFrontend->stopScan();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::tuneFrontend(FrontendConfig config) {
    EXPECT_TRUE(mFrontendCallback)
            << "test with openFrontend/setFrontendCallback/getFrontendInfo first.";

    EXPECT_TRUE(mFrontendInfo.type == config.type)
            << "FrontendConfig does not match the frontend info of the given id.";

    mFrontendCallback->tuneTestOnLock(mFrontend, config.settings);
    return AssertionResult(true);
}

AssertionResult TunerHidlTest::stopTuneFrontend() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontend first.";
    Result status;
    status = mFrontend->stopTune();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::closeFrontend() {
    EXPECT_TRUE(mFrontend) << "Test with openFrontend first.";
    Result status;
    status = mFrontend->close();
    mFrontend = nullptr;
    mFrontendCallback = nullptr;
    return AssertionResult(status == Result::SUCCESS);
}
/*=========================== End Frontend APIs Tests Implementation ===========================*/

/*============================ Start Demux APIs Tests Implementation ============================*/
AssertionResult TunerHidlTest::openDemux() {
    Result status;
    mService->openDemux([&](Result result, uint32_t demuxId, const sp<IDemux>& demux) {
        mDemux = demux;
        mDemuxId = demuxId;
        status = result;
    });
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::setDemuxFrontendDataSource(uint32_t frontendId) {
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mFrontend) << "Test with openFrontend first.";
    auto status = mDemux->setFrontendDataSource(frontendId);
    return AssertionResult(status.isOk());
}

AssertionResult TunerHidlTest::closeDemux() {
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    auto status = mDemux->close();
    mDemux = nullptr;
    return AssertionResult(status.isOk());
}

AssertionResult TunerHidlTest::openFilterInDemux(DemuxFilterType type) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";

    // Create demux callback
    mFilterCallback = new FilterCallback();

    // Add filter to the local demux
    mDemux->openFilter(type, FMQ_SIZE_16M, mFilterCallback,
                       [&](Result result, const sp<IFilter>& filter) {
                           mFilter = filter;
                           status = result;
                       });

    if (status == Result::SUCCESS) {
        mFilterCallback->setFilterEventType(getFilterEventType(type));
    }

    return AssertionResult(status == Result::SUCCESS);
}
/*============================ End Demux APIs Tests Implementation ============================*/

/*=========================== Start Filter APIs Tests Implementation ===========================*/
AssertionResult TunerHidlTest::getNewlyOpenedFilterId(uint32_t& filterId) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mFilter) << "Test with openFilterInDemux first.";
    EXPECT_TRUE(mFilterCallback) << "Test with openFilterInDemux first.";

    mFilter->getId([&](Result result, uint32_t filterId) {
        mFilterId = filterId;
        status = result;
    });

    if (status == Result::SUCCESS) {
        mFilterCallback->setFilterId(mFilterId);
        mFilterCallback->setFilterInterface(mFilter);
        mUsedFilterIds.insert(mUsedFilterIds.end(), mFilterId);
        mFilters[mFilterId] = mFilter;
        mFilterCallbacks[mFilterId] = mFilterCallback;
        filterId = mFilterId;
    }

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::configFilter(DemuxFilterSettings setting, uint32_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    status = mFilters[filterId]->configure(setting);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::getFilterMQDescriptor(uint32_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    EXPECT_TRUE(mFilterCallbacks[filterId]) << "Test with getNewlyOpenedFilterId first.";

    mFilter->getQueueDesc([&](Result result, const MQDesc& filterMQDesc) {
        mFilterMQDescriptor = filterMQDesc;
        status = result;
    });

    if (status == Result::SUCCESS) {
        mFilterCallbacks[filterId]->updateFilterMQ(mFilterMQDescriptor);
    }

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::startFilter(uint32_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->start();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::stopFilter(uint32_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->stop();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::closeFilter(uint32_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->close();
    if (status == Result::SUCCESS) {
        for (int i = 0; i < mUsedFilterIds.size(); i++) {
            if (mUsedFilterIds[i] == filterId) {
                mUsedFilterIds.erase(mUsedFilterIds.begin() + i);
                break;
            }
        }
        mFilterCallbacks.erase(filterId);
        mFilters.erase(filterId);
    }
    return AssertionResult(status == Result::SUCCESS);
}
/*=========================== End Filter APIs Tests Implementation ===========================*/

/*======================== Start Descrambler APIs Tests Implementation ========================*/
AssertionResult TunerHidlTest::createDescrambler() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    mService->openDescrambler([&](Result result, const sp<IDescrambler>& descrambler) {
        mDescrambler = descrambler;
        status = result;
    });
    if (status != Result::SUCCESS) {
        return failure();
    }

    status = mDescrambler->setDemuxSource(mDemuxId);
    if (status != Result::SUCCESS) {
        return failure();
    }

    // Test if demux source can be set more than once.
    status = mDescrambler->setDemuxSource(mDemuxId);
    return AssertionResult(status == Result::INVALID_STATE);
}

AssertionResult TunerHidlTest::closeDescrambler() {
    Result status;
    if (!mDescrambler && createDescrambler() == failure()) {
        return failure();
    }

    status = mDescrambler->close();
    mDescrambler = nullptr;
    return AssertionResult(status == Result::SUCCESS);
}
/*========================= End Descrambler APIs Tests Implementation =========================*/

/*============================ Start Dvr APIs Tests Implementation ============================*/
AssertionResult TunerHidlTest::openDvrInDemux(DvrType type) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";

    // Create dvr callback
    mDvrCallback = new DvrCallback();

    mDemux->openDvr(type, FMQ_SIZE_1M, mDvrCallback, [&](Result result, const sp<IDvr>& dvr) {
        mDvr = dvr;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::configDvr(DvrSettings setting) {
    Result status = mDvr->configure(setting);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult TunerHidlTest::getDvrMQDescriptor() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvr) << "Test with openDvr first.";

    mDvr->getQueueDesc([&](Result result, const MQDesc& dvrMQDesc) {
        mDvrMQDescriptor = dvrMQDesc;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}
/*============================ End Dvr APIs Tests Implementation ============================*/

/*========================== Start Data Flow Tests Implementation ==========================*/
AssertionResult TunerHidlTest::broadcastDataFlowTest(vector<string> /*goldenOutputFiles*/) {
    EXPECT_TRUE(mFrontend) << "Test with openFilterInDemux first.";
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mFilterCallback) << "Test with getFilterMQDescriptor first.";

    // Data Verify Module
    std::map<uint32_t, sp<FilterCallback>>::iterator it;
    for (it = mFilterCallbacks.begin(); it != mFilterCallbacks.end(); it++) {
        it->second->testFilterDataOutput();
    }
    return success();
}

/*
 * TODO: re-enable the tests after finalizing the test refactoring.
 */
/*AssertionResult TunerHidlTest::playbackDataFlowTest(
        vector<FilterConf> filterConf, PlaybackConf playbackConf,
        vector<string> \/\*goldenOutputFiles\*\/) {
    Result status;
    int filterIdsSize;
    // Filter Configuration Module
    for (int i = 0; i < filterConf.size(); i++) {
        if (addFilterToDemux(filterConf[i].type, filterConf[i].setting) ==
                    failure() ||
            // TODO use a map to save the FMQs/EvenFlags and pass to callback
            getFilterMQDescriptor() == failure()) {
            return failure();
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
            return failure();
        }
    }

    // Playback Input Module
    PlaybackSettings playbackSetting = playbackConf.setting;
    if (addPlaybackToDemux(playbackSetting) == failure() ||
        getPlaybackMQDescriptor() == failure()) {
        return failure();
    }
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mDvr->attachFilter(mFilters[mUsedFilterIds[i]]) != Result::SUCCESS) {
            return failure();
        }
    }
    mDvrCallback->startPlaybackInputThread(playbackConf, mPlaybackMQDescriptor);
    status = mDvr->start();
    if (status != Result::SUCCESS) {
        return failure();
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
            return failure();
        }
    }
    if (mDvr->stop() != Result::SUCCESS) {
        return failure();
    }
    mUsedFilterIds.clear();
    mFilterCallbacks.clear();
    mFilters.clear();
    return closeDemux();
}

AssertionResult TunerHidlTest::recordDataFlowTest(vector<FilterConf> filterConf,
                                                  RecordSettings recordSetting,
                                                  vector<string> goldenOutputFiles) {
    Result status;
    hidl_vec<FrontendId> feIds;

    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return failure();
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
                    failure() ||
            // TODO use a map to save the FMQs/EvenFlags and pass to callback
            getFilterMQDescriptor() == failure()) {
            return failure();
        }
        filterIdsSize = mUsedFilterIds.size();
        mUsedFilterIds.resize(filterIdsSize + 1);
        mUsedFilterIds[filterIdsSize] = mFilterId;
        mFilters[mFilterId] = mFilter;
    }

    // Record Config Module
    if (addRecordToDemux(recordSetting) == failure() ||
        getRecordMQDescriptor() == failure()) {
        return failure();
    }
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mDvr->attachFilter(mFilters[mUsedFilterIds[i]]) != Result::SUCCESS) {
            return failure();
        }
    }

    mDvrCallback->startRecordOutputThread(recordSetting, mRecordMQDescriptor);
    status = mDvr->start();
    if (status != Result::SUCCESS) {
        return failure();
    }

    if (setDemuxFrontendDataSource(feIds[0]) != success()) {
        return failure();
    }

    // Data Verify Module
    mDvrCallback->testRecordOutput();

    // Clean Up Module
    for (int i = 0; i <= filterIdsSize; i++) {
        if (mFilters[mUsedFilterIds[i]]->stop() != Result::SUCCESS) {
            return failure();
        }
    }
    if (mFrontend->stopTune() != Result::SUCCESS) {
        return failure();
    }
    mUsedFilterIds.clear();
    mFilterCallbacks.clear();
    mFilters.clear();
    return closeDemux();
}*/
/*========================= End Data Flow Tests Implementation =========================*/

/*=============================== Start Helper Functions ===============================*/
FilterEventType TunerHidlTest::getFilterEventType(DemuxFilterType type) {
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
/*============================== End Helper Functions ==============================*/
/***************************** End Test Implementation *****************************/

/******************************** Start Test Entry **********************************/
/*============================== Start Frontend Tests ==============================*/
TEST_P(TunerHidlTest, getFrontendIds) {
    description("Get Frontend ids and verify frontends exist");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);
}

TEST_P(TunerHidlTest, openFrontend) {
    description("Open all the existing Frontends and close them");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);

    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(closeFrontend());
    }
}

TEST_P(TunerHidlTest, TuneFrontend) {
    description("Tune one Frontend with specific setting and check Lock event");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);
    ALOGW("[vts] expected Frontend type is %d", frontendArray[0].type);
    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        ALOGW("[vts] Frontend type is %d", mFrontendInfo.type);
        if (mFrontendInfo.type != frontendArray[0].type) {
            continue;
        }
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(setFrontendCallback());
        ASSERT_TRUE(tuneFrontend(frontendArray[0]));
        ASSERT_TRUE(stopTuneFrontend());
        ASSERT_TRUE(closeFrontend());
        break;
    }
}

TEST_P(TunerHidlTest, AutoScanFrontend) {
    description("Run an auto frontend scan with specific setting and check lock scanMessage");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);

    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != frontendArray[0].type) {
            continue;
        }
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(setFrontendCallback());
        ASSERT_TRUE(scanFrontend(frontendScanArray[0], FrontendScanType::SCAN_AUTO));
        ASSERT_TRUE(stopScanFrontend());
        ASSERT_TRUE(closeFrontend());
        break;
    }
}

TEST_P(TunerHidlTest, BlindScanFrontend) {
    description("Run an blind frontend scan with specific setting and check lock scanMessage");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);

    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != frontendArray[0].type) {
            continue;
        }
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(setFrontendCallback());
        ASSERT_TRUE(scanFrontend(frontendScanArray[0], FrontendScanType::SCAN_BLIND));
        ASSERT_TRUE(stopScanFrontend());
        ASSERT_TRUE(closeFrontend());
        break;
    }
}
/*=============================== End Frontend Tests ===============================*/

/*============================ Start Demux/Filter Tests ============================*/
TEST_P(TunerHidlTest, OpenDemuxWithFrontendDataSource) {
    description("Open Demux with a Frontend as its data source.");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);

    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != frontendArray[0].type) {
            continue;
        }
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(setFrontendCallback());
        ASSERT_TRUE(openDemux());
        ASSERT_TRUE(setDemuxFrontendDataSource(mFeIds[i]));
        ASSERT_TRUE(closeDemux());
        ASSERT_TRUE(closeFrontend());
        break;
    }
}

TEST_P(TunerHidlTest, OpenFilterInDemux) {
    description("Open a filter in Demux.");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);

    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != frontendArray[0].type) {
            continue;
        }
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(setFrontendCallback());
        ASSERT_TRUE(openDemux());
        ASSERT_TRUE(setDemuxFrontendDataSource(mFeIds[i]));
        ASSERT_TRUE(openFilterInDemux(filterArray[0].type));
        uint32_t filterId;
        ASSERT_TRUE(getNewlyOpenedFilterId(filterId));
        ASSERT_TRUE(closeFilter(filterId));
        ASSERT_TRUE(closeDemux());
        ASSERT_TRUE(closeFrontend());
        break;
    }
}

TEST_P(TunerHidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);

    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != frontendArray[0].type) {
            continue;
        }
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(setFrontendCallback());
        ASSERT_TRUE(openDemux());
        ASSERT_TRUE(setDemuxFrontendDataSource(mFeIds[i]));
        ASSERT_TRUE(openFilterInDemux(filterArray[0].type));
        uint32_t filterId;
        ASSERT_TRUE(getNewlyOpenedFilterId(filterId));
        ASSERT_TRUE(configFilter(filterArray[0].setting, filterId));
        ASSERT_TRUE(getFilterMQDescriptor(filterId));
        ASSERT_TRUE(startFilter(filterId));
        ASSERT_TRUE(stopFilter(filterId));
        ASSERT_TRUE(closeFilter(filterId));
        ASSERT_TRUE(closeDemux());
        ASSERT_TRUE(closeFrontend());
        break;
    }
}
/*============================ End Demux/Filter Tests ============================*/

/*============================ Start Descrambler Tests ============================*/
/*
 * TODO: re-enable the tests after finalizing the test refactoring.
 */
/*TEST_P(TunerHidlTest, CreateDescrambler) {
    description("Create Descrambler");
    ASSERT_TRUE(createDescrambler());
}

TEST_P(TunerHidlTest, CloseDescrambler) {
    description("Close Descrambler");
    ASSERT_TRUE(closeDescrambler());
}*/
/*============================== End Descrambler Tests ==============================*/

/*============================== Start Data Flow Tests ==============================*/
TEST_P(TunerHidlTest, BroadcastDataFlowWithAudioFilterTest) {
    description("Open Demux with a Frontend as its data source.");
    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);

    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != frontendArray[0].type) {
            continue;
        }
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(setFrontendCallback());
        ASSERT_TRUE(openDemux());
        ASSERT_TRUE(setDemuxFrontendDataSource(mFeIds[i]));
        ASSERT_TRUE(openFilterInDemux(filterArray[0].type));
        uint32_t filterId;
        ASSERT_TRUE(getNewlyOpenedFilterId(filterId));
        ASSERT_TRUE(configFilter(filterArray[0].setting, filterId));
        ASSERT_TRUE(getFilterMQDescriptor(filterId));
        ASSERT_TRUE(startFilter(filterId));
        // tune test
        ASSERT_TRUE(tuneFrontend(frontendArray[0]));
        // broadcast data flow test
        ASSERT_TRUE(broadcastDataFlowTest(goldenOutputFiles));
        ASSERT_TRUE(stopTuneFrontend());
        ASSERT_TRUE(stopFilter(filterId));
        ASSERT_TRUE(closeFilter(filterId));
        ASSERT_TRUE(closeDemux());
        ASSERT_TRUE(closeFrontend());
        break;
    }
}

/*
 * TODO: re-enable the tests after finalizing the testing stream.
 */
/*TEST_P(TunerHidlTest, PlaybackDataFlowWithSectionFilterTest) {
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

TEST_P(TunerHidlTest, RecordDataFlowWithTsRecordFilterTest) {
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
}*/

TEST_P(TunerHidlTest, AvBufferTest) {
    description("Test the av filter data bufferring.");

    ASSERT_TRUE(getFrontendIds());
    ASSERT_TRUE(mFeIds.size() > 0);

    for (size_t i = 0; i < mFeIds.size(); i++) {
        ASSERT_TRUE(getFrontendInfo(mFeIds[i]));
        if (mFrontendInfo.type != frontendArray[1].type) {
            continue;
        }
        ASSERT_TRUE(openFrontend(mFeIds[i]));
        ASSERT_TRUE(setFrontendCallback());
        ASSERT_TRUE(openDemux());
        ASSERT_TRUE(openFilterInDemux(filterArray[0].type));
        uint32_t filterId;
        ASSERT_TRUE(getNewlyOpenedFilterId(filterId));
        ASSERT_TRUE(configFilter(filterArray[0].setting, filterId));
        ASSERT_TRUE(startFilter(filterId));
        ASSERT_TRUE(setDemuxFrontendDataSource(mFeIds[i]));
        // tune test
        ASSERT_TRUE(tuneFrontend(frontendArray[1]));
        // broadcast data flow test
        ASSERT_TRUE(broadcastDataFlowTest(goldenOutputFiles));
        ASSERT_TRUE(stopTuneFrontend());
        ASSERT_TRUE(stopFilter(filterId));
        ASSERT_TRUE(closeFilter(filterId));
        ASSERT_TRUE(closeDemux());
        ASSERT_TRUE(closeFrontend());
        break;
    }
}
/*============================== End Data Flow Tests ==============================*/
/******************************** End Test Entry **********************************/
}  // namespace

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);
