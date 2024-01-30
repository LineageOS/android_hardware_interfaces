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
#include <inttypes.h>
#include <utils/Log.h>

#include "Filter.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

#define WAIT_TIMEOUT 3000000000

FilterCallbackScheduler::FilterCallbackScheduler(const std::shared_ptr<IFilterCallback>& cb)
    : mCallback(cb),
      mIsConditionMet(false),
      mDataLength(0),
      mTimeDelayInMs(0),
      mDataSizeDelayInBytes(0) {
    start();
}

FilterCallbackScheduler::~FilterCallbackScheduler() {
    stop();
}

void FilterCallbackScheduler::onFilterEvent(DemuxFilterEvent&& event) {
    std::unique_lock<std::mutex> lock(mLock);
    mCallbackBuffer.push_back(std::move(event));
    mDataLength += getDemuxFilterEventDataLength(event);

    if (isDataSizeDelayConditionMetLocked()) {
        mIsConditionMet = true;
        // unlock, so thread is not immediately blocked when it is notified.
        lock.unlock();
        mCv.notify_all();
    }
}

void FilterCallbackScheduler::onFilterStatus(const DemuxFilterStatus& status) {
    if (mCallback) {
        mCallback->onFilterStatus(status);
    }
}

void FilterCallbackScheduler::flushEvents() {
    std::unique_lock<std::mutex> lock(mLock);
    mCallbackBuffer.clear();
    mDataLength = 0;
}

void FilterCallbackScheduler::setTimeDelayHint(int timeDelay) {
    std::unique_lock<std::mutex> lock(mLock);
    mTimeDelayInMs = timeDelay;
    // always notify condition variable to update timeout
    mIsConditionMet = true;
    lock.unlock();
    mCv.notify_all();
}

void FilterCallbackScheduler::setDataSizeDelayHint(int dataSizeDelay) {
    std::unique_lock<std::mutex> lock(mLock);
    mDataSizeDelayInBytes = dataSizeDelay;
    if (isDataSizeDelayConditionMetLocked()) {
        mIsConditionMet = true;
        lock.unlock();
        mCv.notify_all();
    }
}

bool FilterCallbackScheduler::hasCallbackRegistered() const {
    return mCallback != nullptr;
}

void FilterCallbackScheduler::start() {
    mIsRunning = true;
    mCallbackThread = std::thread(&FilterCallbackScheduler::threadLoop, this);
}

void FilterCallbackScheduler::stop() {
    mIsRunning = false;
    if (mCallbackThread.joinable()) {
        {
            std::lock_guard<std::mutex> lock(mLock);
            mIsConditionMet = true;
        }
        mCv.notify_all();
        mCallbackThread.join();
    }
}

void FilterCallbackScheduler::threadLoop() {
    while (mIsRunning) {
        threadLoopOnce();
    }
}

void FilterCallbackScheduler::threadLoopOnce() {
    std::unique_lock<std::mutex> lock(mLock);
    if (mTimeDelayInMs > 0) {
        // Note: predicate protects from lost and spurious wakeups
        mCv.wait_for(lock, std::chrono::milliseconds(mTimeDelayInMs),
                     [this] { return mIsConditionMet; });
    } else {
        // Note: predicate protects from lost and spurious wakeups
        mCv.wait(lock, [this] { return mIsConditionMet; });
    }
    mIsConditionMet = false;

    // condition_variable wait locks mutex on timeout / notify
    // Note: if stop() has been called in the meantime, do not send more filter
    // events.
    if (mIsRunning && !mCallbackBuffer.empty()) {
        if (mCallback) {
            mCallback->onFilterEvent(mCallbackBuffer);
        }
        mCallbackBuffer.clear();
        mDataLength = 0;
    }
}

// mLock needs to be held to call this function
bool FilterCallbackScheduler::isDataSizeDelayConditionMetLocked() {
    if (mDataSizeDelayInBytes == 0) {
        // Data size delay is disabled.
        if (mTimeDelayInMs == 0) {
            // Events should only be sent immediately if time delay is disabled
            // as well.
            return true;
        }
        return false;
    }

    // Data size delay is enabled.
    return mDataLength >= mDataSizeDelayInBytes;
}

