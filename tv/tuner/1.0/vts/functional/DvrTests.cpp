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

void DvrCallback::startPlaybackInputThread(string& dataInputFile, PlaybackSettings& settings,
                                           MQDesc& playbackMQDescriptor) {
    mInputDataFile = dataInputFile;
    mPlaybackSettings = settings;
    mPlaybackMQ = std::make_unique<FilterMQ>(playbackMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mPlaybackMQ);
    pthread_create(&mPlaybackThread, NULL, __threadLoopPlayback, this);
    pthread_setname_np(mPlaybackThread, "test_playback_input_loop");
}

void DvrCallback::stopPlaybackThread() {
    mPlaybackThreadRunning = false;
    mKeepWritingPlaybackFMQ = false;

    android::Mutex::Autolock autoLock(mPlaybackThreadLock);
}

void* DvrCallback::__threadLoopPlayback(void* user) {
    DvrCallback* const self = static_cast<DvrCallback*>(user);
    self->playbackThreadLoop();
    return 0;
}

void DvrCallback::playbackThreadLoop() {
    android::Mutex::Autolock autoLock(mPlaybackThreadLock);
    mPlaybackThreadRunning = true;

    // Create the EventFlag that is used to signal the HAL impl that data have been
    // written into the Playback FMQ
    EventFlag* playbackMQEventFlag;
    EXPECT_TRUE(EventFlag::createEventFlag(mPlaybackMQ->getEventFlagWord(), &playbackMQEventFlag) ==
                android::OK);

    int fd = open(mInputDataFile.c_str(), O_RDONLY | O_LARGEFILE);
    int readBytes;
    uint32_t regionSize = 0;
    uint8_t* buffer;
    ALOGW("[vts] playback thread loop start %s", mInputDataFile.c_str());
    if (fd < 0) {
        EXPECT_TRUE(fd >= 0) << "Failed to open: " + mInputDataFile;
        mPlaybackThreadRunning = false;
        ALOGW("[vts] Error %s", strerror(errno));
    }

    while (mPlaybackThreadRunning) {
        while (mKeepWritingPlaybackFMQ) {
            int totalWrite = mPlaybackMQ->availableToWrite();
            if (totalWrite * 4 < mPlaybackMQ->getQuantumCount()) {
                // Wait for the HAL implementation to read more data then write.
                continue;
            }
            MessageQueue<uint8_t, kSynchronizedReadWrite>::MemTransaction memTx;
            if (!mPlaybackMQ->beginWrite(totalWrite, &memTx)) {
                ALOGW("[vts] Fail to write into Playback fmq.");
                mPlaybackThreadRunning = false;
                break;
            }
            auto first = memTx.getFirstRegion();
            buffer = first.getAddress();
            regionSize = first.getLength();

            if (regionSize > 0) {
                readBytes = read(fd, buffer, regionSize);
                if (readBytes <= 0) {
                    if (readBytes < 0) {
                        ALOGW("[vts] Read from %s failed.", mInputDataFile.c_str());
                    } else {
                        ALOGW("[vts] playback input EOF.");
                    }
                    mPlaybackThreadRunning = false;
                    break;
                }
            }
            if (regionSize == 0 || (readBytes == regionSize && regionSize < totalWrite)) {
                auto second = memTx.getSecondRegion();
                buffer = second.getAddress();
                regionSize = second.getLength();
                int ret = read(fd, buffer, regionSize);
                if (ret <= 0) {
                    if (ret < 0) {
                        ALOGW("[vts] Read from %s failed.", mInputDataFile.c_str());
                    } else {
                        ALOGW("[vts] playback input EOF.");
                    }
                    mPlaybackThreadRunning = false;
                    break;
                }
                readBytes += ret;
            }
            if (!mPlaybackMQ->commitWrite(readBytes)) {
                ALOGW("[vts] Failed to commit write playback fmq.");
                mPlaybackThreadRunning = false;
                break;
            }
            playbackMQEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY));
        }
    }

    mPlaybackThreadRunning = false;
    ALOGW("[vts] Playback thread end.");
    close(fd);
}

void DvrCallback::testRecordOutput() {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (mDataOutputBuffer.empty()) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "record output matching pid does not output within timeout";
            return;
        }
    }
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
    mKeepReadingRecordFMQ = true;

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
                ALOGW("[vts] record data failed to be filtered. Ending thread");
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
    int readSize = mRecordMQ->availableToRead();
    mDataOutputBuffer.clear();
    mDataOutputBuffer.resize(readSize);
    result = mRecordMQ->read(mDataOutputBuffer.data(), readSize);
    EXPECT_TRUE(result) << "can't read from Record MQ";
    mMsgCondition.signal();
    return result;
}

void DvrCallback::stopRecordThread() {
    mKeepReadingRecordFMQ = false;
    mRecordThreadRunning = false;
}

AssertionResult DvrTests::openDvrInDemux(DvrType type, uint32_t bufferSize) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";

    // Create dvr callback
    if (type == DvrType::PLAYBACK) {
        mDvrPlaybackCallback = new DvrCallback();
        mDemux->openDvr(type, bufferSize, mDvrPlaybackCallback,
                        [&](Result result, const sp<IDvr>& dvr) {
                            mDvrPlayback = dvr;
                            status = result;
                        });
        if (status == Result::SUCCESS) {
            mDvrPlaybackCallback->setDvr(mDvrPlayback);
        }
    }

    if (type == DvrType::RECORD) {
        mDvrRecordCallback = new DvrCallback();
        mDemux->openDvr(type, bufferSize, mDvrRecordCallback,
                        [&](Result result, const sp<IDvr>& dvr) {
                            mDvrRecord = dvr;
                            status = result;
                        });
        if (status == Result::SUCCESS) {
            mDvrRecordCallback->setDvr(mDvrRecord);
        }
    }

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::configDvrPlayback(DvrSettings setting) {
    Result status = mDvrPlayback->configure(setting);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::configDvrRecord(DvrSettings setting) {
    Result status = mDvrRecord->configure(setting);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::getDvrPlaybackMQDescriptor() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrPlayback) << "Test with openDvr first.";

    mDvrPlayback->getQueueDesc([&](Result result, const MQDesc& dvrMQDesc) {
        mDvrPlaybackMQDescriptor = dvrMQDesc;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::getDvrRecordMQDescriptor() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    mDvrRecord->getQueueDesc([&](Result result, const MQDesc& dvrMQDesc) {
        mDvrRecordMQDescriptor = dvrMQDesc;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::attachFilterToDvr(sp<IFilter> filter) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->attachFilter(filter);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::detachFilterToDvr(sp<IFilter> filter) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->detachFilter(filter);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::startDvrPlayback() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrPlayback) << "Test with openDvr first.";

    status = mDvrPlayback->start();

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::stopDvrPlayback() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrPlayback) << "Test with openDvr first.";

    status = mDvrPlayback->stop();

    return AssertionResult(status == Result::SUCCESS);
}

void DvrTests::closeDvrPlayback() {
    ASSERT_TRUE(mDemux);
    ASSERT_TRUE(mDvrPlayback);
    ASSERT_TRUE(mDvrPlayback->close() == Result::SUCCESS);
}

AssertionResult DvrTests::startDvrRecord() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->start();

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult DvrTests::stopDvrRecord() {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->stop();

    return AssertionResult(status == Result::SUCCESS);
}

void DvrTests::closeDvrRecord() {
    ASSERT_TRUE(mDemux);
    ASSERT_TRUE(mDvrRecord);
    ASSERT_TRUE(mDvrRecord->close() == Result::SUCCESS);
}
