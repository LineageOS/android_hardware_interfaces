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

void FilterCallback::testFilterScramblingEvent() {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (mScramblingStatusEvent < 1) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "scrambling event does not output within timeout";
            return;
        }
    }
    mScramblingStatusEvent = 0;
    ALOGW("[vts] pass and stop");
}

void FilterCallback::testFilterIpCidEvent() {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (mIpCidEvent < 1) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "ip cid change event does not output within timeout";
            return;
        }
    }
    mIpCidEvent = 0;
    ALOGW("[vts] pass and stop");
}

void FilterCallback::testStartIdAfterReconfigure() {
    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mStartIdReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "does not receive start id within timeout";
            return;
        }
    }
    mStartIdReceived = false;
    ALOGW("[vts] pass and stop");
}

void FilterCallback::readFilterEventData() {
    ALOGW("[vts] reading filter event");
    // todo separate filter handlers
    for (int i = 0; i < mFilterEvent.events.size(); i++) {
        auto event = mFilterEvent.events[i];
        switch (event.getDiscriminator()) {
            case DemuxFilterEvent::Event::hidl_discriminator::media:
                ALOGD("[vts] Media filter event, avMemHandle numFds=%d.",
                      event.media().avMemory.getNativeHandle()->numFds);
                dumpAvData(event.media());
                break;
            default:
                break;
        }
    }
    for (int i = 0; i < mFilterEventExt.events.size(); i++) {
        auto eventExt = mFilterEventExt.events[i];
        switch (eventExt.getDiscriminator()) {
            case DemuxFilterEventExt::Event::hidl_discriminator::tsRecord:
                ALOGD("[vts] Extended TS record filter event, pts=%" PRIu64 ", firstMbInSlice=%d",
                      eventExt.tsRecord().pts, eventExt.tsRecord().firstMbInSlice);
                break;
            case DemuxFilterEventExt::Event::hidl_discriminator::mmtpRecord:
                ALOGD("[vts] Extended MMTP record filter event, pts=%" PRIu64
                      ", firstMbInSlice=%d, mpuSequenceNumber=%d, tsIndexMask=%d",
                      eventExt.mmtpRecord().pts, eventExt.mmtpRecord().firstMbInSlice,
                      eventExt.mmtpRecord().mpuSequenceNumber, eventExt.mmtpRecord().tsIndexMask);
                break;
            case DemuxFilterEventExt::Event::hidl_discriminator::monitorEvent:
                switch (eventExt.monitorEvent().getDiscriminator()) {
                    case DemuxFilterMonitorEvent::hidl_discriminator::scramblingStatus:
                        mScramblingStatusEvent++;
                        break;
                    case DemuxFilterMonitorEvent::hidl_discriminator::cid:
                        mIpCidEvent++;
                        break;
                    default:
                        break;
                }
                break;
            case DemuxFilterEventExt::Event::hidl_discriminator::startId:
                ALOGD("[vts] Extended restart filter event, startId=%d", eventExt.startId());
                mStartIdReceived = true;
                break;
            default:
                break;
        }
    }
}

bool FilterCallback::dumpAvData(DemuxFilterMediaEvent event) {
    uint32_t length = event.dataLength;
    uint32_t offset = event.offset;
    // read data from buffer pointed by a handle
    hidl_handle handle = event.avMemory;
    if (handle.getNativeHandle()->numFds == 0) {
        if (mAvSharedHandle == NULL) {
            return false;
        }
        handle = mAvSharedHandle;
    }

    int av_fd = handle.getNativeHandle()->data[0];
    uint8_t* buffer = static_cast<uint8_t*>(
            mmap(NULL, length + offset, PROT_READ | PROT_WRITE, MAP_SHARED, av_fd, 0));
    if (buffer == MAP_FAILED) {
        ALOGE("[vts] fail to allocate av buffer, errno=%d", errno);
        return false;
    }
    uint8_t output[length + 1];
    memcpy(output, buffer + offset, length);
    // print buffer and check with golden output.
    ::close(av_fd);
    return true;
}

AssertionResult FilterTests::openFilterInDemux(DemuxFilterType type, uint32_t bufferSize) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";

    // Create demux callback
    mFilterCallback = new FilterCallback();

    // Add filter to the local demux
    mDemux->openFilter(type, bufferSize, mFilterCallback,
                       [&](Result result, const sp<IFilter>& filter) {
                           mFilter = filter;
                           status = result;
                       });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::getNewlyOpenedFilterId_64bit(uint64_t& filterId) {
    Result status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mFilter) << "Test with openFilterInDemux first.";
    EXPECT_TRUE(mFilterCallback) << "Test with openFilterInDemux first.";

    sp<android::hardware::tv::tuner::V1_1::IFilter> filter_v1_1 =
            android::hardware::tv::tuner::V1_1::IFilter::castFrom(mFilter);
    if (filter_v1_1 != NULL) {
        filter_v1_1->getId64Bit([&](Result result, uint64_t filterId) {
            mFilterId = filterId;
            status = result;
        });
    } else {
        ALOGW("[vts] Can't cast IFilter into v1_1.");
        return failure();
    }

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

