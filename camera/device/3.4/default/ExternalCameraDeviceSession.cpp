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
#define LOG_TAG "ExtCamDevSsn@3.4"
//#define LOG_NDEBUG 0
#include <log/log.h>

#include <inttypes.h>
#include "ExternalCameraDeviceSession.h"

#include "android-base/macros.h"
#include "algorithm"
#include <utils/Timers.h>
#include <cmath>
#include <linux/videodev2.h>
#include <sync/sync.h>

#define HAVE_JPEG // required for libyuv.h to export MJPEG decode APIs
#include <libyuv.h>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

// Size of request/result metadata fast message queue. Change to 0 to always use hwbinder buffer.
static constexpr size_t kMetadataMsgQueueSize = 1 << 20 /* 1MB */;
const int ExternalCameraDeviceSession::kMaxProcessedStream;
const int ExternalCameraDeviceSession::kMaxStallStream;
const Size kMaxVideoSize = {1920, 1088}; // Maybe this should be programmable
const int kNumVideoBuffers = 4; // number of v4l2 buffers when streaming <= kMaxVideoSize
const int kNumStillBuffers = 2; // number of v4l2 buffers when streaming > kMaxVideoSize
const int kBadFramesAfterStreamOn = 1; // drop x frames after streamOn to get rid of some initial
                                       // bad frames. TODO: develop a better bad frame detection
                                       // method

// Aspect ratio is defined as width/height here and ExternalCameraDevice
// will guarantee all supported sizes has width >= height (so aspect ratio >= 1.0)
#define ASPECT_RATIO(sz) (static_cast<float>((sz).width) / (sz).height)
const float kMaxAspectRatio = std::numeric_limits<float>::max();
const float kMinAspectRatio = 1.f;

HandleImporter ExternalCameraDeviceSession::sHandleImporter;

bool isAspectRatioClose(float ar1, float ar2) {
    const float kAspectRatioMatchThres = 0.025f; // This threshold is good enough to distinguish
                                                // 4:3/16:9/20:9
                                                // 1.33 / 1.78 / 2
    return (std::abs(ar1 - ar2) < kAspectRatioMatchThres);
}

ExternalCameraDeviceSession::ExternalCameraDeviceSession(
        const sp<ICameraDeviceCallback>& callback,
        const std::vector<SupportedV4L2Format>& supportedFormats,
        const common::V1_0::helper::CameraMetadata& chars,
        unique_fd v4l2Fd) :
        mCallback(callback),
        mCameraCharacteristics(chars),
        mV4l2Fd(std::move(v4l2Fd)),
        mSupportedFormats(sortFormats(supportedFormats)),
        mCroppingType(initCroppingType(mSupportedFormats)),
        mOutputThread(new OutputThread(this, mCroppingType)) {
    mInitFail = initialize();
}

std::vector<SupportedV4L2Format> ExternalCameraDeviceSession::sortFormats(
            const std::vector<SupportedV4L2Format>& inFmts) {
    std::vector<SupportedV4L2Format> fmts = inFmts;
    std::sort(fmts.begin(), fmts.end(),
            [](const SupportedV4L2Format& a, const SupportedV4L2Format& b) -> bool {
                if (a.width == b.width) {
                    return a.height < b.height;
                }
                return a.width < b.width;
            });
    return fmts;
}

CroppingType ExternalCameraDeviceSession::initCroppingType(
        const std::vector<SupportedV4L2Format>& sortedFmts) {
    const auto& maxSize = sortedFmts[sortedFmts.size() - 1];
    float maxSizeAr = ASPECT_RATIO(maxSize);
    float minAr = kMaxAspectRatio;
    float maxAr = kMinAspectRatio;
    for (const auto& fmt : sortedFmts) {
        float ar = ASPECT_RATIO(fmt);
        if (ar < minAr) {
            minAr = ar;
        }
        if (ar > maxAr) {
            maxAr = ar;
        }
    }

    CroppingType ct = VERTICAL;
    if (isAspectRatioClose(maxSizeAr, maxAr)) {
        // Ex: 16:9 sensor, cropping horizontally to get to 4:3
        ct = HORIZONTAL;
    } else if (isAspectRatioClose(maxSizeAr, minAr)) {
        // Ex: 4:3 sensor, cropping vertically to get to 16:9
        ct = VERTICAL;
    } else {
        ALOGI("%s: camera maxSizeAr %f is not close to minAr %f or maxAr %f",
                __FUNCTION__, maxSizeAr, minAr, maxAr);
        if ((maxSizeAr - minAr) < (maxAr - maxSizeAr)) {
            ct = VERTICAL;
        } else {
            ct = HORIZONTAL;
        }
    }
    ALOGI("%s: camera croppingType is %d", __FUNCTION__, ct);
    return ct;
}


bool ExternalCameraDeviceSession::initialize() {
    if (mV4l2Fd.get() < 0) {
        ALOGE("%s: invalid v4l2 device fd %d!", __FUNCTION__, mV4l2Fd.get());
        return true;
    }

    status_t status = initDefaultRequests();
    if (status != OK) {
        ALOGE("%s: init default requests failed!", __FUNCTION__);
        return true;
    }

    mRequestMetadataQueue = std::make_unique<RequestMetadataQueue>(
            kMetadataMsgQueueSize, false /* non blocking */);
    if (!mRequestMetadataQueue->isValid()) {
        ALOGE("%s: invalid request fmq", __FUNCTION__);
        return true;
    }
    mResultMetadataQueue = std::make_shared<RequestMetadataQueue>(
            kMetadataMsgQueueSize, false /* non blocking */);
    if (!mResultMetadataQueue->isValid()) {
        ALOGE("%s: invalid result fmq", __FUNCTION__);
        return true;
    }

    // TODO: check is PRIORITY_DISPLAY enough?
    mOutputThread->run("ExtCamOut", PRIORITY_DISPLAY);
    return false;
}

Status ExternalCameraDeviceSession::initStatus() const {
    Mutex::Autolock _l(mLock);
    Status status = Status::OK;
    if (mInitFail || mClosed) {
        ALOGI("%s: sesssion initFailed %d closed %d", __FUNCTION__, mInitFail, mClosed);
        status = Status::INTERNAL_ERROR;
    }
    return status;
}

ExternalCameraDeviceSession::~ExternalCameraDeviceSession() {
    if (!isClosed()) {
        ALOGE("ExternalCameraDeviceSession deleted before close!");
        close();
    }
}

void ExternalCameraDeviceSession::dumpState(const native_handle_t*) {
    // TODO: b/72261676 dump more runtime information
}

Return<void> ExternalCameraDeviceSession::constructDefaultRequestSettings(
        V3_2::RequestTemplate type,
        V3_2::ICameraDeviceSession::constructDefaultRequestSettings_cb _hidl_cb) {
    V3_2::CameraMetadata outMetadata;
    Status status = constructDefaultRequestSettingsRaw(
            static_cast<RequestTemplate>(type), &outMetadata);
    _hidl_cb(status, outMetadata);
    return Void();
}

Return<void> ExternalCameraDeviceSession::constructDefaultRequestSettings_3_4(
        RequestTemplate type,
        ICameraDeviceSession::constructDefaultRequestSettings_cb _hidl_cb)  {
    V3_2::CameraMetadata outMetadata;
    Status status = constructDefaultRequestSettingsRaw(type, &outMetadata);
    _hidl_cb(status, outMetadata);
    return Void();
}

Status ExternalCameraDeviceSession::constructDefaultRequestSettingsRaw(RequestTemplate type,
        V3_2::CameraMetadata *outMetadata) {
    CameraMetadata emptyMd;
    Status status = initStatus();
    if (status != Status::OK) {
        return status;
    }

    switch (type) {
        case RequestTemplate::PREVIEW:
        case RequestTemplate::STILL_CAPTURE:
        case RequestTemplate::VIDEO_RECORD:
        case RequestTemplate::VIDEO_SNAPSHOT: {
            *outMetadata = mDefaultRequests[type];
            break;
        }
        case RequestTemplate::MANUAL:
        case RequestTemplate::ZERO_SHUTTER_LAG:
        case RequestTemplate::MOTION_TRACKING_PREVIEW:
        case RequestTemplate::MOTION_TRACKING_BEST:
            // Don't support MANUAL, ZSL, MOTION_TRACKING_* templates
            status = Status::ILLEGAL_ARGUMENT;
            break;
        default:
            ALOGE("%s: unknown request template type %d", __FUNCTION__, static_cast<int>(type));
            status = Status::ILLEGAL_ARGUMENT;
            break;
    }
    return status;
}

Return<void> ExternalCameraDeviceSession::configureStreams(
        const V3_2::StreamConfiguration& streams,
        ICameraDeviceSession::configureStreams_cb _hidl_cb) {
    V3_2::HalStreamConfiguration outStreams;
    V3_3::HalStreamConfiguration outStreams_v33;
    Mutex::Autolock _il(mInterfaceLock);

    Status status = configureStreams(streams, &outStreams_v33);
    size_t size = outStreams_v33.streams.size();
    outStreams.streams.resize(size);
    for (size_t i = 0; i < size; i++) {
        outStreams.streams[i] = outStreams_v33.streams[i].v3_2;
    }
    _hidl_cb(status, outStreams);
    return Void();
}

