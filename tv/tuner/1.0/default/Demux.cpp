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

Demux::Demux(uint32_t demuxId, sp<Tuner> tuner) {
    mDemuxId = demuxId;
    mTunerService = tuner;
}

Demux::~Demux() {}

Return<Result> Demux::setFrontendDataSource(uint32_t frontendId) {
    ALOGV("%s", __FUNCTION__);

    if (mTunerService == nullptr) {
        return Result::NOT_INITIALIZED;
    }

    mFrontend = mTunerService->getFrontendById(frontendId);

    if (mFrontend == nullptr) {
        return Result::INVALID_STATE;
    }

    mFrontendSourceFile = mFrontend->getSourceFile();

    mTunerService->setFrontendAsDemuxSource(frontendId, mDemuxId);
    return startBroadcastInputLoop();
}

Return<void> Demux::addFilter(DemuxFilterType type, uint32_t bufferSize,
                              const sp<IDemuxCallback>& cb, addFilter_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    uint32_t filterId;

    if (!mUnusedFilterIds.empty()) {
        filterId = *mUnusedFilterIds.begin();

        mUnusedFilterIds.erase(filterId);
    } else {
        filterId = ++mLastUsedFilterId;

        mFilterCallbacks.resize(filterId + 1);
        mFilterMQs.resize(filterId + 1);
        mFilterEvents.resize(filterId + 1);
        mFilterEventFlags.resize(filterId + 1);
        mFilterThreadRunning.resize(filterId + 1);
        mFilterThreads.resize(filterId + 1);
        mFilterPids.resize(filterId + 1);
        mFilterOutputs.resize(filterId + 1);
        mFilterStatus.resize(filterId + 1);
    }

    mUsedFilterIds.insert(filterId);

    if ((type != DemuxFilterType::PCR || type != DemuxFilterType::TS) && cb == nullptr) {
        ALOGW("callback can't be null");
        _hidl_cb(Result::INVALID_ARGUMENT, filterId);
        return Void();
    }

    // Add callback
    mFilterCallbacks[filterId] = cb;

    // Mapping from the filter ID to the filter event
    DemuxFilterEvent event{
            .filterId = filterId,
            .filterType = type,
    };
    mFilterEvents[filterId] = event;

    if (!createFilterMQ(bufferSize, filterId)) {
        _hidl_cb(Result::UNKNOWN_ERROR, -1);
        return Void();
    }

    _hidl_cb(Result::SUCCESS, filterId);
    return Void();
}

Return<void> Demux::getFilterQueueDesc(uint32_t filterId, getFilterQueueDesc_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (mUsedFilterIds.find(filterId) == mUsedFilterIds.end()) {
        ALOGW("No filter with id: %d exists to get desc", filterId);
        _hidl_cb(Result::INVALID_ARGUMENT, FilterMQ::Descriptor());
        return Void();
    }

    _hidl_cb(Result::SUCCESS, *mFilterMQs[filterId]->getDesc());
    return Void();
}

Return<Result> Demux::configureFilter(uint32_t filterId, const DemuxFilterSettings& settings) {
    ALOGV("%s", __FUNCTION__);

    switch (mFilterEvents[filterId].filterType) {
        case DemuxFilterType::SECTION:
            mFilterPids[filterId] = settings.section().tpid;
            break;
        case DemuxFilterType::PES:
            mFilterPids[filterId] = settings.pesData().tpid;
            break;
        case DemuxFilterType::TS:
            mFilterPids[filterId] = settings.ts().tpid;
            break;
        case DemuxFilterType::AUDIO:
            mFilterPids[filterId] = settings.audio().tpid;
            break;
        case DemuxFilterType::VIDEO:
            mFilterPids[filterId] = settings.video().tpid;
            break;
        case DemuxFilterType::RECORD:
            mFilterPids[filterId] = settings.record().tpid;
            break;
        case DemuxFilterType::PCR:
            mFilterPids[filterId] = settings.pcr().tpid;
            break;
        default:
            return Result::UNKNOWN_ERROR;
    }
    return Result::SUCCESS;
}

Return<Result> Demux::startFilter(uint32_t filterId) {
    ALOGV("%s", __FUNCTION__);
    Result result;

    if (mUsedFilterIds.find(filterId) == mUsedFilterIds.end()) {
        ALOGW("No filter with id: %d exists to start filter", filterId);
        return Result::INVALID_ARGUMENT;
    }

    result = startFilterLoop(filterId);

    return result;
}

