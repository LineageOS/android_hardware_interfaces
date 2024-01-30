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
#define LOG_TAG "android.hardware.tv.tuner-service.example-Demux"

#include <aidl/android/hardware/tv/tuner/DemuxQueueNotifyBits.h>
#include <aidl/android/hardware/tv/tuner/Result.h>

#include <fmq/AidlMessageQueue.h>
#include <utils/Log.h>
#include <thread>
#include "Demux.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::AidlMessageQueue;
using ::android::hardware::EventFlag;

using FilterMQ = AidlMessageQueue<int8_t, SynchronizedReadWrite>;
using AidlMQ = AidlMessageQueue<int8_t, SynchronizedReadWrite>;
using AidlMQDesc = MQDescriptor<int8_t, SynchronizedReadWrite>;

#define WAIT_TIMEOUT 3000000000

Demux::Demux(int32_t demuxId, uint32_t filterTypes) {
    mDemuxId = demuxId;
    mFilterTypes = filterTypes;
}

void Demux::setTunerService(std::shared_ptr<Tuner> tuner) {
    mTuner = tuner;
}

Demux::~Demux() {
    ALOGV("%s", __FUNCTION__);
    if (mDemuxIptvReadThread.joinable()) {
        mDemuxIptvReadThread.join();
    }
    close();
}

