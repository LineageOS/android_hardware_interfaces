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

    // Some existing gralloc drivers do not support retaining more than once,
    // when we are in passthrough mode.
#ifdef BINDERIZED
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
#else
    bool openGralloc() { return true; }
    void closeGralloc() {}
    buffer_handle_t cloneBuffer(buffer_handle_t handle) { return handle; }
    void releaseBuffer(buffer_handle_t) {}
#endif
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
        hidl_vec<buffer_handle_t>& allBufs,
        hidl_vec<int>& allFences) {
    bool hasInputBuf = (request.inputBuffer.streamId != -1 &&
            request.inputBuffer.buffer.getNativeHandle() == nullptr);
    size_t numOutputBufs = request.outputBuffers.size();
    size_t numBufs = numOutputBufs + (hasInputBuf ? 1 : 0);
    // Validate all I/O buffers
    allBufs.resize(numBufs);
    allFences.resize(numBufs);
    for (size_t i = 0; i < numOutputBufs; i++) {
        allBufs[i] = request.outputBuffers[i].buffer.getNativeHandle();
    }
    if (hasInputBuf) {
        allBufs[numOutputBufs] = request.inputBuffer.buffer.getNativeHandle();
    }
    for (size_t i = 0; i < numBufs; i++) {
        buffer_handle_t buf = allBufs[i];
        sHandleImporter.importBuffer(buf);
        if (buf == nullptr) {
            ALOGE("%s: output buffer %zu is invalid!", __FUNCTION__, i);
            cleanupInflightBufferFences(allBufs, i, allFences, 0);
            return Status::INTERNAL_ERROR;
        } else {
            allBufs[i] = buf;
        }
    }

    // All buffers are imported. Now validate output buffer acquire fences
    for (size_t i = 0; i < numOutputBufs; i++) {
        if (!sHandleImporter.importFence(
                request.outputBuffers[i].acquireFence, allFences[i])) {
            ALOGE("%s: output buffer %zu acquire fence is invalid", __FUNCTION__, i);
            cleanupInflightBufferFences(allBufs, numBufs, allFences, i);
            return Status::INTERNAL_ERROR;
        }
    }

    // Validate input buffer acquire fences
    if (hasInputBuf) {
        if (!sHandleImporter.importFence(
                request.inputBuffer.acquireFence, allFences[numOutputBufs])) {
            ALOGE("%s: input buffer acquire fence is invalid", __FUNCTION__);
            cleanupInflightBufferFences(allBufs, numBufs, allFences, numOutputBufs);
            return Status::INTERNAL_ERROR;
        }
    }
    return Status::OK;
}

