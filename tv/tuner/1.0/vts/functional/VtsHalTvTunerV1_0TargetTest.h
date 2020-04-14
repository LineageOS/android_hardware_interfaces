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

#include <android/hardware/tv/tuner/1.0/IDescrambler.h>
#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <android/hardware/tv/tuner/1.0/IDvrCallback.h>
#include <fstream>
#include <iostream>

#include "DemuxTests.h"
#include "FilterTests.h"
#include "FrontendTests.h"

using android::hardware::tv::tuner::V1_0::DataFormat;
using android::hardware::tv::tuner::V1_0::DvrSettings;
using android::hardware::tv::tuner::V1_0::DvrType;
using android::hardware::tv::tuner::V1_0::IDescrambler;
using android::hardware::tv::tuner::V1_0::IDvr;
using android::hardware::tv::tuner::V1_0::IDvrCallback;
using android::hardware::tv::tuner::V1_0::PlaybackSettings;
using android::hardware::tv::tuner::V1_0::PlaybackStatus;
using android::hardware::tv::tuner::V1_0::RecordSettings;
using android::hardware::tv::tuner::V1_0::RecordStatus;

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

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
};

class TunerDemuxHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initFrontendConfig();
        initFrontendScanConfig();
        initFilterConfig();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    sp<IDemux> mDemux;
    uint32_t mDemuxId;
};

class TunerFilterHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initFrontendConfig();
        initFrontendScanConfig();
        initFilterConfig();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;
    sp<IDemux> mDemux;
    uint32_t mDemuxId;
};

class TunerHidlTest : public testing::TestWithParam<std::string> {
  public:
    sp<ITuner> mService;
    FrontendTests mFrontendTests;
    DemuxTests mDemuxTests;
    FilterTests mFilterTests;

    virtual void SetUp() override {
        mService = ITuner::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        initFrontendConfig();
        initFrontendScanConfig();
        initFilterConfig();

        mFrontendTests.setService(mService);
        mDemuxTests.setService(mService);
        mFilterTests.setService(mService);
    }

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<IDescrambler> mDescrambler;
    sp<IDvr> mDvr;
    sp<IDemux> mDemux;
    uint32_t mDemuxId;

    sp<DvrCallback> mDvrCallback;
    MQDesc mDvrMQDescriptor;
    MQDesc mRecordMQDescriptor;

    pthread_t mPlaybackshread;
    bool mPlaybackThreadRunning;

    AssertionResult openDvrInDemux(DvrType type);
    AssertionResult configDvr(DvrSettings setting);
    AssertionResult getDvrMQDescriptor();

    AssertionResult createDescrambler();
    AssertionResult closeDescrambler();

    AssertionResult playbackDataFlowTest(vector<FilterConfig> filterConf, PlaybackConf playbackConf,
                                         vector<string> goldenOutputFiles);
    AssertionResult recordDataFlowTest(vector<FilterConfig> filterConf,
                                       RecordSettings recordSetting,
                                       vector<string> goldenOutputFiles);
    AssertionResult broadcastDataFlowTest(vector<string> goldenOutputFiles);

    void broadcastSingleFilterTest(FilterConfig filterConf, FrontendConfig frontendConf);
};
}  // namespace