Return<Result> Demux::stopFilter(uint32_t filterId) {
    ALOGV("%s", __FUNCTION__);

    mFilterThreadRunning[filterId] = false;

    std::lock_guard<std::mutex> lock(mFilterThreadLock);

    return Result::SUCCESS;
}

Return<Result> Demux::flushFilter(uint32_t filterId) {
    ALOGV("%s", __FUNCTION__);

    // temp implementation to flush the FMQ
    int size = mFilterMQs[filterId]->availableToRead();
    char* buffer = new char[size];
    mOutputMQ->read((unsigned char*)&buffer[0], size);
    delete[] buffer;
    mFilterStatus[filterId] = DemuxFilterStatus::DATA_READY;

    return Result::SUCCESS;
}

Return<Result> Demux::removeFilter(uint32_t filterId) {
    ALOGV("%s", __FUNCTION__);

    // resetFilterRecords(filterId);
    mUsedFilterIds.erase(filterId);
    mUnusedFilterIds.insert(filterId);

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

    set<uint32_t>::iterator it;
    mInputThread = 0;
    mOutputThread = 0;
    mFilterThreads.clear();
    mUnusedFilterIds.clear();
    mUsedFilterIds.clear();
    mFilterCallbacks.clear();
    mFilterMQs.clear();
    mFilterEvents.clear();
    mFilterEventFlags.clear();
    mFilterOutputs.clear();
    mFilterPids.clear();
    mLastUsedFilterId = -1;

    return Result::SUCCESS;
}

Return<Result> Demux::addOutput(uint32_t bufferSize, const sp<IDemuxCallback>& cb) {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    std::unique_ptr<FilterMQ> tmpFilterMQ =
            std::unique_ptr<FilterMQ>(new (std::nothrow) FilterMQ(bufferSize, true));
    if (!tmpFilterMQ->isValid()) {
        ALOGW("Failed to create output FMQ");
        return Result::UNKNOWN_ERROR;
    }

    mOutputMQ = std::move(tmpFilterMQ);

    if (EventFlag::createEventFlag(mOutputMQ->getEventFlagWord(), &mOutputEventFlag) != OK) {
        return Result::UNKNOWN_ERROR;
    }

    mOutputCallback = cb;

    return Result::SUCCESS;
}

Return<void> Demux::getOutputQueueDesc(getOutputQueueDesc_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (!mOutputMQ) {
        _hidl_cb(Result::NOT_INITIALIZED, FilterMQ::Descriptor());
        return Void();
    }

    _hidl_cb(Result::SUCCESS, *mOutputMQ->getDesc());
    return Void();
}

Return<Result> Demux::configureOutput(const DemuxOutputSettings& settings) {
    ALOGV("%s", __FUNCTION__);

    mOutputConfigured = true;
    mOutputSettings = settings;
    return Result::SUCCESS;
}

