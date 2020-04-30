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

#include "DvrTests.h"

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

void DvrCallback::startRecordOutputThread(RecordSettings recordSettings,
                                          MQDesc& recordMQDescriptor) {
    mRecordMQ = std::make_unique<FilterMQ>(recordMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mRecordMQ);
    struct RecordThreadArgs* threadArgs =
            (struct RecordThreadArgs*)malloc(sizeof(struct RecordThreadArgs));
    threadArgs->user = this;
    threadArgs->recordSettings = &recordSettings;
    threadArgs->keepReadingRecordFMQ = &mKeepReadingRecordFMQ;

    pthread_create(&mRecordThread, NULL, __threadLoopRecord, (void*)threadArgs);
    pthread_setname_np(mRecordThread, "test_record_input_loop");
}

void* DvrCallback::__threadLoopRecord(void* threadArgs) {
    DvrCallback* const self =
            static_cast<DvrCallback*>(((struct RecordThreadArgs*)threadArgs)->user);
    self->recordThreadLoop(((struct RecordThreadArgs*)threadArgs)->recordSettings,
                           ((struct RecordThreadArgs*)threadArgs)->keepReadingRecordFMQ);
    return 0;
}

void DvrCallback::recordThreadLoop(RecordSettings* /*recordSettings*/, bool* keepReadingRecordFMQ) {
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

AssertionResult DvrTests::openDvrInDemux(DvrType type, uint32_t bufferSize) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";

    // Create dvr callback
    mDvrCallback = new DvrCallback();

    mDemux->openDvr(type, bufferSize, mDvrCallback, [&](Result result, const sp<IDvr>& dvr) {
        mDvr = dvr;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::configDvr(DvrSettings setting) {
    Result status = mDvr->configure(setting);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::getDvrMQDescriptor() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvr) << "Test with openDvr first.";

    mDvr->getQueueDesc([&](Result result, const MQDesc& dvrMQDesc) {
        mDvrMQDescriptor = dvrMQDesc;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::attachFilterToDvr(sp<IFilter> filter) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvr) << "Test with openDvr first.";

    status = mDvr->attachFilter(filter);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::detachFilterToDvr(sp<IFilter> filter) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvr) << "Test with openDvr first.";

    status = mDvr->detachFilter(filter);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::startDvr() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvr) << "Test with openDvr first.";

    status = mDvr->start();

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::stopDvr() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvr) << "Test with openDvr first.";

    status = mDvr->stop();

    return AssertionResult(status == Result::SUCCESS);
}

void DvrTests::closeDvr() {
    ASSERT_TRUE(mDemux);
    ASSERT_TRUE(mDvr);
    ASSERT_TRUE(mDvr->close() == Result::SUCCESS);
}
