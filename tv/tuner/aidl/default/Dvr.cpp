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
#define LOG_TAG "android.hardware.tv.tuner-service.example-Dvr"

#include <aidl/android/hardware/tv/tuner/DemuxQueueNotifyBits.h>
#include <aidl/android/hardware/tv/tuner/Result.h>

#include <utils/Log.h>
#include "Dvr.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

#define WAIT_TIMEOUT 3000000000

Dvr::Dvr(DvrType type, uint32_t bufferSize, const std::shared_ptr<IDvrCallback>& cb,
         std::shared_ptr<Demux> demux) {
    mType = type;
    mBufferSize = bufferSize;
    mCallback = cb;
    mDemux = demux;
}

Dvr::~Dvr() {
    // make sure thread has joined
    close();
}

::ndk::ScopedAStatus Dvr::getQueueDesc(MQDescriptor<int8_t, SynchronizedReadWrite>* out_queue) {
    ALOGV("%s", __FUNCTION__);

    *out_queue = mDvrMQ->dupeDesc();

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Dvr::configure(const DvrSettings& in_settings) {
    ALOGV("%s", __FUNCTION__);

    mDvrSettings = in_settings;
    mDvrConfigured = true;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Dvr::attachFilter(const std::shared_ptr<IFilter>& in_filter) {
    ALOGV("%s", __FUNCTION__);

    int64_t filterId;
    ::ndk::ScopedAStatus status = in_filter->getId64Bit(&filterId);
    if (!status.isOk()) {
        return status;
    }

    if (!mDemux->attachRecordFilter(filterId)) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Dvr::detachFilter(const std::shared_ptr<IFilter>& in_filter) {
    ALOGV("%s", __FUNCTION__);

    int64_t filterId;
    ::ndk::ScopedAStatus status = in_filter->getId64Bit(&filterId);
    if (!status.isOk()) {
        return status;
    }

    if (!mDemux->detachRecordFilter(filterId)) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Dvr::start() {
    ALOGV("%s", __FUNCTION__);
    if (mDvrThreadRunning) {
        return ::ndk::ScopedAStatus::ok();
    }

    if (!mCallback) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::NOT_INITIALIZED));
    }

    if (!mDvrConfigured) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_STATE));
    }

    if (mType == DvrType::PLAYBACK) {
        mDvrThreadRunning = true;
        mDvrThread = std::thread(&Dvr::playbackThreadLoop, this);
    } else if (mType == DvrType::RECORD) {
        mRecordStatus = RecordStatus::DATA_READY;
        mDemux->setIsRecording(mType == DvrType::RECORD);
    }

    // TODO start another thread to send filter status callback to the framework

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Dvr::stop() {
    ALOGV("%s", __FUNCTION__);

    mDvrThreadRunning = false;
    if (mDvrThread.joinable()) {
        mDvrThread.join();
    }
    // thread should always be joinable if it is running,
    // so it should be safe to assume recording stopped.
    mDemux->setIsRecording(false);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Dvr::flush() {
    ALOGV("%s", __FUNCTION__);

    mRecordStatus = RecordStatus::DATA_READY;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Dvr::close() {
    ALOGV("%s", __FUNCTION__);

    stop();

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Dvr::setStatusCheckIntervalHint(int64_t /* in_milliseconds */) {
    ALOGV("%s", __FUNCTION__);

    // There is no active polling in this default implementation,
    // so directly return ok here.
    return ::ndk::ScopedAStatus::ok();
}

bool Dvr::createDvrMQ() {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    unique_ptr<DvrMQ> tmpDvrMQ = unique_ptr<DvrMQ>(new (nothrow) DvrMQ(mBufferSize, true));
    if (!tmpDvrMQ->isValid()) {
        ALOGW("[Dvr] Failed to create FMQ of DVR");
        return false;
    }

    mDvrMQ = std::move(tmpDvrMQ);

    if (EventFlag::createEventFlag(mDvrMQ->getEventFlagWord(), &mDvrEventFlag) != ::android::OK) {
        return false;
    }

    return true;
}

EventFlag* Dvr::getDvrEventFlag() {
    return mDvrEventFlag;
}

binder_status_t Dvr::dump(int fd, const char** /* args */, uint32_t /* numArgs */) {
    dprintf(fd, "    Dvr:\n");
    dprintf(fd, "      mType: %hhd\n", mType);
    dprintf(fd, "      mDvrThreadRunning: %d\n", (bool)mDvrThreadRunning);
    return STATUS_OK;
}

void Dvr::playbackThreadLoop() {
    ALOGD("[Dvr] playback threadLoop start.");

    while (mDvrThreadRunning) {
        uint32_t efState = 0;
        ::android::status_t status =
                mDvrEventFlag->wait(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY),
                                    &efState, WAIT_TIMEOUT, true /* retry on spurious wake */);
        if (status != ::android::OK) {
            ALOGD("[Dvr] wait for data ready on the playback FMQ");
            continue;
        }

        // If the both dvr playback and dvr record are created, the playback will be treated as
        // the source of the record. isVirtualFrontend set to true would direct the dvr playback
        // input to the demux record filters or live broadcast filters.
        bool isRecording = mDemux->isRecording();
        bool isVirtualFrontend = isRecording;

        if (mDvrSettings.get<DvrSettings::Tag::playback>().dataFormat == DataFormat::ES) {
            if (!processEsDataOnPlayback(isVirtualFrontend, isRecording)) {
                ALOGE("[Dvr] playback es data failed to be filtered. Ending thread");
                break;
            }
            maySendPlaybackStatusCallback();
            continue;
        }

        // Our current implementation filter the data and write it into the filter FMQ immediately
        // after the DATA_READY from the VTS/framework
        // This is for the non-ES data source, real playback use case handling.
        if (!readPlaybackFMQ(isVirtualFrontend, isRecording) ||
            !startFilterDispatcher(isVirtualFrontend, isRecording)) {
            ALOGE("[Dvr] playback data failed to be filtered. Ending thread");
            break;
        }

        maySendPlaybackStatusCallback();
    }

    mDvrThreadRunning = false;
    ALOGD("[Dvr] playback thread ended.");
}

void Dvr::maySendIptvPlaybackStatusCallback() {
    lock_guard<mutex> lock(mPlaybackStatusLock);
    int availableToRead = mDvrMQ->availableToRead();
    int availableToWrite = mDvrMQ->availableToWrite();

    PlaybackStatus newStatus = checkPlaybackStatusChange(availableToWrite, availableToRead,
                                                         IPTV_PLAYBACK_STATUS_THRESHOLD_HIGH,
                                                         IPTV_PLAYBACK_STATUS_THRESHOLD_LOW);
    if (mPlaybackStatus != newStatus) {
        map<int64_t, std::shared_ptr<Filter>>::iterator it;
        for (it = mFilters.begin(); it != mFilters.end(); it++) {
            std::shared_ptr<Filter> currentFilter = it->second;
            currentFilter->setIptvDvrPlaybackStatus(newStatus);
        }
        mCallback->onPlaybackStatus(newStatus);
        mPlaybackStatus = newStatus;
    }
}

void Dvr::maySendPlaybackStatusCallback() {
    lock_guard<mutex> lock(mPlaybackStatusLock);
    int availableToRead = mDvrMQ->availableToRead();
    int availableToWrite = mDvrMQ->availableToWrite();

    PlaybackStatus newStatus =
            checkPlaybackStatusChange(availableToWrite, availableToRead,
                                      mDvrSettings.get<DvrSettings::Tag::playback>().highThreshold,
                                      mDvrSettings.get<DvrSettings::Tag::playback>().lowThreshold);
    if (mPlaybackStatus != newStatus) {
        mCallback->onPlaybackStatus(newStatus);
        mPlaybackStatus = newStatus;
    }
}

PlaybackStatus Dvr::checkPlaybackStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                              int64_t highThreshold, int64_t lowThreshold) {
    if (availableToWrite == 0) {
        return PlaybackStatus::SPACE_FULL;
    } else if (availableToRead > highThreshold) {
        return PlaybackStatus::SPACE_ALMOST_FULL;
    } else if (availableToRead < lowThreshold) {
        return PlaybackStatus::SPACE_ALMOST_EMPTY;
    } else if (availableToRead == 0) {
        return PlaybackStatus::SPACE_EMPTY;
    }
    return mPlaybackStatus;
}

bool Dvr::readPlaybackFMQ(bool isVirtualFrontend, bool isRecording) {
    // Read playback data from the input FMQ
    size_t size = mDvrMQ->availableToRead();
    int64_t playbackPacketSize = mDvrSettings.get<DvrSettings::Tag::playback>().packetSize;
    vector<int8_t> dataOutputBuffer;
    dataOutputBuffer.resize(playbackPacketSize);
    // Dispatch the packet to the PID matching filter output buffer
    for (int i = 0; i < size / playbackPacketSize; i++) {
        if (!mDvrMQ->read(dataOutputBuffer.data(), playbackPacketSize)) {
            return false;
        }
        if (isVirtualFrontend) {
            if (isRecording) {
                mDemux->sendFrontendInputToRecord(dataOutputBuffer);
            } else {
                mDemux->startBroadcastTsFilter(dataOutputBuffer);
            }
        } else {
            startTpidFilter(dataOutputBuffer);
        }
    }

    return true;
}

bool Dvr::processEsDataOnPlayback(bool isVirtualFrontend, bool isRecording) {
    // Read ES from the DVR FMQ
    // Note that currently we only provides ES with metaData in a specific format to be parsed.
    // The ES size should be smaller than the Playback FMQ size to avoid reading truncated data.
    int size = mDvrMQ->availableToRead();
    vector<int8_t> dataOutputBuffer;
    dataOutputBuffer.resize(size);
    if (!mDvrMQ->read(dataOutputBuffer.data(), size)) {
        return false;
    }

    int metaDataSize = size;
    int totalFrames = 0;
    int videoEsDataSize = 0;
    int audioEsDataSize = 0;
    int audioPid = 0;
    int videoPid = 0;

    vector<MediaEsMetaData> esMeta;
    int videoReadPointer = 0;
    int audioReadPointer = 0;
    int frameCount = 0;
    // Get meta data from the es
    for (int i = 0; i < metaDataSize; i++) {
        switch (dataOutputBuffer[i]) {
            case 'm':
                metaDataSize = 0;
                getMetaDataValue(i, dataOutputBuffer.data(), metaDataSize);
                videoReadPointer = metaDataSize;
                continue;
            case 'l':
                getMetaDataValue(i, dataOutputBuffer.data(), totalFrames);
                esMeta.resize(totalFrames);
                continue;
            case 'V':
                getMetaDataValue(i, dataOutputBuffer.data(), videoEsDataSize);
                audioReadPointer = metaDataSize + videoEsDataSize;
                continue;
            case 'A':
                getMetaDataValue(i, dataOutputBuffer.data(), audioEsDataSize);
                continue;
            case 'p':
                if (dataOutputBuffer[++i] == 'a') {
                    getMetaDataValue(i, dataOutputBuffer.data(), audioPid);
                } else if (dataOutputBuffer[i] == 'v') {
                    getMetaDataValue(i, dataOutputBuffer.data(), videoPid);
                }
                continue;
            case 'v':
            case 'a':
                if (dataOutputBuffer[i + 1] != ',') {
                    ALOGE("[Dvr] Invalid format meta data.");
                    return false;
                }
                esMeta[frameCount] = {
                        .isAudio = dataOutputBuffer[i] == 'a' ? true : false,
                };
                i += 5;  // Move to Len
                getMetaDataValue(i, dataOutputBuffer.data(), esMeta[frameCount].len);
                if (esMeta[frameCount].isAudio) {
                    esMeta[frameCount].startIndex = audioReadPointer;
                    audioReadPointer += esMeta[frameCount].len;
                } else {
                    esMeta[frameCount].startIndex = videoReadPointer;
                    videoReadPointer += esMeta[frameCount].len;
                }
                i += 4;  // move to PTS
                getMetaDataValue(i, dataOutputBuffer.data(), esMeta[frameCount].pts);
                frameCount++;
                continue;
            default:
                continue;
        }
    }

    if (frameCount != totalFrames) {
        ALOGE("[Dvr] Invalid meta data, frameCount=%d, totalFrames reported=%d", frameCount,
              totalFrames);
        return false;
    }

    if (metaDataSize + audioEsDataSize + videoEsDataSize != size) {
        ALOGE("[Dvr] Invalid meta data, metaSize=%d, videoSize=%d, audioSize=%d, totolSize=%d",
              metaDataSize, videoEsDataSize, audioEsDataSize, size);
        return false;
    }

    // Read es raw data from the FMQ per meta data built previously
    vector<int8_t> frameData;
    map<int64_t, std::shared_ptr<Filter>>::iterator it;
    int pid = 0;
    for (int i = 0; i < totalFrames; i++) {
        frameData.resize(esMeta[i].len);
        pid = esMeta[i].isAudio ? audioPid : videoPid;
        memcpy(frameData.data(), dataOutputBuffer.data() + esMeta[i].startIndex, esMeta[i].len);
        // Send to the media filters or record filters
        if (!isRecording) {
            for (it = mFilters.begin(); it != mFilters.end(); it++) {
                if (pid == mDemux->getFilterTpid(it->first)) {
                    mDemux->updateMediaFilterOutput(it->first, frameData,
                                                    static_cast<uint64_t>(esMeta[i].pts));
                }
            }
        } else {
            mDemux->sendFrontendInputToRecord(frameData, pid, static_cast<uint64_t>(esMeta[i].pts));
        }
        startFilterDispatcher(isVirtualFrontend, isRecording);
        frameData.clear();
    }

    return true;
}

void Dvr::getMetaDataValue(int& index, int8_t* dataOutputBuffer, int& value) {
    index += 2;  // Move the pointer across the ":" to the value
    while (dataOutputBuffer[index] != ',' && dataOutputBuffer[index] != '\n') {
        value = ((dataOutputBuffer[index++] - 48) + value * 10);
    }
}

void Dvr::startTpidFilter(vector<int8_t> data) {
    map<int64_t, std::shared_ptr<Filter>>::iterator it;
    for (it = mFilters.begin(); it != mFilters.end(); it++) {
        uint16_t pid = ((data[1] & 0x1f) << 8) | ((data[2] & 0xff));
        if (DEBUG_DVR) {
            ALOGW("[Dvr] start ts filter pid: %d", pid);
        }
        if (pid == mDemux->getFilterTpid(it->first)) {
            mDemux->updateFilterOutput(it->first, data);
        }
    }
}

bool Dvr::startFilterDispatcher(bool isVirtualFrontend, bool isRecording) {
    if (isVirtualFrontend) {
        if (isRecording) {
            return mDemux->startRecordFilterDispatcher();
        } else {
            return mDemux->startBroadcastFilterDispatcher();
        }
    }

    map<int64_t, std::shared_ptr<Filter>>::iterator it;
    // Handle the output data per filter type
    for (it = mFilters.begin(); it != mFilters.end(); it++) {
        if (!mDemux->startFilterHandler(it->first).isOk()) {
            return false;
        }
    }

    return true;
}

int Dvr::writePlaybackFMQ(void* buf, size_t size) {
    lock_guard<mutex> lock(mWriteLock);
    ALOGI("Playback status: %d", mPlaybackStatus);
    if (mPlaybackStatus == PlaybackStatus::SPACE_FULL) {
        ALOGW("[Dvr] stops writing and wait for the client side flushing.");
        return DVR_WRITE_FAILURE_REASON_FMQ_FULL;
    }
    ALOGI("availableToWrite before: %zu", mDvrMQ->availableToWrite());
    if (mDvrMQ->write((int8_t*)buf, size)) {
        mDvrEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY));
        ALOGI("availableToWrite: %zu", mDvrMQ->availableToWrite());
        maySendIptvPlaybackStatusCallback();
        return DVR_WRITE_SUCCESS;
    }
    maySendIptvPlaybackStatusCallback();
    return DVR_WRITE_FAILURE_REASON_UNKNOWN;
}