::ndk::ScopedAStatus Demux::openDvr(DvrType in_type, int32_t in_bufferSize,
                                    const std::shared_ptr<IDvrCallback>& in_cb,
                                    std::shared_ptr<IDvr>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (in_cb == nullptr) {
        ALOGW("[Demux] DVR callback can't be null");
        *_aidl_return = nullptr;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    set<int64_t>::iterator it;
    switch (in_type) {
        case DvrType::PLAYBACK:
            mDvrPlayback = ndk::SharedRefBase::make<Dvr>(in_type, in_bufferSize, in_cb,
                                                         this->ref<Demux>());
            if (!mDvrPlayback->createDvrMQ()) {
                ALOGE("[Demux] cannot create dvr message queue");
                mDvrPlayback = nullptr;
                *_aidl_return = mDvrPlayback;
                return ::ndk::ScopedAStatus::fromServiceSpecificError(
                        static_cast<int32_t>(Result::UNKNOWN_ERROR));
            }

            for (it = mPlaybackFilterIds.begin(); it != mPlaybackFilterIds.end(); it++) {
                if (!mDvrPlayback->addPlaybackFilter(*it, mFilters[*it])) {
                    ALOGE("[Demux] Can't get filter info for DVR playback");
                    mDvrPlayback = nullptr;
                    *_aidl_return = mDvrPlayback;
                    return ::ndk::ScopedAStatus::fromServiceSpecificError(
                            static_cast<int32_t>(Result::UNKNOWN_ERROR));
                }
            }

            ALOGI("Playback normal case");

            *_aidl_return = mDvrPlayback;
            return ::ndk::ScopedAStatus::ok();
        case DvrType::RECORD:
            mDvrRecord = ndk::SharedRefBase::make<Dvr>(in_type, in_bufferSize, in_cb,
                                                       this->ref<Demux>());
            if (!mDvrRecord->createDvrMQ()) {
                mDvrRecord = nullptr;
                *_aidl_return = mDvrRecord;
                return ::ndk::ScopedAStatus::fromServiceSpecificError(
                        static_cast<int32_t>(Result::UNKNOWN_ERROR));
            }

            *_aidl_return = mDvrRecord;
            return ::ndk::ScopedAStatus::ok();
        default:
            *_aidl_return = nullptr;
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }
}

void Demux::setIptvThreadRunning(bool isIptvThreadRunning) {
    std::unique_lock<std::mutex> lock(mIsIptvThreadRunningMutex);
    mIsIptvReadThreadRunning = isIptvThreadRunning;
    mIsIptvThreadRunningCv.notify_all();
}

void Demux::readIptvThreadLoop(dtv_plugin* interface, dtv_streamer* streamer, size_t buf_size,
                               int timeout_ms, int buffer_timeout) {
    Timer *timer, *fullBufferTimer;
    while (true) {
        std::unique_lock<std::mutex> lock(mIsIptvThreadRunningMutex);
        mIsIptvThreadRunningCv.wait(lock, [this] { return mIsIptvReadThreadRunning; });
        if (mIsIptvDvrFMQFull && fullBufferTimer->get_elapsed_time_ms() > buffer_timeout) {
            ALOGE("DVR FMQ has not been flushed within timeout of %d ms", buffer_timeout);
            delete fullBufferTimer;
            break;
        }
        timer = new Timer();
        void* buf = malloc(sizeof(char) * IPTV_BUFFER_SIZE);
        if (buf == nullptr) ALOGI("Buffer allocation failed");
        ssize_t bytes_read = interface->read_stream(streamer, buf, buf_size, timeout_ms);
        if (bytes_read == 0) {
            double elapsed_time = timer->get_elapsed_time_ms();
            if (elapsed_time > timeout_ms) {
                ALOGE("[Demux] timeout reached - elapsed_time: %f, timeout: %d", elapsed_time,
                      timeout_ms);
            }
            ALOGE("[Demux] Cannot read data from the socket");
            delete timer;
            break;
        }

        delete timer;
        ALOGI("Number of bytes read: %zd", bytes_read);
        int result = mDvrPlayback->writePlaybackFMQ(buf, bytes_read);

        switch (result) {
            case DVR_WRITE_FAILURE_REASON_FMQ_FULL:
                if (!mIsIptvDvrFMQFull) {
                    mIsIptvDvrFMQFull = true;
                    fullBufferTimer = new Timer();
                }
                ALOGI("Waiting for client to flush DVR FMQ.");
                break;
            case DVR_WRITE_FAILURE_REASON_UNKNOWN:
                ALOGE("Failed to write data into DVR FMQ for unknown reason");
                break;
            case DVR_WRITE_SUCCESS:
                ALOGI("Wrote %zd bytes to DVR FMQ", bytes_read);
                break;
            default:
                ALOGI("Invalid DVR Status");
        }

        free(buf);
    }
}

::ndk::ScopedAStatus Demux::setFrontendDataSource(int32_t in_frontendId) {
    ALOGV("%s", __FUNCTION__);

    if (mTuner == nullptr) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::NOT_INITIALIZED));
    }
    mFrontend = mTuner->getFrontendById(in_frontendId);
    if (mFrontend == nullptr) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }

    mTuner->setFrontendAsDemuxSource(in_frontendId, mDemuxId);

    // if mFrontend is an IPTV frontend, create streamer to read TS data from socket
    if (mFrontend->getFrontendType() == FrontendType::IPTV) {
        // create a DVR instance on the demux
        shared_ptr<IDvr> iptvDvr;

        std::shared_ptr<IDvrCallback> dvrPlaybackCallback =
                ::ndk::SharedRefBase::make<DvrPlaybackCallback>();

        ::ndk::ScopedAStatus status =
                openDvr(DvrType::PLAYBACK, IPTV_BUFFER_SIZE, dvrPlaybackCallback, &iptvDvr);
        if (status.isOk()) {
            ALOGI("DVR instance created");
        }

        // get plugin interface from frontend
        dtv_plugin* interface = mFrontend->getIptvPluginInterface();
        if (interface == nullptr) {
            ALOGE("[Demux] getIptvPluginInterface(): plugin interface is null");
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_STATE));
        }
        ALOGI("[Demux] getIptvPluginInterface(): plugin interface is not null");

        // get streamer object from Frontend instance
        dtv_streamer* streamer = mFrontend->getIptvPluginStreamer();
        if (streamer == nullptr) {
            ALOGE("[Demux] getIptvPluginStreamer(): streamer is null");
            return ::ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(Result::INVALID_STATE));
        }
        ALOGI("[Demux] getIptvPluginStreamer(): streamer is not null");

        // get transport description from frontend
        string transport_desc = mFrontend->getIptvTransportDescription();
        ALOGI("[Demux] getIptvTransportDescription(): transport_desc: %s", transport_desc.c_str());

        // call read_stream on the socket to populate the buffer with TS data
        // while thread is alive, keep reading data
        int timeout_ms = 20;
        int buffer_timeout = 10000;  // 10s
        mDemuxIptvReadThread = std::thread(&Demux::readIptvThreadLoop, this, interface, streamer,
                                           IPTV_BUFFER_SIZE, timeout_ms, buffer_timeout);
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Demux::openFilter(const DemuxFilterType& in_type, int32_t in_bufferSize,
                                       const std::shared_ptr<IFilterCallback>& in_cb,
                                       std::shared_ptr<IFilter>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    int64_t filterId;
    filterId = ++mLastUsedFilterId;

    if (in_cb == nullptr) {
        ALOGW("[Demux] callback can't be null");
        *_aidl_return = nullptr;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    std::shared_ptr<Filter> filter = ndk::SharedRefBase::make<Filter>(
            in_type, filterId, in_bufferSize, in_cb, this->ref<Demux>());
    if (!filter->createFilterMQ()) {
        *_aidl_return = nullptr;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNKNOWN_ERROR));
    }

    mFilters[filterId] = filter;
    if (filter->isPcrFilter()) {
        mPcrFilterIds.insert(filterId);
    }
    bool result = true;
    if (!filter->isRecordFilter()) {
        // Only save non-record filters for now. Record filters are saved when the
        // IDvr.attacheFilter is called.
        mPlaybackFilterIds.insert(filterId);
        if (mDvrPlayback != nullptr) {
            result = mDvrPlayback->addPlaybackFilter(filterId, filter);
        }
    }

    if (!result) {
        *_aidl_return = nullptr;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    *_aidl_return = filter;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Demux::openTimeFilter(std::shared_ptr<ITimeFilter>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    mTimeFilter = ndk::SharedRefBase::make<TimeFilter>(this->ref<Demux>());

    *_aidl_return = mTimeFilter;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Demux::getAvSyncHwId(const std::shared_ptr<IFilter>& in_filter,
                                          int32_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    int64_t id;
    ::ndk::ScopedAStatus status;

    status = in_filter->getId64Bit(&id);
    if (!status.isOk()) {
        ALOGE("[Demux] Can't get filter Id.");
        *_aidl_return = -1;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }

    if (!mFilters[id]->isMediaFilter()) {
        ALOGE("[Demux] Given filter is not a media filter.");
        *_aidl_return = -1;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }

    if (!mPcrFilterIds.empty()) {
        // Return the lowest pcr filter id in the default implementation as the av sync id
        *_aidl_return = *mPcrFilterIds.begin();
        return ::ndk::ScopedAStatus::ok();
    }

    ALOGE("[Demux] No PCR filter opened.");
    *_aidl_return = -1;
    return ::ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(Result::INVALID_STATE));
}

::ndk::ScopedAStatus Demux::getAvSyncTime(int32_t in_avSyncHwId, int64_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (mPcrFilterIds.empty()) {
        *_aidl_return = -1;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }
    if (in_avSyncHwId != *mPcrFilterIds.begin()) {
        *_aidl_return = -1;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    *_aidl_return = -1;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Demux::close() {
    ALOGV("%s", __FUNCTION__);

    stopFrontendInput();

    set<int64_t>::iterator it;
    for (it = mPlaybackFilterIds.begin(); it != mPlaybackFilterIds.end(); it++) {
        mDvrPlayback->removePlaybackFilter(*it);
    }
    mPlaybackFilterIds.clear();
    mRecordFilterIds.clear();
    mFilters.clear();
    mLastUsedFilterId = -1;
    if (mTuner != nullptr) {
        mTuner->removeDemux(mDemuxId);
        mTuner = nullptr;
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Demux::connectCiCam(int32_t in_ciCamId) {
    ALOGV("%s", __FUNCTION__);

    mCiCamId = in_ciCamId;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Demux::disconnectCiCam() {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Demux::removeFilter(int64_t filterId) {
    ALOGV("%s", __FUNCTION__);

    if (mDvrPlayback != nullptr) {
        mDvrPlayback->removePlaybackFilter(filterId);
    }
    mPlaybackFilterIds.erase(filterId);
    mRecordFilterIds.erase(filterId);
    mFilters.erase(filterId);

    return ::ndk::ScopedAStatus::ok();
}

void Demux::startBroadcastTsFilter(vector<int8_t> data) {
    set<int64_t>::iterator it;
    uint16_t pid = ((data[1] & 0x1f) << 8) | ((data[2] & 0xff));
    if (DEBUG_DEMUX) {
        ALOGW("[Demux] start ts filter pid: %d", pid);
    }
    for (it = mPlaybackFilterIds.begin(); it != mPlaybackFilterIds.end(); it++) {
        if (pid == mFilters[*it]->getTpid()) {
            mFilters[*it]->updateFilterOutput(data);
        }
    }
}

void Demux::sendFrontendInputToRecord(vector<int8_t> data) {
    set<int64_t>::iterator it;
    if (DEBUG_DEMUX) {
        ALOGW("[Demux] update record filter output");
    }
    for (it = mRecordFilterIds.begin(); it != mRecordFilterIds.end(); it++) {
        mFilters[*it]->updateRecordOutput(data);
    }
}

void Demux::sendFrontendInputToRecord(vector<int8_t> data, uint16_t pid, uint64_t pts) {
    sendFrontendInputToRecord(data);
    set<int64_t>::iterator it;
    for (it = mRecordFilterIds.begin(); it != mRecordFilterIds.end(); it++) {
        if (pid == mFilters[*it]->getTpid()) {
            mFilters[*it]->updatePts(pts);
        }
    }
}

bool Demux::startBroadcastFilterDispatcher() {
    set<int64_t>::iterator it;

    // Handle the output data per filter type
    for (it = mPlaybackFilterIds.begin(); it != mPlaybackFilterIds.end(); it++) {
        if (!mFilters[*it]->startFilterHandler().isOk()) {
            return false;
        }
    }

    return true;
}

bool Demux::startRecordFilterDispatcher() {
    set<int64_t>::iterator it;

    for (it = mRecordFilterIds.begin(); it != mRecordFilterIds.end(); it++) {
        if (!mFilters[*it]->startRecordFilterHandler().isOk()) {
            return false;
        }
    }

    return true;
}

::ndk::ScopedAStatus Demux::startFilterHandler(int64_t filterId) {
    return mFilters[filterId]->startFilterHandler();
}

void Demux::updateFilterOutput(int64_t filterId, vector<int8_t> data) {
    mFilters[filterId]->updateFilterOutput(data);
}

void Demux::updateMediaFilterOutput(int64_t filterId, vector<int8_t> data, uint64_t pts) {
    updateFilterOutput(filterId, data);
    mFilters[filterId]->updatePts(pts);
}

uint16_t Demux::getFilterTpid(int64_t filterId) {
    return mFilters[filterId]->getTpid();
}

int32_t Demux::getDemuxId() {
    return mDemuxId;
}

bool Demux::isInUse() {
    return mInUse;
}

void Demux::setInUse(bool inUse) {
    mInUse = inUse;
}

void Demux::getDemuxInfo(DemuxInfo* demuxInfo) {
    *demuxInfo = {.filterTypes = mFilterTypes};
}

void Demux::startFrontendInputLoop() {
    ALOGD("[Demux] start frontend on demux");
    // Stop current Frontend thread loop first, in case the user starts a new
    // tuning before stopping current tuning.
    stopFrontendInput();
    mFrontendInputThreadRunning = true;
    mFrontendInputThread = std::thread(&Demux::frontendInputThreadLoop, this);
}

void Demux::frontendInputThreadLoop() {
    if (!mFrontendInputThreadRunning) {
        return;
    }

    if (!mDvrPlayback) {
        ALOGW("[Demux] No software Frontend input configured. Ending Frontend thread loop.");
        mFrontendInputThreadRunning = false;
        return;
    }

    while (mFrontendInputThreadRunning) {
        uint32_t efState = 0;
        ::android::status_t status = mDvrPlayback->getDvrEventFlag()->wait(
                static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY), &efState, WAIT_TIMEOUT,
                true /* retry on spurious wake */);
        if (status != ::android::OK) {
            ALOGD("[Demux] wait for data ready on the playback FMQ");
            continue;
        }
        if (mDvrPlayback->getSettings().get<DvrSettings::Tag::playback>().dataFormat ==
            DataFormat::ES) {
            if (!mDvrPlayback->processEsDataOnPlayback(true /*isVirtualFrontend*/, mIsRecording)) {
                ALOGE("[Demux] playback es data failed to be filtered. Ending thread");
                break;
            }
            continue;
        }
        // Our current implementation filter the data and write it into the filter FMQ immediately
        // after the DATA_READY from the VTS/framework
        // This is for the non-ES data source, real playback use case handling.
        if (!mDvrPlayback->readPlaybackFMQ(true /*isVirtualFrontend*/, mIsRecording) ||
            !mDvrPlayback->startFilterDispatcher(true /*isVirtualFrontend*/, mIsRecording)) {
            ALOGE("[Demux] playback data failed to be filtered. Ending thread");
            break;
        }
    }

    mFrontendInputThreadRunning = false;
    ALOGW("[Demux] Frontend Input thread end.");
}

void Demux::stopFrontendInput() {
    ALOGD("[Demux] stop frontend on demux");
    mKeepFetchingDataFromFrontend = false;
    mFrontendInputThreadRunning = false;
    if (mFrontendInputThread.joinable()) {
        mFrontendInputThread.join();
    }
}

void Demux::setIsRecording(bool isRecording) {
    mIsRecording = isRecording;
}

bool Demux::isRecording() {
    return mIsRecording;
}

binder_status_t Demux::dump(int fd, const char** args, uint32_t numArgs) {
    dprintf(fd, " Demux %d:\n", mDemuxId);
    dprintf(fd, "  mIsRecording %d\n", mIsRecording);
    {
        dprintf(fd, "  Filters:\n");
        map<int64_t, std::shared_ptr<Filter>>::iterator it;
        for (it = mFilters.begin(); it != mFilters.end(); it++) {
            it->second->dump(fd, args, numArgs);
        }
    }
    {
        dprintf(fd, "  TimeFilter:\n");
        if (mTimeFilter != nullptr) {
            mTimeFilter->dump(fd, args, numArgs);
        }
    }
    {
        dprintf(fd, "  DvrPlayback:\n");
        if (mDvrPlayback != nullptr) {
            mDvrPlayback->dump(fd, args, numArgs);
        }
    }
    {
        dprintf(fd, "  DvrRecord:\n");
        if (mDvrRecord != nullptr) {
            mDvrRecord->dump(fd, args, numArgs);
        }
    }
    return STATUS_OK;
}

bool Demux::attachRecordFilter(int64_t filterId) {
    if (mFilters[filterId] == nullptr || mDvrRecord == nullptr ||
        !mFilters[filterId]->isRecordFilter()) {
        return false;
    }

    mRecordFilterIds.insert(filterId);
    mFilters[filterId]->attachFilterToRecord(mDvrRecord);

    return true;
}

bool Demux::detachRecordFilter(int64_t filterId) {
    if (mFilters[filterId] == nullptr || mDvrRecord == nullptr) {
        return false;
    }

    mRecordFilterIds.erase(filterId);
    mFilters[filterId]->detachFilterFromRecord();

    return true;
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
