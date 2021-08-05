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

//#define LOG_NDEBUG 0
#define LOG_TAG "android.hardware.tv.tuner-service.example-Filter"

#include <BufferAllocator/BufferAllocator.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterMonitorEventType.h>
#include <aidl/android/hardware/tv/tuner/DemuxQueueNotifyBits.h>
#include <aidl/android/hardware/tv/tuner/Result.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <utils/Log.h>

#include "Filter.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

#define WAIT_TIMEOUT 3000000000

Filter::Filter() {}

Filter::Filter(DemuxFilterType type, int64_t filterId, uint32_t bufferSize,
               const std::shared_ptr<IFilterCallback>& cb, std::shared_ptr<Demux> demux) {
    mType = type;
    mFilterId = filterId;
    mBufferSize = bufferSize;
    mDemux = demux;
    mCallback = cb;

    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            if (mType.subType.get<DemuxFilterSubType::Tag::tsFilterType>() ==
                        DemuxTsFilterType::AUDIO ||
                mType.subType.get<DemuxFilterSubType::Tag::tsFilterType>() ==
                        DemuxTsFilterType::VIDEO) {
                mIsMediaFilter = true;
            }
            if (mType.subType.get<DemuxFilterSubType::Tag::tsFilterType>() ==
                DemuxTsFilterType::PCR) {
                mIsPcrFilter = true;
            }
            if (mType.subType.get<DemuxFilterSubType::Tag::tsFilterType>() ==
                DemuxTsFilterType::RECORD) {
                mIsRecordFilter = true;
            }
            break;
        case DemuxFilterMainType::MMTP:
            if (mType.subType.get<DemuxFilterSubType::Tag::mmtpFilterType>() ==
                        DemuxMmtpFilterType::AUDIO ||
                mType.subType.get<DemuxFilterSubType::Tag::mmtpFilterType>() ==
                        DemuxMmtpFilterType::VIDEO) {
                mIsMediaFilter = true;
            }
            if (mType.subType.get<DemuxFilterSubType::Tag::mmtpFilterType>() ==
                DemuxMmtpFilterType::RECORD) {
                mIsRecordFilter = true;
            }
            break;
        case DemuxFilterMainType::IP:
            break;
        case DemuxFilterMainType::TLV:
            break;
        case DemuxFilterMainType::ALP:
            break;
        default:
            break;
    }
}

Filter::~Filter() {
    mFilterThreadRunning = false;
    std::lock_guard<std::mutex> lock(mFilterThreadLock);
}