Return<void> ExternalCameraDeviceSession::configureStreams_3_3(
        const V3_2::StreamConfiguration& streams,
        ICameraDeviceSession::configureStreams_3_3_cb _hidl_cb) {
    V3_3::HalStreamConfiguration outStreams;
    Mutex::Autolock _il(mInterfaceLock);

    Status status = configureStreams(streams, &outStreams);
    _hidl_cb(status, outStreams);
    return Void();
}

Return<void> ExternalCameraDeviceSession::configureStreams_3_4(
        const V3_4::StreamConfiguration& requestedConfiguration,
        ICameraDeviceSession::configureStreams_3_4_cb _hidl_cb)  {
    V3_2::StreamConfiguration config_v32;
    V3_3::HalStreamConfiguration outStreams_v33;
    Mutex::Autolock _il(mInterfaceLock);

    config_v32.operationMode = requestedConfiguration.operationMode;
    config_v32.streams.resize(requestedConfiguration.streams.size());
    for (size_t i = 0; i < config_v32.streams.size(); i++) {
        config_v32.streams[i] = requestedConfiguration.streams[i].v3_2;
    }

    // Ignore requestedConfiguration.sessionParams. External camera does not support it
    Status status = configureStreams(config_v32, &outStreams_v33);

    V3_4::HalStreamConfiguration outStreams;
    outStreams.streams.resize(outStreams_v33.streams.size());
    for (size_t i = 0; i < outStreams.streams.size(); i++) {
        outStreams.streams[i].v3_3 = outStreams_v33.streams[i];
    }
    _hidl_cb(status, outStreams);
    return Void();
}

Return<void> ExternalCameraDeviceSession::getCaptureRequestMetadataQueue(
    ICameraDeviceSession::getCaptureRequestMetadataQueue_cb _hidl_cb) {
    Mutex::Autolock _il(mInterfaceLock);
    _hidl_cb(*mRequestMetadataQueue->getDesc());
    return Void();
}

Return<void> ExternalCameraDeviceSession::getCaptureResultMetadataQueue(
    ICameraDeviceSession::getCaptureResultMetadataQueue_cb _hidl_cb) {
    Mutex::Autolock _il(mInterfaceLock);
    _hidl_cb(*mResultMetadataQueue->getDesc());
    return Void();
}

Return<void> ExternalCameraDeviceSession::processCaptureRequest(
        const hidl_vec<CaptureRequest>& requests,
        const hidl_vec<BufferCache>& cachesToRemove,
        ICameraDeviceSession::processCaptureRequest_cb _hidl_cb) {
    Mutex::Autolock _il(mInterfaceLock);
    updateBufferCaches(cachesToRemove);

    uint32_t numRequestProcessed = 0;
    Status s = Status::OK;
    for (size_t i = 0; i < requests.size(); i++, numRequestProcessed++) {
        s = processOneCaptureRequest(requests[i]);
        if (s != Status::OK) {
            break;
        }
    }

    _hidl_cb(s, numRequestProcessed);
    return Void();
}

Return<void> ExternalCameraDeviceSession::processCaptureRequest_3_4(
        const hidl_vec<V3_4::CaptureRequest>& requests,
        const hidl_vec<V3_2::BufferCache>& cachesToRemove,
        ICameraDeviceSession::processCaptureRequest_3_4_cb _hidl_cb) {
    Mutex::Autolock _il(mInterfaceLock);
    updateBufferCaches(cachesToRemove);

    uint32_t numRequestProcessed = 0;
    Status s = Status::OK;
    for (size_t i = 0; i < requests.size(); i++, numRequestProcessed++) {
        s = processOneCaptureRequest(requests[i].v3_2);
        if (s != Status::OK) {
            break;
        }
    }

    _hidl_cb(s, numRequestProcessed);
    return Void();
}

Return<Status> ExternalCameraDeviceSession::flush() {
    return Status::OK;
}

