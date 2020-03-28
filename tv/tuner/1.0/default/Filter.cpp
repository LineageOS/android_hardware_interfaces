/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.tuner@1.0-Filter"

#include "Filter.h"
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

#define WAIT_TIMEOUT 3000000000

Filter::Filter() {}

Filter::Filter(DemuxFilterType type, uint32_t filterId, uint32_t bufferSize,
               const sp<IFilterCallback>& cb, sp<Demux> demux) {
    mType = type;
    mFilterId = filterId;
    mBufferSize = bufferSize;
    mCallback = cb;
    mDemux = demux;
}

Filter::~Filter() {}

Return<void> Filter::getId(getId_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    _hidl_cb(Result::SUCCESS, mFilterId);
    return Void();
}

Return<Result> Filter::setDataSource(const sp<IFilter>& filter) {
    ALOGV("%s", __FUNCTION__);

    mDataSource = filter;
    mIsDataSourceDemux = false;

    return Result::SUCCESS;
}

Return<void> Filter::getQueueDesc(getQueueDesc_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    mIsUsingFMQ = true;

    _hidl_cb(Result::SUCCESS, *mFilterMQ->getDesc());
    return Void();
}

Return<Result> Filter::configure(const DemuxFilterSettings& settings) {
    ALOGV("%s", __FUNCTION__);

    mFilterSettings = settings;
    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            mTpid = settings.ts().tpid;
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

    return Result::SUCCESS;
}

Return<Result> Filter::start() {
    ALOGV("%s", __FUNCTION__);

    return startFilterLoop();
}

Return<Result> Filter::stop() {
    ALOGV("%s", __FUNCTION__);

    mFilterThreadRunning = false;

    std::lock_guard<std::mutex> lock(mFilterThreadLock);

    return Result::SUCCESS;
}

Return<Result> Filter::flush() {
    ALOGV("%s", __FUNCTION__);

    // temp implementation to flush the FMQ
    int size = mFilterMQ->availableToRead();
    char* buffer = new char[size];
    mFilterMQ->read((unsigned char*)&buffer[0], size);
    delete[] buffer;
    mFilterStatus = DemuxFilterStatus::DATA_READY;

    return Result::SUCCESS;
}

Return<Result> Filter::releaseAvHandle(const hidl_handle& /*avMemory*/, uint64_t avDataId) {
    ALOGV("%s", __FUNCTION__);
    if (mDataId2Avfd.find(avDataId) == mDataId2Avfd.end()) {
        return Result::INVALID_ARGUMENT;
    }

    ::close(mDataId2Avfd[avDataId]);
    return Result::SUCCESS;
}

Return<Result> Filter::close() {
    ALOGV("%s", __FUNCTION__);

    return mDemux->removeFilter(mFilterId);
}

bool Filter::createFilterMQ() {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    std::unique_ptr<FilterMQ> tmpFilterMQ =
            std::unique_ptr<FilterMQ>(new (std::nothrow) FilterMQ(mBufferSize, true));
    if (!tmpFilterMQ->isValid()) {
        ALOGW("Failed to create FMQ of filter with id: %d", mFilterId);
        return false;
    }

    mFilterMQ = std::move(tmpFilterMQ);

    if (EventFlag::createEventFlag(mFilterMQ->getEventFlagWord(), &mFilterEventFlag) != OK) {
        return false;
    }

    return true;
}

Result Filter::startFilterLoop() {
    pthread_create(&mFilterThread, NULL, __threadLoopFilter, this);
    pthread_setname_np(mFilterThread, "filter_waiting_loop");

    return Result::SUCCESS;
}

void* Filter::__threadLoopFilter(void* user) {
    Filter* const self = static_cast<Filter*>(user);
    self->filterThreadLoop();
    return 0;
}