Return<Result> Demux::attachOutputFilter(uint32_t /*filterId*/) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::detachOutputFilter(uint32_t /* filterId */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::startOutput() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::stopOutput() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::flushOutput() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::removeOutput() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::addInput(uint32_t bufferSize, const sp<IDemuxCallback>& cb) {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    std::unique_ptr<FilterMQ> tmpInputMQ =
            std::unique_ptr<FilterMQ>(new (std::nothrow) FilterMQ(bufferSize, true));
    if (!tmpInputMQ->isValid()) {
        ALOGW("Failed to create input FMQ");
        return Result::UNKNOWN_ERROR;
    }

    mInputMQ = std::move(tmpInputMQ);

    if (EventFlag::createEventFlag(mInputMQ->getEventFlagWord(), &mInputEventFlag) != OK) {
        return Result::UNKNOWN_ERROR;
    }

    mInputCallback = cb;

    return Result::SUCCESS;
}

Return<void> Demux::getInputQueueDesc(getInputQueueDesc_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (!mInputMQ) {
        _hidl_cb(Result::NOT_INITIALIZED, FilterMQ::Descriptor());
        return Void();
    }

    _hidl_cb(Result::SUCCESS, *mInputMQ->getDesc());
    return Void();
}

Return<Result> Demux::configureInput(const DemuxInputSettings& settings) {
    ALOGV("%s", __FUNCTION__);

    mInputConfigured = true;
    mInputSettings = settings;

    return Result::SUCCESS;
}

Return<Result> Demux::startInput() {
    ALOGV("%s", __FUNCTION__);

    if (!mInputCallback) {
        return Result::NOT_INITIALIZED;
    }

    if (!mInputConfigured) {
        return Result::INVALID_STATE;
    }

    pthread_create(&mInputThread, NULL, __threadLoopInput, this);
    pthread_setname_np(mInputThread, "demux_input_waiting_loop");

    // TODO start another thread to send filter status callback to the framework

    return Result::SUCCESS;
}

Return<Result> Demux::stopInput() {
    ALOGV("%s", __FUNCTION__);

    mInputThreadRunning = false;

    std::lock_guard<std::mutex> lock(mInputThreadLock);

    return Result::SUCCESS;
}

Return<Result> Demux::flushInput() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Demux::removeInput() {
    ALOGV("%s", __FUNCTION__);

    mInputMQ = nullptr;

    return Result::SUCCESS;
}

Result Demux::startFilterLoop(uint32_t filterId) {
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    threadArgs->user = this;
    threadArgs->filterId = filterId;

    pthread_t mFilterThread;
    pthread_create(&mFilterThread, NULL, __threadLoopFilter, (void*)threadArgs);
    mFilterThreads[filterId] = mFilterThread;
    pthread_setname_np(mFilterThread, "demux_filter_waiting_loop");

    return Result::SUCCESS;
}

Result Demux::startSectionFilterHandler(uint32_t filterId) {
    if (mFilterOutputs[filterId].empty()) {
        return Result::SUCCESS;
    }
    if (!writeSectionsAndCreateEvent(filterId, mFilterOutputs[filterId])) {
        ALOGD("[Demux] filter %d fails to write into FMQ. Ending thread", filterId);
        return Result::UNKNOWN_ERROR;
    }

    mFilterOutputs[filterId].clear();

    return Result::SUCCESS;
}

Result Demux::startPesFilterHandler(uint32_t filterId) {
    std::lock_guard<std::mutex> lock(mFilterEventLock);
    if (mFilterOutputs[filterId].empty()) {
        return Result::SUCCESS;
    }

    for (int i = 0; i < mFilterOutputs[filterId].size(); i += 188) {
        if (mPesSizeLeft == 0) {
            uint32_t prefix = (mFilterOutputs[filterId][i + 4] << 16) |
                              (mFilterOutputs[filterId][i + 5] << 8) |
                              mFilterOutputs[filterId][i + 6];
            ALOGD("[Demux] prefix %d", prefix);
            if (prefix == 0x000001) {
                // TODO handle mulptiple Pes filters
                mPesSizeLeft =
                        (mFilterOutputs[filterId][i + 8] << 8) | mFilterOutputs[filterId][i + 9];
                mPesSizeLeft += 6;
                ALOGD("[Demux] pes data length %d", mPesSizeLeft);
            } else {
                continue;
            }
        }

        int endPoint = min(184, mPesSizeLeft);
        // append data and check size
        vector<uint8_t>::const_iterator first = mFilterOutputs[filterId].begin() + i + 4;
        vector<uint8_t>::const_iterator last = mFilterOutputs[filterId].begin() + i + 4 + endPoint;
        mPesOutput.insert(mPesOutput.end(), first, last);
        // size does not match then continue
        mPesSizeLeft -= endPoint;
        if (mPesSizeLeft > 0) {
            continue;
        }
        // size match then create event
        if (!writeDataToFilterMQ(mPesOutput, filterId)) {
            mFilterOutputs[filterId].clear();
            return Result::INVALID_STATE;
        }
        maySendFilterStatusCallback(filterId);
        DemuxFilterPesEvent pesEvent;
        pesEvent = {
                // temp dump meta data
                .streamId = mPesOutput[3],
                .dataLength = static_cast<uint16_t>(mPesOutput.size()),
        };
        ALOGD("[Demux] assembled pes data length %d", pesEvent.dataLength);

        int size = mFilterEvents[filterId].events.size();
        mFilterEvents[filterId].events.resize(size + 1);
        mFilterEvents[filterId].events[size].pes(pesEvent);
        mPesOutput.clear();
    }

    mFilterOutputs[filterId].clear();

    return Result::SUCCESS;
}

Result Demux::startTsFilterHandler() {
    // TODO handle starting TS filter
    return Result::SUCCESS;
}

Result Demux::startMediaFilterHandler(uint32_t filterId) {
    DemuxFilterMediaEvent mediaEvent;
    mediaEvent = {
            // temp dump meta data
            .pts = 0,
            .dataLength = 530,
            .secureMemory = nullptr,
    };
    mFilterEvents[filterId].events.resize(1);
    mFilterEvents[filterId].events[0].media() = mediaEvent;

    mFilterOutputs[filterId].clear();
    // TODO handle write FQM for media stream
    return Result::SUCCESS;
}

Result Demux::startRecordFilterHandler(uint32_t filterId) {
    DemuxFilterRecordEvent recordEvent;
    recordEvent = {
            // temp dump meta data
            .tpid = 0,
            .packetNum = 0,
    };
    recordEvent.indexMask.tsIndexMask() = 0x01;
    mFilterEvents[filterId].events.resize(1);
    mFilterEvents[filterId].events[0].ts() = recordEvent;

    mFilterOutputs[filterId].clear();
    return Result::SUCCESS;
}

Result Demux::startPcrFilterHandler() {
    // TODO handle starting PCR filter
    return Result::SUCCESS;
}

bool Demux::createFilterMQ(uint32_t bufferSize, uint32_t filterId) {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    std::unique_ptr<FilterMQ> tmpFilterMQ =
            std::unique_ptr<FilterMQ>(new (std::nothrow) FilterMQ(bufferSize, true));
    if (!tmpFilterMQ->isValid()) {
        ALOGW("Failed to create FMQ of filter with id: %d", filterId);
        return false;
    }

    mFilterMQs[filterId] = std::move(tmpFilterMQ);

    EventFlag* filterEventFlag;
    if (EventFlag::createEventFlag(mFilterMQs[filterId]->getEventFlagWord(), &filterEventFlag) !=
        OK) {
        return false;
    }
    mFilterEventFlags[filterId] = filterEventFlag;

    return true;
}

bool Demux::writeSectionsAndCreateEvent(uint32_t filterId, vector<uint8_t> data) {
    // TODO check how many sections has been read
    std::lock_guard<std::mutex> lock(mFilterEventLock);
    if (!writeDataToFilterMQ(data, filterId)) {
        return false;
    }
    int size = mFilterEvents[filterId].events.size();
    mFilterEvents[filterId].events.resize(size + 1);
    DemuxFilterSectionEvent secEvent;
    secEvent = {
            // temp dump meta data
            .tableId = 0,
            .version = 1,
            .sectionNum = 1,
            .dataLength = static_cast<uint16_t>(data.size()),
    };
    mFilterEvents[filterId].events[size].section(secEvent);
    return true;
}

bool Demux::writeDataToFilterMQ(const std::vector<uint8_t>& data, uint32_t filterId) {
    std::lock_guard<std::mutex> lock(mWriteLock);
    if (mFilterMQs[filterId]->write(data.data(), data.size())) {
        return true;
    }
    return false;
}

bool Demux::readInputFMQ() {
    // Read input data from the input FMQ
    int size = mInputMQ->availableToRead();
    int inputPacketSize = mInputSettings.packetSize;
    vector<uint8_t> dataOutputBuffer;
    dataOutputBuffer.resize(inputPacketSize);

    // Dispatch the packet to the PID matching filter output buffer
    for (int i = 0; i < size / inputPacketSize; i++) {
        if (!mInputMQ->read(dataOutputBuffer.data(), inputPacketSize)) {
            return false;
        }
        startTsFilter(dataOutputBuffer);
    }

    return true;
}

void Demux::startTsFilter(vector<uint8_t> data) {
    set<uint32_t>::iterator it;
    for (it = mUsedFilterIds.begin(); it != mUsedFilterIds.end(); it++) {
        uint16_t pid = ((data[1] & 0x1f) << 8) | ((data[2] & 0xff));
        ALOGW("start ts filter pid: %d", pid);
        if (pid == mFilterPids[*it]) {
            mFilterOutputs[*it].insert(mFilterOutputs[*it].end(), data.begin(), data.end());
        }
    }
}

bool Demux::startFilterDispatcher() {
    Result result;
    set<uint32_t>::iterator it;

    // Handle the output data per filter type
    for (it = mUsedFilterIds.begin(); it != mUsedFilterIds.end(); it++) {
        switch (mFilterEvents[*it].filterType) {
            case DemuxFilterType::SECTION:
                result = startSectionFilterHandler(*it);
                break;
            case DemuxFilterType::PES:
                result = startPesFilterHandler(*it);
                break;
            case DemuxFilterType::TS:
                result = startTsFilterHandler();
                break;
            case DemuxFilterType::AUDIO:
            case DemuxFilterType::VIDEO:
                result = startMediaFilterHandler(*it);
                break;
            case DemuxFilterType::RECORD:
                result = startRecordFilterHandler(*it);
                break;
            case DemuxFilterType::PCR:
                result = startPcrFilterHandler();
                break;
            default:
                return false;
        }
    }

    return result == Result::SUCCESS;
}

void* Demux::__threadLoopFilter(void* threadArg) {
    Demux* const self = static_cast<Demux*>(((struct ThreadArgs*)threadArg)->user);
    self->filterThreadLoop(((struct ThreadArgs*)threadArg)->filterId);
    return 0;
}

void* Demux::__threadLoopInput(void* user) {
    Demux* const self = static_cast<Demux*>(user);
    self->inputThreadLoop();
    return 0;
}

void Demux::filterThreadLoop(uint32_t filterId) {
    ALOGD("[Demux] filter %d threadLoop start.", filterId);
    std::lock_guard<std::mutex> lock(mFilterThreadLock);
    mFilterThreadRunning[filterId] = true;

    // For the first time of filter output, implementation needs to send the filter
    // Event Callback without waiting for the DATA_CONSUMED to init the process.
    while (mFilterThreadRunning[filterId]) {
        if (mFilterEvents[filterId].events.size() == 0) {
            ALOGD("[Demux] wait for filter data output.");
            usleep(1000 * 1000);
            continue;
        }
        // After successfully write, send a callback and wait for the read to be done
        mFilterCallbacks[filterId]->onFilterEvent(mFilterEvents[filterId]);
        mFilterEvents[filterId].events.resize(0);
        mFilterStatus[filterId] = DemuxFilterStatus::DATA_READY;
        mFilterCallbacks[filterId]->onFilterStatus(filterId, mFilterStatus[filterId]);
        break;
    }

    while (mFilterThreadRunning[filterId]) {
        uint32_t efState = 0;
        // We do not wait for the last round of writen data to be read to finish the thread
        // because the VTS can verify the reading itself.
        for (int i = 0; i < SECTION_WRITE_COUNT; i++) {
            while (mFilterThreadRunning[filterId]) {
                status_t status = mFilterEventFlags[filterId]->wait(
                        static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_CONSUMED), &efState,
                        WAIT_TIMEOUT, true /* retry on spurious wake */);
                if (status != OK) {
                    ALOGD("[Demux] wait for data consumed");
                    continue;
                }
                break;
            }

            if (mFilterCallbacks[filterId] == nullptr) {
                ALOGD("[Demux] filter %d does not hava callback. Ending thread", filterId);
                break;
            }

            maySendFilterStatusCallback(filterId);

            while (mFilterThreadRunning[filterId]) {
                std::lock_guard<std::mutex> lock(mFilterEventLock);
                if (mFilterEvents[filterId].events.size() == 0) {
                    continue;
                }
                // After successfully write, send a callback and wait for the read to be done
                mFilterCallbacks[filterId]->onFilterEvent(mFilterEvents[filterId]);
                mFilterEvents[filterId].events.resize(0);
                break;
            }
            // We do not wait for the last read to be done
            // VTS can verify the read result itself.
            if (i == SECTION_WRITE_COUNT - 1) {
                ALOGD("[Demux] filter %d writing done. Ending thread", filterId);
                break;
            }
        }
        mFilterThreadRunning[filterId] = false;
    }

    ALOGD("[Demux] filter thread ended.");
}

