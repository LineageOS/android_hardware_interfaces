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

#define LOG_TAG "android.hardware.tv.tuner@1.0-Demux"

#include "Demux.h"
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

#define WAIT_TIMEOUT 3000000000

const std::vector<uint8_t> fakeDataInputBuffer{
        0x00, 0x00, 0x00, 0x01, 0x09, 0xf0, 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1e, 0xdb,
        0x01, 0x40, 0x16, 0xec, 0x04, 0x40, 0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 0x0f, 0x03,
        0xc5, 0x8b, 0xb8, 0x00, 0x00, 0x00, 0x01, 0x68, 0xca, 0x8c, 0xb2, 0x00, 0x00, 0x01, 0x06,
        0x05, 0xff, 0xff, 0x70, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8,
        0x20, 0xd9, 0x23, 0xee, 0xef, 0x78, 0x32, 0x36, 0x34, 0x20, 0x2d, 0x20, 0x63, 0x6f, 0x72,
        0x65, 0x20, 0x31, 0x34, 0x32, 0x20, 0x2d, 0x20, 0x48, 0x2e, 0x32, 0x36, 0x34, 0x2f, 0x4d,
        0x50, 0x45, 0x47, 0x2d, 0x34, 0x20, 0x41, 0x56, 0x43, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x63,
        0x20, 0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x6c, 0x65, 0x66, 0x74, 0x20, 0x32, 0x30, 0x30,
        0x33, 0x2d, 0x32, 0x30, 0x31, 0x34, 0x20, 0x2d, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
        0x2f, 0x77, 0x77, 0x77, 0x2e, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x6c, 0x61, 0x6e, 0x2e, 0x6f,
        0x72, 0x67, 0x2f, 0x78, 0x32, 0x36, 0x34, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x20, 0x2d, 0x20,
        0x6f, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x3a, 0x20, 0x63, 0x61, 0x62, 0x61, 0x63, 0x3d,
        0x30, 0x20, 0x72, 0x65, 0x66, 0x3d, 0x32, 0x20, 0x64, 0x65, 0x62, 0x6c, 0x6f, 0x63, 0x6b,
        0x3d, 0x31, 0x3a, 0x30, 0x3a, 0x30, 0x20, 0x61, 0x6e, 0x61, 0x6c, 0x79, 0x73, 0x65, 0x3d,
        0x30, 0x78, 0x31, 0x3a, 0x30, 0x78, 0x31, 0x31, 0x31, 0x20, 0x6d, 0x65, 0x3d, 0x68, 0x65,
        0x78, 0x20, 0x73, 0x75, 0x62, 0x6d, 0x65, 0x3d, 0x37, 0x20, 0x70, 0x73, 0x79, 0x3d, 0x31,
        0x20, 0x70, 0x73, 0x79, 0x5f, 0x72, 0x64, 0x3d, 0x31, 0x2e, 0x30, 0x30, 0x3a, 0x30, 0x2e,
        0x30, 0x30, 0x20, 0x6d, 0x69, 0x78, 0x65, 0x64, 0x5f, 0x72, 0x65, 0x66, 0x3d, 0x31, 0x20,
        0x6d, 0x65, 0x5f, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x3d, 0x31, 0x36, 0x20, 0x63, 0x68, 0x72,
        0x6f, 0x6d, 0x61, 0x5f, 0x6d, 0x65, 0x3d, 0x31, 0x20, 0x74, 0x72, 0x65, 0x6c, 0x6c, 0x69,
        0x73, 0x3d, 0x31, 0x20, 0x38, 0x78, 0x38, 0x64, 0x63, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x71,
        0x6d, 0x3d, 0x30, 0x20, 0x64, 0x65, 0x61, 0x64, 0x7a, 0x6f, 0x6e, 0x65, 0x3d, 0x32, 0x31,
        0x2c, 0x31, 0x31, 0x20, 0x66, 0x61, 0x73, 0x74, 0x5f, 0x70, 0x73, 0x6b, 0x69, 0x70, 0x3d,
        0x31, 0x20, 0x63, 0x68, 0x72, 0x6f, 0x6d, 0x61, 0x5f, 0x71, 0x70, 0x5f, 0x6f, 0x66, 0x66,
        0x73, 0x65, 0x74, 0x3d, 0x2d, 0x32, 0x20, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d,
        0x36, 0x30, 0x20, 0x6c, 0x6f, 0x6f, 0x6b, 0x61, 0x68, 0x65, 0x61, 0x64, 0x5f, 0x74, 0x68,
        0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x35, 0x20, 0x73, 0x6c, 0x69, 0x63, 0x65, 0x64, 0x5f,
        0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x73, 0x3d, 0x30, 0x20, 0x6e, 0x72, 0x3d, 0x30, 0x20,
        0x64, 0x65, 0x63, 0x69, 0x6d, 0x61, 0x74, 0x65, 0x3d, 0x31, 0x20, 0x69, 0x6e, 0x74, 0x65,
        0x72, 0x6c, 0x61, 0x63, 0x65, 0x64, 0x3d, 0x30, 0x20, 0x62, 0x6c, 0x75, 0x72, 0x61, 0x79,
        0x5f, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74,
        0x72, 0x61, 0x69, 0x6e, 0x65, 0x64, 0x5f, 0x69, 0x6e, 0x74, 0x72, 0x61, 0x3d, 0x30, 0x20,
        0x62, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x73, 0x3d, 0x30, 0x20, 0x77, 0x65, 0x69, 0x67, 0x68,
        0x74, 0x70, 0x3d, 0x30, 0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x3d, 0x32, 0x35, 0x30,
        0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x5f, 0x6d, 0x69, 0x6e, 0x3d, 0x32, 0x35, 0x20,
        0x73, 0x63, 0x65, 0x6e, 0x65,
};