::ndk::ScopedAStatus Filter::getId64Bit(int64_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    *_aidl_return = mFilterId;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::getId(int32_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    *_aidl_return = static_cast<int32_t>(mFilterId);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::setDataSource(const std::shared_ptr<IFilter>& in_filter) {
    ALOGV("%s", __FUNCTION__);

    mDataSource = in_filter;
    mIsDataSourceDemux = false;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::getQueueDesc(MQDescriptor<int8_t, SynchronizedReadWrite>* out_queue) {
    ALOGV("%s", __FUNCTION__);

    mIsUsingFMQ = mIsRecordFilter ? false : true;

    *out_queue = mFilterMQ->dupeDesc();
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::configure(const DemuxFilterSettings& in_settings) {
    ALOGV("%s", __FUNCTION__);

    mFilterSettings = in_settings;
    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            mTpid = in_settings.get<DemuxFilterSettings::Tag::ts>().tpid;
            break;
        case DemuxFilterMainType::MMTP:
            break;
        case DemuxFilterMainType::IP:
            break;
        case DemuxFilterMainType::TLV:
            break;
        case DemuxFilterMainType::ALP:
            break;
        default:
            break;
    }

    mConfigured = true;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::start() {
    ALOGV("%s", __FUNCTION__);
    mFilterThreadRunning = true;
    vector<DemuxFilterEvent> events;
    // All the filter event callbacks in start are for testing purpose.
    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            createMediaEvent(events);
            mCallback->onFilterEvent(events);
            createTsRecordEvent(events);
            mCallback->onFilterEvent(events);
            createTemiEvent(events);
            mCallback->onFilterEvent(events);
            break;
        case DemuxFilterMainType::MMTP:
            createDownloadEvent(events);
            mCallback->onFilterEvent(events);
            createMmtpRecordEvent(events);
            mCallback->onFilterEvent(events);
            break;
        case DemuxFilterMainType::IP:
            createSectionEvent(events);
            mCallback->onFilterEvent(events);
            createIpPayloadEvent(events);
            mCallback->onFilterEvent(events);
            break;
        case DemuxFilterMainType::TLV:
            createMonitorEvent(events);
            mCallback->onFilterEvent(events);
            break;
        case DemuxFilterMainType::ALP:
            createMonitorEvent(events);
            mCallback->onFilterEvent(events);
            break;
        default:
            break;
    }
    return startFilterLoop();
}

::ndk::ScopedAStatus Filter::stop() {
    ALOGV("%s", __FUNCTION__);
    mFilterThreadRunning = false;
    std::lock_guard<std::mutex> lock(mFilterThreadLock);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::flush() {
    ALOGV("%s", __FUNCTION__);

    // temp implementation to flush the FMQ
    int size = mFilterMQ->availableToRead();
    int8_t* buffer = new int8_t[size];
    mFilterMQ->read(buffer, size);
    delete[] buffer;
    mFilterStatus = DemuxFilterStatus::DATA_READY;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::releaseAvHandle(const NativeHandle& in_avMemory, int64_t in_avDataId) {
    ALOGV("%s", __FUNCTION__);

    if ((mSharedAvMemHandle != nullptr) && (in_avMemory.fds.size() > 0) &&
        (sameFile(in_avMemory.fds[0].get(), mSharedAvMemHandle->data[0]))) {
        freeSharedAvHandle();
        return ::ndk::ScopedAStatus::ok();
    }

    if (mDataId2Avfd.find(in_avDataId) == mDataId2Avfd.end()) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    ::close(mDataId2Avfd[in_avDataId]);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::close() {
    ALOGV("%s", __FUNCTION__);

    mFilterThreadRunning = false;
    std::lock_guard<std::mutex> lock(mFilterThreadLock);
    return mDemux->removeFilter(mFilterId);
}

::ndk::ScopedAStatus Filter::configureIpCid(int32_t in_ipCid) {
    ALOGV("%s", __FUNCTION__);

    if (mType.mainType != DemuxFilterMainType::IP) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }

    mCid = in_ipCid;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::getAvSharedHandle(NativeHandle* out_avMemory, int64_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (!mIsMediaFilter) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }

    if (mSharedAvMemHandle != nullptr) {
        *out_avMemory = ::android::dupToAidl(mSharedAvMemHandle);
        *_aidl_return = BUFFER_SIZE_16M;
        mUsingSharedAvMem = true;
        return ::ndk::ScopedAStatus::ok();
    }

    int av_fd = createAvIonFd(BUFFER_SIZE_16M);
    if (av_fd < 0) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::OUT_OF_MEMORY));
    }

    mSharedAvMemHandle = createNativeHandle(av_fd);
    if (mSharedAvMemHandle == nullptr) {
        ::close(av_fd);
        *_aidl_return = 0;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }
    ::close(av_fd);
    mUsingSharedAvMem = true;

    *out_avMemory = ::android::dupToAidl(mSharedAvMemHandle);
    *_aidl_return = BUFFER_SIZE_16M;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::configureAvStreamType(const AvStreamType& in_avStreamType) {
    ALOGV("%s", __FUNCTION__);

    if (!mIsMediaFilter) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNAVAILABLE));
    }

    switch (in_avStreamType.getTag()) {
        case AvStreamType::Tag::audio:
            mAudioStreamType =
                    static_cast<uint32_t>(in_avStreamType.get<AvStreamType::Tag::audio>());
            break;
        case AvStreamType::Tag::video:
            mVideoStreamType =
                    static_cast<uint32_t>(in_avStreamType.get<AvStreamType::Tag::video>());
            break;
        default:
            break;
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::configureMonitorEvent(int in_monitorEventTypes) {
    ALOGV("%s", __FUNCTION__);

    int32_t newScramblingStatus =
            in_monitorEventTypes &
            static_cast<int32_t>(DemuxFilterMonitorEventType::SCRAMBLING_STATUS);
    int32_t newIpCid =
            in_monitorEventTypes & static_cast<int32_t>(DemuxFilterMonitorEventType::IP_CID_CHANGE);

    // if scrambling status monitoring flipped, record the new state and send msg on enabling
    if (newScramblingStatus ^ mScramblingStatusMonitored) {
        mScramblingStatusMonitored = newScramblingStatus;
        if (mScramblingStatusMonitored) {
            if (mCallback != nullptr) {
                // Assuming current status is always NOT_SCRAMBLED
                vector<DemuxFilterEvent> events;
                DemuxFilterMonitorEvent monitorEvent;
                events.resize(1);
                monitorEvent.set<DemuxFilterMonitorEvent::Tag::scramblingStatus>(
                        ScramblingStatus::NOT_SCRAMBLED);
                events[0].set<DemuxFilterEvent::monitorEvent>(monitorEvent);
                mCallback->onFilterEvent(events);
            } else {
                return ::ndk::ScopedAStatus::fromServiceSpecificError(
                        static_cast<int32_t>(Result::INVALID_STATE));
            }
        }
    }

    // if ip cid monitoring flipped, record the new state and send msg on enabling
    if (newIpCid ^ mIpCidMonitored) {
        mIpCidMonitored = newIpCid;
        if (mIpCidMonitored) {
            if (mCallback != nullptr) {
                // Return random cid
                vector<DemuxFilterEvent> events;
                DemuxFilterMonitorEvent monitorEvent;
                events.resize(1);
                monitorEvent.set<DemuxFilterMonitorEvent::Tag::cid>(1);
                events[0].set<DemuxFilterEvent::monitorEvent>(monitorEvent);
                mCallback->onFilterEvent(events);
            } else {
                return ::ndk::ScopedAStatus::fromServiceSpecificError(
                        static_cast<int32_t>(Result::INVALID_STATE));
            }
        }
    }

    return ::ndk::ScopedAStatus::ok();
}

bool Filter::createFilterMQ() {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    std::unique_ptr<FilterMQ> tmpFilterMQ =
            std::unique_ptr<FilterMQ>(new (std::nothrow) FilterMQ(mBufferSize, true));
    if (!tmpFilterMQ->isValid()) {
        ALOGW("[Filter] Failed to create FMQ of filter with id: %" PRIu64, mFilterId);
        return false;
    }

    mFilterMQ = std::move(tmpFilterMQ);

    if (EventFlag::createEventFlag(mFilterMQ->getEventFlagWord(), &mFilterEventsFlag) !=
        ::android::OK) {
        return false;
    }

    return true;
}

::ndk::ScopedAStatus Filter::startFilterLoop() {
    pthread_create(&mFilterThread, NULL, __threadLoopFilter, this);
    pthread_setname_np(mFilterThread, "filter_waiting_loop");
    return ::ndk::ScopedAStatus::ok();
}

void* Filter::__threadLoopFilter(void* user) {
    Filter* const self = static_cast<Filter*>(user);
    self->filterThreadLoop();
    return 0;
}

void Filter::filterThreadLoop() {
    if (!mFilterThreadRunning) {
        return;
    }
    std::lock_guard<std::mutex> lock(mFilterThreadLock);
    ALOGD("[Filter] filter %" PRIu64 " threadLoop start.", mFilterId);

    // For the first time of filter output, implementation needs to send the filter
    // Event Callback without waiting for the DATA_CONSUMED to init the process.
    while (mFilterThreadRunning) {
        if (mFilterEvents.size() == 0) {
            if (DEBUG_FILTER) {
                ALOGD("[Filter] wait for filter data output.");
            }
            usleep(1000 * 1000);
            continue;
        }

        // After successfully write, send a callback and wait for the read to be done
        if (mCallback != nullptr) {
            if (mConfigured) {
                vector<DemuxFilterEvent> startEvent;
                startEvent.resize(1);
                startEvent[0].set<DemuxFilterEvent::Tag::startId>(mStartId++);
                mCallback->onFilterEvent(startEvent);
                mConfigured = false;
            }
            mCallback->onFilterEvent(mFilterEvents);
        } else {
            ALOGD("[Filter] filter callback is not configured yet.");
            mFilterThreadRunning = false;
            return;
        }

        mFilterEvents.resize(0);
        mFilterStatus = DemuxFilterStatus::DATA_READY;
        if (mCallback != nullptr) {
            mCallback->onFilterStatus(mFilterStatus);
        }
        break;
    }

    while (mFilterThreadRunning) {
        uint32_t efState = 0;
        // We do not wait for the last round of written data to be read to finish the thread
        // because the VTS can verify the reading itself.
        for (int i = 0; i < SECTION_WRITE_COUNT; i++) {
            if (!mFilterThreadRunning) {
                break;
            }
            while (mFilterThreadRunning && mIsUsingFMQ) {
                ::android::status_t status = mFilterEventsFlag->wait(
                        static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_CONSUMED), &efState,
                        WAIT_TIMEOUT, true /* retry on spurious wake */);
                if (status != ::android::OK) {
                    ALOGD("[Filter] wait for data consumed");
                    continue;
                }
                break;
            }

            maySendFilterStatusCallback();

            while (mFilterThreadRunning) {
                std::lock_guard<std::mutex> lock(mFilterEventsLock);
                if (mFilterEvents.size() == 0) {
                    continue;
                }
                // After successfully write, send a callback and wait for the read to be done
                if (mCallback != nullptr) {
                    mCallback->onFilterEvent(mFilterEvents);
                }
                mFilterEvents.resize(0);
                break;
            }
            // We do not wait for the last read to be done
            // VTS can verify the read result itself.
            if (i == SECTION_WRITE_COUNT - 1) {
                ALOGD("[Filter] filter %" PRIu64 " writing done. Ending thread", mFilterId);
                break;
            }
        }
        break;
    }
    ALOGD("[Filter] filter thread ended.");
}

void Filter::freeSharedAvHandle() {
    if (!mIsMediaFilter) {
        return;
    }
    native_handle_close(mSharedAvMemHandle);
    native_handle_delete(mSharedAvMemHandle);
    mSharedAvMemHandle = nullptr;
}

void Filter::maySendFilterStatusCallback() {
    if (!mIsUsingFMQ) {
        return;
    }
    std::lock_guard<std::mutex> lock(mFilterStatusLock);
    int availableToRead = mFilterMQ->availableToRead();
    int availableToWrite = mFilterMQ->availableToWrite();
    int fmqSize = mFilterMQ->getQuantumCount();

    DemuxFilterStatus newStatus = checkFilterStatusChange(
            availableToWrite, availableToRead, ceil(fmqSize * 0.75), ceil(fmqSize * 0.25));
    if (mFilterStatus != newStatus) {
        if (mCallback != nullptr) {
            mCallback->onFilterStatus(newStatus);
        }
        mFilterStatus = newStatus;
    }
}

DemuxFilterStatus Filter::checkFilterStatusChange(uint32_t availableToWrite,
                                                  uint32_t availableToRead, uint32_t highThreshold,
                                                  uint32_t lowThreshold) {
    if (availableToWrite == 0) {
        return DemuxFilterStatus::OVERFLOW;
    } else if (availableToRead > highThreshold) {
        return DemuxFilterStatus::HIGH_WATER;
    } else if (availableToRead < lowThreshold) {
        return DemuxFilterStatus::LOW_WATER;
    }
    return mFilterStatus;
}

uint16_t Filter::getTpid() {
    return mTpid;
}

void Filter::updateFilterOutput(vector<int8_t>& data) {
    std::lock_guard<std::mutex> lock(mFilterOutputLock);
    mFilterOutput.insert(mFilterOutput.end(), data.begin(), data.end());
}

void Filter::updatePts(uint64_t pts) {
    std::lock_guard<std::mutex> lock(mFilterOutputLock);
    mPts = pts;
}

void Filter::updateRecordOutput(vector<int8_t>& data) {
    std::lock_guard<std::mutex> lock(mRecordFilterOutputLock);
    mRecordFilterOutput.insert(mRecordFilterOutput.end(), data.begin(), data.end());
}

::ndk::ScopedAStatus Filter::startFilterHandler() {
    std::lock_guard<std::mutex> lock(mFilterOutputLock);
    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            switch (mType.subType.get<DemuxFilterSubType::Tag::tsFilterType>()) {
                case DemuxTsFilterType::UNDEFINED:
                    break;
                case DemuxTsFilterType::SECTION:
                    startSectionFilterHandler();
                    break;
                case DemuxTsFilterType::PES:
                    startPesFilterHandler();
                    break;
                case DemuxTsFilterType::TS:
                    startTsFilterHandler();
                    break;
                case DemuxTsFilterType::AUDIO:
                case DemuxTsFilterType::VIDEO:
                    startMediaFilterHandler();
                    break;
                case DemuxTsFilterType::PCR:
                    startPcrFilterHandler();
                    break;
                case DemuxTsFilterType::TEMI:
                    startTemiFilterHandler();
                    break;
                default:
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
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::startSectionFilterHandler() {
    if (mFilterOutput.empty()) {
        return ::ndk::ScopedAStatus::ok();
    }
    if (!writeSectionsAndCreateEvent(mFilterOutput)) {
        ALOGD("[Filter] filter %" PRIu64 " fails to write into FMQ. Ending thread", mFilterId);
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }

    mFilterOutput.clear();

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::startPesFilterHandler() {
    std::lock_guard<std::mutex> lock(mFilterEventsLock);
    if (mFilterOutput.empty()) {
        return ::ndk::ScopedAStatus::ok();
    }

    for (int i = 0; i < mFilterOutput.size(); i += 188) {
        if (mPesSizeLeft == 0) {
            uint32_t prefix = (mFilterOutput[i + 4] << 16) | (mFilterOutput[i + 5] << 8) |
                              mFilterOutput[i + 6];
            if (DEBUG_FILTER) {
                ALOGD("[Filter] prefix %d", prefix);
            }
            if (prefix == 0x000001) {
                // TODO handle mulptiple Pes filters
                mPesSizeLeft = (mFilterOutput[i + 8] << 8) | mFilterOutput[i + 9];
                mPesSizeLeft += 6;
                if (DEBUG_FILTER) {
                    ALOGD("[Filter] pes data length %d", mPesSizeLeft);
                }
            } else {
                continue;
            }
        }

        int endPoint = min(184, mPesSizeLeft);
        // append data and check size
        vector<int8_t>::const_iterator first = mFilterOutput.begin() + i + 4;
        vector<int8_t>::const_iterator last = mFilterOutput.begin() + i + 4 + endPoint;
        mPesOutput.insert(mPesOutput.end(), first, last);
        // size does not match then continue
        mPesSizeLeft -= endPoint;
        if (DEBUG_FILTER) {
            ALOGD("[Filter] pes data left %d", mPesSizeLeft);
        }
        if (mPesSizeLeft > 0) {
            continue;
        }
        // size match then create event
        if (!writeDataToFilterMQ(mPesOutput)) {
            ALOGD("[Filter] pes data write failed");
            mFilterOutput.clear();
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
        }
        maySendFilterStatusCallback();
        DemuxFilterPesEvent pesEvent;
        pesEvent = {
                // temp dump meta data
                .streamId = static_cast<char16_t>(mPesOutput[3]),
                .dataLength = static_cast<char16_t>(mPesOutput.size()),
        };
        if (DEBUG_FILTER) {
            ALOGD("[Filter] assembled pes data length %d", pesEvent.dataLength);
        }

        int size = mFilterEvents.size();
        mFilterEvents.resize(size + 1);
        mFilterEvents[size].set<DemuxFilterEvent::Tag::pes>(pesEvent);
        mPesOutput.clear();
    }

    mFilterOutput.clear();

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::startTsFilterHandler() {
    // TODO handle starting TS filter
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::startMediaFilterHandler() {
    std::lock_guard<std::mutex> lock(mFilterEventsLock);
    if (mFilterOutput.empty()) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus result;
    if (mPts) {
        result = createMediaFilterEventWithIon(mFilterOutput);
        if (result.isOk()) {
            mFilterOutput.clear();
        }
        return result;
    }

    for (int i = 0; i < mFilterOutput.size(); i += 188) {
        if (mPesSizeLeft == 0) {
            uint32_t prefix = (mFilterOutput[i + 4] << 16) | (mFilterOutput[i + 5] << 8) |
                              mFilterOutput[i + 6];
            if (DEBUG_FILTER) {
                ALOGD("[Filter] prefix %d", prefix);
            }
            if (prefix == 0x000001) {
                // TODO handle mulptiple Pes filters
                mPesSizeLeft = (mFilterOutput[i + 8] << 8) | mFilterOutput[i + 9];
                mPesSizeLeft += 6;
                if (DEBUG_FILTER) {
                    ALOGD("[Filter] pes data length %d", mPesSizeLeft);
                }
            } else {
                continue;
            }
        }

        int endPoint = min(184, mPesSizeLeft);
        // append data and check size
        vector<int8_t>::const_iterator first = mFilterOutput.begin() + i + 4;
        vector<int8_t>::const_iterator last = mFilterOutput.begin() + i + 4 + endPoint;
        mPesOutput.insert(mPesOutput.end(), first, last);
        // size does not match then continue
        mPesSizeLeft -= endPoint;
        if (DEBUG_FILTER) {
            ALOGD("[Filter] pes data left %d", mPesSizeLeft);
        }
        if (mPesSizeLeft > 0 || mAvBufferCopyCount++ < 10) {
            continue;
        }

        result = createMediaFilterEventWithIon(mPesOutput);
        if (result.isOk()) {
            return result;
        }
    }

    mFilterOutput.clear();

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::createMediaFilterEventWithIon(vector<int8_t>& output) {
    if (mUsingSharedAvMem) {
        if (mSharedAvMemHandle == nullptr) {
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::UNKNOWN_ERROR));
        }
        return createShareMemMediaEvents(output);
    }

    return createIndependentMediaEvents(output);
}

::ndk::ScopedAStatus Filter::startRecordFilterHandler() {
    std::lock_guard<std::mutex> lock(mRecordFilterOutputLock);
    if (mRecordFilterOutput.empty()) {
        return ::ndk::ScopedAStatus::ok();
    }

    if (mDvr == nullptr || !mDvr->writeRecordFMQ(mRecordFilterOutput)) {
        ALOGD("[Filter] dvr fails to write into record FMQ.");
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }

    DemuxFilterTsRecordEvent recordEvent;
    recordEvent = {
            .byteNumber = static_cast<int64_t>(mRecordFilterOutput.size()),
            .pts = (mPts == 0) ? static_cast<int64_t>(time(NULL)) * 900000 : mPts,
            .firstMbInSlice = 0,  // random address
    };

    int size;
    size = mFilterEvents.size();
    mFilterEvents.resize(size + 1);
    mFilterEvents[size].set<DemuxFilterEvent::Tag::tsRecord>(recordEvent);

    mRecordFilterOutput.clear();
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::startPcrFilterHandler() {
    // TODO handle starting PCR filter
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::startTemiFilterHandler() {
    // TODO handle starting TEMI filter
    return ::ndk::ScopedAStatus::ok();
}

bool Filter::writeSectionsAndCreateEvent(vector<int8_t>& data) {
    // TODO check how many sections has been read
    ALOGD("[Filter] section handler");
    std::lock_guard<std::mutex> lock(mFilterEventsLock);
    if (!writeDataToFilterMQ(data)) {
        return false;
    }
    int size = mFilterEvents.size();
    mFilterEvents.resize(size + 1);
    DemuxFilterSectionEvent secEvent;
    secEvent = {
            // temp dump meta data
            .tableId = 0,
            .version = 1,
            .sectionNum = 1,
            .dataLength = static_cast<char16_t>(data.size()),
    };
    mFilterEvents[size].set<DemuxFilterEvent::Tag::section>(secEvent);
    return true;
}

bool Filter::writeDataToFilterMQ(const std::vector<int8_t>& data) {
    std::lock_guard<std::mutex> lock(mWriteLock);
    if (mFilterMQ->write(data.data(), data.size())) {
        return true;
    }
    return false;
}

void Filter::attachFilterToRecord(const std::shared_ptr<Dvr> dvr) {
    mDvr = dvr;
}

void Filter::detachFilterFromRecord() {
    mDvr = nullptr;
}

int Filter::createAvIonFd(int size) {
    // Create an DMA-BUF fd and allocate an av fd mapped to a buffer to it.
    auto buffer_allocator = std::make_unique<BufferAllocator>();
    if (!buffer_allocator) {
        ALOGE("[Filter] Unable to create BufferAllocator object");
        return -1;
    }
    int av_fd = -1;
    av_fd = buffer_allocator->Alloc("system-uncached", size);
    if (av_fd < 0) {
        ALOGE("[Filter] Failed to create av fd %d", errno);
        return -1;
    }
    return av_fd;
}

uint8_t* Filter::getIonBuffer(int fd, int size) {
    uint8_t* avBuf = static_cast<uint8_t*>(
            mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 /*offset*/));
    if (avBuf == MAP_FAILED) {
        ALOGE("[Filter] fail to allocate buffer %d", errno);
        return NULL;
    }
    return avBuf;
}

native_handle_t* Filter::createNativeHandle(int fd) {
    native_handle_t* nativeHandle;
    if (fd < 0) {
        nativeHandle = native_handle_create(/*numFd*/ 0, 0);
    } else {
        // Create a native handle to pass the av fd via the callback event.
        nativeHandle = native_handle_create(/*numFd*/ 1, 0);
    }
    if (nativeHandle == NULL) {
        ALOGE("[Filter] Failed to create native_handle %d", errno);
        return NULL;
    }
    if (nativeHandle->numFds > 0) {
        nativeHandle->data[0] = dup(fd);
    }
    return nativeHandle;
}

::ndk::ScopedAStatus Filter::createIndependentMediaEvents(vector<int8_t>& output) {
    int av_fd = createAvIonFd(output.size());
    if (av_fd == -1) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }
    // copy the filtered data to the buffer
    uint8_t* avBuffer = getIonBuffer(av_fd, output.size());
    if (avBuffer == NULL) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }
    memcpy(avBuffer, output.data(), output.size() * sizeof(uint8_t));

    native_handle_t* nativeHandle = createNativeHandle(av_fd);
    if (nativeHandle == NULL) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }

    // Create a dataId and add a <dataId, av_fd> pair into the dataId2Avfd map
    uint64_t dataId = mLastUsedDataId++ /*createdUID*/;
    mDataId2Avfd[dataId] = dup(av_fd);

    // Create mediaEvent and send callback
    int size = mFilterEvents.size();
    mFilterEvents.resize(size + 1);

    mFilterEvents[size] = DemuxFilterEvent::make<DemuxFilterEvent::Tag::media>();
    mFilterEvents[size].get<DemuxFilterEvent::Tag::media>().avMemory =
            ::android::dupToAidl(nativeHandle);
    mFilterEvents[size].get<DemuxFilterEvent::Tag::media>().dataLength =
            static_cast<int32_t>(output.size());
    mFilterEvents[size].get<DemuxFilterEvent::Tag::media>().avDataId = static_cast<int64_t>(dataId);
    if (mPts) {
        mFilterEvents[size].get<DemuxFilterEvent::Tag::media>().pts = mPts;
        mPts = 0;
    }

    // Clear and log
    native_handle_close(nativeHandle);
    native_handle_delete(nativeHandle);
    output.clear();
    mAvBufferCopyCount = 0;
    if (DEBUG_FILTER) {
        ALOGD("[Filter] av data length %d", static_cast<int32_t>(output.size()));
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::createShareMemMediaEvents(vector<int8_t>& output) {
    // copy the filtered data to the shared buffer
    uint8_t* sharedAvBuffer =
            getIonBuffer(mSharedAvMemHandle->data[0], output.size() + mSharedAvMemOffset);
    if (sharedAvBuffer == NULL) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }
    memcpy(sharedAvBuffer + mSharedAvMemOffset, output.data(), output.size() * sizeof(uint8_t));

    // Create a memory handle with numFds == 0
    native_handle_t* nativeHandle = createNativeHandle(-1);
    if (nativeHandle == NULL) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }

    // Create mediaEvent and send callback
    int size = mFilterEvents.size();
    mFilterEvents.resize(size + 1);
    mFilterEvents[size] = DemuxFilterEvent::make<DemuxFilterEvent::Tag::media>();
    mFilterEvents[size].get<DemuxFilterEvent::Tag::media>().avMemory =
            ::android::dupToAidl(nativeHandle);
    mFilterEvents[size].get<DemuxFilterEvent::Tag::media>().offset =
            static_cast<int32_t>(mSharedAvMemOffset);
    mFilterEvents[size].get<DemuxFilterEvent::Tag::media>().dataLength =
            static_cast<int32_t>(output.size());
    if (mPts) {
        mFilterEvents[size].get<DemuxFilterEvent::Tag::media>().pts = mPts;
        mPts = 0;
    }
    mSharedAvMemOffset += output.size();

    // Clear and log
    native_handle_close(nativeHandle);
    native_handle_delete(nativeHandle);
    output.clear();
    if (DEBUG_FILTER) {
        ALOGD("[Filter] shared av data length %d", static_cast<int32_t>(output.size()));
    }
    return ::ndk::ScopedAStatus::ok();
}

bool Filter::sameFile(int fd1, int fd2) {
    struct stat stat1, stat2;
    if (fstat(fd1, &stat1) < 0 || fstat(fd2, &stat2) < 0) {
        return false;
    }
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

void Filter::createMediaEvent(vector<DemuxFilterEvent>& events) {
    AudioExtraMetaData audio;
    events.resize(1);

    audio.adFade = 1;
    audio.adPan = 2;
    audio.versionTextTag = 3;
    audio.adGainCenter = 4;
    audio.adGainFront = 5;
    audio.adGainSurround = 6;

    events[0] = DemuxFilterEvent::make<DemuxFilterEvent::Tag::media>();
    events[0].get<DemuxFilterEvent::Tag::media>().streamId = 1;
    events[0].get<DemuxFilterEvent::Tag::media>().isPtsPresent = true;
    events[0].get<DemuxFilterEvent::Tag::media>().dataLength = 3;
    events[0].get<DemuxFilterEvent::Tag::media>().offset = 4;
    events[0].get<DemuxFilterEvent::Tag::media>().isSecureMemory = true;
    events[0].get<DemuxFilterEvent::Tag::media>().mpuSequenceNumber = 6;
    events[0].get<DemuxFilterEvent::Tag::media>().isPesPrivateData = true;
    events[0]
            .get<DemuxFilterEvent::Tag::media>()
            .extraMetaData.set<DemuxFilterMediaEventExtraMetaData::Tag::audio>(audio);

    int av_fd = createAvIonFd(BUFFER_SIZE_16M);
    if (av_fd == -1) {
        return;
    }

    native_handle_t* nativeHandle = createNativeHandle(av_fd);
    if (nativeHandle == nullptr) {
        ::close(av_fd);
        ALOGE("[Filter] Failed to create native_handle %d", errno);
        return;
    }

    // Create a dataId and add a <dataId, av_fd> pair into the dataId2Avfd map
    uint64_t dataId = mLastUsedDataId++ /*createdUID*/;
    mDataId2Avfd[dataId] = dup(av_fd);

    events[0].get<DemuxFilterEvent::Tag::media>().avDataId = static_cast<int64_t>(dataId);
    events[0].get<DemuxFilterEvent::Tag::media>().avMemory = ::android::dupToAidl(nativeHandle);

    native_handle_close(nativeHandle);
    native_handle_delete(nativeHandle);
}

void Filter::createTsRecordEvent(vector<DemuxFilterEvent>& events) {
    events.resize(2);

    DemuxPid pid;
    DemuxFilterScIndexMask mask;
    DemuxFilterTsRecordEvent tsRecord1;
    pid.set<DemuxPid::Tag::tPid>(1);
    mask.set<DemuxFilterScIndexMask::Tag::scIndex>(1);
    tsRecord1.pid = pid;
    tsRecord1.tsIndexMask = 1;
    tsRecord1.scIndexMask = mask;
    tsRecord1.byteNumber = 2;

    DemuxFilterTsRecordEvent tsRecord2;
    tsRecord2.pts = 1;
    tsRecord2.firstMbInSlice = 2;  // random address

    events[0].set<DemuxFilterEvent::Tag::tsRecord>(tsRecord1);
    events[1].set<DemuxFilterEvent::Tag::tsRecord>(tsRecord2);
}

void Filter::createMmtpRecordEvent(vector<DemuxFilterEvent>& events) {
    events.resize(2);

    DemuxFilterMmtpRecordEvent mmtpRecord1;
    mmtpRecord1.scHevcIndexMask = 1;
    mmtpRecord1.byteNumber = 2;

    DemuxFilterMmtpRecordEvent mmtpRecord2;
    mmtpRecord2.pts = 1;
    mmtpRecord2.mpuSequenceNumber = 2;
    mmtpRecord2.firstMbInSlice = 3;
    mmtpRecord2.tsIndexMask = 4;

    events[0].set<DemuxFilterEvent::Tag::mmtpRecord>(mmtpRecord1);
    events[1].set<DemuxFilterEvent::Tag::mmtpRecord>(mmtpRecord2);
}

void Filter::createSectionEvent(vector<DemuxFilterEvent>& events) {
    events.resize(1);

    DemuxFilterSectionEvent section;
    section.tableId = 1;
    section.version = 2;
    section.sectionNum = 3;
    section.dataLength = 0;

    events[0].set<DemuxFilterEvent::Tag::section>(section);
}

void Filter::createPesEvent(vector<DemuxFilterEvent>& events) {
    events.resize(1);

    DemuxFilterPesEvent pes;
    pes.streamId = 1;
    pes.dataLength = 1;
    pes.mpuSequenceNumber = 2;

    events[0].set<DemuxFilterEvent::Tag::pes>(pes);
}

void Filter::createDownloadEvent(vector<DemuxFilterEvent>& events) {
    events.resize(1);

    DemuxFilterDownloadEvent download;
    download.itemId = 1;
    download.mpuSequenceNumber = 2;
    download.itemFragmentIndex = 3;
    download.lastItemFragmentIndex = 4;
    download.dataLength = 0;

    events[0].set<DemuxFilterEvent::Tag::download>(download);
}

void Filter::createIpPayloadEvent(vector<DemuxFilterEvent>& events) {
    events.resize(1);

    DemuxFilterIpPayloadEvent ipPayload;
    ipPayload.dataLength = 0;

    events[0].set<DemuxFilterEvent::Tag::ipPayload>(ipPayload);
}

void Filter::createTemiEvent(vector<DemuxFilterEvent>& events) {
    events.resize(1);

    DemuxFilterTemiEvent temi;
    temi.pts = 1;
    temi.descrTag = 2;
    temi.descrData = {3};

    events[0].set<DemuxFilterEvent::Tag::temi>(temi);
}

void Filter::createMonitorEvent(vector<DemuxFilterEvent>& events) {
    events.resize(1);

    DemuxFilterMonitorEvent monitor;
    monitor.set<DemuxFilterMonitorEvent::Tag::scramblingStatus>(ScramblingStatus::SCRAMBLED);
    events[0].set<DemuxFilterEvent::Tag::monitorEvent>(monitor);
}

void Filter::createRestartEvent(vector<DemuxFilterEvent>& events) {
    events.resize(1);

    events[0].set<DemuxFilterEvent::Tag::startId>(1);
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
