/*
 * Copyright (C) 2017-2018 The Android Open Source Project
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

#define LOG_TAG "CamDevSession@3.4-impl"
#include <android/log.h>

#include <set>
#include <utils/Trace.h>
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include "CameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

CameraDeviceSession::CameraDeviceSession(
    camera3_device_t* device,
    const camera_metadata_t* deviceInfo,
    const sp<V3_2::ICameraDeviceCallback>& callback) :
        V3_3::implementation::CameraDeviceSession(device, deviceInfo, callback),
        mResultBatcher_3_4(callback) {

    mHasCallback_3_4 = false;

    auto castResult = ICameraDeviceCallback::castFrom(callback);
    if (castResult.isOk()) {
        sp<ICameraDeviceCallback> callback3_4 = castResult;
        if (callback3_4 != nullptr) {
            process_capture_result = sProcessCaptureResult_3_4;
            notify = sNotify_3_4;
            mHasCallback_3_4 = true;
            if (!mInitFail) {
                mResultBatcher_3_4.setResultMetadataQueue(mResultMetadataQueue);
            }
        }
    }

    mResultBatcher_3_4.setNumPartialResults(mNumPartialResults);

    camera_metadata_entry_t capabilities =
            mDeviceInfo.find(ANDROID_REQUEST_AVAILABLE_CAPABILITIES);
    bool isLogicalMultiCamera = false;
    for (size_t i = 0; i < capabilities.count; i++) {
        if (capabilities.data.u8[i] ==
                ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
            isLogicalMultiCamera = true;
            break;
        }
    }
    if (isLogicalMultiCamera) {
        camera_metadata_entry entry =
                mDeviceInfo.find(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS);
        const uint8_t* ids = entry.data.u8;
        size_t start = 0;
        for (size_t i = 0; i < entry.count; ++i) {
            if (ids[i] == '\0') {
                if (start != i) {
                    const char* physicalId = reinterpret_cast<const char*>(ids+start);
                    mPhysicalCameraIds.emplace(physicalId);
                }
                start = i + 1;
            }
        }
    }
}

CameraDeviceSession::~CameraDeviceSession() {
}

Return<void> CameraDeviceSession::configureStreams_3_4(
        const StreamConfiguration& requestedConfiguration,
        ICameraDeviceSession::configureStreams_3_4_cb _hidl_cb)  {
    Status status = initStatus();
    HalStreamConfiguration outStreams;

    // If callback is 3.2, make sure no physical stream is configured
    if (!mHasCallback_3_4) {
        for (size_t i = 0; i < requestedConfiguration.streams.size(); i++) {
            if (requestedConfiguration.streams[i].physicalCameraId.size() > 0) {
                ALOGE("%s: trying to configureStreams with physical camera id with V3.2 callback",
                        __FUNCTION__);
                _hidl_cb(Status::INTERNAL_ERROR, outStreams);
                return Void();
            }
        }
    }

    // hold the inflight lock for entire configureStreams scope since there must not be any
    // inflight request/results during stream configuration.
    Mutex::Autolock _l(mInflightLock);
    if (!mInflightBuffers.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight buffers!",
                __FUNCTION__, mInflightBuffers.size());
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    if (!mInflightAETriggerOverrides.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight"
                " trigger overrides!", __FUNCTION__,
                mInflightAETriggerOverrides.size());
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    if (!mInflightRawBoostPresent.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight"
                " boost overrides!", __FUNCTION__,
                mInflightRawBoostPresent.size());
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    if (status != Status::OK) {
        _hidl_cb(status, outStreams);
        return Void();
    }

    const camera_metadata_t *paramBuffer = nullptr;
    if (0 < requestedConfiguration.sessionParams.size()) {
        V3_2::implementation::convertFromHidl(requestedConfiguration.sessionParams, &paramBuffer);
    }

    camera3_stream_configuration_t stream_list{};
    hidl_vec<camera3_stream_t*> streams;
    stream_list.session_parameters = paramBuffer;
    if (!preProcessConfigurationLocked_3_4(requestedConfiguration, &stream_list, &streams)) {
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    ATRACE_BEGIN("camera3->configure_streams");
    status_t ret = mDevice->ops->configure_streams(mDevice, &stream_list);
    ATRACE_END();

    // In case Hal returns error most likely it was not able to release
    // the corresponding resources of the deleted streams.
    if (ret == OK) {
        postProcessConfigurationLocked_3_4(requestedConfiguration);
    }

    if (ret == -EINVAL) {
        status = Status::ILLEGAL_ARGUMENT;
    } else if (ret != OK) {
        status = Status::INTERNAL_ERROR;
    } else {
        V3_4::implementation::convertToHidl(stream_list, &outStreams);
        mFirstRequest = true;
    }

    _hidl_cb(status, outStreams);
    return Void();
}

bool CameraDeviceSession::preProcessConfigurationLocked_3_4(
        const StreamConfiguration& requestedConfiguration,
        camera3_stream_configuration_t *stream_list /*out*/,
        hidl_vec<camera3_stream_t*> *streams /*out*/) {

    if ((stream_list == nullptr) || (streams == nullptr)) {
        return false;
    }

    stream_list->operation_mode = (uint32_t) requestedConfiguration.operationMode;
    stream_list->num_streams = requestedConfiguration.streams.size();
    streams->resize(stream_list->num_streams);
    stream_list->streams = streams->data();

    for (uint32_t i = 0; i < stream_list->num_streams; i++) {
        int id = requestedConfiguration.streams[i].v3_2.id;

        if (mStreamMap.count(id) == 0) {
            Camera3Stream stream;
            convertFromHidl(requestedConfiguration.streams[i], &stream);
            mStreamMap[id] = stream;
            mPhysicalCameraIdMap[id] = requestedConfiguration.streams[i].physicalCameraId;
            mStreamMap[id].data_space = mapToLegacyDataspace(
                    mStreamMap[id].data_space);
            mStreamMap[id].physical_camera_id = mPhysicalCameraIdMap[id].c_str();
            mCirculatingBuffers.emplace(stream.mId, CirculatingBuffers{});
        } else {
            // width/height/format must not change, but usage/rotation might need to change
            if (mStreamMap[id].stream_type !=
                    (int) requestedConfiguration.streams[i].v3_2.streamType ||
                    mStreamMap[id].width != requestedConfiguration.streams[i].v3_2.width ||
                    mStreamMap[id].height != requestedConfiguration.streams[i].v3_2.height ||
                    mStreamMap[id].format != (int) requestedConfiguration.streams[i].v3_2.format ||
                    mStreamMap[id].data_space !=
                            mapToLegacyDataspace( static_cast<android_dataspace_t> (
                                    requestedConfiguration.streams[i].v3_2.dataSpace)) ||
                    mPhysicalCameraIdMap[id] != requestedConfiguration.streams[i].physicalCameraId) {
                ALOGE("%s: stream %d configuration changed!", __FUNCTION__, id);
                return false;
            }
            mStreamMap[id].rotation = (int) requestedConfiguration.streams[i].v3_2.rotation;
            mStreamMap[id].usage = (uint32_t) requestedConfiguration.streams[i].v3_2.usage;
        }
        (*streams)[i] = &mStreamMap[id];
    }

    return true;
}