int FilterCallbackScheduler::getDemuxFilterEventDataLength(const DemuxFilterEvent& event) {
    // there is a risk that dataLength could be a negative value, but it
    // *should* be safe to assume that it is always positive.
    switch (event.getTag()) {
        case DemuxFilterEvent::Tag::section:
            return event.get<DemuxFilterEvent::Tag::section>().dataLength;
        case DemuxFilterEvent::Tag::media:
            return event.get<DemuxFilterEvent::Tag::media>().dataLength;
        case DemuxFilterEvent::Tag::pes:
            return event.get<DemuxFilterEvent::Tag::pes>().dataLength;
        case DemuxFilterEvent::Tag::download:
            return event.get<DemuxFilterEvent::Tag::download>().dataLength;
        case DemuxFilterEvent::Tag::ipPayload:
            return event.get<DemuxFilterEvent::Tag::ipPayload>().dataLength;

        case DemuxFilterEvent::Tag::tsRecord:
        case DemuxFilterEvent::Tag::mmtpRecord:
        case DemuxFilterEvent::Tag::temi:
        case DemuxFilterEvent::Tag::monitorEvent:
        case DemuxFilterEvent::Tag::startId:
            // these events do not include a payload and should therefore return
            // 0.
            // do not add a default option, so this will not compile when new types
            // are added.
            return 0;
    }
}

Filter::Filter(DemuxFilterType type, int64_t filterId, uint32_t bufferSize,
               const std::shared_ptr<IFilterCallback>& cb, std::shared_ptr<Demux> demux)
    : mDemux(demux),
      mCallbackScheduler(cb),
      mFilterId(filterId),
      mBufferSize(bufferSize),
      mType(type) {
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
    close();
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

::ndk::ScopedAStatus Filter::setDelayHint(const FilterDelayHint& in_hint) {
    if (mIsMediaFilter) {
        // delay hint is not supported for media filters
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNAVAILABLE));
    }

    ALOGV("%s", __FUNCTION__);
    if (in_hint.hintValue < 0) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    switch (in_hint.hintType) {
        case FilterDelayHintType::TIME_DELAY_IN_MS:
            mCallbackScheduler.setTimeDelayHint(in_hint.hintValue);
            break;
        case FilterDelayHintType::DATA_SIZE_DELAY_IN_BYTES:
            mCallbackScheduler.setDataSizeDelayHint(in_hint.hintValue);
            break;
        default:
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

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
    std::vector<DemuxFilterEvent> events;

    mFilterCount += 1;
    mDemux->setIptvThreadRunning(true);

    // All the filter event callbacks in start are for testing purpose.
    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            createMediaEvent(events, false);
            createMediaEvent(events, true);
            createTsRecordEvent(events);
            createTemiEvent(events);
            break;
        case DemuxFilterMainType::MMTP:
            createDownloadEvent(events);
            createMmtpRecordEvent(events);
            break;
        case DemuxFilterMainType::IP:
            createSectionEvent(events);
            createIpPayloadEvent(events);
            break;
        case DemuxFilterMainType::TLV:
            createMonitorEvent(events);
            break;
        case DemuxFilterMainType::ALP:
            createMonitorEvent(events);
            break;
        default:
            break;
    }

    for (auto&& event : events) {
        mCallbackScheduler.onFilterEvent(std::move(event));
    }

    return startFilterLoop();
}