bool Dvr::writeRecordFMQ(const vector<int8_t>& data) {
    lock_guard<mutex> lock(mWriteLock);
    if (mRecordStatus == RecordStatus::OVERFLOW) {
        ALOGW("[Dvr] stops writing and wait for the client side flushing.");
        return true;
    }
    if (mDvrMQ->write(data.data(), data.size())) {
        mDvrEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY));
        maySendRecordStatusCallback();
        return true;
    }

    maySendRecordStatusCallback();
    return false;
}

void Dvr::maySendRecordStatusCallback() {
    lock_guard<mutex> lock(mRecordStatusLock);
    int availableToRead = mDvrMQ->availableToRead();
    int availableToWrite = mDvrMQ->availableToWrite();

    RecordStatus newStatus =
            checkRecordStatusChange(availableToWrite, availableToRead,
                                    mDvrSettings.get<DvrSettings::Tag::record>().highThreshold,
                                    mDvrSettings.get<DvrSettings::Tag::record>().lowThreshold);
    if (mRecordStatus != newStatus) {
        mCallback->onRecordStatus(newStatus);
        mRecordStatus = newStatus;
    }
}

RecordStatus Dvr::checkRecordStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                          int64_t highThreshold, int64_t lowThreshold) {
    if (availableToWrite == 0) {
        return RecordStatus::OVERFLOW;
    } else if (availableToRead > highThreshold) {
        return RecordStatus::HIGH_WATER;
    } else if (availableToRead < lowThreshold) {
        return RecordStatus::LOW_WATER;
    }
    return mRecordStatus;
}

bool Dvr::addPlaybackFilter(int64_t filterId, std::shared_ptr<Filter> filter) {
    mFilters[filterId] = filter;
    return true;
}

bool Dvr::removePlaybackFilter(int64_t filterId) {
    mFilters.erase(filterId);
    return true;
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