void CameraDeviceSession::postProcessConfigurationLocked_3_4(
        const StreamConfiguration& requestedConfiguration) {
    // delete unused streams, note we do this after adding new streams to ensure new stream
    // will not have the same address as deleted stream, and HAL has a chance to reference
    // the to be deleted stream in configure_streams call
    for(auto it = mStreamMap.begin(); it != mStreamMap.end();) {
        int id = it->first;
        bool found = false;
        for (const auto& stream : requestedConfiguration.streams) {
            if (id == stream.v3_2.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Unmap all buffers of deleted stream
            // in case the configuration call succeeds and HAL
            // is able to release the corresponding resources too.
            cleanupBuffersLocked(id);
            it = mStreamMap.erase(it);
        } else {
            ++it;
        }
    }

    // Track video streams
    mVideoStreamIds.clear();
    for (const auto& stream : requestedConfiguration.streams) {
        if (stream.v3_2.streamType == StreamType::OUTPUT &&
            stream.v3_2.usage &
                graphics::common::V1_0::BufferUsage::VIDEO_ENCODER) {
            mVideoStreamIds.push_back(stream.v3_2.id);
        }
    }
    mResultBatcher_3_4.setBatchedStreams(mVideoStreamIds);
}

Return<void> CameraDeviceSession::processCaptureRequest_3_4(
        const hidl_vec<V3_4::CaptureRequest>& requests,
        const hidl_vec<V3_2::BufferCache>& cachesToRemove,
        ICameraDeviceSession::processCaptureRequest_3_4_cb _hidl_cb)  {
    updateBufferCaches(cachesToRemove);

    uint32_t numRequestProcessed = 0;
    Status s = Status::OK;
    for (size_t i = 0; i < requests.size(); i++, numRequestProcessed++) {
        s = processOneCaptureRequest_3_4(requests[i]);
        if (s != Status::OK) {
            break;
        }
    }

    if (s == Status::OK && requests.size() > 1) {
        mResultBatcher_3_4.registerBatch(requests[0].v3_2.frameNumber, requests.size());
    }

    _hidl_cb(s, numRequestProcessed);
    return Void();
}

Status CameraDeviceSession::processOneCaptureRequest_3_4(const V3_4::CaptureRequest& request)  {
    Status status = initStatus();
    if (status != Status::OK) {
        ALOGE("%s: camera init failed or disconnected", __FUNCTION__);
        return status;
    }
    // If callback is 3.2, make sure there are no physical settings.
    if (!mHasCallback_3_4) {
        if (request.physicalCameraSettings.size() > 0) {
            ALOGE("%s: trying to call processCaptureRequest_3_4 with physical camera id "
                    "and V3.2 callback", __FUNCTION__);
            return Status::INTERNAL_ERROR;
        }
    }

    camera3_capture_request_t halRequest;
    halRequest.frame_number = request.v3_2.frameNumber;

    bool converted = true;
    V3_2::CameraMetadata settingsFmq;  // settings from FMQ
    if (request.v3_2.fmqSettingsSize > 0) {
        // non-blocking read; client must write metadata before calling
        // processOneCaptureRequest
        settingsFmq.resize(request.v3_2.fmqSettingsSize);
        bool read = mRequestMetadataQueue->read(settingsFmq.data(), request.v3_2.fmqSettingsSize);
        if (read) {
            converted = V3_2::implementation::convertFromHidl(settingsFmq, &halRequest.settings);
        } else {
            ALOGE("%s: capture request settings metadata couldn't be read from fmq!", __FUNCTION__);
            converted = false;
        }
    } else {
        converted = V3_2::implementation::convertFromHidl(request.v3_2.settings,
                &halRequest.settings);
    }

    if (!converted) {
        ALOGE("%s: capture request settings metadata is corrupt!", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    if (mFirstRequest && halRequest.settings == nullptr) {
        ALOGE("%s: capture request settings must not be null for first request!",
                __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    hidl_vec<buffer_handle_t*> allBufPtrs;
    hidl_vec<int> allFences;
    bool hasInputBuf = (request.v3_2.inputBuffer.streamId != -1 &&
            request.v3_2.inputBuffer.bufferId != 0);
    size_t numOutputBufs = request.v3_2.outputBuffers.size();
    size_t numBufs = numOutputBufs + (hasInputBuf ? 1 : 0);

    if (numOutputBufs == 0) {
        ALOGE("%s: capture request must have at least one output buffer!", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    status = importRequest(request.v3_2, allBufPtrs, allFences);
    if (status != Status::OK) {
        return status;
    }

    hidl_vec<camera3_stream_buffer_t> outHalBufs;
    outHalBufs.resize(numOutputBufs);
    bool aeCancelTriggerNeeded = false;
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata settingsOverride;
    {
        Mutex::Autolock _l(mInflightLock);
        if (hasInputBuf) {
            auto streamId = request.v3_2.inputBuffer.streamId;
            auto key = std::make_pair(request.v3_2.inputBuffer.streamId, request.v3_2.frameNumber);
            auto& bufCache = mInflightBuffers[key] = camera3_stream_buffer_t{};
            convertFromHidl(
                    allBufPtrs[numOutputBufs], request.v3_2.inputBuffer.status,
                    &mStreamMap[request.v3_2.inputBuffer.streamId], allFences[numOutputBufs],
                    &bufCache);
            bufCache.stream->physical_camera_id = mPhysicalCameraIdMap[streamId].c_str();
            halRequest.input_buffer = &bufCache;
        } else {
            halRequest.input_buffer = nullptr;
        }

        halRequest.num_output_buffers = numOutputBufs;
        for (size_t i = 0; i < numOutputBufs; i++) {
            auto streamId = request.v3_2.outputBuffers[i].streamId;
            auto key = std::make_pair(streamId, request.v3_2.frameNumber);
            auto& bufCache = mInflightBuffers[key] = camera3_stream_buffer_t{};
            convertFromHidl(
                    allBufPtrs[i], request.v3_2.outputBuffers[i].status,
                    &mStreamMap[streamId], allFences[i],
                    &bufCache);
            bufCache.stream->physical_camera_id = mPhysicalCameraIdMap[streamId].c_str();
            outHalBufs[i] = bufCache;
        }
        halRequest.output_buffers = outHalBufs.data();

        AETriggerCancelOverride triggerOverride;
        aeCancelTriggerNeeded = handleAePrecaptureCancelRequestLocked(
                halRequest, &settingsOverride /*out*/, &triggerOverride/*out*/);
        if (aeCancelTriggerNeeded) {
            mInflightAETriggerOverrides[halRequest.frame_number] =
                    triggerOverride;
            halRequest.settings = settingsOverride.getAndLock();
        }
    }

    std::vector<const char *> physicalCameraIds;
    std::vector<const camera_metadata_t *> physicalCameraSettings;
    std::vector<V3_2::CameraMetadata> physicalFmq;
    size_t settingsCount = request.physicalCameraSettings.size();
    if (settingsCount > 0) {
        physicalCameraIds.reserve(settingsCount);
        physicalCameraSettings.reserve(settingsCount);
        physicalFmq.reserve(settingsCount);

        for (size_t i = 0; i < settingsCount; i++) {
            uint64_t settingsSize = request.physicalCameraSettings[i].fmqSettingsSize;
            const camera_metadata_t *settings = nullptr;
            if (settingsSize > 0) {
                physicalFmq.push_back(V3_2::CameraMetadata(settingsSize));
                bool read = mRequestMetadataQueue->read(physicalFmq[i].data(), settingsSize);
                if (read) {
                    converted = V3_2::implementation::convertFromHidl(physicalFmq[i], &settings);
                    physicalCameraSettings.push_back(settings);
                } else {
                    ALOGE("%s: physical camera settings metadata couldn't be read from fmq!",
                            __FUNCTION__);
                    converted = false;
                }
            } else {
                converted = V3_2::implementation::convertFromHidl(
                        request.physicalCameraSettings[i].settings, &settings);
                physicalCameraSettings.push_back(settings);
            }

            if (!converted) {
                ALOGE("%s: physical camera settings metadata is corrupt!", __FUNCTION__);
                return Status::ILLEGAL_ARGUMENT;
            }

            if (mFirstRequest && settings == nullptr) {
                ALOGE("%s: Individual request settings must not be null for first request!",
                        __FUNCTION__);
                return Status::ILLEGAL_ARGUMENT;
            }

            physicalCameraIds.push_back(request.physicalCameraSettings[i].physicalCameraId.c_str());
        }
    }
    halRequest.num_physcam_settings = settingsCount;
    halRequest.physcam_id = physicalCameraIds.data();
    halRequest.physcam_settings = physicalCameraSettings.data();

    ATRACE_ASYNC_BEGIN("frame capture", request.v3_2.frameNumber);
    ATRACE_BEGIN("camera3->process_capture_request");
    status_t ret = mDevice->ops->process_capture_request(mDevice, &halRequest);
    ATRACE_END();
    if (aeCancelTriggerNeeded) {
        settingsOverride.unlock(halRequest.settings);
    }
    if (ret != OK) {
        Mutex::Autolock _l(mInflightLock);
        ALOGE("%s: HAL process_capture_request call failed!", __FUNCTION__);

        cleanupInflightFences(allFences, numBufs);
        if (hasInputBuf) {
            auto key = std::make_pair(request.v3_2.inputBuffer.streamId, request.v3_2.frameNumber);
            mInflightBuffers.erase(key);
        }
        for (size_t i = 0; i < numOutputBufs; i++) {
            auto key = std::make_pair(request.v3_2.outputBuffers[i].streamId,
                    request.v3_2.frameNumber);
            mInflightBuffers.erase(key);
        }
        if (aeCancelTriggerNeeded) {
            mInflightAETriggerOverrides.erase(request.v3_2.frameNumber);
        }

        if (ret == BAD_VALUE) {
            return Status::ILLEGAL_ARGUMENT;
        } else {
            return Status::INTERNAL_ERROR;
        }
    }

    mFirstRequest = false;
    return Status::OK;
}

/**
 * Static callback forwarding methods from HAL to instance
 */
void CameraDeviceSession::sProcessCaptureResult_3_4(
        const camera3_callback_ops *cb,
        const camera3_capture_result *hal_result) {
    CameraDeviceSession *d =
            const_cast<CameraDeviceSession*>(static_cast<const CameraDeviceSession*>(cb));

    CaptureResult result = {};
    camera3_capture_result shadowResult;
    bool handlePhysCam = (d->mDeviceVersion >= CAMERA_DEVICE_API_VERSION_3_5);
    std::vector<::android::hardware::camera::common::V1_0::helper::CameraMetadata> compactMds;
    std::vector<const camera_metadata_t*> physCamMdArray;
    sShrinkCaptureResult(&shadowResult, hal_result, &compactMds, &physCamMdArray, handlePhysCam);

    status_t ret = d->constructCaptureResult(result.v3_2, &shadowResult);
    if (ret != OK) {
        return;
    }

    if (handlePhysCam) {
        if (shadowResult.num_physcam_metadata > d->mPhysicalCameraIds.size()) {
            ALOGE("%s: Fatal: Invalid num_physcam_metadata %u", __FUNCTION__,
                    shadowResult.num_physcam_metadata);
            return;
        }
        result.physicalCameraMetadata.resize(shadowResult.num_physcam_metadata);
        for (uint32_t i = 0; i < shadowResult.num_physcam_metadata; i++) {
            std::string physicalId = shadowResult.physcam_ids[i];
            if (d->mPhysicalCameraIds.find(physicalId) == d->mPhysicalCameraIds.end()) {
                ALOGE("%s: Fatal: Invalid physcam_ids[%u]: %s", __FUNCTION__,
                      i, shadowResult.physcam_ids[i]);
                return;
            }
            V3_2::CameraMetadata physicalMetadata;
            V3_2::implementation::convertToHidl(
                    shadowResult.physcam_metadata[i], &physicalMetadata);
            PhysicalCameraMetadata physicalCameraMetadata = {
                    .fmqMetadataSize = 0,
                    .physicalCameraId = physicalId,
                    .metadata = physicalMetadata };
            result.physicalCameraMetadata[i] = physicalCameraMetadata;
        }
    }
    d->mResultBatcher_3_4.processCaptureResult_3_4(result);
}

void CameraDeviceSession::sNotify_3_4(
        const camera3_callback_ops *cb,
        const camera3_notify_msg *msg) {
    CameraDeviceSession *d =
            const_cast<CameraDeviceSession*>(static_cast<const CameraDeviceSession*>(cb));
    V3_2::NotifyMsg hidlMsg;
    V3_2::implementation::convertToHidl(msg, &hidlMsg);

    if (hidlMsg.type == (V3_2::MsgType) CAMERA3_MSG_ERROR &&
            hidlMsg.msg.error.errorStreamId != -1) {
        if (d->mStreamMap.count(hidlMsg.msg.error.errorStreamId) != 1) {
            ALOGE("%s: unknown stream ID %d reports an error!",
                    __FUNCTION__, hidlMsg.msg.error.errorStreamId);
            return;
        }
    }

    if (static_cast<camera3_msg_type_t>(hidlMsg.type) == CAMERA3_MSG_ERROR) {
        switch (hidlMsg.msg.error.errorCode) {
            case V3_2::ErrorCode::ERROR_DEVICE:
            case V3_2::ErrorCode::ERROR_REQUEST:
            case V3_2::ErrorCode::ERROR_RESULT: {
                Mutex::Autolock _l(d->mInflightLock);
                auto entry = d->mInflightAETriggerOverrides.find(
                        hidlMsg.msg.error.frameNumber);
                if (d->mInflightAETriggerOverrides.end() != entry) {
                    d->mInflightAETriggerOverrides.erase(
                            hidlMsg.msg.error.frameNumber);
                }

                auto boostEntry = d->mInflightRawBoostPresent.find(
                        hidlMsg.msg.error.frameNumber);
                if (d->mInflightRawBoostPresent.end() != boostEntry) {
                    d->mInflightRawBoostPresent.erase(
                            hidlMsg.msg.error.frameNumber);
                }

            }
                break;
            case V3_2::ErrorCode::ERROR_BUFFER:
            default:
                break;
        }

    }

    d->mResultBatcher_3_4.notify(hidlMsg);
}

CameraDeviceSession::ResultBatcher_3_4::ResultBatcher_3_4(
        const sp<V3_2::ICameraDeviceCallback>& callback) :
        V3_3::implementation::CameraDeviceSession::ResultBatcher(callback) {
    auto castResult = ICameraDeviceCallback::castFrom(callback);
    if (castResult.isOk()) {
        mCallback_3_4 = castResult;
    }
}

void CameraDeviceSession::ResultBatcher_3_4::processCaptureResult_3_4(CaptureResult& result) {
    auto pair = getBatch(result.v3_2.frameNumber);
    int batchIdx = pair.first;
    if (batchIdx == NOT_BATCHED) {
        processOneCaptureResult_3_4(result);
        return;
    }
    std::shared_ptr<InflightBatch> batch = pair.second;
    {
        Mutex::Autolock _l(batch->mLock);
        // Check if the batch is removed (mostly by notify error) before lock was acquired
        if (batch->mRemoved) {
            // Fall back to non-batch path
            processOneCaptureResult_3_4(result);
            return;
        }

        // queue metadata
        if (result.v3_2.result.size() != 0) {
            // Save a copy of metadata
            batch->mResultMds[result.v3_2.partialResult].mMds.push_back(
                    std::make_pair(result.v3_2.frameNumber, result.v3_2.result));
        }

        // queue buffer
        std::vector<int> filledStreams;
        std::vector<V3_2::StreamBuffer> nonBatchedBuffers;
        for (auto& buffer : result.v3_2.outputBuffers) {
            auto it = batch->mBatchBufs.find(buffer.streamId);
            if (it != batch->mBatchBufs.end()) {
                InflightBatch::BufferBatch& bb = it->second;
                pushStreamBuffer(std::move(buffer), bb.mBuffers);
                filledStreams.push_back(buffer.streamId);
            } else {
                pushStreamBuffer(std::move(buffer), nonBatchedBuffers);
            }
        }

        // send non-batched buffers up
        if (nonBatchedBuffers.size() > 0 || result.v3_2.inputBuffer.streamId != -1) {
            CaptureResult nonBatchedResult;
            nonBatchedResult.v3_2.frameNumber = result.v3_2.frameNumber;
            nonBatchedResult.v3_2.fmqResultSize = 0;
            nonBatchedResult.v3_2.outputBuffers.resize(nonBatchedBuffers.size());
            for (size_t i = 0; i < nonBatchedBuffers.size(); i++) {
                moveStreamBuffer(
                        std::move(nonBatchedBuffers[i]), nonBatchedResult.v3_2.outputBuffers[i]);
            }
            moveStreamBuffer(std::move(result.v3_2.inputBuffer), nonBatchedResult.v3_2.inputBuffer);
            nonBatchedResult.v3_2.partialResult = 0; // 0 for buffer only results
            processOneCaptureResult_3_4(nonBatchedResult);
        }

        if (result.v3_2.frameNumber == batch->mLastFrame) {
            // Send data up
            if (result.v3_2.partialResult > 0) {
                sendBatchMetadataLocked(batch, result.v3_2.partialResult);
            }
            // send buffer up
            if (filledStreams.size() > 0) {
                sendBatchBuffersLocked(batch, filledStreams);
            }
        }
    } // end of batch lock scope

    // see if the batch is complete
    if (result.v3_2.frameNumber == batch->mLastFrame) {
        checkAndRemoveFirstBatch();
    }
}

void CameraDeviceSession::ResultBatcher_3_4::processOneCaptureResult_3_4(CaptureResult& result) {
    hidl_vec<CaptureResult> results;
    results.resize(1);
    results[0] = std::move(result);
    invokeProcessCaptureResultCallback_3_4(results, /* tryWriteFmq */true);
    freeReleaseFences_3_4(results);
    return;
}

void CameraDeviceSession::ResultBatcher_3_4::invokeProcessCaptureResultCallback_3_4(
        hidl_vec<CaptureResult> &results, bool tryWriteFmq) {
    if (mProcessCaptureResultLock.tryLock() != OK) {
        ALOGV("%s: previous call is not finished! waiting 1s...", __FUNCTION__);
        if (mProcessCaptureResultLock.timedLock(1000000000 /* 1s */) != OK) {
            ALOGE("%s: cannot acquire lock in 1s, cannot proceed",
                    __FUNCTION__);
            return;
        }
    }
    if (tryWriteFmq && mResultMetadataQueue->availableToWrite() > 0) {
        for (CaptureResult &result : results) {
            if (result.v3_2.result.size() > 0) {
                if (mResultMetadataQueue->write(result.v3_2.result.data(),
                        result.v3_2.result.size())) {
                    result.v3_2.fmqResultSize = result.v3_2.result.size();
                    result.v3_2.result.resize(0);
                } else {
                    ALOGW("%s: couldn't utilize fmq, fall back to hwbinder", __FUNCTION__);
                    result.v3_2.fmqResultSize = 0;
                }
            }

            for (auto& onePhysMetadata : result.physicalCameraMetadata) {
                if (mResultMetadataQueue->write(onePhysMetadata.metadata.data(),
                        onePhysMetadata.metadata.size())) {
                    onePhysMetadata.fmqMetadataSize = onePhysMetadata.metadata.size();
                    onePhysMetadata.metadata.resize(0);
                } else {
                    ALOGW("%s: couldn't utilize fmq, fall back to hwbinder", __FUNCTION__);
                    onePhysMetadata.fmqMetadataSize = 0;
                }
            }
        }
    }
    mCallback_3_4->processCaptureResult_3_4(results);
    mProcessCaptureResultLock.unlock();
}

void CameraDeviceSession::ResultBatcher_3_4::freeReleaseFences_3_4(hidl_vec<CaptureResult>& results) {
    for (auto& result : results) {
        if (result.v3_2.inputBuffer.releaseFence.getNativeHandle() != nullptr) {
            native_handle_t* handle = const_cast<native_handle_t*>(
                    result.v3_2.inputBuffer.releaseFence.getNativeHandle());
            native_handle_close(handle);
            native_handle_delete(handle);
        }
        for (auto& buf : result.v3_2.outputBuffers) {
            if (buf.releaseFence.getNativeHandle() != nullptr) {
                native_handle_t* handle = const_cast<native_handle_t*>(
                        buf.releaseFence.getNativeHandle());
                native_handle_close(handle);
                native_handle_delete(handle);
            }
        }
    }
    return;
}

} // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