Demux::Demux(uint32_t demuxId) {
    mDemuxId = demuxId;
}

Demux::~Demux() {}

bool Demux::createAndSaveMQ(uint32_t bufferSize, uint32_t filterId) {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    std::unique_ptr<FilterMQ> tmpFilterMQ =
            std::unique_ptr<FilterMQ>(new (std::nothrow) FilterMQ(bufferSize, true));
    if (!tmpFilterMQ->isValid()) {
        ALOGW("Failed to create FMQ of filter with id: %d", filterId);
        return false;
    }

    mFilterMQs.resize(filterId + 1);
    mFilterMQs[filterId] = std::move(tmpFilterMQ);

    EventFlag* mFilterEventFlag;
    if (EventFlag::createEventFlag(mFilterMQs[filterId]->getEventFlagWord(), &mFilterEventFlag) !=
        OK) {
        return false;
    }
    mFilterEventFlags.resize(filterId + 1);
    mFilterEventFlags[filterId] = mFilterEventFlag;
    mFilterWriteCount.resize(filterId + 1);
    mFilterWriteCount[filterId] = 0;
    mThreadRunning.resize(filterId + 1);

    return true;
}

Return<Result> Demux::setFrontendDataSource(uint32_t frontendId) {
    ALOGV("%s", __FUNCTION__);

    mSourceFrontendId = frontendId;

    return Result::SUCCESS;
}

Return<void> Demux::addFilter(DemuxFilterType type, uint32_t bufferSize,
                              const sp<IDemuxCallback>& cb, addFilter_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    uint32_t filterId = mLastUsedFilterId + 1;
    mLastUsedFilterId += 1;

    if ((type != DemuxFilterType::PCR || type != DemuxFilterType::TS) && cb == nullptr) {
        ALOGW("callback can't be null");
        _hidl_cb(Result::INVALID_ARGUMENT, filterId);
        return Void();
    }
    // Add callback
    mDemuxCallbacks.resize(filterId + 1);
    mDemuxCallbacks[filterId] = cb;

    // Mapping from the filter ID to the filter type
    mFilterTypes.resize(filterId + 1);
    mFilterTypes[filterId] = type;

    if (!createAndSaveMQ(bufferSize, filterId)) {
        _hidl_cb(Result::UNKNOWN_ERROR, -1);
        return Void();
    }

    _hidl_cb(Result::SUCCESS, filterId);
    return Void();
}

