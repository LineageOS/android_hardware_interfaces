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

#include "DvrTests.h"

#include <aidl/android/hardware/tv/tuner/DemuxQueueNotifyBits.h>

void DvrCallback::startPlaybackInputThread(string& dataInputFile, PlaybackSettings& settings,
                                           MQDesc& playbackMQDescriptor) {
    mInputDataFile = dataInputFile;
    mPlaybackSettings = settings;
    mPlaybackMQ = std::make_unique<FilterMQ>(playbackMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mPlaybackMQ);

    mPlaybackThread = std::thread(&DvrCallback::playbackThreadLoop, this);
}

void DvrCallback::stopPlaybackThread() {
    mPlaybackThreadRunning = false;
    mKeepWritingPlaybackFMQ = false;

    if (mPlaybackThread.joinable()) {
        mPlaybackThread.join();
    }
}

void DvrCallback::playbackThreadLoop() {
    mPlaybackThreadRunning = true;
    mKeepWritingPlaybackFMQ = true;

    // Create the EventFlag that is used to signal the HAL impl that data have been
    // written into the Playback FMQ
    EventFlag* playbackMQEventFlag;
    EXPECT_TRUE(EventFlag::createEventFlag(mPlaybackMQ->getEventFlagWord(), &playbackMQEventFlag) ==
                android::OK);

    int fd = open(mInputDataFile.c_str(), O_RDONLY | O_LARGEFILE);
    int readBytes;
    uint32_t regionSize = 0;
    int8_t* buffer;
    ALOGW("[vts] playback thread loop start %s", mInputDataFile.c_str());
    if (fd < 0) {
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
            AidlMessageQueue<int8_t, SynchronizedReadWrite>::MemTransaction memTx;
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
    bool passed = true;
    {
        android::Mutex::Autolock autoLock(mMsgLock);
        while (mDataOutputBuffer.empty()) {
            if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
                EXPECT_TRUE(false) << "record output matching pid does not output within timeout";
                passed = false;
                break;
            }
        }
    }
    stopRecordThread();
    if (passed) ALOGW("[vts] record pass and stop");
}

void DvrCallback::startRecordOutputThread(RecordSettings /* recordSettings */,
                                          MQDesc& recordMQDescriptor) {
    mRecordMQ = std::make_unique<FilterMQ>(recordMQDescriptor, true /* resetPointers */);
    EXPECT_TRUE(mRecordMQ);

    mRecordThread = std::thread(&DvrCallback::recordThreadLoop, this);
}

void DvrCallback::recordThreadLoop() {
    ALOGD("[vts] DvrCallback record threadLoop start.");
    mRecordThreadRunning = true;
    mKeepReadingRecordFMQ = true;

    // Create the EventFlag that is used to signal the HAL impl that data have been
    // read from the Record FMQ
    EventFlag* recordMQEventFlag;
    EXPECT_TRUE(EventFlag::createEventFlag(mRecordMQ->getEventFlagWord(), &recordMQEventFlag) ==
                android::OK);

    while (mRecordThreadRunning) {
        while (mKeepReadingRecordFMQ) {
            uint32_t efState = 0;
            android::status_t status = recordMQEventFlag->wait(
                    static_cast<int32_t>(DemuxQueueNotifyBits::DATA_READY), &efState, WAIT_TIMEOUT,
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

    if (mRecordThread.joinable()) {
        mRecordThread.join();
    }
}

AssertionResult DvrTests::openDvrInDemux(DvrType type, int32_t bufferSize) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";

    // Create dvr callback
    if (type == DvrType::PLAYBACK) {
        mDvrPlaybackCallback = ndk::SharedRefBase::make<DvrCallback>();
        status = mDemux->openDvr(type, bufferSize, mDvrPlaybackCallback, &mDvrPlayback);
        if (status.isOk()) {
            mDvrPlaybackCallback->setDvr(mDvrPlayback);
        }
    }

    if (type == DvrType::RECORD) {
        mDvrRecordCallback = ndk::SharedRefBase::make<DvrCallback>();
        status = mDemux->openDvr(type, bufferSize, mDvrRecordCallback, &mDvrRecord);
        if (status.isOk()) {
            mDvrRecordCallback->setDvr(mDvrRecord);
        }
    }

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::configDvrPlayback(DvrSettings setting) {
    ndk::ScopedAStatus status = mDvrPlayback->configure(setting);

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::configDvrRecord(DvrSettings setting) {
    ndk::ScopedAStatus status = mDvrRecord->configure(setting);

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::getDvrPlaybackMQDescriptor() {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrPlayback) << "Test with openDvr first.";

    status = mDvrPlayback->getQueueDesc(&mDvrPlaybackMQDescriptor);

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::getDvrRecordMQDescriptor() {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->getQueueDesc(&mDvrRecordMQDescriptor);

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::attachFilterToDvr(std::shared_ptr<IFilter> filter) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->attachFilter(filter);

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::detachFilterToDvr(std::shared_ptr<IFilter> filter) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->detachFilter(filter);

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::startDvrPlayback() {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrPlayback) << "Test with openDvr first.";

    status = mDvrPlayback->start();

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::stopDvrPlayback() {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrPlayback) << "Test with openDvr first.";

    status = mDvrPlayback->stop();

    return AssertionResult(status.isOk());
}

void DvrTests::closeDvrPlayback() {
    ASSERT_TRUE(mDemux);
    ASSERT_TRUE(mDvrPlayback);
    ASSERT_TRUE(mDvrPlayback->close().isOk());
}

AssertionResult DvrTests::startDvrRecord() {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->start();

    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::stopDvrRecord() {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->stop();

    return AssertionResult(status.isOk());
}

void DvrTests::closeDvrRecord() {
    ASSERT_TRUE(mDemux);
    ASSERT_TRUE(mDvrRecord);
    ASSERT_TRUE(mDvrRecord->close().isOk());
}

AssertionResult DvrTests::setPlaybackStatusCheckIntervalHint(int64_t milliseconds) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrPlayback) << "Test with openDvr first.";

    status = mDvrPlayback->setStatusCheckIntervalHint(milliseconds);

    if (getDvrPlaybackInterfaceVersion() < 2) {
        return AssertionResult(status.getStatus() == STATUS_UNKNOWN_TRANSACTION);
    }
    return AssertionResult(status.isOk());
}

AssertionResult DvrTests::setRecordStatusCheckIntervalHint(int64_t milliseconds) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mDvrRecord) << "Test with openDvr first.";

    status = mDvrRecord->setStatusCheckIntervalHint(milliseconds);

    if (getDvrRecordInterfaceVersion() < 2) {
        return AssertionResult(status.getStatus() == STATUS_UNKNOWN_TRANSACTION);
    }
    return AssertionResult(status.isOk());
}

int32_t DvrTests::getDvrPlaybackInterfaceVersion() {
    int32_t version;
    mDvrPlayback->getInterfaceVersion(&version);
    return version;
}

int32_t DvrTests::getDvrRecordInterfaceVersion() {
    int32_t version;
    mDvrRecord->getInterfaceVersion(&version);
    return version;
}
