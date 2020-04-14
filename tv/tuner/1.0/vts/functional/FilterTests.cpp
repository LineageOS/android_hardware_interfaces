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

#include "FilterTests.h"

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

AssertionResult FilterTests::openFilterInDemux(DemuxFilterType type) {
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

AssertionResult FilterTests::getNewlyOpenedFilterId(uint32_t& filterId) {
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

AssertionResult FilterTests::configFilter(DemuxFilterSettings setting, uint32_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    status = mFilters[filterId]->configure(setting);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::getFilterMQDescriptor(uint32_t filterId) {
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

AssertionResult FilterTests::startFilter(uint32_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->start();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::stopFilter(uint32_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->stop();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::closeFilter(uint32_t filterId) {
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