Return<void> ExternalCameraDeviceSession::close() {
    Mutex::Autolock _il(mInterfaceLock);
    Mutex::Autolock _l(mLock);
    if (!mClosed) {
        // TODO: b/72261676 Cleanup inflight buffers/V4L2 buffer queue
        ALOGV("%s: closing V4L2 camera FD %d", __FUNCTION__, mV4l2Fd.get());
        mV4l2Fd.reset();
        mOutputThread->requestExit(); // TODO: join?

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

Status ExternalCameraDeviceSession::importRequest(
        const CaptureRequest& request,
        hidl_vec<buffer_handle_t*>& allBufPtrs,
        hidl_vec<int>& allFences) {
    size_t numOutputBufs = request.outputBuffers.size();
    size_t numBufs = numOutputBufs;
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
    return Status::OK;
}

void ExternalCameraDeviceSession::cleanupInflightFences(
        hidl_vec<int>& allFences, size_t numFences) {
    for (size_t j = 0; j < numFences; j++) {
        sHandleImporter.closeFence(allFences[j]);
    }
}

Status ExternalCameraDeviceSession::processOneCaptureRequest(const CaptureRequest& request)  {
    Status status = initStatus();
    if (status != Status::OK) {
        return status;
    }

    if (request.inputBuffer.streamId != -1) {
        ALOGE("%s: external camera does not support reprocessing!", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    Mutex::Autolock _l(mLock);
    if (!mV4l2Streaming) {
        ALOGE("%s: cannot process request in streamOff state!", __FUNCTION__);
        return Status::INTERNAL_ERROR;
    }

    const camera_metadata_t *rawSettings = nullptr;
    bool converted = true;
    CameraMetadata settingsFmq;  // settings from FMQ
    if (request.fmqSettingsSize > 0) {
        // non-blocking read; client must write metadata before calling
        // processOneCaptureRequest
        settingsFmq.resize(request.fmqSettingsSize);
        bool read = mRequestMetadataQueue->read(settingsFmq.data(), request.fmqSettingsSize);
        if (read) {
            converted = V3_2::implementation::convertFromHidl(settingsFmq, &rawSettings);
        } else {
            ALOGE("%s: capture request settings metadata couldn't be read from fmq!", __FUNCTION__);
            converted = false;
        }
    } else {
        converted = V3_2::implementation::convertFromHidl(request.settings, &rawSettings);
    }

    if (converted && rawSettings != nullptr) {
        mLatestReqSetting = rawSettings;
    }

    if (!converted) {
        ALOGE("%s: capture request settings metadata is corrupt!", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    if (mFirstRequest && rawSettings == nullptr) {
        ALOGE("%s: capture request settings must not be null for first request!",
                __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    hidl_vec<buffer_handle_t*> allBufPtrs;
    hidl_vec<int> allFences;
    size_t numOutputBufs = request.outputBuffers.size();

    if (numOutputBufs == 0) {
        ALOGE("%s: capture request must have at least one output buffer!", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    status = importRequest(request, allBufPtrs, allFences);
    if (status != Status::OK) {
        return status;
    }

    // TODO: program fps range per capture request here
    //       or limit the set of availableFpsRange

    sp<V4L2Frame> frameIn = dequeueV4l2FrameLocked();
    if ( frameIn == nullptr) {
        ALOGE("%s: V4L2 deque frame failed!", __FUNCTION__);
        return Status::INTERNAL_ERROR;
    }
    // TODO: This can probably be replaced by use v4lbuffer timestamp
    //       if the device supports it
    nsecs_t shutterTs = systemTime(SYSTEM_TIME_MONOTONIC);


    // TODO: reduce object copy in this path
    HalRequest halReq = {
            .frameNumber = request.frameNumber,
            .setting = mLatestReqSetting,
            .frameIn = frameIn,
            .shutterTs = shutterTs};
    halReq.buffers.resize(numOutputBufs);
    for (size_t i = 0; i < numOutputBufs; i++) {
        HalStreamBuffer& halBuf = halReq.buffers[i];
        int streamId = halBuf.streamId = request.outputBuffers[i].streamId;
        halBuf.bufferId = request.outputBuffers[i].bufferId;
        const Stream& stream = mStreamMap[streamId];
        halBuf.width = stream.width;
        halBuf.height = stream.height;
        halBuf.format = stream.format;
        halBuf.usage = stream.usage;
        halBuf.bufPtr = allBufPtrs[i];
        halBuf.acquireFence = allFences[i];
        halBuf.fenceTimeout = false;
    }
    mInflightFrames.insert(halReq.frameNumber);
    // Send request to OutputThread for the rest of processing
    mOutputThread->submitRequest(halReq);
    mFirstRequest = false;
    return Status::OK;
}

void ExternalCameraDeviceSession::notifyShutter(uint32_t frameNumber, nsecs_t shutterTs) {
    NotifyMsg msg;
    msg.type = MsgType::SHUTTER;
    msg.msg.shutter.frameNumber = frameNumber;
    msg.msg.shutter.timestamp = shutterTs;
    mCallback->notify({msg});
}

void ExternalCameraDeviceSession::notifyError(
        uint32_t frameNumber, int32_t streamId, ErrorCode ec) {
    NotifyMsg msg;
    msg.type = MsgType::ERROR;
    msg.msg.error.frameNumber = frameNumber;
    msg.msg.error.errorStreamId = streamId;
    msg.msg.error.errorCode = ec;
    mCallback->notify({msg});
}

//TODO: refactor with processCaptureResult
Status ExternalCameraDeviceSession::processCaptureRequestError(HalRequest& req) {
    // Return V4L2 buffer to V4L2 buffer queue
    enqueueV4l2Frame(req.frameIn);

    // NotifyShutter
    notifyShutter(req.frameNumber, req.shutterTs);

    notifyError(/*frameNum*/req.frameNumber, /*stream*/-1, ErrorCode::ERROR_REQUEST);

    // Fill output buffers
    hidl_vec<CaptureResult> results;
    results.resize(1);
    CaptureResult& result = results[0];
    result.frameNumber = req.frameNumber;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    result.outputBuffers.resize(req.buffers.size());
    for (size_t i = 0; i < req.buffers.size(); i++) {
        result.outputBuffers[i].streamId = req.buffers[i].streamId;
        result.outputBuffers[i].bufferId = req.buffers[i].bufferId;
        result.outputBuffers[i].status = BufferStatus::ERROR;
        if (req.buffers[i].acquireFence >= 0) {
            native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
            handle->data[0] = req.buffers[i].acquireFence;
            result.outputBuffers[i].releaseFence.setTo(handle, /*shouldOwn*/true);
        }
    }

    // update inflight records
    {
        Mutex::Autolock _l(mLock);
        mInflightFrames.erase(req.frameNumber);
    }

    // Callback into framework
    invokeProcessCaptureResultCallback(results, /* tryWriteFmq */true);
    freeReleaseFences(results);
    return Status::OK;
}

Status ExternalCameraDeviceSession::processCaptureResult(HalRequest& req) {
    // Return V4L2 buffer to V4L2 buffer queue
    enqueueV4l2Frame(req.frameIn);

    // NotifyShutter
    notifyShutter(req.frameNumber, req.shutterTs);

    // Fill output buffers
    hidl_vec<CaptureResult> results;
    results.resize(1);
    CaptureResult& result = results[0];
    result.frameNumber = req.frameNumber;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    result.outputBuffers.resize(req.buffers.size());
    for (size_t i = 0; i < req.buffers.size(); i++) {
        result.outputBuffers[i].streamId = req.buffers[i].streamId;
        result.outputBuffers[i].bufferId = req.buffers[i].bufferId;
        if (req.buffers[i].fenceTimeout) {
            result.outputBuffers[i].status = BufferStatus::ERROR;
            native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
            handle->data[0] = req.buffers[i].acquireFence;
            result.outputBuffers[i].releaseFence.setTo(handle, /*shouldOwn*/true);
            notifyError(req.frameNumber, req.buffers[i].streamId, ErrorCode::ERROR_BUFFER);
        } else {
            result.outputBuffers[i].status = BufferStatus::OK;
            // TODO: refactor
            if (req.buffers[i].acquireFence > 0) {
                native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
                handle->data[0] = req.buffers[i].acquireFence;
                result.outputBuffers[i].releaseFence.setTo(handle, /*shouldOwn*/true);
            }
        }
    }

    // Fill capture result metadata
    fillCaptureResult(req.setting, req.shutterTs);
    const camera_metadata_t *rawResult = req.setting.getAndLock();
    V3_2::implementation::convertToHidl(rawResult, &result.result);
    req.setting.unlock(rawResult);

    // update inflight records
    {
        Mutex::Autolock _l(mLock);
        mInflightFrames.erase(req.frameNumber);
    }

    // Callback into framework
    invokeProcessCaptureResultCallback(results, /* tryWriteFmq */true);
    freeReleaseFences(results);
    return Status::OK;
}

void ExternalCameraDeviceSession::invokeProcessCaptureResultCallback(
        hidl_vec<CaptureResult> &results, bool tryWriteFmq) {
    if (mProcessCaptureResultLock.tryLock() != OK) {
        const nsecs_t NS_TO_SECOND = 1000000000;
        ALOGV("%s: previous call is not finished! waiting 1s...", __FUNCTION__);
        if (mProcessCaptureResultLock.timedLock(/* 1s */NS_TO_SECOND) != OK) {
            ALOGE("%s: cannot acquire lock in 1s, cannot proceed",
                    __FUNCTION__);
            return;
        }
    }
    if (tryWriteFmq && mResultMetadataQueue->availableToWrite() > 0) {
        for (CaptureResult &result : results) {
            if (result.result.size() > 0) {
                if (mResultMetadataQueue->write(result.result.data(), result.result.size())) {
                    result.fmqResultSize = result.result.size();
                    result.result.resize(0);
                } else {
                    ALOGW("%s: couldn't utilize fmq, fall back to hwbinder", __FUNCTION__);
                    result.fmqResultSize = 0;
                }
            } else {
                result.fmqResultSize = 0;
            }
        }
    }
    mCallback->processCaptureResult(results);
    mProcessCaptureResultLock.unlock();
}

void ExternalCameraDeviceSession::freeReleaseFences(hidl_vec<CaptureResult>& results) {
    for (auto& result : results) {
        if (result.inputBuffer.releaseFence.getNativeHandle() != nullptr) {
            native_handle_t* handle = const_cast<native_handle_t*>(
                    result.inputBuffer.releaseFence.getNativeHandle());
            native_handle_close(handle);
            native_handle_delete(handle);
        }
        for (auto& buf : result.outputBuffers) {
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

ExternalCameraDeviceSession::OutputThread::OutputThread(
        wp<ExternalCameraDeviceSession> parent,
        CroppingType ct) : mParent(parent), mCroppingType(ct) {}

ExternalCameraDeviceSession::OutputThread::~OutputThread() {}

uint32_t ExternalCameraDeviceSession::OutputThread::getFourCcFromLayout(
        const YCbCrLayout& layout) {
    intptr_t cb = reinterpret_cast<intptr_t>(layout.cb);
    intptr_t cr = reinterpret_cast<intptr_t>(layout.cr);
    if (std::abs(cb - cr) == 1 && layout.chromaStep == 2) {
        // Interleaved format
        if (layout.cb > layout.cr) {
            return V4L2_PIX_FMT_NV21;
        } else {
            return V4L2_PIX_FMT_NV12;
        }
    } else if (layout.chromaStep == 1) {
        // Planar format
        if (layout.cb > layout.cr) {
            return V4L2_PIX_FMT_YVU420; // YV12
        } else {
            return V4L2_PIX_FMT_YUV420; // YU12
        }
    } else {
        return FLEX_YUV_GENERIC;
    }
}

int ExternalCameraDeviceSession::OutputThread::getCropRect(
        CroppingType ct, const Size& inSize, const Size& outSize, IMapper::Rect* out) {
    if (out == nullptr) {
        ALOGE("%s: out is null", __FUNCTION__);
        return -1;
    }

    uint32_t inW = inSize.width;
    uint32_t inH = inSize.height;
    uint32_t outW = outSize.width;
    uint32_t outH = outSize.height;

    // Handle special case where aspect ratio is close to input but scaled
    // dimension is slightly larger than input
    float arIn = ASPECT_RATIO(inSize);
    float arOut = ASPECT_RATIO(outSize);
    if (isAspectRatioClose(arIn, arOut)) {
        out->left = 0;
        out->top = 0;
        out->width = inW;
        out->height = inH;
        return 0;
    }

    if (ct == VERTICAL) {
        uint64_t scaledOutH = static_cast<uint64_t>(outH) * inW / outW;
        if (scaledOutH > inH) {
            ALOGE("%s: Output size %dx%d cannot be vertically cropped from input size %dx%d",
                    __FUNCTION__, outW, outH, inW, inH);
            return -1;
        }
        scaledOutH = scaledOutH & ~0x1; // make it multiple of 2

        out->left = 0;
        out->top = ((inH - scaledOutH) / 2) & ~0x1;
        out->width = inW;
        out->height = static_cast<int32_t>(scaledOutH);
        ALOGV("%s: crop %dx%d to %dx%d: top %d, scaledH %d",
                __FUNCTION__, inW, inH, outW, outH, out->top, static_cast<int32_t>(scaledOutH));
    } else {
        uint64_t scaledOutW = static_cast<uint64_t>(outW) * inH / outH;
        if (scaledOutW > inW) {
            ALOGE("%s: Output size %dx%d cannot be horizontally cropped from input size %dx%d",
                    __FUNCTION__, outW, outH, inW, inH);
            return -1;
        }
        scaledOutW = scaledOutW & ~0x1; // make it multiple of 2

        out->left = ((inW - scaledOutW) / 2) & ~0x1;
        out->top = 0;
        out->width = static_cast<int32_t>(scaledOutW);
        out->height = inH;
        ALOGV("%s: crop %dx%d to %dx%d: top %d, scaledW %d",
                __FUNCTION__, inW, inH, outW, outH, out->top, static_cast<int32_t>(scaledOutW));
    }

    return 0;
}

int ExternalCameraDeviceSession::OutputThread::cropAndScaleLocked(
        sp<AllocatedFrame>& in, const HalStreamBuffer& halBuf, YCbCrLayout* out) {
    Size inSz = {in->mWidth, in->mHeight};
    Size outSz = {halBuf.width, halBuf.height};
    int ret;
    if (inSz == outSz) {
        ret = in->getLayout(out);
        if (ret != 0) {
            ALOGE("%s: failed to get input image layout", __FUNCTION__);
            return ret;
        }
        return ret;
    }

    // Cropping to output aspect ratio
    IMapper::Rect inputCrop;
    ret = getCropRect(mCroppingType, inSz, outSz, &inputCrop);
    if (ret != 0) {
        ALOGE("%s: failed to compute crop rect for output size %dx%d",
                __FUNCTION__, outSz.width, outSz.height);
        return ret;
    }

    YCbCrLayout croppedLayout;
    ret = in->getCroppedLayout(inputCrop, &croppedLayout);
    if (ret != 0) {
        ALOGE("%s: failed to crop input image %dx%d to output size %dx%d",
                __FUNCTION__, inSz.width, inSz.height, outSz.width, outSz.height);
        return ret;
    }

    if ((mCroppingType == VERTICAL && inSz.width == outSz.width) ||
            (mCroppingType == HORIZONTAL && inSz.height == outSz.height)) {
        // No scale is needed
        *out = croppedLayout;
        return 0;
    }

    auto it = mScaledYu12Frames.find(outSz);
    sp<AllocatedFrame> scaledYu12Buf;
    if (it != mScaledYu12Frames.end()) {
        scaledYu12Buf = it->second;
    } else {
        it = mIntermediateBuffers.find(outSz);
        if (it == mIntermediateBuffers.end()) {
            ALOGE("%s: failed to find intermediate buffer size %dx%d",
                    __FUNCTION__, outSz.width, outSz.height);
            return -1;
        }
        scaledYu12Buf = it->second;
    }
    // Scale
    YCbCrLayout outLayout;
    ret = scaledYu12Buf->getLayout(&outLayout);
    if (ret != 0) {
        ALOGE("%s: failed to get output buffer layout", __FUNCTION__);
        return ret;
    }

    ret = libyuv::I420Scale(
            static_cast<uint8_t*>(croppedLayout.y),
            croppedLayout.yStride,
            static_cast<uint8_t*>(croppedLayout.cb),
            croppedLayout.cStride,
            static_cast<uint8_t*>(croppedLayout.cr),
            croppedLayout.cStride,
            inputCrop.width,
            inputCrop.height,
            static_cast<uint8_t*>(outLayout.y),
            outLayout.yStride,
            static_cast<uint8_t*>(outLayout.cb),
            outLayout.cStride,
            static_cast<uint8_t*>(outLayout.cr),
            outLayout.cStride,
            outSz.width,
            outSz.height,
            // TODO: b/72261744 see if we can use better filter without losing too much perf
            libyuv::FilterMode::kFilterNone);

    if (ret != 0) {
        ALOGE("%s: failed to scale buffer from %dx%d to %dx%d. Ret %d",
                __FUNCTION__, inputCrop.width, inputCrop.height,
                outSz.width, outSz.height, ret);
        return ret;
    }

    *out = outLayout;
    mScaledYu12Frames.insert({outSz, scaledYu12Buf});
    return 0;
}

int ExternalCameraDeviceSession::OutputThread::formatConvertLocked(
        const YCbCrLayout& in, const YCbCrLayout& out, Size sz, uint32_t format) {
    int ret = 0;
    switch (format) {
        case V4L2_PIX_FMT_NV21:
            ret = libyuv::I420ToNV21(
                    static_cast<uint8_t*>(in.y),
                    in.yStride,
                    static_cast<uint8_t*>(in.cb),
                    in.cStride,
                    static_cast<uint8_t*>(in.cr),
                    in.cStride,
                    static_cast<uint8_t*>(out.y),
                    out.yStride,
                    static_cast<uint8_t*>(out.cr),
                    out.cStride,
                    sz.width,
                    sz.height);
            if (ret != 0) {
                ALOGE("%s: convert to NV21 buffer failed! ret %d",
                            __FUNCTION__, ret);
                return ret;
            }
            break;
        case V4L2_PIX_FMT_NV12:
            ret = libyuv::I420ToNV12(
                    static_cast<uint8_t*>(in.y),
                    in.yStride,
                    static_cast<uint8_t*>(in.cb),
                    in.cStride,
                    static_cast<uint8_t*>(in.cr),
                    in.cStride,
                    static_cast<uint8_t*>(out.y),
                    out.yStride,
                    static_cast<uint8_t*>(out.cb),
                    out.cStride,
                    sz.width,
                    sz.height);
            if (ret != 0) {
                ALOGE("%s: convert to NV12 buffer failed! ret %d",
                            __FUNCTION__, ret);
                return ret;
            }
            break;
        case V4L2_PIX_FMT_YVU420: // YV12
        case V4L2_PIX_FMT_YUV420: // YU12
            // TODO: maybe we can speed up here by somehow save this copy?
            ret = libyuv::I420Copy(
                    static_cast<uint8_t*>(in.y),
                    in.yStride,
                    static_cast<uint8_t*>(in.cb),
                    in.cStride,
                    static_cast<uint8_t*>(in.cr),
                    in.cStride,
                    static_cast<uint8_t*>(out.y),
                    out.yStride,
                    static_cast<uint8_t*>(out.cb),
                    out.cStride,
                    static_cast<uint8_t*>(out.cr),
                    out.cStride,
                    sz.width,
                    sz.height);
            if (ret != 0) {
                ALOGE("%s: copy to YV12 or YU12 buffer failed! ret %d",
                            __FUNCTION__, ret);
                return ret;
            }
            break;
        case FLEX_YUV_GENERIC:
            // TODO: b/72261744 write to arbitrary flexible YUV layout. Slow.
            ALOGE("%s: unsupported flexible yuv layout"
                    " y %p cb %p cr %p y_str %d c_str %d c_step %d",
                    __FUNCTION__, out.y, out.cb, out.cr,
                    out.yStride, out.cStride, out.chromaStep);
            return -1;
        default:
            ALOGE("%s: unknown YUV format 0x%x!", __FUNCTION__, format);
            return -1;
    }
    return 0;
}

bool ExternalCameraDeviceSession::OutputThread::threadLoop() {
    HalRequest req;
    auto parent = mParent.promote();
    if (parent == nullptr) {
       ALOGE("%s: session has been disconnected!", __FUNCTION__);
       return false;
    }

    // TODO: maybe we need to setup a sensor thread to dq/enq v4l frames
    //       regularly to prevent v4l buffer queue filled with stale buffers
    //       when app doesn't program a preveiw request
    waitForNextRequest(&req);
    if (req.frameIn == nullptr) {
        // No new request, wait again
        return true;
    }

    if (req.frameIn->mFourcc != V4L2_PIX_FMT_MJPEG) {
        ALOGE("%s: do not support V4L2 format %c%c%c%c", __FUNCTION__,
                req.frameIn->mFourcc & 0xFF,
                (req.frameIn->mFourcc >> 8) & 0xFF,
                (req.frameIn->mFourcc >> 16) & 0xFF,
                (req.frameIn->mFourcc >> 24) & 0xFF);
        parent->notifyError(
                /*frameNum*/req.frameNumber, /*stream*/-1, ErrorCode::ERROR_DEVICE);
        return false;
    }

    std::unique_lock<std::mutex> lk(mLock);

    // Convert input V4L2 frame to YU12 of the same size
    // TODO: see if we can save some computation by converting to YV12 here
    uint8_t* inData;
    size_t inDataSize;
    req.frameIn->map(&inData, &inDataSize);
    // TODO: profile
    // TODO: in some special case maybe we can decode jpg directly to gralloc output?
    int res = libyuv::MJPGToI420(
            inData, inDataSize,
            static_cast<uint8_t*>(mYu12FrameLayout.y),
            mYu12FrameLayout.yStride,
            static_cast<uint8_t*>(mYu12FrameLayout.cb),
            mYu12FrameLayout.cStride,
            static_cast<uint8_t*>(mYu12FrameLayout.cr),
            mYu12FrameLayout.cStride,
            mYu12Frame->mWidth, mYu12Frame->mHeight,
            mYu12Frame->mWidth, mYu12Frame->mHeight);

    if (res != 0) {
        // For some webcam, the first few V4L2 frames might be malformed...
        ALOGE("%s: Convert V4L2 frame to YU12 failed! res %d", __FUNCTION__, res);
        lk.unlock();
        Status st = parent->processCaptureRequestError(req);
        if (st != Status::OK) {
            ALOGE("%s: failed to process capture request error!", __FUNCTION__);
            parent->notifyError(
                    /*frameNum*/req.frameNumber, /*stream*/-1, ErrorCode::ERROR_DEVICE);
            return false;
        }
        return true;
    }

    ALOGV("%s processing new request", __FUNCTION__);
    const int kSyncWaitTimeoutMs = 500;
    for (auto& halBuf : req.buffers) {
        if (halBuf.acquireFence != -1) {
            int ret = sync_wait(halBuf.acquireFence, kSyncWaitTimeoutMs);
            if (ret) {
                halBuf.fenceTimeout = true;
            } else {
                ::close(halBuf.acquireFence);
            }
        }

        if (halBuf.fenceTimeout) {
            continue;
        }

        // Gralloc lockYCbCr the buffer
        switch (halBuf.format) {
            case PixelFormat::BLOB:
                // TODO: b/72261675 implement JPEG output path
                break;
            case PixelFormat::YCBCR_420_888:
            case PixelFormat::YV12: {
                IMapper::Rect outRect {0, 0,
                        static_cast<int32_t>(halBuf.width),
                        static_cast<int32_t>(halBuf.height)};
                YCbCrLayout outLayout = sHandleImporter.lockYCbCr(
                        *(halBuf.bufPtr), halBuf.usage, outRect);
                ALOGV("%s: outLayout y %p cb %p cr %p y_str %d c_str %d c_step %d",
                        __FUNCTION__, outLayout.y, outLayout.cb, outLayout.cr,
                        outLayout.yStride, outLayout.cStride, outLayout.chromaStep);

                // Convert to output buffer size/format
                uint32_t outputFourcc = getFourCcFromLayout(outLayout);
                ALOGV("%s: converting to format %c%c%c%c", __FUNCTION__,
                        outputFourcc & 0xFF,
                        (outputFourcc >> 8) & 0xFF,
                        (outputFourcc >> 16) & 0xFF,
                        (outputFourcc >> 24) & 0xFF);

                YCbCrLayout cropAndScaled;
                int ret = cropAndScaleLocked(
                        mYu12Frame, halBuf, &cropAndScaled);
                if (ret != 0) {
                    ALOGE("%s: crop and scale failed!", __FUNCTION__);
                    lk.unlock();
                    parent->notifyError(
                            /*frameNum*/req.frameNumber, /*stream*/-1, ErrorCode::ERROR_DEVICE);
                    return false;
                }

                Size sz {halBuf.width, halBuf.height};
                ret = formatConvertLocked(cropAndScaled, outLayout, sz, outputFourcc);
                if (ret != 0) {
                    ALOGE("%s: format coversion failed!", __FUNCTION__);
                    lk.unlock();
                    parent->notifyError(
                            /*frameNum*/req.frameNumber, /*stream*/-1, ErrorCode::ERROR_DEVICE);
                    return false;
                }
                int relFence = sHandleImporter.unlock(*(halBuf.bufPtr));
                if (relFence > 0) {
                    halBuf.acquireFence = relFence;
                }
            } break;
            default:
                ALOGE("%s: unknown output format %x", __FUNCTION__, halBuf.format);
                lk.unlock();
                parent->notifyError(
                        /*frameNum*/req.frameNumber, /*stream*/-1, ErrorCode::ERROR_DEVICE);
                return false;
        }
    } // for each buffer
    mScaledYu12Frames.clear();

    // Don't hold the lock while calling back to parent
    lk.unlock();
    Status st = parent->processCaptureResult(req);
    if (st != Status::OK) {
        ALOGE("%s: failed to process capture result!", __FUNCTION__);
        parent->notifyError(
                /*frameNum*/req.frameNumber, /*stream*/-1, ErrorCode::ERROR_DEVICE);
        return false;
    }
    return true;
}

Status ExternalCameraDeviceSession::OutputThread::allocateIntermediateBuffers(
        const Size& v4lSize, const hidl_vec<Stream>& streams) {
    std::lock_guard<std::mutex> lk(mLock);
    if (mScaledYu12Frames.size() != 0) {
        ALOGE("%s: intermediate buffer pool has %zu inflight buffers! (expect 0)",
                __FUNCTION__, mScaledYu12Frames.size());
        return Status::INTERNAL_ERROR;
    }

    // Allocating intermediate YU12 frame
    if (mYu12Frame == nullptr || mYu12Frame->mWidth != v4lSize.width ||
            mYu12Frame->mHeight != v4lSize.height) {
        mYu12Frame.clear();
        mYu12Frame = new AllocatedFrame(v4lSize.width, v4lSize.height);
        int ret = mYu12Frame->allocate(&mYu12FrameLayout);
        if (ret != 0) {
            ALOGE("%s: allocating YU12 frame failed!", __FUNCTION__);
            return Status::INTERNAL_ERROR;
        }
    }

    // Allocating scaled buffers
    for (const auto& stream : streams) {
        Size sz = {stream.width, stream.height};
        if (sz == v4lSize) {
            continue; // Don't need an intermediate buffer same size as v4lBuffer
        }
        if (mIntermediateBuffers.count(sz) == 0) {
            // Create new intermediate buffer
            sp<AllocatedFrame> buf = new AllocatedFrame(stream.width, stream.height);
            int ret = buf->allocate();
            if (ret != 0) {
                ALOGE("%s: allocating intermediate YU12 frame %dx%d failed!",
                            __FUNCTION__, stream.width, stream.height);
                return Status::INTERNAL_ERROR;
            }
            mIntermediateBuffers[sz] = buf;
        }
    }

    // Remove unconfigured buffers
    auto it = mIntermediateBuffers.begin();
    while (it != mIntermediateBuffers.end()) {
        bool configured = false;
        auto sz = it->first;
        for (const auto& stream : streams) {
            if (stream.width == sz.width && stream.height == sz.height) {
                configured = true;
                break;
            }
        }
        if (configured) {
            it++;
        } else {
            it = mIntermediateBuffers.erase(it);
        }
    }
    return Status::OK;
}

Status ExternalCameraDeviceSession::OutputThread::submitRequest(const HalRequest& req) {
    std::lock_guard<std::mutex> lk(mLock);
    // TODO: reduce object copy in this path
    mRequestList.push_back(req);
    mRequestCond.notify_one();
    return Status::OK;
}

void ExternalCameraDeviceSession::OutputThread::flush() {
    std::lock_guard<std::mutex> lk(mLock);
    // TODO: send buffer/request errors back to framework
    mRequestList.clear();
}

void ExternalCameraDeviceSession::OutputThread::waitForNextRequest(HalRequest* out) {
    if (out == nullptr) {
        ALOGE("%s: out is null", __FUNCTION__);
        return;
    }

    std::unique_lock<std::mutex> lk(mLock);
    while (mRequestList.empty()) {
        std::chrono::seconds timeout = std::chrono::seconds(kReqWaitTimeoutSec);
        auto st = mRequestCond.wait_for(lk, timeout);
        if (st == std::cv_status::timeout) {
            // no new request, return
            return;
        }
    }
    *out = mRequestList.front();
    mRequestList.pop_front();
}

void ExternalCameraDeviceSession::cleanupBuffersLocked(int id) {
    for (auto& pair : mCirculatingBuffers.at(id)) {
        sHandleImporter.freeBuffer(pair.second);
    }
    mCirculatingBuffers[id].clear();
    mCirculatingBuffers.erase(id);
}

void ExternalCameraDeviceSession::updateBufferCaches(const hidl_vec<BufferCache>& cachesToRemove) {
    Mutex::Autolock _l(mLock);
    for (auto& cache : cachesToRemove) {
        auto cbsIt = mCirculatingBuffers.find(cache.streamId);
        if (cbsIt == mCirculatingBuffers.end()) {
            // The stream could have been removed
            continue;
        }
        CirculatingBuffers& cbs = cbsIt->second;
        auto it = cbs.find(cache.bufferId);
        if (it != cbs.end()) {
            sHandleImporter.freeBuffer(it->second);
            cbs.erase(it);
        } else {
            ALOGE("%s: stream %d buffer %" PRIu64 " is not cached",
                    __FUNCTION__, cache.streamId, cache.bufferId);
        }
    }
}

bool ExternalCameraDeviceSession::isSupported(const Stream& stream) {
    int32_t ds = static_cast<int32_t>(stream.dataSpace);
    PixelFormat fmt = stream.format;
    uint32_t width = stream.width;
    uint32_t height = stream.height;
    // TODO: check usage flags

    if (stream.streamType != StreamType::OUTPUT) {
        ALOGE("%s: does not support non-output stream type", __FUNCTION__);
        return false;
    }

    if (stream.rotation != StreamRotation::ROTATION_0) {
        ALOGE("%s: does not support stream rotation", __FUNCTION__);
        return false;
    }

    if (ds & Dataspace::DEPTH) {
        ALOGI("%s: does not support depth output", __FUNCTION__);
        return false;
    }

    switch (fmt) {
        case PixelFormat::BLOB:
            if (ds != static_cast<int32_t>(Dataspace::V0_JFIF)) {
                ALOGI("%s: BLOB format does not support dataSpace %x", __FUNCTION__, ds);
                return false;
            }
        case PixelFormat::IMPLEMENTATION_DEFINED:
        case PixelFormat::YCBCR_420_888:
        case PixelFormat::YV12:
            // TODO: check what dataspace we can support here.
            // intentional no-ops.
            break;
        default:
            ALOGI("%s: does not support format %x", __FUNCTION__, fmt);
            return false;
    }

    // Assume we can convert any V4L2 format to any of supported output format for now, i.e,
    // ignoring v4l2Fmt.fourcc for now. Might need more subtle check if we support more v4l format
    // in the futrue.
    for (const auto& v4l2Fmt : mSupportedFormats) {
        if (width == v4l2Fmt.width && height == v4l2Fmt.height) {
            return true;
        }
    }
    ALOGI("%s: resolution %dx%d is not supported", __FUNCTION__, width, height);
    return false;
}

int ExternalCameraDeviceSession::v4l2StreamOffLocked() {
    if (!mV4l2Streaming) {
        return OK;
    }

    {
        std::lock_guard<std::mutex> lk(mV4l2BufferLock);
        if (mNumDequeuedV4l2Buffers != 0)  {
            ALOGE("%s: there are %zu inflight V4L buffers",
                __FUNCTION__, mNumDequeuedV4l2Buffers);
            return -1;
        }
    }
    mV4l2Buffers.clear(); // VIDIOC_REQBUFS will fail if FDs are not clear first

    // VIDIOC_STREAMOFF
    v4l2_buf_type capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_STREAMOFF, &capture_type)) < 0) {
        ALOGE("%s: STREAMOFF failed: %s", __FUNCTION__, strerror(errno));
        return -errno;
    }

    // VIDIOC_REQBUFS: clear buffers
    v4l2_requestbuffers req_buffers{};
    req_buffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_buffers.memory = V4L2_MEMORY_MMAP;
    req_buffers.count = 0;
    if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_REQBUFS, &req_buffers)) < 0) {
        ALOGE("%s: REQBUFS failed: %s", __FUNCTION__, strerror(errno));
        return -errno;
    }

    mV4l2Streaming = false;
    return OK;
}

int ExternalCameraDeviceSession::configureV4l2StreamLocked(const SupportedV4L2Format& v4l2Fmt) {
    int ret = v4l2StreamOffLocked();
    if (ret != OK) {
        ALOGE("%s: stop v4l2 streaming failed: ret %d", __FUNCTION__, ret);
        return ret;
    }

    // VIDIOC_S_FMT w/h/fmt
    v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = v4l2Fmt.width;
    fmt.fmt.pix.height = v4l2Fmt.height;
    fmt.fmt.pix.pixelformat = v4l2Fmt.fourcc;
    ret = TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_S_FMT, &fmt));
    if (ret < 0) {
        ALOGE("%s: S_FMT ioctl failed: %s", __FUNCTION__, strerror(errno));
        return -errno;
    }

    if (v4l2Fmt.width != fmt.fmt.pix.width || v4l2Fmt.height != fmt.fmt.pix.height ||
            v4l2Fmt.fourcc != fmt.fmt.pix.pixelformat) {
        ALOGE("%s: S_FMT expect %c%c%c%c %dx%d, got %c%c%c%c %dx%d instead!", __FUNCTION__,
                v4l2Fmt.fourcc & 0xFF,
                (v4l2Fmt.fourcc >> 8) & 0xFF,
                (v4l2Fmt.fourcc >> 16) & 0xFF,
                (v4l2Fmt.fourcc >> 24) & 0xFF,
                v4l2Fmt.width, v4l2Fmt.height,
                fmt.fmt.pix.pixelformat & 0xFF,
                (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
                (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
                (fmt.fmt.pix.pixelformat >> 24) & 0xFF,
                fmt.fmt.pix.width, fmt.fmt.pix.height);
        return -EINVAL;
    }
    uint32_t bufferSize = fmt.fmt.pix.sizeimage;
    ALOGI("%s: V4L2 buffer size is %d", __FUNCTION__, bufferSize);

    float maxFps = -1.f;
    float fps = 1000.f;
    const float kDefaultFps = 30.f;
    // Try to pick the slowest fps that is at least 30
    for (const auto& f : v4l2Fmt.frameRates) {
        if (maxFps < f) {
            maxFps = f;
        }
        if (f >= kDefaultFps && f < fps) {
            fps = f;
        }
    }
    if (fps == 1000.f) {
        fps = maxFps;
    }

    // VIDIOC_G_PARM/VIDIOC_S_PARM: set fps
    v4l2_streamparm streamparm = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
    // The following line checks that the driver knows about framerate get/set.
    if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_G_PARM, &streamparm)) >= 0) {
        // Now check if the device is able to accept a capture framerate set.
        if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
            // |frame_rate| is float, approximate by a fraction.
            const int kFrameRatePrecision = 10000;
            streamparm.parm.capture.timeperframe.numerator = kFrameRatePrecision;
            streamparm.parm.capture.timeperframe.denominator =
                (fps * kFrameRatePrecision);

            if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_S_PARM, &streamparm)) < 0) {
                ALOGE("%s: failed to set framerate to %f", __FUNCTION__, fps);
                return UNKNOWN_ERROR;
            }
        }
    }
    float retFps = streamparm.parm.capture.timeperframe.denominator /
                streamparm.parm.capture.timeperframe.numerator;
    if (std::fabs(fps - retFps) > std::numeric_limits<float>::epsilon()) {
        ALOGE("%s: expect fps %f, got %f instead", __FUNCTION__, fps, retFps);
        return BAD_VALUE;
    }

    uint32_t v4lBufferCount = (v4l2Fmt.width <= kMaxVideoSize.width &&
            v4l2Fmt.height <= kMaxVideoSize.height) ? kNumVideoBuffers : kNumStillBuffers;
    // VIDIOC_REQBUFS: create buffers
    v4l2_requestbuffers req_buffers{};
    req_buffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_buffers.memory = V4L2_MEMORY_MMAP;
    req_buffers.count = v4lBufferCount;
    if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_REQBUFS, &req_buffers)) < 0) {
        ALOGE("%s: VIDIOC_REQBUFS failed: %s", __FUNCTION__, strerror(errno));
        return -errno;
    }

    // Driver can indeed return more buffer if it needs more to operate
    if (req_buffers.count < v4lBufferCount) {
        ALOGE("%s: VIDIOC_REQBUFS expected %d buffers, got %d instead",
                __FUNCTION__, v4lBufferCount, req_buffers.count);
        return NO_MEMORY;
    }

    // VIDIOC_EXPBUF:  export buffers as FD
    // VIDIOC_QBUF: send buffer to driver
    mV4l2Buffers.resize(req_buffers.count);
    for (uint32_t i = 0; i < req_buffers.count; i++) {
        v4l2_exportbuffer expbuf {};
        expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        expbuf.index = i;
        if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_EXPBUF, &expbuf)) < 0) {
            ALOGE("%s: EXPBUF %d failed: %s", __FUNCTION__, i,  strerror(errno));
            return -errno;
        }
        mV4l2Buffers[i].reset(expbuf.fd);

        v4l2_buffer buffer = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .index = i,
            .memory = V4L2_MEMORY_MMAP};

        if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_QBUF, &buffer)) < 0) {
            ALOGE("%s: QBUF %d failed: %s", __FUNCTION__, i,  strerror(errno));
            return -errno;
        }
    }

    // VIDIOC_STREAMON: start streaming
    v4l2_buf_type capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_STREAMON, &capture_type)) < 0) {
        ALOGE("%s: VIDIOC_STREAMON failed: %s", __FUNCTION__, strerror(errno));
        return -errno;
    }

    // Swallow first few frames after streamOn to account for bad frames from some devices
    for (int i = 0; i < kBadFramesAfterStreamOn; i++) {
        v4l2_buffer buffer{};
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_DQBUF, &buffer)) < 0) {
            ALOGE("%s: DQBUF fails: %s", __FUNCTION__, strerror(errno));
            return -errno;
        }

        if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_QBUF, &buffer)) < 0) {
            ALOGE("%s: QBUF index %d fails: %s", __FUNCTION__, buffer.index, strerror(errno));
            return -errno;
        }
    }

    mV4l2StreamingFmt = v4l2Fmt;
    mV4l2Streaming = true;
    return OK;
}