void Demux::inputThreadLoop() {
    ALOGD("[Demux] input threadLoop start.");
    std::lock_guard<std::mutex> lock(mInputThreadLock);
    mInputThreadRunning = true;

    while (mInputThreadRunning) {
        uint32_t efState = 0;
        status_t status =
                mInputEventFlag->wait(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY),
                                      &efState, WAIT_TIMEOUT, true /* retry on spurious wake */);
        if (status != OK) {
            ALOGD("[Demux] wait for data ready on the input FMQ");
            continue;
        }
        // Our current implementation filter the data and write it into the filter FMQ immediately
        // after the DATA_READY from the VTS/framework
        if (!readInputFMQ() || !startFilterDispatcher()) {
            ALOGD("[Demux] input data failed to be filtered. Ending thread");
            break;
        }

        maySendInputStatusCallback();
    }

    mInputThreadRunning = false;
    ALOGD("[Demux] input thread ended.");
}

void Demux::maySendInputStatusCallback() {
    std::lock_guard<std::mutex> lock(mInputStatusLock);
    int availableToRead = mInputMQ->availableToRead();
    int availableToWrite = mInputMQ->availableToWrite();

    DemuxInputStatus newStatus =
            checkInputStatusChange(availableToWrite, availableToRead, mInputSettings.highThreshold,
                                   mInputSettings.lowThreshold);
    if (mIntputStatus != newStatus) {
        mInputCallback->onInputStatus(newStatus);
        mIntputStatus = newStatus;
    }
}

