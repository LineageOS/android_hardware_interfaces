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
#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <android/hardware/tv/tuner/1.0/IDvrCallback.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <android/hardware/tv/tuner/1.1/ITuner.h>
#include <fcntl.h>
#include <fmq/MessageQueue.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
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
using android::hardware::tv::tuner::V1_0::DemuxQueueNotifyBits;
using android::hardware::tv::tuner::V1_0::DvrSettings;
using android::hardware::tv::tuner::V1_0::DvrType;
using android::hardware::tv::tuner::V1_0::IDvr;
using android::hardware::tv::tuner::V1_0::IDvrCallback;
using android::hardware::tv::tuner::V1_0::PlaybackSettings;
using android::hardware::tv::tuner::V1_0::PlaybackStatus;
using android::hardware::tv::tuner::V1_0::RecordSettings;
using android::hardware::tv::tuner::V1_0::RecordStatus;
using android::hardware::tv::tuner::V1_0::Result;
using android::hardware::tv::tuner::V1_1::ITuner;

using namespace std;

#define WAIT_TIMEOUT 3000000000

class DvrCallback : public IDvrCallback {
  public:
    virtual Return<void> onRecordStatus(DemuxFilterStatus status) override {
        ALOGD("[vts] record status %hhu", status);
        switch (status) {
            case DemuxFilterStatus::DATA_READY:
                break;
            case DemuxFilterStatus::LOW_WATER:
                break;
            case DemuxFilterStatus::HIGH_WATER:
            case DemuxFilterStatus::OVERFLOW:
                ALOGD("[vts] record overflow. Flushing.");
                EXPECT_TRUE(mDvr) << "Dvr callback is not set with an IDvr";
                if (mDvr) {
                    Result result = mDvr->flush();
                    ALOGD("[vts] Flushing result %d.", result);
                }
                break;
        }
        return Void();
    }

    virtual Return<void> onPlaybackStatus(PlaybackStatus status) override {
        // android::Mutex::Autolock autoLock(mMsgLock);
        ALOGD("[vts] playback status %d", status);
        switch (status) {
            case PlaybackStatus::SPACE_EMPTY:
            case PlaybackStatus::SPACE_ALMOST_EMPTY:
                ALOGD("[vts] keep playback inputing %d", status);
                mKeepWritingPlaybackFMQ = true;
                break;
            case PlaybackStatus::SPACE_ALMOST_FULL:
            case PlaybackStatus::SPACE_FULL:
                ALOGD("[vts] stop playback inputing %d", status);
                mKeepWritingPlaybackFMQ = false;
                break;
        }
        return Void();
    }

    void stopPlaybackThread();
    void testRecordOutput();
    void stopRecordThread();

    void startPlaybackInputThread(string& dataInputFile, PlaybackSettings& settings,
                                  MQDesc& playbackMQDescriptor);
    void startRecordOutputThread(RecordSettings recordSettings, MQDesc& recordMQDescriptor);
    static void* __threadLoopPlayback(void* user);
    static void* __threadLoopRecord(void* threadArgs);
    void playbackThreadLoop();
    void recordThreadLoop(RecordSettings* recordSetting, bool* keepWritingPlaybackFMQ);

    bool readRecordFMQ();

    void setDvr(sp<IDvr> dvr) { mDvr = dvr; }

  private:
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
    string mInputDataFile;
    PlaybackSettings mPlaybackSettings;

    sp<IDvr> mDvr = nullptr;

    // int mPidFilterOutputCount = 0;
};

class DvrTests {
  public:
    void setService(sp<ITuner> tuner) { mService = tuner; }
    void setDemux(sp<IDemux> demux) { mDemux = demux; }

    void startPlaybackInputThread(string& dataInputFile, PlaybackSettings& settings) {
        mDvrPlaybackCallback->startPlaybackInputThread(dataInputFile, settings,
                                                       mDvrPlaybackMQDescriptor);
    };

    void startRecordOutputThread(RecordSettings settings) {
        mDvrRecordCallback->startRecordOutputThread(settings, mDvrRecordMQDescriptor);
    };

    void stopPlaybackThread() { mDvrPlaybackCallback->stopPlaybackThread(); }
    void testRecordOutput() { mDvrRecordCallback->testRecordOutput(); }
    void stopRecordThread() { mDvrRecordCallback->stopRecordThread(); }

    AssertionResult openDvrInDemux(DvrType type, uint32_t bufferSize);
    AssertionResult configDvrPlayback(DvrSettings setting);
    AssertionResult configDvrRecord(DvrSettings setting);
    AssertionResult getDvrPlaybackMQDescriptor();
    AssertionResult getDvrRecordMQDescriptor();
    AssertionResult attachFilterToDvr(sp<IFilter> filter);
    AssertionResult detachFilterToDvr(sp<IFilter> filter);
    AssertionResult stopDvrPlayback();
    AssertionResult startDvrPlayback();
    AssertionResult stopDvrRecord();
    AssertionResult startDvrRecord();
    void closeDvrPlayback();
    void closeDvrRecord();

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    sp<ITuner> mService;
    sp<IDvr> mDvrPlayback;
    sp<IDvr> mDvrRecord;
    sp<IDemux> mDemux;
    sp<DvrCallback> mDvrPlaybackCallback;
    sp<DvrCallback> mDvrRecordCallback;
    MQDesc mDvrPlaybackMQDescriptor;
    MQDesc mDvrRecordMQDescriptor;
};