sp<V4L2Frame> ExternalCameraDeviceSession::dequeueV4l2FrameLocked() {
    sp<V4L2Frame> ret = nullptr;

    {
        std::unique_lock<std::mutex> lk(mV4l2BufferLock);
        if (mNumDequeuedV4l2Buffers == mV4l2Buffers.size()) {
            std::chrono::seconds timeout = std::chrono::seconds(kBufferWaitTimeoutSec);
            mLock.unlock();
            auto st = mV4L2BufferReturned.wait_for(lk, timeout);
            mLock.lock();
            if (st == std::cv_status::timeout) {
                ALOGE("%s: wait for V4L2 buffer return timeout!", __FUNCTION__);
                return ret;
            }
        }
    }

    v4l2_buffer buffer{};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_DQBUF, &buffer)) < 0) {
        ALOGE("%s: DQBUF fails: %s", __FUNCTION__, strerror(errno));
        return ret;
    }

    if (buffer.index >= mV4l2Buffers.size()) {
        ALOGE("%s: Invalid buffer id: %d", __FUNCTION__, buffer.index);
        return ret;
    }

    if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
        ALOGE("%s: v4l2 buf error! buf flag 0x%x", __FUNCTION__, buffer.flags);
        // TODO: try to dequeue again
    }

    {
        std::lock_guard<std::mutex> lk(mV4l2BufferLock);
        mNumDequeuedV4l2Buffers++;
    }
    return new V4L2Frame(
            mV4l2StreamingFmt.width, mV4l2StreamingFmt.height, mV4l2StreamingFmt.fourcc,
            buffer.index, mV4l2Buffers[buffer.index].get(), buffer.bytesused);
}