::ndk::ScopedAStatus Filter::stop() {
    ALOGV("%s", __FUNCTION__);

    mFilterCount -= 1;
    if (mFilterCount == 0) {
        mDemux->setIptvThreadRunning(false);
    }

    mFilterThreadRunning = false;
    if (mFilterThread.joinable()) {
        mFilterThread.join();
    }

    mCallbackScheduler.flushEvents();

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

    stop();

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
        *_aidl_return = BUFFER_SIZE;
        mUsingSharedAvMem = true;
        return ::ndk::ScopedAStatus::ok();
    }

    int av_fd = createAvIonFd(BUFFER_SIZE);
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
    *_aidl_return = BUFFER_SIZE;
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
            if (mCallbackScheduler.hasCallbackRegistered()) {
                // Assuming current status is always NOT_SCRAMBLED
                auto monitorEvent = DemuxFilterMonitorEvent::make<
                        DemuxFilterMonitorEvent::Tag::scramblingStatus>(
                        ScramblingStatus::NOT_SCRAMBLED);
                auto event =
                        DemuxFilterEvent::make<DemuxFilterEvent::Tag::monitorEvent>(monitorEvent);
                mCallbackScheduler.onFilterEvent(std::move(event));
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
            if (mCallbackScheduler.hasCallbackRegistered()) {
                // Return random cid
                auto monitorEvent =
                        DemuxFilterMonitorEvent::make<DemuxFilterMonitorEvent::Tag::cid>(1);
                auto event =
                        DemuxFilterEvent::make<DemuxFilterEvent::Tag::monitorEvent>(monitorEvent);
                mCallbackScheduler.onFilterEvent(std::move(event));
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
    mFilterThread = std::thread(&Filter::filterThreadLoop, this);
    return ::ndk::ScopedAStatus::ok();
}

void Filter::filterThreadLoop() {
    if (!mFilterThreadRunning) {
        return;
    }

    ALOGD("[Filter] filter %" PRIu64 " threadLoop start.", mFilterId);

    ALOGI("IPTV DVR Playback status on Filter: %d", mIptvDvrPlaybackStatus);

    // For the first time of filter output, implementation needs to send the filter
    // Event Callback without waiting for the DATA_CONSUMED to init the process.
    while (mFilterThreadRunning) {
        std::unique_lock<std::mutex> lock(mFilterEventsLock);
        if (mFilterEvents.size() == 0) {
            lock.unlock();
            if (DEBUG_FILTER) {
                ALOGD("[Filter] wait for filter data output.");
            }
            usleep(1000 * 1000);
            continue;
        }

        // After successfully write, send a callback and wait for the read to be done
        if (mCallbackScheduler.hasCallbackRegistered()) {
            if (mConfigured) {
                auto startEvent =
                        DemuxFilterEvent::make<DemuxFilterEvent::Tag::startId>(mStartId++);
                mCallbackScheduler.onFilterEvent(std::move(startEvent));
                mConfigured = false;
            }

            // lock is still being held
            for (auto&& event : mFilterEvents) {
                mCallbackScheduler.onFilterEvent(std::move(event));
            }
        } else {
            ALOGD("[Filter] filter callback is not configured yet.");
            mFilterThreadRunning = false;
            return;
        }

        mFilterEvents.clear();
        mFilterStatus = DemuxFilterStatus::DATA_READY;
        mCallbackScheduler.onFilterStatus(mFilterStatus);
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
                for (auto&& event : mFilterEvents) {
                    mCallbackScheduler.onFilterEvent(std::move(event));
                }
                mFilterEvents.clear();
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

binder_status_t Filter::dump(int fd, const char** /* args */, uint32_t /* numArgs */) {
    dprintf(fd, "    Filter %" PRIu64 ":\n", mFilterId);
    dprintf(fd, "      Main type: %d\n", mType.mainType);
    dprintf(fd, "      mIsMediaFilter: %d\n", mIsMediaFilter);
    dprintf(fd, "      mIsPcrFilter: %d\n", mIsPcrFilter);
    dprintf(fd, "      mIsRecordFilter: %d\n", mIsRecordFilter);
    dprintf(fd, "      mIsUsingFMQ: %d\n", mIsUsingFMQ);
    dprintf(fd, "      mFilterThreadRunning: %d\n", (bool)mFilterThreadRunning);
    return STATUS_OK;
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
        mCallbackScheduler.onFilterStatus(newStatus);
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
    } else if (availableToRead == 0) {
        return DemuxFilterStatus::NO_DATA;
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
                mPesSizeLeft = (static_cast<uint8_t>(mFilterOutput[i + 8]) << 8) |
                               static_cast<uint8_t>(mFilterOutput[i + 9]);
                mPesSizeLeft += 6;
                if (DEBUG_FILTER) {
                    ALOGD("[Filter] pes data length %d", mPesSizeLeft);
                }
            } else {
                continue;
            }
        }

        uint32_t endPoint = min(184u, mPesSizeLeft);
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
                .streamId = static_cast<int32_t>(mPesOutput[3]),
                .dataLength = static_cast<int32_t>(mPesOutput.size()),
        };
        if (DEBUG_FILTER) {
            ALOGD("[Filter] assembled pes data length %d", pesEvent.dataLength);
        }

        {
            std::lock_guard<std::mutex> lock(mFilterEventsLock);
            mFilterEvents.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::pes>(pesEvent));
        }

        mPesOutput.clear();
    }

    mFilterOutput.clear();

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Filter::startTsFilterHandler() {
    // TODO handle starting TS filter
    return ::ndk::ScopedAStatus::ok();
}