void Filter::filterThreadLoop() {
    ALOGD("[Filter] filter %d threadLoop start.", mFilterId);
    std::lock_guard<std::mutex> lock(mFilterThreadLock);
    mFilterThreadRunning = true;

    // For the first time of filter output, implementation needs to send the filter
    // Event Callback without waiting for the DATA_CONSUMED to init the process.
    while (mFilterThreadRunning) {
        if (mFilterEvent.events.size() == 0) {
            if (DEBUG_FILTER) {
                ALOGD("[Filter] wait for filter data output.");
            }
            usleep(1000 * 1000);
            continue;
        }
        // After successfully write, send a callback and wait for the read to be done
        mCallback->onFilterEvent(mFilterEvent);
        freeAvHandle();
        mFilterEvent.events.resize(0);
        mFilterStatus = DemuxFilterStatus::DATA_READY;
        if (mCallback == nullptr) {
            ALOGD("[Filter] filter %d does not hava callback. Ending thread", mFilterId);
            break;
        }
        mCallback->onFilterStatus(mFilterStatus);
        break;
    }

    while (mFilterThreadRunning) {
        uint32_t efState = 0;
        // We do not wait for the last round of written data to be read to finish the thread
        // because the VTS can verify the reading itself.
        for (int i = 0; i < SECTION_WRITE_COUNT; i++) {
            while (mFilterThreadRunning && mIsUsingFMQ) {
                status_t status = mFilterEventFlag->wait(
                        static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_CONSUMED), &efState,
                        WAIT_TIMEOUT, true /* retry on spurious wake */);
                if (status != OK) {
                    ALOGD("[Filter] wait for data consumed");
                    continue;
                }
                break;
            }

            maySendFilterStatusCallback();

            while (mFilterThreadRunning) {
                std::lock_guard<std::mutex> lock(mFilterEventLock);
                if (mFilterEvent.events.size() == 0) {
                    continue;
                }
                // After successfully write, send a callback and wait for the read to be done
                mCallback->onFilterEvent(mFilterEvent);
                mFilterEvent.events.resize(0);
                break;
            }
            // We do not wait for the last read to be done
            // VTS can verify the read result itself.
            if (i == SECTION_WRITE_COUNT - 1) {
                ALOGD("[Filter] filter %d writing done. Ending thread", mFilterId);
                break;
            }
        }
        mFilterThreadRunning = false;
    }

    ALOGD("[Filter] filter thread ended.");
}