AssertionResult FilterTests::getSharedAvMemoryHandle(uint64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Open media filter first.";
    Result status = Result::UNKNOWN_ERROR;
    sp<android::hardware::tv::tuner::V1_1::IFilter> filter_v1_1 =
            android::hardware::tv::tuner::V1_1::IFilter::castFrom(mFilters[filterId]);
    if (filter_v1_1 != NULL) {
        filter_v1_1->getAvSharedHandle([&](Result r, hidl_handle avMemory, uint64_t avMemSize) {
            status = r;
            if (status == Result::SUCCESS) {
                mFilterCallbacks[mFilterId]->setSharedHandle(avMemory);
                mFilterCallbacks[mFilterId]->setMemSize(avMemSize);
                mAvSharedHandle = avMemory;
            }
        });
    }
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::releaseShareAvHandle(uint64_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Open media filter first.";
    EXPECT_TRUE(mAvSharedHandle) << "No shared av handle to release.";
    status = mFilters[filterId]->releaseAvHandle(mAvSharedHandle, 0 /*dataId*/);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::configFilter(DemuxFilterSettings setting, uint64_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    status = mFilters[filterId]->configure(setting);

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::configAvFilterStreamType(AvStreamType type, uint64_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    sp<android::hardware::tv::tuner::V1_1::IFilter> filter_v1_1 =
            android::hardware::tv::tuner::V1_1::IFilter::castFrom(mFilters[filterId]);
    if (filter_v1_1 != NULL) {
        status = filter_v1_1->configureAvStreamType(type);
    } else {
        ALOGW("[vts] Can't cast IFilter into v1_1.");
        return failure();
    }
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::configIpFilterCid(uint32_t ipCid, uint64_t filterId) {
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Open Ip filter first.";

    sp<android::hardware::tv::tuner::V1_1::IFilter> filter_v1_1 =
            android::hardware::tv::tuner::V1_1::IFilter::castFrom(mFilters[filterId]);
    if (filter_v1_1 != NULL) {
        status = filter_v1_1->configureIpCid(ipCid);
    } else {
        ALOGW("[vts] Can't cast IFilter into v1_1.");
        return failure();
    }

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::getFilterMQDescriptor(uint64_t filterId, bool getMqDesc) {
    if (!getMqDesc) {
        ALOGE("[vts] Filter does not need FMQ.");
        return success();
    }
    Result status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    EXPECT_TRUE(mFilterCallbacks[filterId]) << "Test with getNewlyOpenedFilterId first.";

    mFilter->getQueueDesc([&](Result result, const MQDesc& filterMQDesc) {
        mFilterMQDescriptor = filterMQDesc;
        status = result;
    });

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::startFilter(uint64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->start();
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::stopFilter(uint64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status = mFilters[filterId]->stop();

    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::closeFilter(uint64_t filterId) {
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

AssertionResult FilterTests::configureMonitorEvent(uint64_t filterId, uint32_t monitorEventTypes) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    Result status;

    sp<android::hardware::tv::tuner::V1_1::IFilter> filter_v1_1 =
            android::hardware::tv::tuner::V1_1::IFilter::castFrom(mFilters[filterId]);
    if (filter_v1_1 != NULL) {
        status = filter_v1_1->configureMonitorEvent(monitorEventTypes);
        if (monitorEventTypes & DemuxFilterMonitorEventType::SCRAMBLING_STATUS) {
            mFilterCallbacks[filterId]->testFilterScramblingEvent();
        }
        if (monitorEventTypes & DemuxFilterMonitorEventType::IP_CID_CHANGE) {
            mFilterCallbacks[filterId]->testFilterIpCidEvent();
        }
    } else {
        ALOGW("[vts] Can't cast IFilter into v1_1.");
        return failure();
    }
    return AssertionResult(status == Result::SUCCESS);
}

AssertionResult FilterTests::startIdTest(uint64_t filterId) {
    EXPECT_TRUE(mFilterCallbacks[filterId]) << "Test with getNewlyOpenedFilterId first.";
    mFilterCallbacks[filterId]->testStartIdAfterReconfigure();
    return AssertionResult(true);
}
