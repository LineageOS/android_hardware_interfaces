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

#include <fcntl.h>
#include <fmq/AidlMessageQueue.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <atomic>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>

#include <aidl/android/hardware/tv/tuner/BnDvrCallback.h>
#include <aidl/android/hardware/tv/tuner/IDvr.h>
#include <aidl/android/hardware/tv/tuner/ITuner.h>

#include "FilterTests.h"

using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::AidlMessageQueue;
using ::android::Condition;
using ::android::Mutex;
using ::android::hardware::EventFlag;

using namespace aidl::android::hardware::tv::tuner;
using namespace std;

#define WAIT_TIMEOUT 3000000000

class DvrCallback : public BnDvrCallback {
  public:
    virtual ::ndk::ScopedAStatus onRecordStatus(RecordStatus status) override {
        ALOGD("[vts] record status %hhu", status);
        switch (status) {
            case RecordStatus::DATA_READY:
                break;
            case RecordStatus::LOW_WATER:
                break;
            case RecordStatus::HIGH_WATER:
            case RecordStatus::OVERFLOW:
                ALOGD("[vts] record overflow. Flushing.");
                EXPECT_TRUE(mDvr) << "Dvr callback is not set with an IDvr";
                if (mDvr) {
                    ndk::ScopedAStatus result = mDvr->flush();
                    ALOGD("[vts] Flushing result %s.", result.getMessage());
                }
                break;
        }
        return ndk::ScopedAStatus::ok();
    }

    virtual ::ndk::ScopedAStatus onPlaybackStatus(PlaybackStatus status) override {
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
        return ndk::ScopedAStatus::ok();
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
    void recordThreadLoop();

    bool readRecordFMQ();

    void setDvr(std::shared_ptr<IDvr> dvr) { mDvr = dvr; }

  private:
    struct RecordThreadArgs {
        DvrCallback* user;
        RecordSettings* recordSettings;
        bool* keepReadingRecordFMQ;
    };
    // uint16_t mDataLength = 0;
    std::vector<int8_t> mDataOutputBuffer;

    std::map<uint32_t, std::unique_ptr<FilterMQ>> mFilterMQ;
    std::unique_ptr<FilterMQ> mPlaybackMQ;
    std::unique_ptr<FilterMQ> mRecordMQ;
    std::map<uint32_t, EventFlag*> mFilterMQEventFlag;

    android::Mutex mMsgLock;
    android::Condition mMsgCondition;

    std::atomic<bool> mKeepWritingPlaybackFMQ = true;
    std::atomic<bool> mKeepReadingRecordFMQ = true;
    std::atomic<bool> mPlaybackThreadRunning;
    std::atomic<bool> mRecordThreadRunning;
    std::thread mPlaybackThread;
    std::thread mRecordThread;
    string mInputDataFile;
    PlaybackSettings mPlaybackSettings;

    std::shared_ptr<IDvr> mDvr = nullptr;

    // int mPidFilterOutputCount = 0;
};

class DvrTests {
  public:
    void setService(std::shared_ptr<ITuner> tuner) { mService = tuner; }
    void setDemux(std::shared_ptr<IDemux> demux) { mDemux = demux; }

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

    AssertionResult openDvrInDemux(DvrType type, int32_t bufferSize);
    AssertionResult configDvrPlayback(DvrSettings setting);
    AssertionResult configDvrRecord(DvrSettings setting);
    AssertionResult getDvrPlaybackMQDescriptor();
    AssertionResult getDvrRecordMQDescriptor();
    AssertionResult attachFilterToDvr(std::shared_ptr<IFilter> filter);
    AssertionResult detachFilterToDvr(std::shared_ptr<IFilter> filter);
    AssertionResult stopDvrPlayback();
    AssertionResult startDvrPlayback();
    AssertionResult stopDvrRecord();
    AssertionResult startDvrRecord();
    AssertionResult setPlaybackStatusCheckIntervalHint(int64_t milliseconds);
    AssertionResult setRecordStatusCheckIntervalHint(int64_t milliseconds);
    void closeDvrPlayback();
    void closeDvrRecord();
    int32_t getDvrPlaybackInterfaceVersion();
    int32_t getDvrRecordInterfaceVersion();

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    std::shared_ptr<ITuner> mService;
    std::shared_ptr<IDvr> mDvrPlayback;
    std::shared_ptr<IDvr> mDvrRecord;
    std::shared_ptr<IDemux> mDemux;
    std::shared_ptr<DvrCallback> mDvrPlaybackCallback;
    std::shared_ptr<DvrCallback> mDvrRecordCallback;
    MQDesc mDvrPlaybackMQDescriptor;
    MQDesc mDvrRecordMQDescriptor;
};