void Filter::freeAvHandle() {
    if (mType.mainType != DemuxFilterMainType::TS ||
        (mType.subType.tsFilterType() == DemuxTsFilterType::AUDIO &&
         mType.subType.tsFilterType() == DemuxTsFilterType::VIDEO)) {
        return;
    }
    for (int i = 0; i < mFilterEvent.events.size(); i++) {
        ::close(mFilterEvent.events[i].media().avMemory.getNativeHandle()->data[0]);
        native_handle_close(mFilterEvent.events[i].media().avMemory.getNativeHandle());
    }
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
        mCallback->onFilterStatus(newStatus);
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

void Filter::updateFilterOutput(vector<uint8_t> data) {
    std::lock_guard<std::mutex> lock(mFilterOutputLock);
    ALOGD("[Filter] filter output updated");
    mFilterOutput.insert(mFilterOutput.end(), data.begin(), data.end());
}

void Filter::updateRecordOutput(vector<uint8_t> data) {
    std::lock_guard<std::mutex> lock(mRecordFilterOutputLock);
    ALOGD("[Filter] record filter output updated");
    mRecordFilterOutput.insert(mRecordFilterOutput.end(), data.begin(), data.end());
}

Result Filter::startFilterHandler() {
    std::lock_guard<std::mutex> lock(mFilterOutputLock);
    switch (mType.mainType) {
        case DemuxFilterMainType::TS:
            switch (mType.subType.tsFilterType()) {
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
    return Result::SUCCESS;
}

Result Filter::startSectionFilterHandler() {
    if (mFilterOutput.empty()) {
        return Result::SUCCESS;
    }
    if (!writeSectionsAndCreateEvent(mFilterOutput)) {
        ALOGD("[Filter] filter %d fails to write into FMQ. Ending thread", mFilterId);
        return Result::UNKNOWN_ERROR;
    }

    mFilterOutput.clear();

    return Result::SUCCESS;
}

Result Filter::startPesFilterHandler() {
    std::lock_guard<std::mutex> lock(mFilterEventLock);
    if (mFilterOutput.empty()) {
        return Result::SUCCESS;
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
        vector<uint8_t>::const_iterator first = mFilterOutput.begin() + i + 4;
        vector<uint8_t>::const_iterator last = mFilterOutput.begin() + i + 4 + endPoint;
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
            return Result::INVALID_STATE;
        }
        maySendFilterStatusCallback();
        DemuxFilterPesEvent pesEvent;
        pesEvent = {
                // temp dump meta data
                .streamId = mPesOutput[3],
                .dataLength = static_cast<uint16_t>(mPesOutput.size()),
        };
        if (DEBUG_FILTER) {
            ALOGD("[Filter] assembled pes data length %d", pesEvent.dataLength);
        }

        int size = mFilterEvent.events.size();
        mFilterEvent.events.resize(size + 1);
        mFilterEvent.events[size].pes(pesEvent);
        mPesOutput.clear();
    }

    mFilterOutput.clear();

    return Result::SUCCESS;
}

Result Filter::startTsFilterHandler() {
    // TODO handle starting TS filter
    return Result::SUCCESS;
}

Result Filter::startMediaFilterHandler() {
    std::lock_guard<std::mutex> lock(mFilterEventLock);
    if (mFilterOutput.empty()) {
        return Result::SUCCESS;
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
        vector<uint8_t>::const_iterator first = mFilterOutput.begin() + i + 4;
        vector<uint8_t>::const_iterator last = mFilterOutput.begin() + i + 4 + endPoint;
        mPesOutput.insert(mPesOutput.end(), first, last);
        // size does not match then continue
        mPesSizeLeft -= endPoint;
        if (DEBUG_FILTER) {
            ALOGD("[Filter] pes data left %d", mPesSizeLeft);
        }
        if (mPesSizeLeft > 0 || mAvBufferCopyCount++ < 10) {
            continue;
        }

        int av_fd = createAvIonFd(mPesOutput.size());
        if (av_fd == -1) {
            return Result::UNKNOWN_ERROR;
        }
        // copy the filtered data to the buffer
        uint8_t* avBuffer = getIonBuffer(av_fd, mPesOutput.size());
        if (avBuffer == NULL) {
            return Result::UNKNOWN_ERROR;
        }
        memcpy(avBuffer, mPesOutput.data(), mPesOutput.size() * sizeof(uint8_t));

        native_handle_t* nativeHandle = createNativeHandle(av_fd);
        if (nativeHandle == NULL) {
            return Result::UNKNOWN_ERROR;
        }
        hidl_handle handle;
        handle.setTo(nativeHandle, /*shouldOwn=*/true);

        // Create a dataId and add a <dataId, av_fd> pair into the dataId2Avfd map
        uint64_t dataId = mLastUsedDataId++ /*createdUID*/;
        mDataId2Avfd[dataId] = dup(av_fd);

        // Create mediaEvent and send callback
        DemuxFilterMediaEvent mediaEvent;
        mediaEvent = {
                .avMemory = std::move(handle),
                .dataLength = static_cast<uint32_t>(mPesOutput.size()),
                .avDataId = dataId,
        };
        int size = mFilterEvent.events.size();
        mFilterEvent.events.resize(size + 1);
        mFilterEvent.events[size].media(mediaEvent);

        // Clear and log
        mPesOutput.clear();
        mAvBufferCopyCount = 0;
        ::close(av_fd);
        if (DEBUG_FILTER) {
            ALOGD("[Filter] assembled av data length %d", mediaEvent.dataLength);
        }
    }

    mFilterOutput.clear();

    return Result::SUCCESS;
}

Result Filter::startRecordFilterHandler() {
    /*DemuxFilterTsRecordEvent tsRecordEvent;
    tsRecordEvent.pid.tPid(0);
    tsRecordEvent.indexMask.tsIndexMask(0x01);
    mFilterEvent.events.resize(1);
    mFilterEvent.events[0].tsRecord(tsRecordEvent);
*/
    std::lock_guard<std::mutex> lock(mRecordFilterOutputLock);
    if (mRecordFilterOutput.empty()) {
        return Result::SUCCESS;
    }

    if (mDvr == nullptr || !mDvr->writeRecordFMQ(mRecordFilterOutput)) {
        ALOGD("[Filter] dvr fails to write into record FMQ.");
        return Result::UNKNOWN_ERROR;
    }

    mRecordFilterOutput.clear();
    return Result::SUCCESS;
}

Result Filter::startPcrFilterHandler() {
    // TODO handle starting PCR filter
    return Result::SUCCESS;
}

Result Filter::startTemiFilterHandler() {
    // TODO handle starting TEMI filter
    return Result::SUCCESS;
}

bool Filter::writeSectionsAndCreateEvent(vector<uint8_t> data) {
    // TODO check how many sections has been read
    ALOGD("[Filter] section hander");
    std::lock_guard<std::mutex> lock(mFilterEventLock);
    if (!writeDataToFilterMQ(data)) {
        return false;
    }
    int size = mFilterEvent.events.size();
    mFilterEvent.events.resize(size + 1);
    DemuxFilterSectionEvent secEvent;
    secEvent = {
            // temp dump meta data
            .tableId = 0,
            .version = 1,
            .sectionNum = 1,
            .dataLength = static_cast<uint16_t>(data.size()),
    };
    mFilterEvent.events[size].section(secEvent);
    return true;
}

bool Filter::writeDataToFilterMQ(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mWriteLock);
    if (mFilterMQ->write(data.data(), data.size())) {
        return true;
    }
    return false;
}

void Filter::attachFilterToRecord(const sp<Dvr> dvr) {
    mDvr = dvr;
}

void Filter::detachFilterFromRecord() {
    mDvr = nullptr;
}

int Filter::createAvIonFd(int size) {
    // Create an ion fd and allocate an av fd mapped to a buffer to it.
    int ion_fd = ion_open();
    if (ion_fd == -1) {
        ALOGE("[Filter] Failed to open ion fd %d", errno);
        return -1;
    }
    int av_fd = -1;
    ion_alloc_fd(dup(ion_fd), size, 0 /*align*/, ION_HEAP_SYSTEM_MASK, 0 /*flags*/, &av_fd);
    if (av_fd == -1) {
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
    // Create a native handle to pass the av fd via the callback event.
    native_handle_t* nativeHandle = native_handle_create(/*numFd*/ 1, 0);
    if (nativeHandle == NULL) {
        ALOGE("[Filter] Failed to create native_handle %d", errno);
        return NULL;
    }
    nativeHandle->data[0] = dup(fd);
    return nativeHandle;
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
