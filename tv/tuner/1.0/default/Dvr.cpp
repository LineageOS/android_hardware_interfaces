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

#define LOG_TAG "android.hardware.tv.tuner@1.0-Dvr"

#include "Dvr.h"
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

#define WAIT_TIMEOUT 3000000000

Dvr::Dvr() {}

Dvr::Dvr(DvrType type, uint32_t bufferSize, const sp<IDvrCallback>& cb, sp<Demux> demux) {
    mType = type;
    mBufferSize = bufferSize;
    mCallback = cb;
    mDemux = demux;
}

Dvr::~Dvr() {}

Return<void> Dvr::getQueueDesc(getQueueDesc_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    _hidl_cb(Result::SUCCESS, *mDvrMQ->getDesc());
    return Void();
}

Return<Result> Dvr::configure(const DvrSettings& settings) {
    ALOGV("%s", __FUNCTION__);

    mDvrSettings = settings;
    mDvrConfigured = true;

    return Result::SUCCESS;
}

Return<Result> Dvr::attachFilter(const sp<IFilter>& filter) {
    ALOGV("%s", __FUNCTION__);

    uint32_t filterId;
    Result status;

    filter->getId([&](Result result, uint32_t id) {
        filterId = id;
        status = result;
    });

    if (status != Result::SUCCESS) {
        return status;
    }

    // TODO check if the attached filter is a record filter
    if (!mDemux->attachRecordFilter(filterId)) {
        return Result::INVALID_ARGUMENT;
    }

    return Result::SUCCESS;
}

Return<Result> Dvr::detachFilter(const sp<IFilter>& filter) {
    ALOGV("%s", __FUNCTION__);

    uint32_t filterId;
    Result status;

    filter->getId([&](Result result, uint32_t id) {
        filterId = id;
        status = result;
    });

    if (status != Result::SUCCESS) {
        return status;
    }

    if (!mDemux->detachRecordFilter(filterId)) {
        return Result::INVALID_ARGUMENT;
    }

    return Result::SUCCESS;
}

Return<Result> Dvr::start() {
    ALOGV("%s", __FUNCTION__);

    if (!mCallback) {
        return Result::NOT_INITIALIZED;
    }

    if (!mDvrConfigured) {
        return Result::INVALID_STATE;
    }

    if (mType == DvrType::PLAYBACK) {
        pthread_create(&mDvrThread, NULL, __threadLoopPlayback, this);
        pthread_setname_np(mDvrThread, "playback_waiting_loop");
    } else if (mType == DvrType::RECORD) {
        mRecordStatus = RecordStatus::DATA_READY;
        mDemux->setIsRecording(mType == DvrType::RECORD);
    }

    // TODO start another thread to send filter status callback to the framework

    return Result::SUCCESS;
}

Return<Result> Dvr::stop() {
    ALOGV("%s", __FUNCTION__);

    mDvrThreadRunning = false;

    lock_guard<mutex> lock(mDvrThreadLock);

    mIsRecordStarted = false;
    mDemux->setIsRecording(false);

    return Result::SUCCESS;
}

Return<Result> Dvr::flush() {
    ALOGV("%s", __FUNCTION__);

    mRecordStatus = RecordStatus::DATA_READY;

    return Result::SUCCESS;
}

Return<Result> Dvr::close() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

bool Dvr::createDvrMQ() {
    ALOGV("%s", __FUNCTION__);

    // Create a synchronized FMQ that supports blocking read/write
    unique_ptr<DvrMQ> tmpDvrMQ = unique_ptr<DvrMQ>(new (nothrow) DvrMQ(mBufferSize, true));
    if (!tmpDvrMQ->isValid()) {
        ALOGW("[Dvr] Failed to create FMQ of DVR");
        return false;
    }

    mDvrMQ = move(tmpDvrMQ);

    if (EventFlag::createEventFlag(mDvrMQ->getEventFlagWord(), &mDvrEventFlag) != OK) {
        return false;
    }

    return true;
}

EventFlag* Dvr::getDvrEventFlag() {
    return mDvrEventFlag;
}

void* Dvr::__threadLoopPlayback(void* user) {
    Dvr* const self = static_cast<Dvr*>(user);
    self->playbackThreadLoop();
    return 0;
}