void Demux::maySendFilterStatusCallback(uint32_t filterId) {
    std::lock_guard<std::mutex> lock(mFilterStatusLock);
    int availableToRead = mFilterMQs[filterId]->availableToRead();
    int availableToWrite = mFilterMQs[filterId]->availableToWrite();
    int fmqSize = mFilterMQs[filterId]->getQuantumCount();

    DemuxFilterStatus newStatus =
            checkFilterStatusChange(filterId, availableToWrite, availableToRead,
                                    ceil(fmqSize * 0.75), ceil(fmqSize * 0.25));
    if (mFilterStatus[filterId] != newStatus) {
        mFilterCallbacks[filterId]->onFilterStatus(filterId, newStatus);
        mFilterStatus[filterId] = newStatus;
    }
}

DemuxInputStatus Demux::checkInputStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                               uint32_t highThreshold, uint32_t lowThreshold) {
    if (availableToWrite == 0) {
        return DemuxInputStatus::SPACE_FULL;
    } else if (availableToRead > highThreshold) {
        return DemuxInputStatus::SPACE_ALMOST_FULL;
    } else if (availableToRead < lowThreshold) {
        return DemuxInputStatus::SPACE_ALMOST_EMPTY;
    } else if (availableToRead == 0) {
        return DemuxInputStatus::SPACE_EMPTY;
    }
    return mIntputStatus;
}

