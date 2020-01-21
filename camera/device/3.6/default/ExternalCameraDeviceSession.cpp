/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "ExtCamDevSsn@3.6"
#include <android/log.h>

#include <utils/Trace.h>
#include "ExternalCameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_6 {
namespace implementation {

ExternalCameraDeviceSession::ExternalCameraDeviceSession(
        const sp<V3_2::ICameraDeviceCallback>& callback,
        const ExternalCameraConfig& cfg,
        const std::vector<SupportedV4L2Format>& sortedFormats,
        const CroppingType& croppingType,
        const common::V1_0::helper::CameraMetadata& chars,
        const std::string& cameraId,
        unique_fd v4l2Fd) :
        V3_5::implementation::ExternalCameraDeviceSession(
                callback, cfg, sortedFormats, croppingType, chars, cameraId, std::move(v4l2Fd)) {
}

ExternalCameraDeviceSession::~ExternalCameraDeviceSession() {}


Return<void> ExternalCameraDeviceSession::configureStreams_3_6(
        const StreamConfiguration& requestedConfiguration,
        ICameraDeviceSession::configureStreams_3_6_cb _hidl_cb)  {
    V3_2::StreamConfiguration config_v32;
    V3_3::HalStreamConfiguration outStreams_v33;
    V3_6::HalStreamConfiguration outStreams;
    const V3_4::StreamConfiguration& requestedConfiguration_3_4 = requestedConfiguration.v3_4;
    Mutex::Autolock _il(mInterfaceLock);

    config_v32.operationMode = requestedConfiguration_3_4.operationMode;
    config_v32.streams.resize(requestedConfiguration_3_4.streams.size());
    uint32_t blobBufferSize = 0;
    int numStallStream = 0;
    for (size_t i = 0; i < config_v32.streams.size(); i++) {
        config_v32.streams[i] = requestedConfiguration_3_4.streams[i].v3_2;
        if (config_v32.streams[i].format == PixelFormat::BLOB) {
            blobBufferSize = requestedConfiguration_3_4.streams[i].bufferSize;
            numStallStream++;
        }
    }

    // Fail early if there are multiple BLOB streams
    if (numStallStream > kMaxStallStream) {
        ALOGE("%s: too many stall streams (expect <= %d, got %d)", __FUNCTION__,
                kMaxStallStream, numStallStream);
        _hidl_cb(Status::ILLEGAL_ARGUMENT, outStreams);
        return Void();
    }

    Status status = configureStreams(config_v32, &outStreams_v33, blobBufferSize);

    fillOutputStream3_6(outStreams_v33, &outStreams);

    _hidl_cb(status, outStreams);
    return Void();
}

Return<void> ExternalCameraDeviceSession::switchToOffline(
        const hidl_vec<int32_t>& streamsToKeep,
        ICameraDeviceSession::switchToOffline_cb _hidl_cb) {
    std::vector<NotifyMsg> msgs;
    std::vector<CaptureResult> results;
    CameraOfflineSessionInfo info;
    sp<ICameraOfflineSession> session;

    Status st = switchToOffline(streamsToKeep, &msgs, &results, &info, &session);

    mCallback->notify(msgs);
    hidl_vec<CaptureResult> hidlResults(std::move(results));
    invokeProcessCaptureResultCallback(hidlResults, /* tryWriteFmq */true);
    V3_4::implementation::freeReleaseFences(hidlResults);

    _hidl_cb(st, info, session);
    return Void();
}

void ExternalCameraDeviceSession::fillOutputStream3_6(
        const V3_3::HalStreamConfiguration& outStreams_v33,
        /*out*/V3_6::HalStreamConfiguration* outStreams_v36) {
    if (outStreams_v36 == nullptr) {
        ALOGE("%s: outStreams_v36 must not be null!", __FUNCTION__);
        return;
    }
    Mutex::Autolock _l(mLock);
    outStreams_v36->streams.resize(outStreams_v33.streams.size());
    for (size_t i = 0; i < outStreams_v36->streams.size(); i++) {
        outStreams_v36->streams[i].v3_4.v3_3 = outStreams_v33.streams[i];
        outStreams_v36->streams[i].supportOffline =
                supportOfflineLocked(outStreams_v33.streams[i].v3_2.id);
    }
}

bool ExternalCameraDeviceSession::supportOfflineLocked(int32_t streamId) {
    const Stream& stream = mStreamMap[streamId];
    if (stream.format == PixelFormat::BLOB &&
            stream.dataSpace == static_cast<int32_t>(Dataspace::V0_JFIF)) {
        return true;
    }
    // TODO: support YUV output stream?
    return false;
}

bool ExternalCameraDeviceSession::canDropRequest(const hidl_vec<int32_t>& offlineStreams,
        std::shared_ptr<V3_4::implementation::HalRequest> halReq) {
    for (const auto& buffer : halReq->buffers) {
        for (auto offlineStreamId : offlineStreams) {
            if (buffer.streamId == offlineStreamId) {
                return false;
            }
        }
    }
    // Only drop a request completely if it has no offline output
    return true;
}

void ExternalCameraDeviceSession::fillOfflineSessionInfo(const hidl_vec<int32_t>& offlineStreams,
        std::deque<std::shared_ptr<HalRequest>>& offlineReqs,
        const std::map<int, CirculatingBuffers>& circulatingBuffers,
        /*out*/CameraOfflineSessionInfo* info) {
    if (info == nullptr) {
        ALOGE("%s: output info must not be null!", __FUNCTION__);
        return;
    }

    info->offlineStreams.resize(offlineStreams.size());
    info->offlineRequests.resize(offlineReqs.size());

    std::unordered_map<int32_t, uint32_t> outstandingBufs(offlineStreams.size());
    for (const auto streamId : offlineStreams) {
        outstandingBufs.insert({streamId, 0});
    }
    // Fill in offline reqs and count outstanding buffers
    for (size_t i = 0; i < offlineReqs.size(); i++) {
        info->offlineRequests[i].frameNumber = offlineReqs[i]->frameNumber;
        info->offlineRequests[i].pendingStreams.resize(offlineReqs[i]->buffers.size());
        for (size_t bIdx = 0; bIdx < offlineReqs[i]->buffers.size(); bIdx++) {
            int32_t streamId = offlineReqs[i]->buffers[bIdx].streamId;
            info->offlineRequests[i].pendingStreams[bIdx] = streamId;
            outstandingBufs[streamId]++;
        }
    }

    for (size_t i = 0; i < offlineStreams.size(); i++) {
        int32_t streamId = offlineStreams[i];
        info->offlineStreams[i].id = streamId;
        info->offlineStreams[i].numOutstandingBuffers = outstandingBufs[streamId];
        const CirculatingBuffers& bufIdMap = circulatingBuffers.at(streamId);
        info->offlineStreams[i].circulatingBufferIds.resize(bufIdMap.size());
        size_t bIdx = 0;
        for (const auto& pair : bufIdMap) {
            // Fill in bufferId
            info->offlineStreams[i].circulatingBufferIds[bIdx++] = pair.first;
        }

    }
}

Status ExternalCameraDeviceSession::switchToOffline(const hidl_vec<int32_t>& offlineStreams,
        /*out*/std::vector<NotifyMsg>* msgs,
        /*out*/std::vector<CaptureResult>* results,
        /*out*/CameraOfflineSessionInfo* info,
        /*out*/sp<ICameraOfflineSession>* session) {
    ATRACE_CALL();
    if (offlineStreams.size() > 1) {
        ALOGE("%s: more than one offline stream is not supported", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    if (msgs == nullptr || results == nullptr || info == nullptr || session == nullptr) {
        ALOGE("%s: output arguments (%p, %p, %p, %p) must not be null", __FUNCTION__,
                msgs, results, info, session);
        return Status::ILLEGAL_ARGUMENT;
    }

    msgs->clear();
    results->clear();

    Mutex::Autolock _il(mInterfaceLock);
    Status status = initStatus();
    if (status != Status::OK) {
        return status;
    }

    Mutex::Autolock _l(mLock);
    for (auto streamId : offlineStreams) {
        if (!supportOfflineLocked(streamId)) {
            return Status::ILLEGAL_ARGUMENT;
        }
    }

    // pause output thread and get all remaining inflight requests
    auto remainingReqs = mOutputThread->switchToOffline();
    std::vector<std::shared_ptr<V3_4::implementation::HalRequest>> halReqs;

    // Send out buffer/request error for remaining requests and filter requests
    // to be handled in offline mode
    for (auto& halReq : remainingReqs) {
        bool dropReq = canDropRequest(offlineStreams, halReq);
        if (dropReq) {
            // Request is dropped completely. Just send request error and
            // there is no need to send the request to offline session
            processCaptureRequestError(halReq, msgs, results);
            continue;
        }

        // All requests reach here must have at least one offline stream output
        NotifyMsg shutter;
        shutter.type = MsgType::SHUTTER;
        shutter.msg.shutter.frameNumber = halReq->frameNumber;
        shutter.msg.shutter.timestamp = halReq->shutterTs;
        msgs->push_back(shutter);

        std::vector<V3_4::implementation::HalStreamBuffer> offlineBuffers;
        for (const auto& buffer : halReq->buffers) {
            bool dropBuffer = true;
            for (auto offlineStreamId : offlineStreams) {
                if (buffer.streamId == offlineStreamId) {
                    dropBuffer = false;
                    break;
                }
            }
            if (dropBuffer) {
                NotifyMsg error;
                error.type = MsgType::ERROR;
                error.msg.error.frameNumber = halReq->frameNumber;
                error.msg.error.errorStreamId = buffer.streamId;
                error.msg.error.errorCode = ErrorCode::ERROR_BUFFER;
                msgs->push_back(error);

                CaptureResult result;
                result.frameNumber = halReq->frameNumber;
                result.partialResult = 0; // buffer only result
                result.inputBuffer.streamId = -1;
                result.outputBuffers.resize(1);
                result.outputBuffers[0].streamId = buffer.streamId;
                result.outputBuffers[0].bufferId = buffer.bufferId;
                result.outputBuffers[0].status = BufferStatus::ERROR;
                if (buffer.acquireFence >= 0) {
                    native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
                    handle->data[0] = buffer.acquireFence;
                    result.outputBuffers[0].releaseFence.setTo(handle, /*shouldOwn*/false);
                }
                results->push_back(result);
            } else {
                offlineBuffers.push_back(buffer);
            }
        }
        halReq->buffers = offlineBuffers;
        halReqs.push_back(halReq);
    }

    // convert hal requests to offline request
    std::deque<std::shared_ptr<HalRequest>> offlineReqs(halReqs.size());
    for (auto& v4lReq : halReqs) {
        std::shared_ptr<HalRequest> halReq = std::make_shared<HalRequest>();
        halReq->frameNumber = v4lReq->frameNumber;
        halReq->setting = v4lReq->setting;
        halReq->shutterTs = v4lReq->shutterTs;
        halReq->buffers = v4lReq->buffers;
        sp<V3_4::implementation::V4L2Frame> v4l2Frame =
                static_cast<V3_4::implementation::V4L2Frame*>(v4lReq->frameIn.get());
        halReq->frameIn = new AllocatedV4L2Frame(v4l2Frame);
        offlineReqs.push_back(halReq);
        // enqueue V4L2 frame
        enqueueV4l2Frame(v4l2Frame);
    }

    // Collect buffer caches/streams
    hidl_vec<Stream> streamInfos;
    streamInfos.resize(offlineStreams.size());
    std::map<int, CirculatingBuffers> circulatingBuffers;
    {
        Mutex::Autolock _l(mCbsLock);
        size_t idx = 0;
        for(auto streamId : offlineStreams) {
            circulatingBuffers[streamId] = mCirculatingBuffers.at(streamId);
            mCirculatingBuffers.erase(streamId);
            streamInfos[idx++] = mStreamMap.at(streamId);
            mStreamMap.erase(streamId);
        }
    }

    fillOfflineSessionInfo(offlineStreams, offlineReqs, circulatingBuffers, info);

    // create the offline session object
    bool afTrigger;
    {
        std::lock_guard<std::mutex> lk(mAfTriggerLock);
        afTrigger = mAfTrigger;
    }
    sp<ExternalCameraOfflineSession> sessionImpl = new ExternalCameraOfflineSession(
            mCroppingType, mCameraCharacteristics, mCameraId,
            mExifMake, mExifModel, mBlobBufferSize, afTrigger,
            streamInfos, offlineReqs, circulatingBuffers);

    bool initFailed = sessionImpl->initialize();
    if (initFailed) {
        ALOGE("%s: offline session initialize failed!", __FUNCTION__);
        return Status::INTERNAL_ERROR;
    }

    // cleanup stream and buffer caches
    {
        Mutex::Autolock _l(mCbsLock);
        for(auto pair : mStreamMap) {
            cleanupBuffersLocked(/*Stream ID*/pair.first);
        }
        mCirculatingBuffers.clear();
    }
    mStreamMap.clear();

    // update inflight records
    {
        std::lock_guard<std::mutex> lk(mInflightFramesLock);
        mInflightFrames.clear();
    }

    // stop v4l2 streaming
    if (v4l2StreamOffLocked() !=0) {
        ALOGE("%s: stop V4L2 streaming failed!", __FUNCTION__);
        return Status::INTERNAL_ERROR;
    }

    *session = sessionImpl->getInterface();
    return Status::OK;
}

} // namespace implementation
}  // namespace V3_6
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