void CameraDeviceSession::cleanupInflightBufferFences(
        hidl_vec<buffer_handle_t>& allBufs, size_t numBufs,
        hidl_vec<int>& allFences, size_t numFences) {
    for (size_t j = 0; j < numBufs; j++) {
        sHandleImporter.freeBuffer(allBufs[j]);
    }
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
    if (!mInflightBuffers.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight buffers!",
                __FUNCTION__, mInflightBuffers.size());
        status = Status::INTERNAL_ERROR;
    }

    HalStreamConfiguration outStreams;
    if (status == Status::OK) {
        camera3_stream_configuration_t stream_list;
        hidl_vec<camera3_stream_t*> streams;

        stream_list.operation_mode = (uint32_t) requestedConfiguration.operationMode;
        stream_list.num_streams = requestedConfiguration.streams.size();
        streams.resize(stream_list.num_streams);
        stream_list.streams = streams.data();

        mStreamMap.clear();
        for (uint32_t i = 0; i < stream_list.num_streams; i++) {
            Camera3Stream stream;
            convertFromHidl(requestedConfiguration.streams[i], &stream);
            mStreamMap[stream.mId] = stream;
            streams[i] = &mStreamMap[stream.mId];
        }

        ATRACE_BEGIN("camera3->configure_streams");
        status_t ret = mDevice->ops->configure_streams(mDevice, &stream_list);
        ATRACE_END();

        if (ret == -EINVAL) {
            status = Status::ILLEGAL_ARGUMENT;
        } else if (ret != OK) {
            status = Status::INTERNAL_ERROR;
        } else {
            convertToHidl(stream_list, &outStreams);
        }

    }
    _hidl_cb(status, outStreams);
    return Void();
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

    hidl_vec<buffer_handle_t> allBufs;
    hidl_vec<int> allFences;
    bool hasInputBuf = (request.inputBuffer.streamId != -1 &&
            request.inputBuffer.buffer.getNativeHandle() == nullptr);
    size_t numOutputBufs = request.outputBuffers.size();
    size_t numBufs = numOutputBufs + (hasInputBuf ? 1 : 0);
    status = importRequest(request, allBufs, allFences);
    if (status != Status::OK) {
        return status;
    }

    if (hasInputBuf) {
        auto key = std::make_pair(request.inputBuffer.streamId, request.frameNumber);
        auto& bufCache = mInflightBuffers[key] = StreamBufferCache{};
        // The last parameter (&bufCache) must be in heap, or we will
        // send a pointer pointing to stack memory to HAL and later HAL will break
        // when trying to accessing it after this call returned.
        convertFromHidl(
                allBufs[numOutputBufs], request.inputBuffer.status,
                &mStreamMap[request.inputBuffer.streamId], allFences[numOutputBufs],
                &bufCache);
        halRequest.input_buffer = &(bufCache.mStreamBuffer);
    } else {
        halRequest.input_buffer = nullptr;
    }

    halRequest.num_output_buffers = numOutputBufs;
    hidl_vec<camera3_stream_buffer_t> outHalBufs;
    outHalBufs.resize(numOutputBufs);
    for (size_t i = 0; i < numOutputBufs; i++) {
        auto key = std::make_pair(request.outputBuffers[i].streamId, request.frameNumber);
        auto& bufCache = mInflightBuffers[key] = StreamBufferCache{};
        // The last parameter (&bufCache) must be in heap, or we will
        // send a pointer pointing to stack memory to HAL and later HAL will break
        // when trying to accessing it after this call returned.
        convertFromHidl(
                allBufs[i], request.outputBuffers[i].status,
                &mStreamMap[request.outputBuffers[i].streamId], allFences[i],
                &bufCache);
        outHalBufs[i] = bufCache.mStreamBuffer;
    }
    halRequest.output_buffers = outHalBufs.data();

    ATRACE_ASYNC_BEGIN("frame capture", request.frameNumber);
    ATRACE_BEGIN("camera3->process_capture_request");
    status_t ret = mDevice->ops->process_capture_request(mDevice, &halRequest);
    ATRACE_END();
    if (ret != OK) {
        ALOGE("%s: HAL process_capture_request call failed!", __FUNCTION__);

        cleanupInflightBufferFences(allBufs, numBufs, allFences, numBufs);
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
        ATRACE_BEGIN("camera3->close");
        mDevice->common.close(&mDevice->common);
        ATRACE_END();
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
        }
    }

    d->mCallback->processCaptureResult(result);

    // Free cached buffer/fences.
    if (hasInputBuf) {
        int streamId = static_cast<Camera3Stream*>(hal_result->input_buffer->stream)->mId;
        auto key = std::make_pair(streamId, frameNumber);
        sHandleImporter.closeFence(d->mInflightBuffers[key].mStreamBuffer.acquire_fence);
        sHandleImporter.freeBuffer(d->mInflightBuffers[key].mBuffer);
        d->mInflightBuffers.erase(key);
    }

    for (size_t i = 0; i < numOutputBufs; i++) {
        int streamId = static_cast<Camera3Stream*>(hal_result->output_buffers[i].stream)->mId;
        auto key = std::make_pair(streamId, frameNumber);
        sHandleImporter.closeFence(d->mInflightBuffers[key].mStreamBuffer.acquire_fence);
        sHandleImporter.freeBuffer(d->mInflightBuffers[key].mBuffer);
        d->mInflightBuffers.erase(key);
    }

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
    if (hidlMsg.type == (MsgType) CAMERA3_MSG_ERROR) {
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