void Dvr::playbackThreadLoop() {
    ALOGD("[Dvr] playback threadLoop start.");
    lock_guard<mutex> lock(mDvrThreadLock);
    mDvrThreadRunning = true;

    while (mDvrThreadRunning) {
        uint32_t efState = 0;
        status_t status =
                mDvrEventFlag->wait(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY),
                                    &efState, WAIT_TIMEOUT, true /* retry on spurious wake */);
        if (status != OK) {
            ALOGD("[Dvr] wait for data ready on the playback FMQ");
            continue;
        }

        if (mDvrSettings.playback().dataFormat == DataFormat::ES) {
            if (!processEsDataOnPlayback(false /*isVirtualFrontend*/, false /*isRecording*/)) {
                ALOGE("[Dvr] playback es data failed to be filtered. Ending thread");
                break;
            }
            maySendPlaybackStatusCallback();
        }
        // Our current implementation filter the data and write it into the filter FMQ immediately
        // after the DATA_READY from the VTS/framework
        if (!readPlaybackFMQ(false /*isVirtualFrontend*/, false /*isRecording*/) ||
            !startFilterDispatcher(false /*isVirtualFrontend*/, false /*isRecording*/)) {
            ALOGE("[Dvr] playback data failed to be filtered. Ending thread");
            break;
        }

        maySendPlaybackStatusCallback();
    }

    mDvrThreadRunning = false;
    ALOGD("[Dvr] playback thread ended.");
}

void Dvr::maySendPlaybackStatusCallback() {
    lock_guard<mutex> lock(mPlaybackStatusLock);
    int availableToRead = mDvrMQ->availableToRead();
    int availableToWrite = mDvrMQ->availableToWrite();

    PlaybackStatus newStatus = checkPlaybackStatusChange(availableToWrite, availableToRead,
                                                         mDvrSettings.playback().highThreshold,
                                                         mDvrSettings.playback().lowThreshold);
    if (mPlaybackStatus != newStatus) {
        mCallback->onPlaybackStatus(newStatus);
        mPlaybackStatus = newStatus;
    }
}

PlaybackStatus Dvr::checkPlaybackStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                              uint32_t highThreshold, uint32_t lowThreshold) {
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
    int size = mDvrMQ->availableToRead();
    int playbackPacketSize = mDvrSettings.playback().packetSize;
    vector<uint8_t> dataOutputBuffer;
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
    vector<uint8_t> dataOutputBuffer;
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
    vector<uint8_t> frameData;
    map<uint32_t, sp<IFilter>>::iterator it;
    int pid = 0;
    for (int i = 0; i < totalFrames; i++) {
        frameData.resize(esMeta[i].len);
        pid = esMeta[i].isAudio ? audioPid : videoPid;
        memcpy(frameData.data(), dataOutputBuffer.data() + esMeta[i].startIndex, esMeta[i].len);
        // Send to the media filter
        if (isVirtualFrontend && isRecording) {
            // TODO validate record
            mDemux->sendFrontendInputToRecord(frameData);
        } else {
            for (it = mFilters.begin(); it != mFilters.end(); it++) {
                if (pid == mDemux->getFilterTpid(it->first)) {
                    mDemux->updateMediaFilterOutput(it->first, frameData,
                                                    static_cast<uint64_t>(esMeta[i].pts));
                    startFilterDispatcher(isVirtualFrontend, isRecording);
                }
            }
        }
    }

    return true;
}

void Dvr::getMetaDataValue(int& index, uint8_t* dataOutputBuffer, int& value) {
    index += 2;  // Move the pointer across the ":" to the value
    while (dataOutputBuffer[index] != ',' && dataOutputBuffer[index] != '\n') {
        value = ((dataOutputBuffer[index++] - 48) + value * 10);
    }
}

void Dvr::startTpidFilter(vector<uint8_t> data) {
    map<uint32_t, sp<IFilter>>::iterator it;
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

    map<uint32_t, sp<IFilter>>::iterator it;
    // Handle the output data per filter type
    for (it = mFilters.begin(); it != mFilters.end(); it++) {
        if (mDemux->startFilterHandler(it->first) != Result::SUCCESS) {
            return false;
        }
    }

    return true;
}

bool Dvr::writeRecordFMQ(const vector<uint8_t>& data) {
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

    RecordStatus newStatus = checkRecordStatusChange(availableToWrite, availableToRead,
                                                     mDvrSettings.record().highThreshold,
                                                     mDvrSettings.record().lowThreshold);
    if (mRecordStatus != newStatus) {
        mCallback->onRecordStatus(newStatus);
        mRecordStatus = newStatus;
    }
}

RecordStatus Dvr::checkRecordStatusChange(uint32_t availableToWrite, uint32_t availableToRead,
                                          uint32_t highThreshold, uint32_t lowThreshold) {
    if (availableToWrite == 0) {
        return DemuxFilterStatus::OVERFLOW;
    } else if (availableToRead > highThreshold) {
        return DemuxFilterStatus::HIGH_WATER;
    } else if (availableToRead < lowThreshold) {
        return DemuxFilterStatus::LOW_WATER;
    }
    return mRecordStatus;
}

bool Dvr::addPlaybackFilter(uint32_t filterId, sp<IFilter> filter) {
    mFilters[filterId] = filter;
    return true;
}

bool Dvr::removePlaybackFilter(uint32_t filterId) {
    mFilters.erase(filterId);
    return true;
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
