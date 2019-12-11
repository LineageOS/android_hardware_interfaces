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

    return startFrontendInputLoop();
}

Return<void> Demux::openFilter(const DemuxFilterType& type, uint32_t bufferSize,
                               const sp<IFilterCallback>& cb, openFilter_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    uint32_t filterId;

    if (!mUnusedFilterIds.empty()) {
        filterId = *mUnusedFilterIds.begin();

        mUnusedFilterIds.erase(filterId);
    } else {
        filterId = ++mLastUsedFilterId;
    }

    mUsedFilterIds.insert(filterId);

    if (cb == nullptr) {
        ALOGW("callback can't be null");
        _hidl_cb(Result::INVALID_ARGUMENT, new Filter());
        return Void();
    }

    sp<Filter> filter = new Filter(type, filterId, bufferSize, cb, this);

    if (!filter->createFilterMQ()) {
        _hidl_cb(Result::UNKNOWN_ERROR, filter);
        return Void();
    }

    mFilters[filterId] = filter;

    _hidl_cb(Result::SUCCESS, filter);
    return Void();
}

Return<void> Demux::openTimeFilter(openTimeFilter_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    sp<TimeFilter> timeFilter = new TimeFilter(this);

    _hidl_cb(Result::SUCCESS, timeFilter);
    return Void();
}

Return<void> Demux::getAvSyncHwId(const sp<IFilter>& /* filter */, getAvSyncHwId_cb _hidl_cb) {
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

    mUnusedFilterIds.clear();
    mUsedFilterIds.clear();
    mLastUsedFilterId = -1;

    return Result::SUCCESS;
}

Return<void> Demux::openDvr(DvrType type, uint32_t bufferSize, const sp<IDvrCallback>& cb,
                            openDvr_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (cb == nullptr) {
        ALOGW("DVR callback can't be null");
        _hidl_cb(Result::INVALID_ARGUMENT, new Dvr());
        return Void();
    }

    mDvr = new Dvr(type, bufferSize, cb, this);

    if (!mDvr->createDvrMQ()) {
        _hidl_cb(Result::UNKNOWN_ERROR, mDvr);
        return Void();
    }

    _hidl_cb(Result::SUCCESS, mDvr);
    return Void();
}

Return<Result> Demux::connectCiCam(uint32_t ciCamId) {
    ALOGV("%s", __FUNCTION__);

    mCiCamId = ciCamId;

    return Result::SUCCESS;
}

Return<Result> Demux::disconnectCiCam() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Result Demux::removeFilter(uint32_t filterId) {
    ALOGV("%s", __FUNCTION__);

    // resetFilterRecords(filterId);
    mUsedFilterIds.erase(filterId);
    mRecordFilterIds.erase(filterId);
    mUnusedFilterIds.insert(filterId);
    mFilters.erase(filterId);

    return Result::SUCCESS;
}

void Demux::startBroadcastTsFilter(vector<uint8_t> data) {
    set<uint32_t>::iterator it;
    for (it = mUsedFilterIds.begin(); it != mUsedFilterIds.end(); it++) {
        uint16_t pid = ((data[1] & 0x1f) << 8) | ((data[2] & 0xff));
        if (DEBUG_FILTER) {
            ALOGW("start ts filter pid: %d", pid);
        }
        if (pid == mFilters[*it]->getTpid()) {
            mFilters[*it]->updateFilterOutput(data);
        }
    }
}

void Demux::sendFrontendInputToRecord(vector<uint8_t> data) {
    set<uint32_t>::iterator it;
    for (it = mRecordFilterIds.begin(); it != mRecordFilterIds.end(); it++) {
        if (DEBUG_FILTER) {
            ALOGW("update record filter output");
        }
        mFilters[*it]->updateRecordOutput(data);
    }
}

bool Demux::startBroadcastFilterDispatcher() {
    set<uint32_t>::iterator it;

    // Handle the output data per filter type
    for (it = mUsedFilterIds.begin(); it != mUsedFilterIds.end(); it++) {
        if (mFilters[*it]->startFilterHandler() != Result::SUCCESS) {
            return false;
        }
    }

    return true;
}

