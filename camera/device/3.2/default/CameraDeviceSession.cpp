/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "CamDevSession@3.2-impl"
#include <android/log.h>

#include <utils/Trace.h>
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include "CameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_2 {
namespace implementation {

namespace {

// Copy pasted from Hwc.cpp. Use this until gralloc mapper HAL is working
class HandleImporter {
public:
    HandleImporter() : mInitialized(false) {}

    bool initialize()
    {
        // allow only one client
        if (mInitialized) {
            return false;
        }

        if (!openGralloc()) {
            return false;
        }

        mInitialized = true;
        return true;
    }

    void cleanup()
    {
        if (!mInitialized) {
            return;
        }

        closeGralloc();
        mInitialized = false;
    }

    // In IComposer, any buffer_handle_t is owned by the caller and we need to
    // make a clone for hwcomposer2.  We also need to translate empty handle
    // to nullptr.  This function does that, in-place.
    bool importBuffer(buffer_handle_t& handle)
    {
        if (!handle->numFds && !handle->numInts) {
            handle = nullptr;
            return true;
        }

        buffer_handle_t clone = cloneBuffer(handle);
        if (!clone) {
            return false;
        }

        handle = clone;
        return true;
    }

    void freeBuffer(buffer_handle_t handle)
    {
        if (!handle) {
            return;
        }

        releaseBuffer(handle);
    }

    bool importFence(const native_handle_t* handle, int& fd)
    {
        if (handle == nullptr || handle->numFds == 0) {
            fd = -1;
        } else if (handle->numFds == 1) {
            fd = dup(handle->data[0]);
            if (fd < 0) {
                ALOGE("failed to dup fence fd %d", handle->data[0]);
                return false;
            }
        } else {
            ALOGE("invalid fence handle with %d file descriptors",
                    handle->numFds);
            return false;
        }

        return true;
    }

    void closeFence(int fd)
    {
        if (fd >= 0) {
            close(fd);
        }
    }

private:
    bool mInitialized;

    bool openGralloc()
    {
        const hw_module_t* module;
        int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        if (err) {
            ALOGE("failed to get gralloc module");
            return false;
        }

        uint8_t major = (module->module_api_version >> 8) & 0xff;
        if (major > 1) {
            ALOGE("unknown gralloc module major version %d", major);
            return false;
        }

        if (major == 1) {
            err = gralloc1_open(module, &mDevice);
            if (err) {
                ALOGE("failed to open gralloc1 device");
                return false;
            }

            mRetain = reinterpret_cast<GRALLOC1_PFN_RETAIN>(
                    mDevice->getFunction(mDevice, GRALLOC1_FUNCTION_RETAIN));
            mRelease = reinterpret_cast<GRALLOC1_PFN_RELEASE>(
                    mDevice->getFunction(mDevice, GRALLOC1_FUNCTION_RELEASE));
            if (!mRetain || !mRelease) {
                ALOGE("invalid gralloc1 device");
                gralloc1_close(mDevice);
                return false;
            }
        } else {
            mModule = reinterpret_cast<const gralloc_module_t*>(module);
        }

        return true;
    }

    void closeGralloc()
    {
        if (mDevice) {
            gralloc1_close(mDevice);
        }
    }

    buffer_handle_t cloneBuffer(buffer_handle_t handle)
    {
        native_handle_t* clone = native_handle_clone(handle);
        if (!clone) {
            ALOGE("failed to clone buffer %p", handle);
            return nullptr;
        }

        bool err;
        if (mDevice) {
            err = (mRetain(mDevice, clone) != GRALLOC1_ERROR_NONE);
        } else {
            err = (mModule->registerBuffer(mModule, clone) != 0);
        }

        if (err) {
            ALOGE("failed to retain/register buffer %p", clone);
            native_handle_close(clone);
            native_handle_delete(clone);
            return nullptr;
        }

        return clone;
    }

    void releaseBuffer(buffer_handle_t handle)
    {
        if (mDevice) {
            mRelease(mDevice, handle);
        } else {
            mModule->unregisterBuffer(mModule, handle);
            native_handle_close(handle);
            native_handle_delete(const_cast<native_handle_t*>(handle));
        }
    }

