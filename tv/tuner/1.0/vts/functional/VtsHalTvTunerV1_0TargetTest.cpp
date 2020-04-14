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

#include "VtsHalTvTunerV1_0TargetTest.h"

namespace {
/******************************** Start FilterCallback **********************************/
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

/*================================= Start Test Module =================================*/
void TunerHidlTest::broadcastSingleFilterTest(FilterConfig filterConf,
                                              FrontendConfig frontendConf) {
    uint32_t feId;
    mFrontendTests.getFrontendIdByType(frontendConf.type, feId);
    if (feId == INVALID_ID) {
        // TODO broadcast test on Cuttlefish needs licensed ts input,
        // these tests are runnable on vendor device with real frontend module
        // or with manual ts installing and use DVBT frontend.
        return;
    }
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(openDemux());
    ASSERT_TRUE(setDemuxFrontendDataSource(feId));
    ASSERT_TRUE(openFilterInDemux(filterConf.type));
    uint32_t filterId;
    ASSERT_TRUE(getNewlyOpenedFilterId(filterId));
    ASSERT_TRUE(configFilter(filterConf.setting, filterId));
    ASSERT_TRUE(getFilterMQDescriptor(filterId));
    ASSERT_TRUE(startFilter(filterId));
    // tune test
    ASSERT_TRUE(mFrontendTests.tuneFrontend(frontendConf));
    // broadcast data flow test
    ASSERT_TRUE(broadcastDataFlowTest(goldenOutputFiles));
    ASSERT_TRUE(mFrontendTests.stopTuneFrontend());
    ASSERT_TRUE(stopFilter(filterId));
    ASSERT_TRUE(closeFilter(filterId));
    ASSERT_TRUE(closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
}
/*================================== End Test Module ==================================*/

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
TEST_P(TunerFrontendHidlTest, TuneFrontend) {
    description("Tune one Frontend with specific setting and check Lock event");
    mFrontendTests.tuneTest(frontendArray[DVBT]);
}

TEST_P(TunerFrontendHidlTest, AutoScanFrontend) {
    description("Run an auto frontend scan with specific setting and check lock scanMessage");
    mFrontendTests.scanTest(frontendScanArray[SCAN_DVBT], FrontendScanType::SCAN_AUTO);
}

TEST_P(TunerFrontendHidlTest, BlindScanFrontend) {
    description("Run an blind frontend scan with specific setting and check lock scanMessage");
    mFrontendTests.scanTest(frontendScanArray[SCAN_DVBT], FrontendScanType::SCAN_BLIND);
}

/*============================ Start Demux/Filter Tests ============================*/
TEST_P(TunerHidlTest, StartFilterInDemux) {
    description("Open and start a filter in Demux.");
    uint32_t feId;
    mFrontendTests.getFrontendIdByType(frontendArray[DVBT].type, feId);
    ASSERT_TRUE(feId != INVALID_ID);
    ASSERT_TRUE(mFrontendTests.openFrontendById(feId));
    ASSERT_TRUE(mFrontendTests.setFrontendCallback());
    ASSERT_TRUE(openDemux());
    ASSERT_TRUE(setDemuxFrontendDataSource(feId));
    ASSERT_TRUE(openFilterInDemux(filterArray[TS_VIDEO0].type));
    uint32_t filterId;
    ASSERT_TRUE(getNewlyOpenedFilterId(filterId));
    ASSERT_TRUE(configFilter(filterArray[TS_VIDEO0].setting, filterId));
    ASSERT_TRUE(getFilterMQDescriptor(filterId));
    ASSERT_TRUE(startFilter(filterId));
    ASSERT_TRUE(stopFilter(filterId));
    ASSERT_TRUE(closeFilter(filterId));
    ASSERT_TRUE(closeDemux());
    ASSERT_TRUE(mFrontendTests.closeFrontend());
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
TEST_P(TunerHidlTest, BroadcastDataFlowVideoFilterTest) {
    description("Test Video Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterArray[TS_VIDEO1], frontendArray[DVBS]);
}

TEST_P(TunerHidlTest, BroadcastDataFlowAudioFilterTest) {
    description("Test Audio Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterArray[TS_AUDIO0], frontendArray[DVBS]);
}

TEST_P(TunerHidlTest, BroadcastDataFlowTsFilterTest) {
    description("Test TS Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterArray[TS_TS0], frontendArray[DVBS]);
}

TEST_P(TunerHidlTest, BroadcastDataFlowSectionFilterTest) {
    description("Test Section Filter functionality in Broadcast use case.");
    broadcastSingleFilterTest(filterArray[TS_SECTION0], frontendArray[DVBS]);
}

TEST_P(TunerHidlTest, IonBufferTest) {
    description("Test the av filter data bufferring.");
    broadcastSingleFilterTest(filterArray[TS_VIDEO0], frontendArray[DVBS]);
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
/*============================== End Data Flow Tests ==============================*/
/******************************** End Test Entry **********************************/
INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerFrontendHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, TunerHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ITuner::descriptor)),
        android::hardware::PrintInstanceNameToString);
}  // namespace