bool Demux::startRecordFilterDispatcher() {
    set<uint32_t>::iterator it;

    for (it = mRecordFilterIds.begin(); it != mRecordFilterIds.end(); it++) {
        if (mFilters[*it]->startRecordFilterHandler() != Result::SUCCESS) {
            return false;
        }
    }

    return true;
}

Result Demux::startFilterHandler(uint32_t filterId) {
    return mFilters[filterId]->startFilterHandler();
}

void Demux::updateFilterOutput(uint16_t filterId, vector<uint8_t> data) {
    mFilters[filterId]->updateFilterOutput(data);
}

uint16_t Demux::getFilterTpid(uint32_t filterId) {
    return mFilters[filterId]->getTpid();
}

Result Demux::startFrontendInputLoop() {
    pthread_create(&mFrontendInputThread, NULL, __threadLoopFrontend, this);
    pthread_setname_np(mFrontendInputThread, "frontend_input_thread");

    return Result::SUCCESS;
}

void* Demux::__threadLoopFrontend(void* user) {
    Demux* const self = static_cast<Demux*>(user);
    self->frontendInputThreadLoop();
    return 0;
}

void Demux::frontendInputThreadLoop() {
    std::lock_guard<std::mutex> lock(mFrontendInputThreadLock);
    mFrontendInputThreadRunning = true;
    mKeepFetchingDataFromFrontend = true;

    // open the stream and get its length
    std::ifstream inputData(mFrontendSourceFile, std::ifstream::binary);
    // TODO take the packet size from the frontend setting
    int packetSize = 188;
    int writePacketAmount = 6;
    char* buffer = new char[packetSize];
    ALOGW("[Demux] Frontend input thread loop start %s", mFrontendSourceFile.c_str());
    if (!inputData.is_open()) {
        mFrontendInputThreadRunning = false;
        ALOGW("[Demux] Error %s", strerror(errno));
    }

    while (mFrontendInputThreadRunning) {
        // move the stream pointer for packet size * 6 every read until the end
        while (mKeepFetchingDataFromFrontend) {
            for (int i = 0; i < writePacketAmount; i++) {
                inputData.read(buffer, packetSize);
                if (!inputData) {
                    mKeepFetchingDataFromFrontend = false;
                    mFrontendInputThreadRunning = false;
                    break;
                }
                // filter and dispatch filter output
                vector<uint8_t> byteBuffer;
                byteBuffer.resize(packetSize);
                for (int index = 0; index < byteBuffer.size(); index++) {
                    byteBuffer[index] = static_cast<uint8_t>(buffer[index]);
                }
                if (mIsRecording) {
                    // Feed the data into the Dvr recording input
                    sendFrontendInputToRecord(byteBuffer);
                } else {
                    // Feed the data into the broadcast demux filter
                    startBroadcastTsFilter(byteBuffer);
                }
            }
            if (mIsRecording) {
                // Dispatch the data into the broadcasting filters.
                startRecordFilterDispatcher();
            } else {
                // Dispatch the data into the broadcasting filters.
                startBroadcastFilterDispatcher();
            }
            usleep(100);
        }
    }

    ALOGW("[Demux] Frontend Input thread end.");
    delete[] buffer;
    inputData.close();
}

void Demux::stopFrontendInput() {
    ALOGD("[Demux] stop frontend on demux");
    mKeepFetchingDataFromFrontend = false;
    mFrontendInputThreadRunning = false;
    std::lock_guard<std::mutex> lock(mFrontendInputThreadLock);
}

void Demux::setIsRecording(bool isRecording) {
    mIsRecording = isRecording;
}

bool Demux::attachRecordFilter(int filterId) {
    if (mFilters[filterId] == nullptr || mDvr == nullptr) {
        return false;
    }

    mRecordFilterIds.insert(filterId);
    mFilters[filterId]->attachFilterToRecord(mDvr);

    return true;
}

bool Demux::detachRecordFilter(int filterId) {
    if (mFilters[filterId] == nullptr || mDvr == nullptr) {
        return false;
    }

    mRecordFilterIds.erase(filterId);
    mFilters[filterId]->detachFilterFromRecord();

    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
