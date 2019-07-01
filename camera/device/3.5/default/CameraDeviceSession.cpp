/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "CamDevSession@3.5-impl"
#define ATRACE_TAG ATRACE_TAG_CAMERA
#include <android/log.h>

#include <vector>
#include <utils/Trace.h>
#include "CameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_5 {
namespace implementation {

CameraDeviceSession::CameraDeviceSession(
    camera3_device_t* device,
    const camera_metadata_t* deviceInfo,
    const sp<V3_2::ICameraDeviceCallback>& callback) :
        V3_4::implementation::CameraDeviceSession(device, deviceInfo, callback) {

    mCallback_3_5 = nullptr;

    auto castResult = ICameraDeviceCallback::castFrom(callback);
    if (castResult.isOk()) {
        sp<ICameraDeviceCallback> callback3_5 = castResult;
        if (callback3_5 != nullptr) {
            mCallback_3_5 = callback3_5;
        }
    }

    if (mCallback_3_5 != nullptr) {
        camera_metadata_entry bufMgrVersion = mDeviceInfo.find(
                ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION);
        if (bufMgrVersion.count > 0) {
            mSupportBufMgr = (bufMgrVersion.data.u8[0] ==
                    ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION_HIDL_DEVICE_3_5);
            if (mSupportBufMgr) {
                request_stream_buffers = sRequestStreamBuffers;
                return_stream_buffers = sReturnStreamBuffers;
            }
        }
    }
}

CameraDeviceSession::~CameraDeviceSession() {
}

Return<void> CameraDeviceSession::configureStreams_3_5(
        const StreamConfiguration& requestedConfiguration,
        ICameraDeviceSession::configureStreams_3_5_cb _hidl_cb)  {
    configureStreams_3_4_Impl(requestedConfiguration.v3_4, _hidl_cb,
            requestedConfiguration.streamConfigCounter, false /*useOverriddenFields*/);
    return Void();
}

Return<void> CameraDeviceSession::signalStreamFlush(
        const hidl_vec<int32_t>& streamIds, uint32_t streamConfigCounter) {
    if (mDevice->ops->signal_stream_flush == nullptr) {
        return Void();
    }

    uint32_t currentCounter = 0;
    {
        Mutex::Autolock _l(mStreamConfigCounterLock);
        currentCounter = mStreamConfigCounter;
    }

    if (streamConfigCounter < currentCounter) {
        ALOGV("%s: streamConfigCounter %d is stale (current %d), skipping signal_stream_flush call",
                __FUNCTION__, streamConfigCounter, mStreamConfigCounter);
        return Void();
    }

    std::vector<camera3_stream_t*> streams(streamIds.size());
    {
        Mutex::Autolock _l(mInflightLock);
        for (size_t i = 0; i < streamIds.size(); i++) {
            int32_t id = streamIds[i];
            if (mStreamMap.count(id) == 0) {
                ALOGE("%s: unknown streamId %d", __FUNCTION__, id);
                return Void();
            }
            streams[i] = &mStreamMap[id];
        }
    }

    mDevice->ops->signal_stream_flush(mDevice, streams.size(), streams.data());
    return Void();
}

Status CameraDeviceSession::importRequest(
        const CaptureRequest& request,
        hidl_vec<buffer_handle_t*>& allBufPtrs,
        hidl_vec<int>& allFences) {
    if (mSupportBufMgr) {
        return importRequestImpl(request, allBufPtrs, allFences, /*allowEmptyBuf*/ true);
    }
    return importRequestImpl(request, allBufPtrs, allFences, /*allowEmptyBuf*/ false);
}

void CameraDeviceSession::pushBufferId(
        const buffer_handle_t& buf, uint64_t bufferId, int streamId) {
    std::lock_guard<std::mutex> lock(mBufferIdMapLock);

    // emplace will return existing entry if there is one.
    auto pair = mBufferIdMaps.emplace(streamId, BufferIdMap{});
    BufferIdMap& bIdMap = pair.first->second;
    bIdMap[buf] = bufferId;
}

uint64_t CameraDeviceSession::popBufferId(
        const buffer_handle_t& buf, int streamId) {
    std::lock_guard<std::mutex> lock(mBufferIdMapLock);

    auto streamIt = mBufferIdMaps.find(streamId);
    if (streamIt == mBufferIdMaps.end()) {
        return BUFFER_ID_NO_BUFFER;
    }
    BufferIdMap& bIdMap = streamIt->second;
    auto it = bIdMap.find(buf);
    if (it == bIdMap.end()) {
        return BUFFER_ID_NO_BUFFER;
    }
    uint64_t bufId = it->second;
    bIdMap.erase(it);
    if (bIdMap.empty()) {
        mBufferIdMaps.erase(streamIt);
    }
    return bufId;
}

uint64_t CameraDeviceSession::getCapResultBufferId(const buffer_handle_t& buf, int streamId) {
    if (mSupportBufMgr) {
        return popBufferId(buf, streamId);
    }
    return BUFFER_ID_NO_BUFFER;
}

Camera3Stream* CameraDeviceSession::getStreamPointer(int32_t streamId) {
    Mutex::Autolock _l(mInflightLock);
    if (mStreamMap.count(streamId) == 0) {
        ALOGE("%s: unknown streamId %d", __FUNCTION__, streamId);
        return nullptr;
    }
    return &mStreamMap[streamId];
}

void CameraDeviceSession::cleanupInflightBufferFences(
        std::vector<int>& fences, std::vector<std::pair<buffer_handle_t, int>>& bufs) {
    hidl_vec<int> hFences = fences;
    cleanupInflightFences(hFences, fences.size());
    for (auto& p : bufs) {
        popBufferId(p.first, p.second);
    }
}

camera3_buffer_request_status_t CameraDeviceSession::requestStreamBuffers(
        uint32_t num_buffer_reqs,
        const camera3_buffer_request_t *buffer_reqs,
        /*out*/uint32_t *num_returned_buf_reqs,
        /*out*/camera3_stream_buffer_ret_t *returned_buf_reqs) {
    ATRACE_CALL();
    *num_returned_buf_reqs = 0;
    hidl_vec<BufferRequest> hBufReqs(num_buffer_reqs);
    for (size_t i = 0; i < num_buffer_reqs; i++) {
        hBufReqs[i].streamId =
                static_cast<Camera3Stream*>(buffer_reqs[i].stream)->mId;
        hBufReqs[i].numBuffersRequested = buffer_reqs[i].num_buffers_requested;
    }

    ATRACE_BEGIN("HIDL requestStreamBuffers");
    BufferRequestStatus status;
    hidl_vec<StreamBufferRet> bufRets;
    auto err = mCallback_3_5->requestStreamBuffers(hBufReqs,
            [&status, &bufRets]
            (BufferRequestStatus s, const hidl_vec<StreamBufferRet>& rets) {
                status = s;
                bufRets = std::move(rets);
            });
    if (!err.isOk()) {
        ALOGE("%s: Transaction error: %s", __FUNCTION__, err.description().c_str());
        return CAMERA3_BUF_REQ_FAILED_UNKNOWN;
    }
    ATRACE_END();

    switch (status) {
        case BufferRequestStatus::FAILED_CONFIGURING:
            return CAMERA3_BUF_REQ_FAILED_CONFIGURING;
        case BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS:
            return CAMERA3_BUF_REQ_FAILED_ILLEGAL_ARGUMENTS;
        default:
            break; // Other status Handled by following code
    }

    if (status != BufferRequestStatus::OK && status != BufferRequestStatus::FAILED_PARTIAL &&
            status != BufferRequestStatus::FAILED_UNKNOWN) {
        ALOGE("%s: unknown buffer request error code %d", __FUNCTION__, status);
        return CAMERA3_BUF_REQ_FAILED_UNKNOWN;
    }

    // Only OK, FAILED_PARTIAL and FAILED_UNKNOWN reaches here
    if (bufRets.size() != num_buffer_reqs) {
        ALOGE("%s: expect %d buffer requests returned, only got %zu",
                __FUNCTION__, num_buffer_reqs, bufRets.size());
        return CAMERA3_BUF_REQ_FAILED_UNKNOWN;
    }

    *num_returned_buf_reqs = num_buffer_reqs;
    for (size_t i = 0; i < num_buffer_reqs; i++) {
        // maybe we can query all streams in one call to avoid frequent locking device here?
        Camera3Stream* stream = getStreamPointer(bufRets[i].streamId);
        if (stream == nullptr) {
            ALOGE("%s: unknown streamId %d", __FUNCTION__, bufRets[i].streamId);
            return CAMERA3_BUF_REQ_FAILED_UNKNOWN;
        }
        returned_buf_reqs[i].stream = stream;
    }

    // Handle failed streams
    for (size_t i = 0; i < num_buffer_reqs; i++) {
        if (bufRets[i].val.getDiscriminator() == StreamBuffersVal::hidl_discriminator::error) {
            returned_buf_reqs[i].num_output_buffers = 0;
            switch (bufRets[i].val.error()) {
                case StreamBufferRequestError::NO_BUFFER_AVAILABLE:
                    returned_buf_reqs[i].status = CAMERA3_PS_BUF_REQ_NO_BUFFER_AVAILABLE;
                    break;
                case StreamBufferRequestError::MAX_BUFFER_EXCEEDED:
                    returned_buf_reqs[i].status = CAMERA3_PS_BUF_REQ_MAX_BUFFER_EXCEEDED;
                    break;
                case StreamBufferRequestError::STREAM_DISCONNECTED:
                    returned_buf_reqs[i].status = CAMERA3_PS_BUF_REQ_STREAM_DISCONNECTED;
                    break;
                case StreamBufferRequestError::UNKNOWN_ERROR:
                    returned_buf_reqs[i].status = CAMERA3_PS_BUF_REQ_UNKNOWN_ERROR;
                    break;
                default:
                    ALOGE("%s: Unknown StreamBufferRequestError %d",
                            __FUNCTION__, bufRets[i].val.error());
                    return CAMERA3_BUF_REQ_FAILED_UNKNOWN;
            }
        }
    }

    if (status == BufferRequestStatus::FAILED_UNKNOWN) {
        return CAMERA3_BUF_REQ_FAILED_UNKNOWN;
    }

    // Only BufferRequestStatus::OK and BufferRequestStatus::FAILED_PARTIAL reaches here
    std::vector<int> importedFences;
    std::vector<std::pair<buffer_handle_t, int>> importedBuffers;
    for (size_t i = 0; i < num_buffer_reqs; i++) {
        if (bufRets[i].val.getDiscriminator() !=
                StreamBuffersVal::hidl_discriminator::buffers) {
            continue;
        }
        int streamId = bufRets[i].streamId;
        const hidl_vec<StreamBuffer>& hBufs = bufRets[i].val.buffers();
        camera3_stream_buffer_t* outBufs = returned_buf_reqs[i].output_buffers;
        returned_buf_reqs[i].num_output_buffers = hBufs.size();
        for (size_t b = 0; b < hBufs.size(); b++) {
            const StreamBuffer& hBuf = hBufs[b];
            camera3_stream_buffer_t& outBuf = outBufs[b];
            // maybe add importBuffers API to avoid frequent locking device?
            Status s = importBuffer(streamId,
                    hBuf.bufferId, hBuf.buffer.getNativeHandle(),
                    /*out*/&(outBuf.buffer),
                    /*allowEmptyBuf*/false);
            // Buffer import should never fail - restart HAL since something is very wrong.
            LOG_ALWAYS_FATAL_IF(s != Status::OK,
                    "%s: import stream %d bufferId %" PRIu64 " failed!",
                    __FUNCTION__, streamId, hBuf.bufferId);

            pushBufferId(*(outBuf.buffer), hBuf.bufferId, streamId);
            importedBuffers.push_back(std::make_pair(*(outBuf.buffer), streamId));

            bool succ = sHandleImporter.importFence(hBuf.acquireFence, outBuf.acquire_fence);
            // Fence import should never fail - restart HAL since something is very wrong.
            LOG_ALWAYS_FATAL_IF(!succ,
                        "%s: stream %d bufferId %" PRIu64 "acquire fence is invalid",
                        __FUNCTION__, streamId, hBuf.bufferId);
            importedFences.push_back(outBuf.acquire_fence);
            outBuf.stream = returned_buf_reqs[i].stream;
            outBuf.status = CAMERA3_BUFFER_STATUS_OK;
            outBuf.release_fence = -1;
        }
        returned_buf_reqs[i].status = CAMERA3_PS_BUF_REQ_OK;
    }

    return (status == BufferRequestStatus::OK) ?
            CAMERA3_BUF_REQ_OK : CAMERA3_BUF_REQ_FAILED_PARTIAL;
}

void CameraDeviceSession::returnStreamBuffers(
        uint32_t num_buffers,
        const camera3_stream_buffer_t* const* buffers) {
    ATRACE_CALL();
    hidl_vec<StreamBuffer> hBufs(num_buffers);

    for (size_t i = 0; i < num_buffers; i++) {
        hBufs[i].streamId =
                static_cast<Camera3Stream*>(buffers[i]->stream)->mId;
        hBufs[i].buffer = nullptr; // use bufferId
        hBufs[i].bufferId = popBufferId(*(buffers[i]->buffer), hBufs[i].streamId);
        if (hBufs[i].bufferId == BUFFER_ID_NO_BUFFER) {
            ALOGE("%s: unknown buffer is returned to stream %d",
                    __FUNCTION__, hBufs[i].streamId);
        }
        // ERROR since the buffer is not for application to consume
        hBufs[i].status = BufferStatus::ERROR;
        // skip acquire fence since it's of no use to camera service
        if (buffers[i]->release_fence != -1) {
            native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
            handle->data[0] = buffers[i]->release_fence;
            hBufs[i].releaseFence.setTo(handle, /*shouldOwn*/true);
        }
    }

    mCallback_3_5->returnStreamBuffers(hBufs);
    return;
}

/**
 * Static callback forwarding methods from HAL to instance
 */
camera3_buffer_request_status_t CameraDeviceSession::sRequestStreamBuffers(
        const struct camera3_callback_ops *cb,
        uint32_t num_buffer_reqs,
        const camera3_buffer_request_t *buffer_reqs,
        /*out*/uint32_t *num_returned_buf_reqs,
        /*out*/camera3_stream_buffer_ret_t *returned_buf_reqs) {
    CameraDeviceSession *d =
            const_cast<CameraDeviceSession*>(static_cast<const CameraDeviceSession*>(cb));

    if (num_buffer_reqs == 0 || buffer_reqs == nullptr || num_returned_buf_reqs == nullptr ||
            returned_buf_reqs == nullptr) {
        ALOGE("%s: bad argument: numBufReq %d, bufReqs %p, numRetBufReq %p, retBufReqs %p",
                __FUNCTION__, num_buffer_reqs, buffer_reqs,
                num_returned_buf_reqs, returned_buf_reqs);
        return CAMERA3_BUF_REQ_FAILED_ILLEGAL_ARGUMENTS;
    }

    return d->requestStreamBuffers(num_buffer_reqs, buffer_reqs,
            num_returned_buf_reqs, returned_buf_reqs);
}

void CameraDeviceSession::sReturnStreamBuffers(
        const struct camera3_callback_ops *cb,
        uint32_t num_buffers,
        const camera3_stream_buffer_t* const* buffers) {
    CameraDeviceSession *d =
            const_cast<CameraDeviceSession*>(static_cast<const CameraDeviceSession*>(cb));

    d->returnStreamBuffers(num_buffers, buffers);
}

Return<void> CameraDeviceSession::isReconfigurationRequired(
        const V3_2::CameraMetadata& oldSessionParams, const V3_2::CameraMetadata& newSessionParams,
        ICameraDeviceSession::isReconfigurationRequired_cb _hidl_cb) {
    if (mDevice->ops->is_reconfiguration_required != nullptr) {
        const camera_metadata_t *oldParams, *newParams;
        V3_2::implementation::convertFromHidl(oldSessionParams, &oldParams);
        V3_2::implementation::convertFromHidl(newSessionParams, &newParams);
        auto ret = mDevice->ops->is_reconfiguration_required(mDevice, oldParams, newParams);
        switch (ret) {
            case 0:
                _hidl_cb(Status::OK, true);
                break;
            case -EINVAL:
                _hidl_cb(Status::OK, false);
                break;
            case -ENOSYS:
                _hidl_cb(Status::METHOD_NOT_SUPPORTED, true);
                break;
            default:
                _hidl_cb(Status::INTERNAL_ERROR, true);
                break;
        };
    } else {
        _hidl_cb(Status::METHOD_NOT_SUPPORTED, true);
    }

    return Void();
}

} // namespace implementation
}  // namespace V3_5
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
