/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "ExtCamOfflnSsn"
#include <android/log.h>

#include "ExternalCameraOfflineSession.h"

#include <aidl/android/hardware/camera/device/BufferStatus.h>
#include <aidl/android/hardware/camera/device/ErrorMsg.h>
#include <aidl/android/hardware/camera/device/ShutterMsg.h>
#include <aidl/android/hardware/camera/device/StreamBuffer.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <convert.h>
#include <linux/videodev2.h>
#include <sync/sync.h>
#include <utils/Trace.h>

#define HAVE_JPEG  // required for libyuv.h to export MJPEG decode APIs
#include <libyuv.h>

namespace {

// Size of request/result metadata fast message queue. Change to 0 to always use hwbinder buffer.
constexpr size_t kMetadataMsgQueueSize = 1 << 18 /* 256kB */;

}  // anonymous namespace

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {

using ::aidl::android::hardware::camera::device::BufferStatus;
using ::aidl::android::hardware::camera::device::ErrorMsg;
using ::aidl::android::hardware::camera::device::ShutterMsg;
using ::aidl::android::hardware::camera::device::StreamBuffer;

// Static instance
HandleImporter ExternalCameraOfflineSession::sHandleImporter;

ExternalCameraOfflineSession::ExternalCameraOfflineSession(
        const CroppingType& croppingType, const common::V1_0::helper::CameraMetadata& chars,
        const std::string& cameraId, const std::string& exifMake, const std::string& exifModel,
        uint32_t blobBufferSize, bool afTrigger, const std::vector<Stream>& offlineStreams,
        std::deque<std::shared_ptr<HalRequest>>& offlineReqs,
        const std::map<int, CirculatingBuffers>& circulatingBuffers)
    : mCroppingType(croppingType),
      mChars(chars),
      mCameraId(cameraId),
      mExifMake(exifMake),
      mExifModel(exifModel),
      mBlobBufferSize(blobBufferSize),
      mAfTrigger(afTrigger),
      mOfflineStreams(offlineStreams),
      mOfflineReqs(offlineReqs),
      mCirculatingBuffers(circulatingBuffers) {}

ExternalCameraOfflineSession::~ExternalCameraOfflineSession() {
    close();
}

bool ExternalCameraOfflineSession::initialize() {
    mResultMetadataQueue =
            std::make_shared<ResultMetadataQueue>(kMetadataMsgQueueSize, false /* non blocking */);
    if (!mResultMetadataQueue->isValid()) {
        ALOGE("%s: invalid result fmq", __FUNCTION__);
        return true;
    }
    return false;
}

Status ExternalCameraOfflineSession::importBuffer(int32_t streamId, uint64_t bufId,
                                                  buffer_handle_t buf,
                                                  buffer_handle_t** outBufPtr) {
    Mutex::Autolock _l(mCbsLock);
    return importBufferImpl(mCirculatingBuffers, sHandleImporter, streamId, bufId, buf, outBufPtr);
}

Status ExternalCameraOfflineSession::processCaptureResult(std::shared_ptr<HalRequest>& req) {
    ATRACE_CALL();
    // Fill output buffers
    std::vector<CaptureResult> results;
    results.resize(1);
    CaptureResult& result = results[0];
    result.frameNumber = req->frameNumber;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    result.outputBuffers.resize(req->buffers.size());
    for (size_t i = 0; i < req->buffers.size(); i++) {
        StreamBuffer& outputBuffer = result.outputBuffers[i];
        outputBuffer.streamId = req->buffers[i].streamId;
        outputBuffer.bufferId = req->buffers[i].bufferId;
        if (req->buffers[i].fenceTimeout) {
            outputBuffer.status = BufferStatus::ERROR;
            if (req->buffers[i].acquireFence >= 0) {
                native_handle_t* handle = native_handle_create(/*numFds*/ 1, /*numInts*/ 0);
                handle->data[0] = req->buffers[i].acquireFence;
                result.outputBuffers[i].releaseFence = android::makeToAidl(handle);
            }
            notifyError(req->frameNumber, req->buffers[i].streamId, ErrorCode::ERROR_BUFFER);
        } else {
            result.outputBuffers[i].status = BufferStatus::OK;
            // TODO: refactor
            if (req->buffers[i].acquireFence >= 0) {
                native_handle_t* handle = native_handle_create(/*numFds*/ 1, /*numInts*/ 0);
                handle->data[0] = req->buffers[i].acquireFence;
                outputBuffer.releaseFence = android::makeToAidl(handle);
            }
        }
    }

    // Fill capture result metadata
    fillCaptureResult(req->setting, req->shutterTs);
    const camera_metadata_t* rawResult = req->setting.getAndLock();
    convertToAidl(rawResult, &result.result);
    req->setting.unlock(rawResult);

    // Callback into framework
    invokeProcessCaptureResultCallback(results, /* tryWriteFmq */ true);
    freeReleaseFences(results);
    return Status::OK;
}

#define UPDATE(md, tag, data, size)               \
    do {                                          \
        if ((md).update((tag), (data), (size))) { \
            ALOGE("Update " #tag " failed!");     \
            return BAD_VALUE;                     \
        }                                         \
    } while (0)

status_t ExternalCameraOfflineSession::fillCaptureResult(common::V1_0::helper::CameraMetadata md,
                                                         nsecs_t timestamp) {
    bool afTrigger = false;
    {
        std::lock_guard<std::mutex> lk(mAfTriggerLock);
        afTrigger = mAfTrigger;
        if (md.exists(ANDROID_CONTROL_AF_TRIGGER)) {
            camera_metadata_entry entry = md.find(ANDROID_CONTROL_AF_TRIGGER);
            if (entry.data.u8[0] == ANDROID_CONTROL_AF_TRIGGER_START) {
                mAfTrigger = afTrigger = true;
            } else if (entry.data.u8[0] == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
                mAfTrigger = afTrigger = false;
            }
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

    camera_metadata_ro_entry activeArraySize = mChars.find(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);

    return fillCaptureResultCommon(md, timestamp, activeArraySize);
}
void ExternalCameraOfflineSession::invokeProcessCaptureResultCallback(
        std::vector<CaptureResult>& results, bool tryWriteFmq) {
    if (mProcessCaptureResultLock.tryLock() != OK) {
        const nsecs_t NS_TO_SECOND = 1E9;
        ALOGV("%s: previous call is not finished! waiting 1s...", __FUNCTION__);
        if (mProcessCaptureResultLock.timedLock(/* 1s */ NS_TO_SECOND) != OK) {
            ALOGE("%s: cannot acquire lock in 1s, cannot proceed", __FUNCTION__);
            return;
        }
    }
    if (tryWriteFmq && mResultMetadataQueue->availableToWrite() > 0) {
        for (CaptureResult& result : results) {
            if (!result.result.metadata.empty()) {
                if (mResultMetadataQueue->write(
                            reinterpret_cast<int8_t*>(result.result.metadata.data()),
                            result.result.metadata.size())) {
                    result.fmqResultSize = result.result.metadata.size();
                    result.result.metadata.clear();
                } else {
                    ALOGW("%s: couldn't utilize fmq, fall back to hwbinder", __FUNCTION__);
                    result.fmqResultSize = 0;
                }
            } else {
                result.fmqResultSize = 0;
            }
        }
    }
    auto status = mCallback->processCaptureResult(results);
    if (!status.isOk()) {
        ALOGE("%s: processCaptureResult ERROR : %d:%d", __FUNCTION__, status.getExceptionCode(),
              status.getServiceSpecificError());
    }

    mProcessCaptureResultLock.unlock();
}

Status ExternalCameraOfflineSession::processCaptureRequestError(
        const std::shared_ptr<HalRequest>& req, std::vector<NotifyMsg>* outMsgs,
        std::vector<CaptureResult>* outResults) {
    ATRACE_CALL();

    if (outMsgs == nullptr) {
        notifyError(/*frameNum*/ req->frameNumber, /*stream*/ -1, ErrorCode::ERROR_REQUEST);
    } else {
        NotifyMsg shutter;
        shutter.set<NotifyMsg::Tag::shutter>(ShutterMsg{
                .frameNumber = req->frameNumber,
                .timestamp = req->shutterTs,
        });

        NotifyMsg error;
        error.set<NotifyMsg::Tag::error>(ErrorMsg{.frameNumber = req->frameNumber,
                                                  .errorStreamId = -1,
                                                  .errorCode = ErrorCode::ERROR_REQUEST});
        outMsgs->push_back(shutter);
        outMsgs->push_back(error);
    }

    // Fill output buffers
    CaptureResult result;
    result.frameNumber = req->frameNumber;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    result.outputBuffers.resize(req->buffers.size());
    for (size_t i = 0; i < req->buffers.size(); i++) {
        StreamBuffer& outputBuffer = result.outputBuffers[i];
        outputBuffer.streamId = req->buffers[i].streamId;
        outputBuffer.bufferId = req->buffers[i].bufferId;
        outputBuffer.status = BufferStatus::ERROR;
        if (req->buffers[i].acquireFence >= 0) {
            native_handle_t* handle = native_handle_create(/*numFds*/ 1, /*numInts*/ 0);
            handle->data[0] = req->buffers[i].acquireFence;
            outputBuffer.releaseFence = makeToAidl(handle);
        }
    }

    if (outResults == nullptr) {
        // Callback into framework
        std::vector<CaptureResult> results(1);
        results[0] = std::move(result);
        invokeProcessCaptureResultCallback(results, /* tryWriteFmq */ true);
        freeReleaseFences(results);
    } else {
        outResults->push_back(std::move(result));
    }
    return Status::OK;
}

ssize_t ExternalCameraOfflineSession::getJpegBufferSize(int32_t, int32_t) const {
    // Empty implementation here as the jpeg buffer size is passed in by ctor
    return 0;
}

void ExternalCameraOfflineSession::notifyError(int32_t frameNumber, int32_t streamId,
                                               ErrorCode ec) {
    NotifyMsg msg;
    msg.set<NotifyMsg::Tag::error>(
            ErrorMsg{.frameNumber = frameNumber, .errorStreamId = streamId, .errorCode = ec});
    mCallback->notify({msg});
}

ScopedAStatus ExternalCameraOfflineSession::setCallback(
        const std::shared_ptr<ICameraDeviceCallback>& in_cb) {
    Mutex::Autolock _il(mInterfaceLock);
    if (mCallback != nullptr && in_cb != nullptr) {
        ALOGE("%s: callback must not be set twice!", __FUNCTION__);
        return fromStatus(Status::OK);
    }
    mCallback = in_cb;

    initOutputThread();

    if (mOutputThread == nullptr) {
        ALOGE("%s: init OutputThread failed!", __FUNCTION__);
    }
    return fromStatus(Status::OK);
}
void ExternalCameraOfflineSession::initOutputThread() {
    if (mOutputThread != nullptr) {
        ALOGE("%s: OutputThread already exist!", __FUNCTION__);
        return;
    }

    // Grab a shared_ptr to 'this' from ndk::SharedRefBase::ref()
    std::shared_ptr<ExternalCameraOfflineSession> thiz = ref<ExternalCameraOfflineSession>();

    mBufferRequestThread = std::make_shared<ExternalCameraDeviceSession::BufferRequestThread>(
            /*parent=*/thiz, mCallback);
    mBufferRequestThread->run();

    mOutputThread = std::make_shared<OutputThread>(/*parent=*/thiz, mCroppingType, mChars,
                                                   mBufferRequestThread, mOfflineReqs);

    mOutputThread->setExifMakeModel(mExifMake, mExifModel);

    Size inputSize = {mOfflineReqs[0]->frameIn->mWidth, mOfflineReqs[0]->frameIn->mHeight};
    Size maxThumbSize = getMaxThumbnailResolution(mChars);
    mOutputThread->allocateIntermediateBuffers(inputSize, maxThumbSize, mOfflineStreams,
                                               mBlobBufferSize);

    mOutputThread->run();
}

ScopedAStatus ExternalCameraOfflineSession::getCaptureResultMetadataQueue(
        MQDescriptor<int8_t, SynchronizedReadWrite>* _aidl_return) {
    Mutex::Autolock _il(mInterfaceLock);
    *_aidl_return = mResultMetadataQueue->dupeDesc();
    return fromStatus(Status::OK);
}

ScopedAStatus ExternalCameraOfflineSession::close() {
    Mutex::Autolock _il(mInterfaceLock);
    {
        Mutex::Autolock _l(mLock);
        if (mClosed) {
            ALOGW("%s: offline session already closed!", __FUNCTION__);
            return fromStatus(Status::OK);
        }
    }
    if (mBufferRequestThread != nullptr) {
        mBufferRequestThread->requestExitAndWait();
        mBufferRequestThread.reset();
    }
    if (mOutputThread) {
        mOutputThread->flush();
        mOutputThread->requestExitAndWait();
        mOutputThread.reset();
    }

    Mutex::Autolock _l(mLock);
    // free all buffers
    {
        Mutex::Autolock _cbl(mCbsLock);
        for (auto& stream : mOfflineStreams) {
            cleanupBuffersLocked(stream.id);
        }
    }
    mCallback.reset();
    mClosed = true;
    return fromStatus(Status::OK);
}
void ExternalCameraOfflineSession::cleanupBuffersLocked(int32_t id) {
    for (auto& pair : mCirculatingBuffers.at(id)) {
        sHandleImporter.freeBuffer(pair.second);
    }
    mCirculatingBuffers[id].clear();
    mCirculatingBuffers.erase(id);
}

bool ExternalCameraOfflineSession::OutputThread::threadLoop() {
    auto parent = mParent.lock();
    if (parent == nullptr) {
        ALOGE("%s: session has been disconnected!", __FUNCTION__);
        return false;
    }

    if (mOfflineReqs.empty()) {
        ALOGI("%s: all offline requests are processed. Stopping.", __FUNCTION__);
        return false;
    }

    std::shared_ptr<HalRequest> req = mOfflineReqs.front();
    mOfflineReqs.pop_front();

    auto onDeviceError = [&](auto... args) {
        ALOGE(args...);
        parent->notifyError(req->frameNumber, /*stream*/ -1, ErrorCode::ERROR_DEVICE);
        signalRequestDone();
        return false;
    };

    if (req->frameIn->mFourcc != V4L2_PIX_FMT_MJPEG && req->frameIn->mFourcc != V4L2_PIX_FMT_Z16) {
        return onDeviceError("%s: do not support V4L2 format %c%c%c%c", __FUNCTION__,
                             req->frameIn->mFourcc & 0xFF, (req->frameIn->mFourcc >> 8) & 0xFF,
                             (req->frameIn->mFourcc >> 16) & 0xFF,
                             (req->frameIn->mFourcc >> 24) & 0xFF);
    }

    int res = requestBufferStart(req->buffers);
    if (res != 0) {
        ALOGE("%s: send BufferRequest failed! res %d", __FUNCTION__, res);
        return onDeviceError("%s: failed to send buffer request!", __FUNCTION__);
    }

    std::unique_lock<std::mutex> lk(mBufferLock);
    // Convert input V4L2 frame to YU12 of the same size
    // TODO: see if we can save some computation by converting to YV12 here
    uint8_t* inData;
    size_t inDataSize;
    if (req->frameIn->getData(&inData, &inDataSize) != 0) {
        lk.unlock();
        return onDeviceError("%s: V4L2 buffer map failed", __FUNCTION__);
    }

    // TODO: in some special case maybe we can decode jpg directly to gralloc output?
    if (req->frameIn->mFourcc == V4L2_PIX_FMT_MJPEG) {
        ATRACE_BEGIN("MJPGtoI420");
        int convRes = libyuv::MJPGToI420(
                inData, inDataSize, static_cast<uint8_t*>(mYu12FrameLayout.y),
                mYu12FrameLayout.yStride, static_cast<uint8_t*>(mYu12FrameLayout.cb),
                mYu12FrameLayout.cStride, static_cast<uint8_t*>(mYu12FrameLayout.cr),
                mYu12FrameLayout.cStride, mYu12Frame->mWidth, mYu12Frame->mHeight,
                mYu12Frame->mWidth, mYu12Frame->mHeight);
        ATRACE_END();

        if (convRes != 0) {
            // For some webcam, the first few V4L2 frames might be malformed...
            ALOGE("%s: Convert V4L2 frame to YU12 failed! res %d", __FUNCTION__, convRes);
            lk.unlock();
            Status st = parent->processCaptureRequestError(req);
            if (st != Status::OK) {
                return onDeviceError("%s: failed to process capture request error!", __FUNCTION__);
            }
            signalRequestDone();
            return true;
        }
    }

    ATRACE_BEGIN("Wait for BufferRequest done");
    res = waitForBufferRequestDone(&req->buffers);
    ATRACE_END();

    if (res != 0) {
        ALOGE("%s: wait for BufferRequest done failed! res %d", __FUNCTION__, res);
        lk.unlock();
        return onDeviceError("%s: failed to process buffer request error!", __FUNCTION__);
    }

    ALOGV("%s processing new request", __FUNCTION__);
    const int kSyncWaitTimeoutMs = 500;
    for (auto& halBuf : req->buffers) {
        if (*(halBuf.bufPtr) == nullptr) {
            ALOGW("%s: buffer for stream %d missing", __FUNCTION__, halBuf.streamId);
            halBuf.fenceTimeout = true;
        } else if (halBuf.acquireFence >= 0) {
            int ret = sync_wait(halBuf.acquireFence, kSyncWaitTimeoutMs);
            if (ret) {
                halBuf.fenceTimeout = true;
            } else {
                ::close(halBuf.acquireFence);
                halBuf.acquireFence = -1;
            }
        }

        if (halBuf.fenceTimeout) {
            continue;
        }

        // Gralloc lockYCbCr the buffer
        switch (halBuf.format) {
            case PixelFormat::BLOB: {
                int ret = createJpegLocked(halBuf, req->setting);

                if (ret != 0) {
                    lk.unlock();
                    return onDeviceError("%s: createJpegLocked failed with %d", __FUNCTION__, ret);
                }
            } break;
            case PixelFormat::Y16: {
                void* outLayout = sHandleImporter.lock(
                        *(halBuf.bufPtr), static_cast<uint64_t>(halBuf.usage), inDataSize);

                std::memcpy(outLayout, inData, inDataSize);

                int relFence = sHandleImporter.unlock(*(halBuf.bufPtr));
                if (relFence >= 0) {
                    halBuf.acquireFence = relFence;
                }
            } break;
            case PixelFormat::YCBCR_420_888:
            case PixelFormat::YV12: {
                android::Rect outRect{0, 0, static_cast<int32_t>(halBuf.width),
                                      static_cast<int32_t>(halBuf.height)};
                android_ycbcr result = sHandleImporter.lockYCbCr(
                        *(halBuf.bufPtr), static_cast<uint64_t>(halBuf.usage), outRect);
                ALOGV("%s: outLayout y %p cb %p cr %p y_str %zu c_str %zu c_step %zu", __FUNCTION__,
                      result.y, result.cb, result.cr, result.ystride, result.cstride,
                      result.chroma_step);
                if (result.ystride > UINT32_MAX || result.cstride > UINT32_MAX ||
                    result.chroma_step > UINT32_MAX) {
                    return onDeviceError("%s: lockYCbCr failed. Unexpected values!", __FUNCTION__);
                }
                YCbCrLayout outLayout = {.y = result.y,
                                         .cb = result.cb,
                                         .cr = result.cr,
                                         .yStride = static_cast<uint32_t>(result.ystride),
                                         .cStride = static_cast<uint32_t>(result.cstride),
                                         .chromaStep = static_cast<uint32_t>(result.chroma_step)};

                // Convert to output buffer size/format
                uint32_t outputFourcc = getFourCcFromLayout(outLayout);
                ALOGV("%s: converting to format %c%c%c%c", __FUNCTION__, outputFourcc & 0xFF,
                      (outputFourcc >> 8) & 0xFF, (outputFourcc >> 16) & 0xFF,
                      (outputFourcc >> 24) & 0xFF);

                YCbCrLayout cropAndScaled;
                ATRACE_BEGIN("cropAndScaleLocked");
                int ret = cropAndScaleLocked(mYu12Frame, Size{halBuf.width, halBuf.height},
                                             &cropAndScaled);
                ATRACE_END();
                if (ret != 0) {
                    lk.unlock();
                    return onDeviceError("%s: crop and scale failed!", __FUNCTION__);
                }

                Size sz{halBuf.width, halBuf.height};
                ATRACE_BEGIN("formatConvert");
                ret = formatConvert(cropAndScaled, outLayout, sz, outputFourcc);
                ATRACE_END();
                if (ret != 0) {
                    lk.unlock();
                    return onDeviceError("%s: format coversion failed!", __FUNCTION__);
                }
                int relFence = sHandleImporter.unlock(*(halBuf.bufPtr));
                if (relFence >= 0) {
                    halBuf.acquireFence = relFence;
                }
            } break;
            default:
                lk.unlock();
                return onDeviceError("%s: unknown output format %x", __FUNCTION__, halBuf.format);
        }
    }  // for each buffer
    mScaledYu12Frames.clear();

    // Don't hold the lock while calling back to parent
    lk.unlock();
    Status st = parent->processCaptureResult(req);
    if (st != Status::OK) {
        return onDeviceError("%s: failed to process capture result!", __FUNCTION__);
    }
    signalRequestDone();
    return true;
}

}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
