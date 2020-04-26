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

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <android/hardware/tv/tuner/1.0/IDvrCallback.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/Status.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <fstream>
#include <iostream>
#include <map>

#include "FilterTests.h"

using android::Condition;
using android::Mutex;
using android::sp;
using android::hardware::EventFlag;
using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tv::tuner::V1_0::DemuxFilterStatus;
using android::hardware::tv::tuner::V1_0::DvrSettings;
using android::hardware::tv::tuner::V1_0::DvrType;
using android::hardware::tv::tuner::V1_0::IDvr;
using android::hardware::tv::tuner::V1_0::IDvrCallback;
using android::hardware::tv::tuner::V1_0::ITuner;
using android::hardware::tv::tuner::V1_0::PlaybackSettings;
using android::hardware::tv::tuner::V1_0::PlaybackStatus;
using android::hardware::tv::tuner::V1_0::RecordSettings;
using android::hardware::tv::tuner::V1_0::RecordStatus;
using android::hardware::tv::tuner::V1_0::Result;

#define WAIT_TIMEOUT 3000000000

struct PlaybackConf {
    string inputDataFile;
    PlaybackSettings setting;
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

    void stopPlaybackThread();
    void testRecordOutput();
    void stopRecordThread();

    void startPlaybackInputThread(PlaybackConf playbackConf, MQDesc& playbackMQDescriptor);
    void startRecordOutputThread(RecordSettings recordSettings, MQDesc& recordMQDescriptor);
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
        RecordSettings* recordSettings;
        bool* keepReadingRecordFMQ;
    };
    // uint16_t mDataLength = 0;
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

    // int mPidFilterOutputCount = 0;
};

class DvrTests {
  public:
    void setService(sp<ITuner> tuner) { mService = tuner; }
    void setDemux(sp<IDemux> demux) { mDemux = demux; }

    void startPlaybackInputThread(string dataInputFile, PlaybackSettings settings) {
        PlaybackConf conf{
                .inputDataFile = dataInputFile,
                .setting = settings,
        };
        mDvrCallback->startPlaybackInputThread(conf, mDvrMQDescriptor);
    };

    void startRecordOutputThread(RecordSettings settings) {
        mDvrCallback->startRecordOutputThread(settings, mDvrMQDescriptor);
    };

    void stopPlaybackThread() { mDvrCallback->stopPlaybackThread(); }
    void testRecordOutput() { mDvrCallback->testRecordOutput(); }
    void stopRecordThread() { mDvrCallback->stopPlaybackThread(); }

    AssertionResult openDvrInDemux(DvrType type);
    AssertionResult configDvr(DvrSettings setting);
    AssertionResult getDvrMQDescriptor();
    AssertionResult attachFilterToDvr(sp<IFilter> filter);
    AssertionResult detachFilterToDvr(sp<IFilter> filter);
    AssertionResult stopDvr();
    AssertionResult startDvr();
    void closeDvr();

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    sp<ITuner> mService;
    sp<IDvr> mDvr;
    sp<IDemux> mDemux;
    sp<DvrCallback> mDvrCallback;
    MQDesc mDvrMQDescriptor;

    pthread_t mPlaybackshread;
    bool mPlaybackThreadRunning;
};