Return<void> Demux::getFilterQueueDesc(uint32_t filterId, getFilterQueueDesc_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (filterId < 0 || filterId > mLastUsedFilterId) {
        ALOGW("No filter with id: %d exists", filterId);
        _hidl_cb(Result::INVALID_ARGUMENT, FilterMQ::Descriptor());
        return Void();
    }

    _hidl_cb(Result::SUCCESS, *mFilterMQs[filterId]->getDesc());
    return Void();
}

Return<Result> Demux::configureFilter(uint32_t /* filterId */,
                                      const DemuxFilterSettings& /* settings */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::startFilter(uint32_t filterId) {
    ALOGV("%s", __FUNCTION__);

    if (filterId < 0 || filterId > mLastUsedFilterId) {
        ALOGW("No filter with id: %d exists", filterId);
        return Result::INVALID_ARGUMENT;
    }

    DemuxFilterType filterType = mFilterTypes[filterId];
    Result result;
    DemuxFilterEvent event{
            .filterId = filterId,
            .filterType = filterType,
    };

    switch (filterType) {
        case DemuxFilterType::SECTION:
            result = startSectionFilterHandler(event);
            break;
        case DemuxFilterType::PES:
            result = startPesFilterHandler(event);
            break;
        case DemuxFilterType::TS:
            result = startTsFilterHandler();
            return Result::SUCCESS;
        case DemuxFilterType::AUDIO:
        case DemuxFilterType::VIDEO:
            result = startMediaFilterHandler(event);
            break;
        case DemuxFilterType::RECORD:
            result = startRecordFilterHandler(event);
            break;
        case DemuxFilterType::PCR:
            result = startPcrFilterHandler();
            return Result::SUCCESS;
        default:
            return Result::UNKNOWN_ERROR;
    }

    return result;
}

Return<Result> Demux::stopFilter(uint32_t /* filterId */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::flushFilter(uint32_t /* filterId */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::removeFilter(uint32_t /* filterId */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<void> Demux::getAvSyncHwId(uint32_t /* filterId */, getAvSyncHwId_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    AvSyncHwId avSyncHwId = 0;

    _hidl_cb(Result::SUCCESS, avSyncHwId);
    return Void();
}

Return<void> Demux::getAvSyncTime(AvSyncHwId /* avSyncHwId */, getAvSyncTime_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    uint64_t avSyncTime = 0;

    _hidl_cb(Result::SUCCESS, avSyncTime);
    return Void();
}

Return<Result> Demux::close() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

bool Demux::writeSectionsAndCreateEvent(DemuxFilterEvent& event, uint32_t sectionNum) {
    event.events.resize(sectionNum);
    for (int i = 0; i < sectionNum; i++) {
        DemuxFilterSectionEvent secEvent;
        secEvent = {
                // temp dump meta data
                .tableId = 0,
                .version = 1,
                .sectionNum = 1,
                .dataLength = 530,
        };
        event.events[i].section(secEvent);
        if (!writeDataToFilterMQ(fakeDataInputBuffer, event.filterId)) {
            return false;
        }
    }
    return true;
}

bool Demux::writeDataToFilterMQ(const std::vector<uint8_t>& data, uint32_t filterId) {
    std::lock_guard<std::mutex> lock(mWriteLock);
    if (mFilterMQs[filterId]->write(data.data(), data.size())) {
        return true;
    }
    return false;
}

Result Demux::startSectionFilterHandler(DemuxFilterEvent event) {
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    threadArgs->user = this;
    threadArgs->event = &event;

    pthread_create(&mThreadId, NULL, __threadLoop, (void*)threadArgs);
    pthread_setname_np(mThreadId, "demux_filter_waiting_loop");

    return Result::SUCCESS;
}

Result Demux::startPesFilterHandler(DemuxFilterEvent& event) {
    // TODO generate multiple events in one event callback
    DemuxFilterPesEvent pesEvent;
    pesEvent = {
            // temp dump meta data
            .streamId = 0,
            .dataLength = 530,
    };
    event.events.resize(1);
    event.events[0].pes(pesEvent);
    /*pthread_create(&mThreadId, NULL, __threadLoop, this);
    pthread_setname_np(mThreadId, "demux_section_filter_waiting_loop");*/
    if (!writeDataToFilterMQ(fakeDataInputBuffer, event.filterId)) {
        return Result::INVALID_STATE;
    }

    if (mDemuxCallbacks[event.filterId] == nullptr) {
        return Result::NOT_INITIALIZED;
    }

    mDemuxCallbacks[event.filterId]->onFilterEvent(event);
    return Result::SUCCESS;
}

Result Demux::startTsFilterHandler() {
    // TODO handle starting TS filter
    return Result::SUCCESS;
}

Result Demux::startMediaFilterHandler(DemuxFilterEvent& event) {
    DemuxFilterMediaEvent mediaEvent;
    mediaEvent = {
            // temp dump meta data
            .pts = 0,
            .dataLength = 530,
            .secureMemory = nullptr,
    };
    event.events.resize(1);
    event.events[0].media() = mediaEvent;
    // TODO handle write FQM for media stream
    return Result::SUCCESS;
}

Result Demux::startRecordFilterHandler(DemuxFilterEvent& event) {
    DemuxFilterRecordEvent recordEvent;
    recordEvent = {
            // temp dump meta data
            .tpid = 0,
            .packetNum = 0,
    };
    recordEvent.indexMask.tsIndexMask() = 0x01;
    event.events.resize(1);
    event.events[0].ts() = recordEvent;
    return Result::SUCCESS;
}

Result Demux::startPcrFilterHandler() {
    // TODO handle starting PCR filter
    return Result::SUCCESS;
}

void* Demux::__threadLoop(void* threadArg) {
    Demux* const self = static_cast<Demux*>(((struct ThreadArgs*)threadArg)->user);
    self->filterThreadLoop(((struct ThreadArgs*)threadArg)->event);
    return 0;
}

void Demux::filterThreadLoop(DemuxFilterEvent* event) {
    uint32_t filterId = event->filterId;
    ALOGD("[Demux] filter %d threadLoop start.", filterId);
    mThreadRunning[filterId] = true;

    while (mThreadRunning[filterId]) {
        uint32_t efState = 0;
        // We do not wait for the last round of writen data to be read to finish the thread
        // because the VTS can verify the reading itself.
        for (int i = 0; i < SECTION_WRITE_COUNT; i++) {
            DemuxFilterEvent filterEvent{
                    .filterId = filterId,
                    .filterType = event->filterType,
            };
            if (!writeSectionsAndCreateEvent(filterEvent, 2)) {
                ALOGD("[Demux] filter %d fails to write into FMQ. Ending thread", filterId);
                break;
            }
            mFilterWriteCount[filterId]++;
            if (mDemuxCallbacks[filterId] == nullptr) {
                ALOGD("[Demux] filter %d does not hava callback. Ending thread", filterId);
                break;
            }
            // After successfully write, send a callback and wait for the read to be done
            mDemuxCallbacks[filterId]->onFilterEvent(filterEvent);
            // We do not wait for the last read to be done
            // VTS can verify the read result itself.
            if (i == SECTION_WRITE_COUNT - 1) {
                ALOGD("[Demux] filter %d writing done. Ending thread", filterId);
                break;
            }
            while (mThreadRunning[filterId]) {
                status_t status = mFilterEventFlags[filterId]->wait(
                        static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_CONSUMED), &efState,
                        WAIT_TIMEOUT, true /* retry on spurious wake */);
                if (status != OK) {
                    ALOGD("[Demux] wait for data consumed");
                    continue;
                }
                break;
            }
        }

        mFilterWriteCount[filterId] = 0;
        mThreadRunning[filterId] = false;
    }

    ALOGD("[Demux] filter thread ended.");
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