void ExternalCameraDeviceSession::enqueueV4l2Frame(const sp<V4L2Frame>& frame) {
    Mutex::Autolock _l(mLock);
    frame->unmap();
    v4l2_buffer buffer{};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = frame->mBufferIndex;
    if (TEMP_FAILURE_RETRY(ioctl(mV4l2Fd.get(), VIDIOC_QBUF, &buffer)) < 0) {
        ALOGE("%s: QBUF index %d fails: %s", __FUNCTION__, frame->mBufferIndex, strerror(errno));
        return;
    }

    {
        std::lock_guard<std::mutex> lk(mV4l2BufferLock);
        mNumDequeuedV4l2Buffers--;
        mV4L2BufferReturned.notify_one();
    }
}

Status ExternalCameraDeviceSession::configureStreams(
        const V3_2::StreamConfiguration& config, V3_3::HalStreamConfiguration* out) {
    if (config.operationMode != StreamConfigurationMode::NORMAL_MODE) {
        ALOGE("%s: unsupported operation mode: %d", __FUNCTION__, config.operationMode);
        return Status::ILLEGAL_ARGUMENT;
    }

    if (config.streams.size() == 0) {
        ALOGE("%s: cannot configure zero stream", __FUNCTION__);
        return Status::ILLEGAL_ARGUMENT;
    }

    int numProcessedStream = 0;
    int numStallStream = 0;
    for (const auto& stream : config.streams) {
        // Check if the format/width/height combo is supported
        if (!isSupported(stream)) {
            return Status::ILLEGAL_ARGUMENT;
        }
        if (stream.format == PixelFormat::BLOB) {
            numStallStream++;
        } else {
            numProcessedStream++;
        }
    }

    if (numProcessedStream > kMaxProcessedStream) {
        ALOGE("%s: too many processed streams (expect <= %d, got %d)", __FUNCTION__,
                kMaxProcessedStream, numProcessedStream);
        return Status::ILLEGAL_ARGUMENT;
    }

    if (numStallStream > kMaxStallStream) {
        ALOGE("%s: too many stall streams (expect <= %d, got %d)", __FUNCTION__,
                kMaxStallStream, numStallStream);
        return Status::ILLEGAL_ARGUMENT;
    }

    Status status = initStatus();
    if (status != Status::OK) {
        return status;
    }

    Mutex::Autolock _l(mLock);
    if (!mInflightFrames.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight frames!",
                __FUNCTION__, mInflightFrames.size());
        return Status::INTERNAL_ERROR;
    }

    // Add new streams
    for (const auto& stream : config.streams) {
        if (mStreamMap.count(stream.id) == 0) {
            mStreamMap[stream.id] = stream;
            mCirculatingBuffers.emplace(stream.id, CirculatingBuffers{});
        }
    }

    // Cleanup removed streams
    for(auto it = mStreamMap.begin(); it != mStreamMap.end();) {
        int id = it->first;
        bool found = false;
        for (const auto& stream : config.streams) {
            if (id == stream.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Unmap all buffers of deleted stream
            cleanupBuffersLocked(id);
            it = mStreamMap.erase(it);
        } else {
            ++it;
        }
    }

    // Now select a V4L2 format to produce all output streams
    float desiredAr = (mCroppingType == VERTICAL) ? kMaxAspectRatio : kMinAspectRatio;
    uint32_t maxDim = 0;
    for (const auto& stream : config.streams) {
        float aspectRatio = ASPECT_RATIO(stream);
        if ((mCroppingType == VERTICAL && aspectRatio < desiredAr) ||
                (mCroppingType == HORIZONTAL && aspectRatio > desiredAr)) {
            desiredAr = aspectRatio;
        }

        // The dimension that's not cropped
        uint32_t dim = (mCroppingType == VERTICAL) ? stream.width : stream.height;
        if (dim > maxDim) {
            maxDim = dim;
        }
    }
    // Find the smallest format that matches the desired aspect ratio and is wide/high enough
    SupportedV4L2Format v4l2Fmt {.width = 0, .height = 0};
    for (const auto& fmt : mSupportedFormats) {
        uint32_t dim = (mCroppingType == VERTICAL) ? fmt.width : fmt.height;
        if (dim >= maxDim) {
            float aspectRatio = ASPECT_RATIO(fmt);
            if (isAspectRatioClose(aspectRatio, desiredAr)) {
                v4l2Fmt = fmt;
                // since mSupportedFormats is sorted by width then height, the first matching fmt
                // will be the smallest one with matching aspect ratio
                break;
            }
        }
    }
    if (v4l2Fmt.width == 0) {
        // Cannot find exact good aspect ratio candidate, try to find a close one
        for (const auto& fmt : mSupportedFormats) {
            uint32_t dim = (mCroppingType == VERTICAL) ? fmt.width : fmt.height;
            if (dim >= maxDim) {
                float aspectRatio = ASPECT_RATIO(fmt);
                if ((mCroppingType == VERTICAL && aspectRatio < desiredAr) ||
                        (mCroppingType == HORIZONTAL && aspectRatio > desiredAr)) {
                    v4l2Fmt = fmt;
                    break;
                }
            }
        }
    }

    if (v4l2Fmt.width == 0) {
        ALOGE("%s: unable to find a resolution matching (%s at least %d, aspect ratio %f)"
                , __FUNCTION__, (mCroppingType == VERTICAL) ? "width" : "height",
                maxDim, desiredAr);
        return Status::ILLEGAL_ARGUMENT;
    }

    if (configureV4l2StreamLocked(v4l2Fmt) != 0) {
        ALOGE("V4L configuration failed!, format:%c%c%c%c, w %d, h %d",
            v4l2Fmt.fourcc & 0xFF,
            (v4l2Fmt.fourcc >> 8) & 0xFF,
            (v4l2Fmt.fourcc >> 16) & 0xFF,
            (v4l2Fmt.fourcc >> 24) & 0xFF,
            v4l2Fmt.width, v4l2Fmt.height);
        return Status::INTERNAL_ERROR;
    }

    Size v4lSize = {v4l2Fmt.width, v4l2Fmt.height};
    status = mOutputThread->allocateIntermediateBuffers(v4lSize, config.streams);
    if (status != Status::OK) {
        ALOGE("%s: allocating intermediate buffers failed!", __FUNCTION__);
        return status;
    }

    out->streams.resize(config.streams.size());
    for (size_t i = 0; i < config.streams.size(); i++) {
        out->streams[i].overrideDataSpace = config.streams[i].dataSpace;
        out->streams[i].v3_2.id = config.streams[i].id;
        // TODO: double check should we add those CAMERA flags
        mStreamMap[config.streams[i].id].usage =
                out->streams[i].v3_2.producerUsage = config.streams[i].usage |
                BufferUsage::CPU_WRITE_OFTEN |
                BufferUsage::CAMERA_OUTPUT;
        out->streams[i].v3_2.consumerUsage = 0;
        out->streams[i].v3_2.maxBuffers  = mV4l2Buffers.size();

        switch (config.streams[i].format) {
            case PixelFormat::BLOB:
            case PixelFormat::YCBCR_420_888:
            case PixelFormat::YV12: // Used by SurfaceTexture
                // No override
                out->streams[i].v3_2.overrideFormat = config.streams[i].format;
                break;
            case PixelFormat::IMPLEMENTATION_DEFINED:
                // Override based on VIDEO or not
                out->streams[i].v3_2.overrideFormat =
                        (config.streams[i].usage & BufferUsage::VIDEO_ENCODER) ?
                        PixelFormat::YCBCR_420_888 : PixelFormat::YV12;
                // Save overridden formt in mStreamMap
                mStreamMap[config.streams[i].id].format = out->streams[i].v3_2.overrideFormat;
                break;
            default:
                ALOGE("%s: unsupported format 0x%x", __FUNCTION__, config.streams[i].format);
                return Status::ILLEGAL_ARGUMENT;
        }
    }

    mFirstRequest = true;
    return Status::OK;
}

