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

#include "FilterTests.h"

#include <inttypes.h>
#include <algorithm>

#include <aidl/android/hardware/tv/tuner/DemuxFilterMonitorEventType.h>
#include <aidlcommonsupport/NativeHandle.h>

using ::aidl::android::hardware::common::NativeHandle;

::ndk::ScopedAStatus FilterCallback::onFilterEvent(const vector<DemuxFilterEvent>& events) {
    android::Mutex::Autolock autoLock(mMsgLock);
    // Temprarily we treat the first coming back filter data on the matching pid a success
    // once all of the MQ are cleared, means we got all the expected output
    readFilterEventsData(events);
    mPidFilterOutputCount++;
    mMsgCondition.signal();

    for (auto it = mFilterCallbackVerifiers.begin(); it != mFilterCallbackVerifiers.end();) {
        auto& [verifier, promise] = *it;
        if (verifier(events)) {
            promise.set_value();
            it = mFilterCallbackVerifiers.erase(it);
        } else {
            ++it;
        }
    };

    return ::ndk::ScopedAStatus::ok();
}

std::future<void> FilterCallback::verifyFilterCallback(FilterCallbackVerifier&& verifier) {
    std::promise<void> promise;
    auto future = promise.get_future();
    mFilterCallbackVerifiers.emplace_back(std::move(verifier), std::move(promise));
    return future;
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

void FilterCallback::readFilterEventsData(const vector<DemuxFilterEvent>& events) {
    ALOGW("[vts] reading filter event");
    // todo separate filter handlers
    for (int i = 0; i < events.size(); i++) {
        switch (events[i].getTag()) {
            case DemuxFilterEvent::Tag::media:
                ALOGD("[vts] Media filter event, avMemHandle numFds=%zu.",
                      events[i].get<DemuxFilterEvent::Tag::media>().avMemory.fds.size());
                dumpAvData(events[i].get<DemuxFilterEvent::Tag::media>());
                break;
            case DemuxFilterEvent::Tag::tsRecord:
                ALOGD("[vts] TS record filter event, pts=%" PRIu64 ", firstMbInSlice=%d",
                      events[i].get<DemuxFilterEvent::Tag::tsRecord>().pts,
                      events[i].get<DemuxFilterEvent::Tag::tsRecord>().firstMbInSlice);
                break;
            case DemuxFilterEvent::Tag::mmtpRecord:
                ALOGD("[vts] MMTP record filter event, pts=%" PRIu64
                      ", firstMbInSlice=%d, mpuSequenceNumber=%d, tsIndexMask=%d",
                      events[i].get<DemuxFilterEvent::Tag::mmtpRecord>().pts,
                      events[i].get<DemuxFilterEvent::Tag::mmtpRecord>().firstMbInSlice,
                      events[i].get<DemuxFilterEvent::Tag::mmtpRecord>().mpuSequenceNumber,
                      events[i].get<DemuxFilterEvent::Tag::mmtpRecord>().tsIndexMask);
                break;
            case DemuxFilterEvent::Tag::monitorEvent:
                switch (events[i].get<DemuxFilterEvent::Tag::monitorEvent>().getTag()) {
                    case DemuxFilterMonitorEvent::Tag::scramblingStatus:
                        mScramblingStatusEvent++;
                        break;
                    case DemuxFilterMonitorEvent::Tag::cid:
                        mIpCidEvent++;
                        break;
                    default:
                        break;
                }
                break;
            case DemuxFilterEvent::Tag::startId:
                ALOGD("[vts] Restart filter event, startId=%d",
                      events[i].get<DemuxFilterEvent::Tag::startId>());
                mStartIdReceived = true;
                break;
            default:
                break;
        }
    }
}

bool FilterCallback::dumpAvData(const DemuxFilterMediaEvent& event) {
    int64_t length = event.dataLength;
    int64_t offset = event.offset;
    int av_fd;
    // read data from buffer pointed by a handle
    if (event.avMemory.fds.size() == 0) {
        if (mAvSharedHandle == nullptr) {
            return false;
        }
        av_fd = mAvSharedHandle->data[0];
    } else {
        av_fd = event.avMemory.fds[0].get();
    }
    uint8_t* buffer = static_cast<uint8_t*>(
            mmap(NULL, length + offset, PROT_READ | PROT_WRITE, MAP_SHARED, av_fd, 0));
    if (buffer == MAP_FAILED) {
        ALOGE("[vts] fail to allocate av buffer, errno=%d", errno);
        return false;
    }
    uint8_t output[length + 1];
    memcpy(output, buffer + offset, length);
    // print buffer and check with golden output.
    return true;
}

AssertionResult FilterTests::openFilterInDemux(DemuxFilterType type, int32_t bufferSize) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";

    // Create demux callback
    mFilterCallback = ndk::SharedRefBase::make<FilterCallback>();

    // Add filter to the local demux
    status = mDemux->openFilter(type, bufferSize, mFilterCallback, &mFilter);

    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::getNewlyOpenedFilterId_64bit(int64_t& filterId) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mDemux) << "Test with openDemux first.";
    EXPECT_TRUE(mFilter) << "Test with openFilterInDemux first.";
    EXPECT_TRUE(mFilterCallback) << "Test with openFilterInDemux first.";

    status = mFilter->getId64Bit(&mFilterId);
    if (status.isOk()) {
        mFilterCallback->setFilterId(mFilterId);
        mFilterCallback->setFilterInterface(mFilter);
        mUsedFilterIds.insert(mUsedFilterIds.end(), mFilterId);
        mFilters[mFilterId] = mFilter;
        mFilterCallbacks[mFilterId] = mFilterCallback;
        filterId = mFilterId;

        // Check getId() too.
        int32_t filterId32Bit;
        status = mFilter->getId(&filterId32Bit);
    }

    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::getSharedAvMemoryHandle(int64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Open media filter first.";
    NativeHandle avMemory;
    int64_t avMemSize;
    ndk::ScopedAStatus status = mFilters[filterId]->getAvSharedHandle(&avMemory, &avMemSize);
    if (status.isOk()) {
        mAvSharedHandle = android::dupFromAidl(avMemory);
        mFilterCallbacks[mFilterId]->setSharedHandle(mAvSharedHandle);
        mFilterCallbacks[mFilterId]->setMemSize(avMemSize);
    }
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::releaseShareAvHandle(int64_t filterId) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mFilters[filterId]) << "Open media filter first.";
    EXPECT_TRUE(mAvSharedHandle) << "No shared av handle to release.";
    status = mFilters[filterId]->releaseAvHandle(::android::makeToAidl(mAvSharedHandle),
                                                 0 /*dataId*/);
    native_handle_close(mAvSharedHandle);
    native_handle_delete(mAvSharedHandle);
    mAvSharedHandle = nullptr;

    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::configFilter(DemuxFilterSettings setting, int64_t filterId) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    status = mFilters[filterId]->configure(setting);

    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::configAvFilterStreamType(AvStreamType type, int64_t filterId) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";

    status = mFilters[filterId]->configureAvStreamType(type);
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::configIpFilterCid(int32_t ipCid, int64_t filterId) {
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mFilters[filterId]) << "Open Ip filter first.";

    status = mFilters[filterId]->configureIpCid(ipCid);
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::getFilterMQDescriptor(int64_t filterId, bool getMqDesc) {
    if (!getMqDesc) {
        ALOGE("[vts] Filter does not need FMQ.");
        return success();
    }
    ndk::ScopedAStatus status;
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    EXPECT_TRUE(mFilterCallbacks[filterId]) << "Test with getNewlyOpenedFilterId first.";

    status = mFilters[filterId]->getQueueDesc(&mFilterMQDescriptor);
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::startFilter(int64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";

    ndk::ScopedAStatus status = mFilters[filterId]->start();
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::stopFilter(int64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";

    ndk::ScopedAStatus status = mFilters[filterId]->stop();
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::closeFilter(int64_t filterId) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    ndk::ScopedAStatus status = mFilters[filterId]->close();
    if (status.isOk()) {
        for (int i = 0; i < mUsedFilterIds.size(); i++) {
            if (mUsedFilterIds[i] == filterId) {
                mUsedFilterIds.erase(mUsedFilterIds.begin() + i);
                break;
            }
        }
        mFilterCallbacks.erase(filterId);
        mFilters.erase(filterId);
    }
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::configureMonitorEvent(int64_t filterId, int32_t monitorEventTypes) {
    EXPECT_TRUE(mFilters[filterId]) << "Test with getNewlyOpenedFilterId first.";
    ndk::ScopedAStatus status;

    status = mFilters[filterId]->configureMonitorEvent(monitorEventTypes);
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::testMonitorEvent(uint64_t filterId, uint32_t monitorEventTypes) {
    EXPECT_TRUE(mFilterCallbacks[filterId]) << "Test with getNewlyOpenedFilterId first.";
    if (monitorEventTypes & static_cast<int32_t>(DemuxFilterMonitorEventType::SCRAMBLING_STATUS)) {
        mFilterCallbacks[filterId]->testFilterScramblingEvent();
    }
    if (monitorEventTypes & static_cast<int32_t>(DemuxFilterMonitorEventType::IP_CID_CHANGE)) {
        mFilterCallbacks[filterId]->testFilterIpCidEvent();
    }
    return AssertionResult(true);
}

AssertionResult FilterTests::startIdTest(int64_t filterId) {
    EXPECT_TRUE(mFilterCallbacks[filterId]) << "Test with getNewlyOpenedFilterId first.";
    mFilterCallbacks[filterId]->testStartIdAfterReconfigure();
    return AssertionResult(true);
}

AssertionResult FilterTests::openTimeFilterInDemux() {
    if (!mDemux) {
        ALOGW("[vts] Test with openDemux first.");
        return failure();
    }

    // Add time filter to the local demux
    auto status = mDemux->openTimeFilter(&mTimeFilter);
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::setTimeStamp(int64_t timeStamp) {
    if (!mTimeFilter) {
        ALOGW("[vts] Test with openTimeFilterInDemux first.");
        return failure();
    }

    mBeginTimeStamp = timeStamp;
    return AssertionResult(mTimeFilter->setTimeStamp(timeStamp).isOk());
}

AssertionResult FilterTests::getTimeStamp() {
    if (!mTimeFilter) {
        ALOGW("[vts] Test with openTimeFilterInDemux first.");
        return failure();
    }

    int64_t timeStamp;
    auto status = mTimeFilter->getTimeStamp(&timeStamp);
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::setFilterDataSource(int64_t sourceFilterId, int64_t sinkFilterId) {
    if (!mFilters[sourceFilterId] || !mFilters[sinkFilterId]) {
        ALOGE("[vts] setFilterDataSource filter not opened.");
        return failure();
    }

    auto status = mFilters[sinkFilterId]->setDataSource(mFilters[sourceFilterId]);
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::setFilterDataSourceToDemux(int64_t filterId) {
    if (!mFilters[filterId]) {
        ALOGE("[vts] setFilterDataSourceToDemux filter not opened.");
        return failure();
    }

    auto status = mFilters[filterId]->setDataSource(nullptr);
    return AssertionResult(status.isOk());
}

AssertionResult FilterTests::clearTimeStamp() {
    if (!mTimeFilter) {
        ALOGW("[vts] Test with openTimeFilterInDemux first.");
        return failure();
    }

    return AssertionResult(mTimeFilter->clearTimeStamp().isOk());
}

AssertionResult FilterTests::closeTimeFilter() {
    if (!mTimeFilter) {
        ALOGW("[vts] Test with openTimeFilterInDemux first.");
        return failure();
    }

    return AssertionResult(mTimeFilter->close().isOk());
}