// Read PES (Packetized Elementary Stream) Packets from TransportStreams
// as defined in ISO/IEC 13818-1 Section 2.4.3.6. Create MediaEvents
// containing only their data without TS or PES headers.
::ndk::ScopedAStatus Filter::startMediaFilterHandler() {
    if (mFilterOutput.empty()) {
        return ::ndk::ScopedAStatus::ok();
    }

    // mPts being set before our MediaFilterHandler begins indicates that all
    // metadata has already been handled. We can therefore create an event
    // with the existing data. This method is used when processing ES files.
    ::ndk::ScopedAStatus result;
    if (mPts) {
        result = createMediaFilterEventWithIon(mFilterOutput);
        if (result.isOk()) {
            mFilterOutput.clear();
        }
        return result;
    }

    for (int i = 0; i < mFilterOutput.size(); i += 188) {
        // Every packet has a 4 Byte TS Header preceding it
        uint32_t headerSize = 4;

        if (mPesSizeLeft == 0) {
            // Packet Start Code Prefix is defined as the first 3 bytes of
            // the PES Header and should always have the value 0x000001
            uint32_t prefix = (static_cast<uint8_t>(mFilterOutput[i + 4]) << 16) |
                              (static_cast<uint8_t>(mFilterOutput[i + 5]) << 8) |
                              static_cast<uint8_t>(mFilterOutput[i + 6]);
            if (DEBUG_FILTER) {
                ALOGD("[Filter] prefix %d", prefix);
            }
            if (prefix == 0x000001) {
                // TODO handle multiple Pes filters
                // Location of PES fields from ISO/IEC 13818-1 Section 2.4.3.6
                mPesSizeLeft = (static_cast<uint8_t>(mFilterOutput[i + 8]) << 8) |
                               static_cast<uint8_t>(mFilterOutput[i + 9]);
                bool hasPts = static_cast<uint8_t>(mFilterOutput[i + 11]) & 0x80;
                uint8_t optionalFieldsLength = static_cast<uint8_t>(mFilterOutput[i + 12]);
                headerSize += 9 + optionalFieldsLength;

                if (hasPts) {
                    // Pts is a 33-bit field which is stored across 5 bytes, with
                    // bits in between as reserved fields which must be ignored
                    mPts = 0;
                    mPts |= (static_cast<uint8_t>(mFilterOutput[i + 13]) & 0x0e) << 29;
                    mPts |= (static_cast<uint8_t>(mFilterOutput[i + 14]) & 0xff) << 22;
                    mPts |= (static_cast<uint8_t>(mFilterOutput[i + 15]) & 0xfe) << 14;
                    mPts |= (static_cast<uint8_t>(mFilterOutput[i + 16]) & 0xff) << 7;
                    mPts |= (static_cast<uint8_t>(mFilterOutput[i + 17]) & 0xfe) >> 1;
                }

                if (DEBUG_FILTER) {
                    ALOGD("[Filter] pes data length %d", mPesSizeLeft);
                }
            } else {
                continue;
            }
        }

        uint32_t endPoint = min(188u - headerSize, mPesSizeLeft);
        // append data and check size
        vector<int8_t>::const_iterator first = mFilterOutput.begin() + i + headerSize;
        vector<int8_t>::const_iterator last = mFilterOutput.begin() + i + headerSize + endPoint;
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
        if (!result.isOk()) {
            mFilterOutput.clear();
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

    {
        std::lock_guard<std::mutex> lock(mFilterEventsLock);
        mFilterEvents.push_back(
                DemuxFilterEvent::make<DemuxFilterEvent::Tag::tsRecord>(recordEvent));
    }

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

// Read PSI (Program Specific Information) Sections from TransportStreams
// as defined in ISO/IEC 13818-1 Section 2.4.4
bool Filter::writeSectionsAndCreateEvent(vector<int8_t>& data) {
    // TODO check how many sections has been read
    ALOGD("[Filter] section handler");

    // Transport Stream Packets are 188 bytes long, as defined in the
    // Introduction of ISO/IEC 13818-1
    for (int i = 0; i < data.size(); i += 188) {
        if (mSectionSizeLeft == 0) {
            // Location for sectionSize as defined by Section 2.4.4
            // Note that the first 4 bytes skipped are the TsHeader
            mSectionSizeLeft = ((static_cast<uint8_t>(data[i + 5]) & 0x0f) << 8) |
                               static_cast<uint8_t>(data[i + 6]);
            mSectionSizeLeft += 3;
            if (DEBUG_FILTER) {
                ALOGD("[Filter] section data length %d", mSectionSizeLeft);
            }
        }

        // 184 bytes per packet is derived by subtracting the 4 byte length of
        // the TsHeader from its 188 byte packet size
        uint32_t endPoint = min(184u, mSectionSizeLeft);
        // append data and check size
        vector<int8_t>::const_iterator first = data.begin() + i + 4;
        vector<int8_t>::const_iterator last = data.begin() + i + 4 + endPoint;
        mSectionOutput.insert(mSectionOutput.end(), first, last);
        // size does not match then continue
        mSectionSizeLeft -= endPoint;
        if (DEBUG_FILTER) {
            ALOGD("[Filter] section data left %d", mSectionSizeLeft);
        }
        if (mSectionSizeLeft > 0) {
            continue;
        }

        if (!writeDataToFilterMQ(mSectionOutput)) {
            mSectionOutput.clear();
            return false;
        }

        DemuxFilterSectionEvent secEvent;
        secEvent = {
                // temp dump meta data
                .tableId = 0,
                .version = 1,
                .sectionNum = 1,
                .dataLength = static_cast<int32_t>(mSectionOutput.size()),
        };
        if (DEBUG_FILTER) {
            ALOGD("[Filter] assembled section data length %" PRIu64, secEvent.dataLength);
        }

        {
            std::lock_guard<std::mutex> lock(mFilterEventsLock);
            mFilterEvents.push_back(
                    DemuxFilterEvent::make<DemuxFilterEvent::Tag::section>(secEvent));
        }
        mSectionOutput.clear();
    }

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
    auto event = DemuxFilterEvent::make<DemuxFilterEvent::Tag::media>();
    auto& mediaEvent = event.get<DemuxFilterEvent::Tag::media>();
    mediaEvent.avMemory = ::android::dupToAidl(nativeHandle);
    mediaEvent.dataLength = static_cast<int64_t>(output.size());
    mediaEvent.avDataId = static_cast<int64_t>(dataId);
    if (mPts) {
        mediaEvent.pts = mPts;
        mPts = 0;
    }

    {
        std::lock_guard<std::mutex> lock(mFilterEventsLock);
        mFilterEvents.push_back(std::move(event));
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
    auto event = DemuxFilterEvent::make<DemuxFilterEvent::Tag::media>();
    auto& mediaEvent = event.get<DemuxFilterEvent::Tag::media>();
    mediaEvent.avMemory = ::android::dupToAidl(nativeHandle);
    mediaEvent.offset = mSharedAvMemOffset;
    mediaEvent.dataLength = static_cast<int64_t>(output.size());
    if (mPts) {
        mediaEvent.pts = mPts;
        mPts = 0;
    }

    {
        std::lock_guard<std::mutex> lock(mFilterEventsLock);
        mFilterEvents.push_back(std::move(event));
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

void Filter::createMediaEvent(vector<DemuxFilterEvent>& events, bool isAudioPresentation) {
    DemuxFilterMediaEvent mediaEvent;
    mediaEvent.streamId = 1;
    mediaEvent.isPtsPresent = true;
    mediaEvent.isDtsPresent = false;
    mediaEvent.dataLength = 3;
    mediaEvent.offset = 4;
    mediaEvent.isSecureMemory = true;
    mediaEvent.mpuSequenceNumber = 6;
    mediaEvent.isPesPrivateData = true;

    if (isAudioPresentation) {
        AudioPresentation audioPresentation0{
                .preselection.preselectionId = 0,
                .preselection.labels = {{"en", "Commentator"}, {"es", "Comentarista"}},
                .preselection.language = "en",
                .preselection.renderingIndication =
                        AudioPreselectionRenderingIndicationType::THREE_DIMENSIONAL,
                .preselection.hasAudioDescription = false,
                .preselection.hasSpokenSubtitles = false,
                .preselection.hasDialogueEnhancement = true,
                .ac4ShortProgramId = 42};
        AudioPresentation audioPresentation1{
                .preselection.preselectionId = 1,
                .preselection.labels = {{"en", "Crowd"}, {"es", "Multitud"}},
                .preselection.language = "en",
                .preselection.renderingIndication =
                        AudioPreselectionRenderingIndicationType::THREE_DIMENSIONAL,
                .preselection.hasAudioDescription = false,
                .preselection.hasSpokenSubtitles = false,
                .preselection.hasDialogueEnhancement = false,
                .ac4ShortProgramId = 42};
        vector<AudioPresentation> audioPresentations;
        audioPresentations.push_back(audioPresentation0);
        audioPresentations.push_back(audioPresentation1);
        mediaEvent.extraMetaData.set<DemuxFilterMediaEventExtraMetaData::Tag::audioPresentations>(
                audioPresentations);
    } else {
        AudioExtraMetaData audio;
        audio.adFade = 1;
        audio.adPan = 2;
        audio.versionTextTag = 3;
        audio.adGainCenter = 4;
        audio.adGainFront = 5;
        audio.adGainSurround = 6;
        mediaEvent.extraMetaData.set<DemuxFilterMediaEventExtraMetaData::Tag::audio>(audio);
    }

    int av_fd = createAvIonFd(BUFFER_SIZE);
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

    mediaEvent.avDataId = static_cast<int64_t>(dataId);
    mediaEvent.avMemory = ::android::dupToAidl(nativeHandle);

    events.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::media>(std::move(mediaEvent)));

    native_handle_close(nativeHandle);
    native_handle_delete(nativeHandle);
}

void Filter::createTsRecordEvent(vector<DemuxFilterEvent>& events) {
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

    events.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::tsRecord>(std::move(tsRecord1)));
    events.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::tsRecord>(std::move(tsRecord2)));
}

void Filter::createMmtpRecordEvent(vector<DemuxFilterEvent>& events) {
    DemuxFilterMmtpRecordEvent mmtpRecord1;
    mmtpRecord1.scHevcIndexMask = 1;
    mmtpRecord1.byteNumber = 2;

    DemuxFilterMmtpRecordEvent mmtpRecord2;
    mmtpRecord2.pts = 1;
    mmtpRecord2.mpuSequenceNumber = 2;
    mmtpRecord2.firstMbInSlice = 3;
    mmtpRecord2.tsIndexMask = 4;

    events.push_back(
            DemuxFilterEvent::make<DemuxFilterEvent::Tag::mmtpRecord>(std::move(mmtpRecord1)));
    events.push_back(
            DemuxFilterEvent::make<DemuxFilterEvent::Tag::mmtpRecord>(std::move(mmtpRecord2)));
}

void Filter::createSectionEvent(vector<DemuxFilterEvent>& events) {
    DemuxFilterSectionEvent section;
    section.tableId = 1;
    section.version = 2;
    section.sectionNum = 3;
    section.dataLength = 0;

    events.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::section>(std::move(section)));
}

void Filter::createPesEvent(vector<DemuxFilterEvent>& events) {
    DemuxFilterPesEvent pes;
    pes.streamId = 1;
    pes.dataLength = 1;
    pes.mpuSequenceNumber = 2;

    events.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::pes>(std::move(pes)));
}

void Filter::createDownloadEvent(vector<DemuxFilterEvent>& events) {
    DemuxFilterDownloadEvent download;
    download.itemId = 1;
    download.downloadId = 1;
    download.mpuSequenceNumber = 2;
    download.itemFragmentIndex = 3;
    download.lastItemFragmentIndex = 4;
    download.dataLength = 0;

    events.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::download>(std::move(download)));
}

void Filter::createIpPayloadEvent(vector<DemuxFilterEvent>& events) {
    DemuxFilterIpPayloadEvent ipPayload;
    ipPayload.dataLength = 0;

    events.push_back(
            DemuxFilterEvent::make<DemuxFilterEvent::Tag::ipPayload>(std::move(ipPayload)));
}

void Filter::createTemiEvent(vector<DemuxFilterEvent>& events) {
    DemuxFilterTemiEvent temi;
    temi.pts = 1;
    temi.descrTag = 2;
    temi.descrData = {3};

    events.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::temi>(std::move(temi)));
}

void Filter::createMonitorEvent(vector<DemuxFilterEvent>& events) {
    DemuxFilterMonitorEvent monitor;
    monitor.set<DemuxFilterMonitorEvent::Tag::scramblingStatus>(ScramblingStatus::SCRAMBLED);

    events.push_back(
            DemuxFilterEvent::make<DemuxFilterEvent::Tag::monitorEvent>(std::move(monitor)));
}

void Filter::createRestartEvent(vector<DemuxFilterEvent>& events) {
    events.push_back(DemuxFilterEvent::make<DemuxFilterEvent::Tag::startId>(1));
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