DemuxFilterStatus Demux::checkFilterStatusChange(uint32_t filterId, uint32_t availableToWrite,
                                                 uint32_t availableToRead, uint32_t highThreshold,
                                                 uint32_t lowThreshold) {
    if (availableToWrite == 0) {
        return DemuxFilterStatus::OVERFLOW;
    } else if (availableToRead > highThreshold) {
        return DemuxFilterStatus::HIGH_WATER;
    } else if (availableToRead < lowThreshold) {
        return DemuxFilterStatus::LOW_WATER;
    }
    return mFilterStatus[filterId];
}

Result Demux::startBroadcastInputLoop() {
    pthread_create(&mBroadcastInputThread, NULL, __threadLoopBroadcast, this);
    pthread_setname_np(mBroadcastInputThread, "broadcast_input_thread");

    return Result::SUCCESS;
}

void* Demux::__threadLoopBroadcast(void* user) {
    Demux* const self = static_cast<Demux*>(user);
    self->broadcastInputThreadLoop();
    return 0;
}

void Demux::broadcastInputThreadLoop() {
    std::lock_guard<std::mutex> lock(mBroadcastInputThreadLock);
    mBroadcastInputThreadRunning = true;
    mKeepFetchingDataFromFrontend = true;

    // open the stream and get its length
    std::ifstream inputData(mFrontendSourceFile, std::ifstream::binary);
    // TODO take the packet size from the frontend setting
    int packetSize = 188;
    int writePacketAmount = 6;
    char* buffer = new char[packetSize];
    ALOGW("[Demux] broadcast input thread loop start %s", mFrontendSourceFile.c_str());
    if (!inputData.is_open()) {
        mBroadcastInputThreadRunning = false;
        ALOGW("[Demux] Error %s", strerror(errno));
    }

    while (mBroadcastInputThreadRunning) {
        // move the stream pointer for packet size * 6 every read until the end
        while (mKeepFetchingDataFromFrontend) {
            for (int i = 0; i < writePacketAmount; i++) {
                inputData.read(buffer, packetSize);
                if (!inputData) {
                    mBroadcastInputThreadRunning = false;
                    break;
                }
                // filter and dispatch filter output
                vector<uint8_t> byteBuffer;
                byteBuffer.resize(packetSize);
                for (int index = 0; index < byteBuffer.size(); index++) {
                    byteBuffer[index] = static_cast<uint8_t>(buffer[index]);
                }
                startTsFilter(byteBuffer);
            }
            startFilterDispatcher();
            sleep(1);
        }
    }

    ALOGW("[Demux] Broadcast Input thread end.");
    delete[] buffer;
    inputData.close();
}

void Demux::stopBroadcastInput() {
    mKeepFetchingDataFromFrontend = false;
    mBroadcastInputThreadRunning = false;
    std::lock_guard<std::mutex> lock(mBroadcastInputThreadLock);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