    // gralloc1
    gralloc1_device_t* mDevice;
    GRALLOC1_PFN_RETAIN mRetain;
    GRALLOC1_PFN_RELEASE mRelease;

    // gralloc0
    const gralloc_module_t* mModule;
};

HandleImporter sHandleImporter;

} // Anonymous namespace

CameraDeviceSession::CameraDeviceSession(
    camera3_device_t* device, const sp<ICameraDeviceCallback>& callback) :
        camera3_callback_ops({&sProcessCaptureResult, &sNotify}),
        mDevice(device),
        mCallback(callback) {
    // For now, we init sHandleImporter but do not cleanup (keep it alive until
    // HAL process ends)
    sHandleImporter.initialize();

    mInitFail = initialize();
}

bool CameraDeviceSession::initialize() {
    /** Initialize device with callback functions */
    ATRACE_BEGIN("camera3->initialize");
    status_t res = mDevice->ops->initialize(mDevice, this);
    ATRACE_END();

    if (res != OK) {
        ALOGE("%s: Unable to initialize HAL device: %s (%d)",
                __FUNCTION__, strerror(-res), res);
        mDevice->common.close(&mDevice->common);
        mClosed = true;
        return true;
    }
    return false;
}

CameraDeviceSession::~CameraDeviceSession() {
    if (!isClosed()) {
        ALOGE("CameraDeviceSession deleted before close!");
        close();
    }
}

bool CameraDeviceSession::isClosed() {
    Mutex::Autolock _l(mStateLock);
    return mClosed;
}

Status CameraDeviceSession::initStatus() const {
    Mutex::Autolock _l(mStateLock);
    Status status = Status::OK;
    if (mInitFail) {
        status = Status::INTERNAL_ERROR;
    } else if (mDisconnected) {
        status = Status::CAMERA_DISCONNECTED;
    } else if (mClosed) {
        status = Status::INTERNAL_ERROR;
    }
    return status;
}

void CameraDeviceSession::disconnect() {
    Mutex::Autolock _l(mStateLock);
    mDisconnected = true;
    ALOGW("%s: Camera device is disconnected. Closing.", __FUNCTION__);
    if (!mClosed) {
        mDevice->common.close(&mDevice->common);
        mClosed = true;
    }
}

void CameraDeviceSession::dumpState(const native_handle_t* fd) {
    if (!isClosed()) {
        mDevice->ops->dump(mDevice, fd->data[0]);
    }
}

Status CameraDeviceSession::importRequest(
        const CaptureRequest& request,
        hidl_vec<buffer_handle_t*>& allBufPtrs,
        hidl_vec<int>& allFences) {
    bool hasInputBuf = (request.inputBuffer.streamId != -1 &&
            request.inputBuffer.bufferId != 0);
    size_t numOutputBufs = request.outputBuffers.size();
    size_t numBufs = numOutputBufs + (hasInputBuf ? 1 : 0);
    // Validate all I/O buffers
    hidl_vec<buffer_handle_t> allBufs;
    hidl_vec<uint64_t> allBufIds;
    allBufs.resize(numBufs);
    allBufIds.resize(numBufs);
    allBufPtrs.resize(numBufs);
    allFences.resize(numBufs);
    std::vector<int32_t> streamIds(numBufs);

    for (size_t i = 0; i < numOutputBufs; i++) {
        allBufs[i] = request.outputBuffers[i].buffer.getNativeHandle();
        allBufIds[i] = request.outputBuffers[i].bufferId;
        allBufPtrs[i] = &allBufs[i];
        streamIds[i] = request.outputBuffers[i].streamId;
    }
    if (hasInputBuf) {
        allBufs[numOutputBufs] = request.inputBuffer.buffer.getNativeHandle();
        allBufIds[numOutputBufs] = request.inputBuffer.bufferId;
        allBufPtrs[numOutputBufs] = &allBufs[numOutputBufs];
        streamIds[numOutputBufs] = request.inputBuffer.streamId;
    }

    for (size_t i = 0; i < numBufs; i++) {
        buffer_handle_t buf = allBufs[i];
        uint64_t bufId = allBufIds[i];
        CirculatingBuffers& cbs = mCirculatingBuffers[streamIds[i]];
        if (cbs.count(bufId) == 0) {
            if (buf == nullptr) {
                ALOGE("%s: bufferId %" PRIu64 " has null buffer handle!", __FUNCTION__, bufId);
                return Status::ILLEGAL_ARGUMENT;
            }
            // Register a newly seen buffer
            buffer_handle_t importedBuf = buf;
            sHandleImporter.importBuffer(importedBuf);
            if (importedBuf == nullptr) {
                ALOGE("%s: output buffer %zu is invalid!", __FUNCTION__, i);
                return Status::INTERNAL_ERROR;
            } else {
                cbs[bufId] = importedBuf;
            }
        }
        allBufPtrs[i] = &cbs[bufId];
    }

    // All buffers are imported. Now validate output buffer acquire fences
    for (size_t i = 0; i < numOutputBufs; i++) {
        if (!sHandleImporter.importFence(
                request.outputBuffers[i].acquireFence, allFences[i])) {
            ALOGE("%s: output buffer %zu acquire fence is invalid", __FUNCTION__, i);
            cleanupInflightFences(allFences, i);
            return Status::INTERNAL_ERROR;
        }
    }

    // Validate input buffer acquire fences
    if (hasInputBuf) {
        if (!sHandleImporter.importFence(
                request.inputBuffer.acquireFence, allFences[numOutputBufs])) {
            ALOGE("%s: input buffer acquire fence is invalid", __FUNCTION__);
            cleanupInflightFences(allFences, numOutputBufs);
            return Status::INTERNAL_ERROR;
        }
    }
    return Status::OK;
}

void CameraDeviceSession::cleanupInflightFences(
        hidl_vec<int>& allFences, size_t numFences) {
    for (size_t j = 0; j < numFences; j++) {
        sHandleImporter.closeFence(allFences[j]);
    }
}

// Methods from ::android::hardware::camera::device::V3_2::ICameraDeviceSession follow.
Return<void> CameraDeviceSession::constructDefaultRequestSettings(
        RequestTemplate type, constructDefaultRequestSettings_cb _hidl_cb)  {
    Status status = initStatus();
    CameraMetadata outMetadata;
    const camera_metadata_t *rawRequest;
    if (status == Status::OK) {
        ATRACE_BEGIN("camera3->construct_default_request_settings");
        rawRequest = mDevice->ops->construct_default_request_settings(mDevice, (int) type);
        ATRACE_END();
        if (rawRequest == nullptr) {
            ALOGI("%s: template %d is not supported on this camera device",
                  __FUNCTION__, type);
            status = Status::ILLEGAL_ARGUMENT;
        } else {
            convertToHidl(rawRequest, &outMetadata);
        }
    }
    _hidl_cb(status, outMetadata);
    return Void();
}

Return<void> CameraDeviceSession::configureStreams(
        const StreamConfiguration& requestedConfiguration, configureStreams_cb _hidl_cb)  {
    Status status = initStatus();
    HalStreamConfiguration outStreams;

    // hold the inflight lock for entire configureStreams scope since there must not be any
    // inflight request/results during stream configuration.
    Mutex::Autolock _l(mInflightLock);
    if (!mInflightBuffers.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight buffers!",
                __FUNCTION__, mInflightBuffers.size());
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    if (status != Status::OK) {
        _hidl_cb(status, outStreams);
        return Void();
    }

    camera3_stream_configuration_t stream_list;
    hidl_vec<camera3_stream_t*> streams;

    stream_list.operation_mode = (uint32_t) requestedConfiguration.operationMode;
    stream_list.num_streams = requestedConfiguration.streams.size();
    streams.resize(stream_list.num_streams);
    stream_list.streams = streams.data();

    for (uint32_t i = 0; i < stream_list.num_streams; i++) {
        int id = requestedConfiguration.streams[i].id;

        if (mStreamMap.count(id) == 0) {
            Camera3Stream stream;
            convertFromHidl(requestedConfiguration.streams[i], &stream);
            mStreamMap[id] = stream;
            mCirculatingBuffers.emplace(stream.mId, CirculatingBuffers{});
        } else {
            // width/height/format must not change, but usage/rotation might need to change
            if (mStreamMap[id].stream_type !=
                    (int) requestedConfiguration.streams[i].streamType ||
                    mStreamMap[id].width != requestedConfiguration.streams[i].width ||
                    mStreamMap[id].height != requestedConfiguration.streams[i].height ||
                    mStreamMap[id].format != (int) requestedConfiguration.streams[i].format ||
                    mStreamMap[id].data_space != (android_dataspace_t)
                            requestedConfiguration.streams[i].dataSpace) {
                ALOGE("%s: stream %d configuration changed!", __FUNCTION__, id);
                _hidl_cb(Status::INTERNAL_ERROR, outStreams);
                return Void();
            }
            mStreamMap[id].rotation = (int) requestedConfiguration.streams[i].rotation;
            mStreamMap[id].usage = (uint32_t) requestedConfiguration.streams[i].usage;
        }
        streams[i] = &mStreamMap[id];
    }

    ATRACE_BEGIN("camera3->configure_streams");
    status_t ret = mDevice->ops->configure_streams(mDevice, &stream_list);
    ATRACE_END();

    // In case Hal returns error most likely it was not able to release
    // the corresponding resources of the deleted streams.
    if (ret == OK) {
        // delete unused streams, note we do this after adding new streams to ensure new stream
        // will not have the same address as deleted stream, and HAL has a chance to reference
        // the to be deleted stream in configure_streams call
        for(auto it = mStreamMap.begin(); it != mStreamMap.end();) {
            int id = it->first;
            bool found = false;
            for (const auto& stream : requestedConfiguration.streams) {
                if (id == stream.id) {
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
    }

    if (ret == -EINVAL) {
        status = Status::ILLEGAL_ARGUMENT;
    } else if (ret != OK) {
        status = Status::INTERNAL_ERROR;
    } else {
        convertToHidl(stream_list, &outStreams);
    }

    _hidl_cb(status, outStreams);
    return Void();
}

// Needs to get called after acquiring 'mInflightLock'
void CameraDeviceSession::cleanupBuffersLocked(int id) {
    for (auto& pair : mCirculatingBuffers.at(id)) {
        sHandleImporter.freeBuffer(pair.second);
    }
    mCirculatingBuffers[id].clear();
    mCirculatingBuffers.erase(id);
}

Return<Status> CameraDeviceSession::processCaptureRequest(const CaptureRequest& request)  {
    Status status = initStatus();
    if (status != Status::OK) {
        ALOGE("%s: camera init failed or disconnected", __FUNCTION__);
        return status;
    }

    camera3_capture_request_t halRequest;
    halRequest.frame_number = request.frameNumber;
    bool converted = convertFromHidl(request.settings, &halRequest.settings);
    if (!converted) {
        ALOGE("%s: capture request settings metadata is corrupt!", __FUNCTION__);
        return Status::INTERNAL_ERROR;
    }

    hidl_vec<buffer_handle_t*> allBufPtrs;
    hidl_vec<int> allFences;
    bool hasInputBuf = (request.inputBuffer.streamId != -1 &&
            request.inputBuffer.bufferId != 0);
    size_t numOutputBufs = request.outputBuffers.size();
    size_t numBufs = numOutputBufs + (hasInputBuf ? 1 : 0);
    status = importRequest(request, allBufPtrs, allFences);
    if (status != Status::OK) {
        return status;
    }

    hidl_vec<camera3_stream_buffer_t> outHalBufs;
    outHalBufs.resize(numOutputBufs);
    {
        Mutex::Autolock _l(mInflightLock);
        if (hasInputBuf) {
            auto key = std::make_pair(request.inputBuffer.streamId, request.frameNumber);
            auto& bufCache = mInflightBuffers[key] = camera3_stream_buffer_t{};
            convertFromHidl(
                    allBufPtrs[numOutputBufs], request.inputBuffer.status,
                    &mStreamMap[request.inputBuffer.streamId], allFences[numOutputBufs],
                    &bufCache);
            halRequest.input_buffer = &bufCache;
        } else {
            halRequest.input_buffer = nullptr;
        }

        halRequest.num_output_buffers = numOutputBufs;
        for (size_t i = 0; i < numOutputBufs; i++) {
            auto key = std::make_pair(request.outputBuffers[i].streamId, request.frameNumber);
            auto& bufCache = mInflightBuffers[key] = camera3_stream_buffer_t{};
            convertFromHidl(
                    allBufPtrs[i], request.outputBuffers[i].status,
                    &mStreamMap[request.outputBuffers[i].streamId], allFences[i],
                    &bufCache);
            outHalBufs[i] = bufCache;
        }
        halRequest.output_buffers = outHalBufs.data();
    }

    ATRACE_ASYNC_BEGIN("frame capture", request.frameNumber);
    ATRACE_BEGIN("camera3->process_capture_request");
    status_t ret = mDevice->ops->process_capture_request(mDevice, &halRequest);
    ATRACE_END();
    if (ret != OK) {
        Mutex::Autolock _l(mInflightLock);
        ALOGE("%s: HAL process_capture_request call failed!", __FUNCTION__);

        cleanupInflightFences(allFences, numBufs);
        if (hasInputBuf) {
            auto key = std::make_pair(request.inputBuffer.streamId, request.frameNumber);
            mInflightBuffers.erase(key);
        }
        for (size_t i = 0; i < numOutputBufs; i++) {
            auto key = std::make_pair(request.outputBuffers[i].streamId, request.frameNumber);
            mInflightBuffers.erase(key);
        }
        return Status::INTERNAL_ERROR;
    }

    return Status::OK;
}

Return<Status> CameraDeviceSession::flush()  {
    Status status = initStatus();
    if (status == Status::OK) {
        // Flush is always supported on device 3.1 or later
        status_t ret = mDevice->ops->flush(mDevice);
        if (ret != OK) {
            status = Status::INTERNAL_ERROR;
        }
    }
    return status;
}

Return<void> CameraDeviceSession::close()  {
    Mutex::Autolock _l(mStateLock);
    if (!mClosed) {
        {
            Mutex::Autolock _l(mInflightLock);
            if (!mInflightBuffers.empty()) {
                ALOGE("%s: trying to close while there are still %zu inflight buffers!",
                        __FUNCTION__, mInflightBuffers.size());
            }
        }

        ATRACE_BEGIN("camera3->close");
        mDevice->common.close(&mDevice->common);
        ATRACE_END();

        // free all imported buffers
        for(auto& pair : mCirculatingBuffers) {
            CirculatingBuffers& buffers = pair.second;
            for (auto& p2 : buffers) {
                sHandleImporter.freeBuffer(p2.second);
            }
        }

        mClosed = true;
    }
    return Void();
}

/**
 * Static callback forwarding methods from HAL to instance
 */
void CameraDeviceSession::sProcessCaptureResult(
        const camera3_callback_ops *cb,
        const camera3_capture_result *hal_result) {
    CameraDeviceSession *d =
            const_cast<CameraDeviceSession*>(static_cast<const CameraDeviceSession*>(cb));

    uint32_t frameNumber = hal_result->frame_number;
    bool hasInputBuf = (hal_result->input_buffer != nullptr);
    size_t numOutputBufs = hal_result->num_output_buffers;
    size_t numBufs = numOutputBufs + (hasInputBuf ? 1 : 0);
    Status status = Status::OK;
    {
        Mutex::Autolock _l(d->mInflightLock);
        if (hasInputBuf) {
            int streamId = static_cast<Camera3Stream*>(hal_result->input_buffer->stream)->mId;
            // validate if buffer is inflight
            auto key = std::make_pair(streamId, frameNumber);
            if (d->mInflightBuffers.count(key) != 1) {
                ALOGE("%s: input buffer for stream %d frame %d is not inflight!",
                        __FUNCTION__, streamId, frameNumber);
                return;
            }
        }

        for (size_t i = 0; i < numOutputBufs; i++) {
            int streamId = static_cast<Camera3Stream*>(hal_result->output_buffers[i].stream)->mId;
            // validate if buffer is inflight
            auto key = std::make_pair(streamId, frameNumber);
            if (d->mInflightBuffers.count(key) != 1) {
                ALOGE("%s: output buffer for stream %d frame %d is not inflight!",
                        __FUNCTION__, streamId, frameNumber);
                return;
            }
        }
    }
    // We don't need to validate/import fences here since we will be passing them to camera service
    // within the scope of this function

    CaptureResult result;
    hidl_vec<native_handle_t*> releaseFences;
    releaseFences.resize(numBufs);
    result.frameNumber = frameNumber;
    result.partialResult = hal_result->partial_result;
    convertToHidl(hal_result->result, &result.result);
    if (hasInputBuf) {
        result.inputBuffer.streamId =
                static_cast<Camera3Stream*>(hal_result->input_buffer->stream)->mId;
        result.inputBuffer.buffer = nullptr;
        result.inputBuffer.status = (BufferStatus) hal_result->input_buffer->status;
        // skip acquire fence since it's no use to camera service
        if (hal_result->input_buffer->release_fence != -1) {
            releaseFences[numOutputBufs] = native_handle_create(/*numFds*/1, /*numInts*/0);
            releaseFences[numOutputBufs]->data[0] = hal_result->input_buffer->release_fence;
            result.inputBuffer.releaseFence = releaseFences[numOutputBufs];
        } else {
            releaseFences[numOutputBufs] = nullptr;
        }
    } else {
        result.inputBuffer.streamId = -1;
    }

    result.outputBuffers.resize(numOutputBufs);
    for (size_t i = 0; i < numOutputBufs; i++) {
        result.outputBuffers[i].streamId =
                static_cast<Camera3Stream*>(hal_result->output_buffers[i].stream)->mId;
        result.outputBuffers[i].buffer = nullptr;
        result.outputBuffers[i].status = (BufferStatus) hal_result->output_buffers[i].status;
        // skip acquire fence since it's of no use to camera service
        if (hal_result->output_buffers[i].release_fence != -1) {
            releaseFences[i] = native_handle_create(/*numFds*/1, /*numInts*/0);
            releaseFences[i]->data[0] = hal_result->output_buffers[i].release_fence;
            result.outputBuffers[i].releaseFence = releaseFences[i];
        } else {
            releaseFences[i] = nullptr;
        }
    }

    // Free inflight record/fences.
    // Do this before call back to camera service because camera service might jump to
    // configure_streams right after the processCaptureResult call so we need to finish
    // updating inflight queues first
    {
        Mutex::Autolock _l(d->mInflightLock);
        if (hasInputBuf) {
            int streamId = static_cast<Camera3Stream*>(hal_result->input_buffer->stream)->mId;
            auto key = std::make_pair(streamId, frameNumber);
            // TODO (b/34169301): currently HAL closed the fence
            //sHandleImporter.closeFence(d->mInflightBuffers[key].acquire_fence);
            d->mInflightBuffers.erase(key);
        }

        for (size_t i = 0; i < numOutputBufs; i++) {
            int streamId = static_cast<Camera3Stream*>(hal_result->output_buffers[i].stream)->mId;
            auto key = std::make_pair(streamId, frameNumber);
            // TODO (b/34169301): currently HAL closed the fence
            //sHandleImporter.closeFence(d->mInflightBuffers[key].acquire_fence);
            d->mInflightBuffers.erase(key);
        }

        if (d->mInflightBuffers.empty()) {
            ALOGV("%s: inflight buffer queue is now empty!", __FUNCTION__);
        }
    }

    d->mCallback->processCaptureResult(result);

    for (size_t i = 0; i < releaseFences.size(); i++) {
        // We don't close the FD here as HAL needs to signal it later.
        native_handle_delete(releaseFences[i]);
    }
}

void CameraDeviceSession::sNotify(
        const camera3_callback_ops *cb,
        const camera3_notify_msg *msg) {
    CameraDeviceSession *d =
            const_cast<CameraDeviceSession*>(static_cast<const CameraDeviceSession*>(cb));
    NotifyMsg hidlMsg;
    convertToHidl(msg, &hidlMsg);
    if (hidlMsg.type == (MsgType) CAMERA3_MSG_ERROR &&
            hidlMsg.msg.error.errorStreamId != -1) {
        if (d->mStreamMap.count(hidlMsg.msg.error.errorStreamId) != 1) {
            ALOGE("%s: unknown stream ID %d reports an error!",
                    __FUNCTION__, hidlMsg.msg.error.errorStreamId);
        }
        return;
    }
    d->mCallback->notify(hidlMsg);
}

} // namespace implementation
}  // namespace V3_2
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