bool ExternalCameraDeviceSession::isClosed() {
    Mutex::Autolock _l(mLock);
    return mClosed;
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define UPDATE(md, tag, data, size)               \
do {                                              \
    if ((md).update((tag), (data), (size))) {     \
        ALOGE("Update " #tag " failed!");         \
        return BAD_VALUE;                         \
    }                                             \
} while (0)

status_t ExternalCameraDeviceSession::initDefaultRequests() {
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata md;

    const uint8_t aberrationMode = ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF;
    UPDATE(md, ANDROID_COLOR_CORRECTION_ABERRATION_MODE, &aberrationMode, 1);

    const int32_t exposureCompensation = 0;
    UPDATE(md, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &exposureCompensation, 1);

    const uint8_t videoStabilizationMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    UPDATE(md, ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &videoStabilizationMode, 1);

    const uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    UPDATE(md, ANDROID_CONTROL_AWB_MODE, &awbMode, 1);

    const uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    UPDATE(md, ANDROID_CONTROL_AE_MODE, &aeMode, 1);

    const uint8_t aePrecaptureTrigger = ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
    UPDATE(md, ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, &aePrecaptureTrigger, 1);

    const uint8_t afMode = ANDROID_CONTROL_AF_MODE_AUTO;
    UPDATE(md, ANDROID_CONTROL_AF_MODE, &afMode, 1);

    const uint8_t afTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    UPDATE(md, ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);

    const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    UPDATE(md, ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    const uint8_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    UPDATE(md, ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);

    const uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    UPDATE(md, ANDROID_FLASH_MODE, &flashMode, 1);

    const int32_t thumbnailSize[] = {240, 180};
    UPDATE(md, ANDROID_JPEG_THUMBNAIL_SIZE, thumbnailSize, 2);

    const uint8_t jpegQuality = 90;
    UPDATE(md, ANDROID_JPEG_QUALITY, &jpegQuality, 1);
    UPDATE(md, ANDROID_JPEG_THUMBNAIL_QUALITY, &jpegQuality, 1);

    const int32_t jpegOrientation = 0;
    UPDATE(md, ANDROID_JPEG_ORIENTATION, &jpegOrientation, 1);

    const uint8_t oisMode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    UPDATE(md, ANDROID_LENS_OPTICAL_STABILIZATION_MODE, &oisMode, 1);

    const uint8_t nrMode = ANDROID_NOISE_REDUCTION_MODE_OFF;
    UPDATE(md, ANDROID_NOISE_REDUCTION_MODE, &nrMode, 1);

    const uint8_t fdMode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    UPDATE(md, ANDROID_STATISTICS_FACE_DETECT_MODE, &fdMode, 1);

    const uint8_t hotpixelMode = ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF;
    UPDATE(md, ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE, &hotpixelMode, 1);

    bool support30Fps = false;
    int32_t maxFps = std::numeric_limits<int32_t>::min();
    for (const auto& supportedFormat : mSupportedFormats) {
        for (const auto& frameRate : supportedFormat.frameRates) {
            int32_t framerateInt = static_cast<int32_t>(frameRate);
            if (maxFps < framerateInt) {
                maxFps = framerateInt;
            }
            if (framerateInt == 30) {
                support30Fps = true;
                break;
            }
        }
        if (support30Fps) {
            break;
        }
    }
    int32_t defaultFramerate = support30Fps ? 30 : maxFps;
    int32_t defaultFpsRange[] = {defaultFramerate, defaultFramerate};
    UPDATE(md, ANDROID_CONTROL_AE_TARGET_FPS_RANGE, defaultFpsRange, ARRAY_SIZE(defaultFpsRange));

    uint8_t antibandingMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    UPDATE(md, ANDROID_CONTROL_AE_ANTIBANDING_MODE, &antibandingMode, 1);

    const uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    UPDATE(md, ANDROID_CONTROL_MODE, &controlMode, 1);

    auto requestTemplates = hidl_enum_iterator<RequestTemplate>();
    for (RequestTemplate type : requestTemplates) {
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata mdCopy = md;
        uint8_t intent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        switch (type) {
            case RequestTemplate::PREVIEW:
                intent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
                break;
            case RequestTemplate::STILL_CAPTURE:
                intent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
                break;
            case RequestTemplate::VIDEO_RECORD:
                intent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
                break;
            case RequestTemplate::VIDEO_SNAPSHOT:
                intent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
                break;
            default:
                ALOGV("%s: unsupported RequestTemplate type %d", __FUNCTION__, type);
                continue;
        }
        UPDATE(mdCopy, ANDROID_CONTROL_CAPTURE_INTENT, &intent, 1);

        camera_metadata_t* rawMd = mdCopy.release();
        CameraMetadata hidlMd;
        hidlMd.setToExternal(
                (uint8_t*) rawMd, get_camera_metadata_size(rawMd));
        mDefaultRequests[type] = hidlMd;
        free_camera_metadata(rawMd);
    }

    return OK;
}

status_t ExternalCameraDeviceSession::fillCaptureResult(
        common::V1_0::helper::CameraMetadata &md, nsecs_t timestamp) {
    // android.control
    // For USB camera, we don't know the AE state. Set the state to converged to
    // indicate the frame should be good to use. Then apps don't have to wait the
    // AE state.
    const uint8_t aeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
    UPDATE(md, ANDROID_CONTROL_AE_STATE, &aeState, 1);

    const uint8_t ae_lock = ANDROID_CONTROL_AE_LOCK_OFF;
    UPDATE(md, ANDROID_CONTROL_AE_LOCK, &ae_lock, 1);

    bool afTrigger = mAfTrigger;
    if (md.exists(ANDROID_CONTROL_AF_TRIGGER)) {
        Mutex::Autolock _l(mLock);
        camera_metadata_entry entry = md.find(ANDROID_CONTROL_AF_TRIGGER);
        if (entry.data.u8[0] == ANDROID_CONTROL_AF_TRIGGER_START) {
            mAfTrigger = afTrigger = true;
        } else if (entry.data.u8[0] == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
            mAfTrigger = afTrigger = false;
        }
    }

    // For USB camera, the USB camera handles everything and we don't have control
    // over AF. We only simply fake the AF metadata based on the request
    // received here.
    uint8_t afState;
    if (afTrigger) {
        afState = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
    } else {
        afState = ANDROID_CONTROL_AF_STATE_INACTIVE;
    }
    UPDATE(md, ANDROID_CONTROL_AF_STATE, &afState, 1);

    // Set AWB state to converged to indicate the frame should be good to use.
    const uint8_t awbState = ANDROID_CONTROL_AWB_STATE_CONVERGED;
    UPDATE(md, ANDROID_CONTROL_AWB_STATE, &awbState, 1);

    const uint8_t awbLock = ANDROID_CONTROL_AWB_LOCK_OFF;
    UPDATE(md, ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);

    camera_metadata_ro_entry active_array_size =
        mCameraCharacteristics.find(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);

    if (active_array_size.count == 0) {
        ALOGE("%s: cannot find active array size!", __FUNCTION__);
        return -EINVAL;
    }

    const uint8_t flashState = ANDROID_FLASH_STATE_UNAVAILABLE;
    UPDATE(md, ANDROID_FLASH_STATE, &flashState, 1);

    // android.scaler
    const int32_t crop_region[] = {
          active_array_size.data.i32[0], active_array_size.data.i32[1],
          active_array_size.data.i32[2], active_array_size.data.i32[3],
    };
    UPDATE(md, ANDROID_SCALER_CROP_REGION, crop_region, ARRAY_SIZE(crop_region));

    // android.sensor
    UPDATE(md, ANDROID_SENSOR_TIMESTAMP, &timestamp, 1);

    // android.statistics
    const uint8_t lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
    UPDATE(md, ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, &lensShadingMapMode, 1);

    const uint8_t sceneFlicker = ANDROID_STATISTICS_SCENE_FLICKER_NONE;
    UPDATE(md, ANDROID_STATISTICS_SCENE_FLICKER, &sceneFlicker, 1);

    return OK;
}

#undef ARRAY_SIZE
#undef UPDATE

V4L2Frame::V4L2Frame(
        uint32_t w, uint32_t h, uint32_t fourcc,
        int bufIdx, int fd, uint32_t dataSize) :
        mWidth(w), mHeight(h), mFourcc(fourcc),
        mBufferIndex(bufIdx), mFd(fd), mDataSize(dataSize) {}

int V4L2Frame::map(uint8_t** data, size_t* dataSize) {
    if (data == nullptr || dataSize == nullptr) {
        ALOGI("%s: V4L2 buffer map bad argument: data %p, dataSize %p",
                __FUNCTION__, data, dataSize);
        return -EINVAL;
    }

    Mutex::Autolock _l(mLock);
    if (!mMapped) {
        void* addr = mmap(NULL, mDataSize, PROT_READ, MAP_SHARED, mFd, 0);
        if (addr == MAP_FAILED) {
            ALOGE("%s: V4L2 buffer map failed: %s", __FUNCTION__, strerror(errno));
            return -EINVAL;
        }
        mData = static_cast<uint8_t*>(addr);
        mMapped = true;
    }
    *data = mData;
    *dataSize = mDataSize;
    ALOGV("%s: V4L map FD %d, data %p size %zu", __FUNCTION__, mFd, mData, mDataSize);
    return 0;
}

int V4L2Frame::unmap() {
    Mutex::Autolock _l(mLock);
    if (mMapped) {
        ALOGV("%s: V4L unmap data %p size %zu", __FUNCTION__, mData, mDataSize);
        if (munmap(mData, mDataSize) != 0) {
            ALOGE("%s: V4L2 buffer unmap failed: %s", __FUNCTION__, strerror(errno));
            return -EINVAL;
        }
        mMapped = false;
    }
    return 0;
}

V4L2Frame::~V4L2Frame() {
    unmap();
}

AllocatedFrame::AllocatedFrame(
        uint32_t w, uint32_t h) :
        mWidth(w), mHeight(h), mFourcc(V4L2_PIX_FMT_YUV420) {};

AllocatedFrame::~AllocatedFrame() {}

int AllocatedFrame::allocate(YCbCrLayout* out) {
    if ((mWidth % 2) || (mHeight % 2)) {
        ALOGE("%s: bad dimension %dx%d (not multiple of 2)", __FUNCTION__, mWidth, mHeight);
        return -EINVAL;
    }

    uint32_t dataSize = mWidth * mHeight * 3 / 2; // YUV420
    if (mData.size() != dataSize) {
        mData.resize(dataSize);
    }

    if (out != nullptr) {
        out->y = mData.data();
        out->yStride = mWidth;
        uint8_t* cbStart = mData.data() + mWidth * mHeight;
        uint8_t* crStart = cbStart + mWidth * mHeight / 4;
        out->cb = cbStart;
        out->cr = crStart;
        out->cStride = mWidth / 2;
        out->chromaStep = 1;
    }
    return 0;
}

int AllocatedFrame::getLayout(YCbCrLayout* out) {
    IMapper::Rect noCrop = {0, 0,
            static_cast<int32_t>(mWidth),
            static_cast<int32_t>(mHeight)};
    return getCroppedLayout(noCrop, out);
}

int AllocatedFrame::getCroppedLayout(const IMapper::Rect& rect, YCbCrLayout* out) {
    if (out == nullptr) {
        ALOGE("%s: null out", __FUNCTION__);
        return -1;
    }
    if ((rect.left + rect.width) > static_cast<int>(mWidth) ||
        (rect.top + rect.height) > static_cast<int>(mHeight) ||
            (rect.left % 2) || (rect.top % 2) || (rect.width % 2) || (rect.height % 2)) {
        ALOGE("%s: bad rect left %d top %d w %d h %d", __FUNCTION__,
                rect.left, rect.top, rect.width, rect.height);
        return -1;
    }

    out->y = mData.data() + mWidth * rect.top + rect.left;
    out->yStride = mWidth;
    uint8_t* cbStart = mData.data() + mWidth * mHeight;
    uint8_t* crStart = cbStart + mWidth * mHeight / 4;
    out->cb = cbStart + mWidth * rect.top / 4 + rect.left / 2;
    out->cr = crStart + mWidth * rect.top / 4 + rect.left / 2;
    out->cStride = mWidth / 2;
    out->chromaStep = 1;
    return 0;
}

}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
