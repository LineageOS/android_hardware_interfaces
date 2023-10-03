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

#define LOG_TAG "ExtCamOfflnSsn@3.6"
#define ATRACE_TAG ATRACE_TAG_CAMERA
#include <android/log.h>

#include <linux/videodev2.h>
#include <sync/sync.h>

#define HAVE_JPEG // required for libyuv.h to export MJPEG decode APIs
#include <libyuv.h>

#include <utils/Trace.h>
#include "ExternalCameraOfflineSession.h"

namespace {

// Size of request/result metadata fast message queue. Change to 0 to always use hwbinder buffer.
static constexpr size_t kMetadataMsgQueueSize = 1 << 18 /* 256kB */;

} // anonymous namespace

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_6 {
namespace implementation {

// static instance
HandleImporter ExternalCameraOfflineSession::sHandleImporter;

using V3_5::implementation::ExternalCameraDeviceSession;

ExternalCameraOfflineSession::ExternalCameraOfflineSession(
        const CroppingType& croppingType,
        const common::V1_0::helper::CameraMetadata& chars,
        const std::string& cameraId,
        const std::string& exifMake,
        const std::string& exifModel,
        const uint32_t blobBufferSize,
        const bool afTrigger,
        const hidl_vec<Stream>& offlineStreams,
        std::deque<std::shared_ptr<HalRequest>>& offlineReqs,
        const std::map<int, CirculatingBuffers>& circulatingBuffers) :
        mCroppingType(croppingType), mChars(chars), mCameraId(cameraId),
        mExifMake(exifMake), mExifModel(exifModel), mBlobBufferSize(blobBufferSize),
        mAfTrigger(afTrigger), mOfflineStreams(offlineStreams), mOfflineReqs(offlineReqs),
        mCirculatingBuffers(circulatingBuffers) {}

ExternalCameraOfflineSession::~ExternalCameraOfflineSession() {
    close();
}

bool ExternalCameraOfflineSession::initialize() {
    mResultMetadataQueue = std::make_shared<ResultMetadataQueue>(
            kMetadataMsgQueueSize, false /* non blocking */);
    if (!mResultMetadataQueue->isValid()) {
        ALOGE("%s: invalid result fmq", __FUNCTION__);
        return true;
    }
    return false;
}

void ExternalCameraOfflineSession::initOutputThread() {
    if (mOutputThread != nullptr) {
        ALOGE("%s: OutputThread already exist!", __FUNCTION__);
        return;
    }

    mBufferRequestThread = new ExternalCameraDeviceSession::BufferRequestThread(
            this, mCallback);
    mBufferRequestThread->run("ExtCamBufReq", PRIORITY_DISPLAY);

    mOutputThread = new OutputThread(this, mCroppingType, mChars,
            mBufferRequestThread, mOfflineReqs);

    mOutputThread->setExifMakeModel(mExifMake, mExifModel);

    Size inputSize = { mOfflineReqs[0]->frameIn->mWidth, mOfflineReqs[0]->frameIn->mHeight};
    Size maxThumbSize = V3_4::implementation::getMaxThumbnailResolution(mChars);
    mOutputThread->allocateIntermediateBuffers(
            inputSize, maxThumbSize, mOfflineStreams, mBlobBufferSize);

    mOutputThread->run("ExtCamOfflnOut", PRIORITY_DISPLAY);
}

bool ExternalCameraOfflineSession::OutputThread::threadLoop() {
    auto parent = mParent.promote();
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
        parent->notifyError(
                req->frameNumber, /*stream*/-1, ErrorCode::ERROR_DEVICE);
        signalRequestDone();
        return false;
    };

    if (req->frameIn->mFourcc != V4L2_PIX_FMT_MJPEG && req->frameIn->mFourcc != V4L2_PIX_FMT_Z16) {
        return onDeviceError("%s: do not support V4L2 format %c%c%c%c", __FUNCTION__,
                req->frameIn->mFourcc & 0xFF,
                (req->frameIn->mFourcc >> 8) & 0xFF,
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
        int res = libyuv::MJPGToI420(
            inData, inDataSize, static_cast<uint8_t*>(mYu12FrameLayout.y), mYu12FrameLayout.yStride,
            static_cast<uint8_t*>(mYu12FrameLayout.cb), mYu12FrameLayout.cStride,
            static_cast<uint8_t*>(mYu12FrameLayout.cr), mYu12FrameLayout.cStride,
            mYu12Frame->mWidth, mYu12Frame->mHeight, mYu12Frame->mWidth, mYu12Frame->mHeight);
        ATRACE_END();

        if (res != 0) {
            // For some webcam, the first few V4L2 frames might be malformed...
            ALOGE("%s: Convert V4L2 frame to YU12 failed! res %d", __FUNCTION__, res);
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

                if(ret != 0) {
                    lk.unlock();
                    return onDeviceError("%s: createJpegLocked failed with %d",
                          __FUNCTION__, ret);
                }
            } break;
            case PixelFormat::Y16: {
                void* outLayout = sHandleImporter.lock(*(halBuf.bufPtr), halBuf.usage, inDataSize);

                std::memcpy(outLayout, inData, inDataSize);

                int relFence = sHandleImporter.unlock(*(halBuf.bufPtr));
                if (relFence >= 0) {
                    halBuf.acquireFence = relFence;
                }
            } break;
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
                uint32_t outputFourcc = V3_4::implementation::getFourCcFromLayout(outLayout);
                ALOGV("%s: converting to format %c%c%c%c", __FUNCTION__,
                        outputFourcc & 0xFF,
                        (outputFourcc >> 8) & 0xFF,
                        (outputFourcc >> 16) & 0xFF,
                        (outputFourcc >> 24) & 0xFF);

                YCbCrLayout cropAndScaled;
                ATRACE_BEGIN("cropAndScaleLocked");
                int ret = cropAndScaleLocked(
                        mYu12Frame,
                        Size { halBuf.width, halBuf.height },
                        &cropAndScaled);
                ATRACE_END();
                if (ret != 0) {
                    lk.unlock();
                    return onDeviceError("%s: crop and scale failed!", __FUNCTION__);
                }

                Size sz {halBuf.width, halBuf.height};
                ATRACE_BEGIN("formatConvert");
                ret = V3_4::implementation::formatConvert(cropAndScaled, outLayout, sz, outputFourcc);
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
    } // for each buffer
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

Status ExternalCameraOfflineSession::importBuffer(int32_t streamId,
        uint64_t bufId, buffer_handle_t buf,
        /*out*/buffer_handle_t** outBufPtr,
        bool allowEmptyBuf) {
    Mutex::Autolock _l(mCbsLock);
    return V3_4::implementation::importBufferImpl(
            mCirculatingBuffers, sHandleImporter, streamId,
            bufId, buf, outBufPtr, allowEmptyBuf);
    return Status::OK;
};

#define UPDATE(md, tag, data, size)               \
do {                                              \
    if ((md).update((tag), (data), (size))) {     \
        ALOGE("Update " #tag " failed!");         \
        return BAD_VALUE;                         \
    }                                             \
} while (0)

status_t ExternalCameraOfflineSession::fillCaptureResult(
        common::V1_0::helper::CameraMetadata &md, nsecs_t timestamp) {
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

    camera_metadata_ro_entry activeArraySize =
            mChars.find(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);

    return V3_4::implementation::fillCaptureResultCommon(md, timestamp, activeArraySize);
}

#undef UPDATE

Status ExternalCameraOfflineSession::processCaptureResult(std::shared_ptr<HalRequest>& req) {
    ATRACE_CALL();
    // Fill output buffers
    hidl_vec<CaptureResult> results;
    results.resize(1);
    CaptureResult& result = results[0];
    result.frameNumber = req->frameNumber;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    result.outputBuffers.resize(req->buffers.size());
    for (size_t i = 0; i < req->buffers.size(); i++) {
        result.outputBuffers[i].streamId = req->buffers[i].streamId;
        result.outputBuffers[i].bufferId = req->buffers[i].bufferId;
        if (req->buffers[i].fenceTimeout) {
            result.outputBuffers[i].status = BufferStatus::ERROR;
            if (req->buffers[i].acquireFence >= 0) {
                native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
                handle->data[0] = req->buffers[i].acquireFence;
                result.outputBuffers[i].releaseFence.setTo(handle, /*shouldOwn*/false);
            }
            notifyError(req->frameNumber, req->buffers[i].streamId, ErrorCode::ERROR_BUFFER);
        } else {
            result.outputBuffers[i].status = BufferStatus::OK;
            // TODO: refactor
            if (req->buffers[i].acquireFence >= 0) {
                native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
                handle->data[0] = req->buffers[i].acquireFence;
                result.outputBuffers[i].releaseFence.setTo(handle, /*shouldOwn*/false);
            }
        }
    }

    // Fill capture result metadata
    fillCaptureResult(req->setting, req->shutterTs);
    const camera_metadata_t *rawResult = req->setting.getAndLock();
    V3_2::implementation::convertToHidl(rawResult, &result.result);
    req->setting.unlock(rawResult);

    // Callback into framework
    invokeProcessCaptureResultCallback(results, /* tryWriteFmq */true);
    V3_4::implementation::freeReleaseFences(results);
    return Status::OK;
};

void ExternalCameraOfflineSession::invokeProcessCaptureResultCallback(
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
    auto status = mCallback->processCaptureResult(results);
    if (!status.isOk()) {
        ALOGE("%s: processCaptureResult ERROR : %s", __FUNCTION__,
              status.description().c_str());
    }

    mProcessCaptureResultLock.unlock();
}

Status ExternalCameraOfflineSession::processCaptureRequestError(
        const std::shared_ptr<HalRequest>& req,
        /*out*/std::vector<NotifyMsg>* outMsgs,
        /*out*/std::vector<CaptureResult>* outResults) {
    ATRACE_CALL();

    if (outMsgs == nullptr) {
        notifyError(/*frameNum*/req->frameNumber, /*stream*/-1, ErrorCode::ERROR_REQUEST);
    } else {
        NotifyMsg shutter;
        shutter.type = MsgType::SHUTTER;
        shutter.msg.shutter.frameNumber = req->frameNumber;
        shutter.msg.shutter.timestamp = req->shutterTs;

        NotifyMsg error;
        error.type = MsgType::ERROR;
        error.msg.error.frameNumber = req->frameNumber;
        error.msg.error.errorStreamId = -1;
        error.msg.error.errorCode = ErrorCode::ERROR_REQUEST;
        outMsgs->push_back(shutter);
        outMsgs->push_back(error);
    }

    // Fill output buffers
    hidl_vec<CaptureResult> results;
    results.resize(1);
    CaptureResult& result = results[0];
    result.frameNumber = req->frameNumber;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    result.outputBuffers.resize(req->buffers.size());
    for (size_t i = 0; i < req->buffers.size(); i++) {
        result.outputBuffers[i].streamId = req->buffers[i].streamId;
        result.outputBuffers[i].bufferId = req->buffers[i].bufferId;
        result.outputBuffers[i].status = BufferStatus::ERROR;
        if (req->buffers[i].acquireFence >= 0) {
            native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
            handle->data[0] = req->buffers[i].acquireFence;
            result.outputBuffers[i].releaseFence.setTo(handle, /*shouldOwn*/false);
        }
    }

    if (outResults == nullptr) {
        // Callback into framework
        invokeProcessCaptureResultCallback(results, /* tryWriteFmq */true);
        V3_4::implementation::freeReleaseFences(results);
    } else {
        outResults->push_back(result);
    }
    return Status::OK;
};

ssize_t ExternalCameraOfflineSession::getJpegBufferSize(
        uint32_t /*width*/, uint32_t /*height*/) const {
    // Empty implementation here as the jpeg buffer size is passed in by ctor
    return 0;
};

void ExternalCameraOfflineSession::notifyError(uint32_t frameNumber, int32_t streamId, ErrorCode ec) {
    NotifyMsg msg;
    msg.type = MsgType::ERROR;
    msg.msg.error.frameNumber = frameNumber;
    msg.msg.error.errorStreamId = streamId;
    msg.msg.error.errorCode = ec;
    mCallback->notify({msg});
};

Return<void> ExternalCameraOfflineSession::setCallback(const sp<ICameraDeviceCallback>& cb) {
    Mutex::Autolock _il(mInterfaceLock);
    if (mCallback != nullptr && cb != nullptr) {
        ALOGE("%s: callback must not be set twice!", __FUNCTION__);
        return Void();
    }
    mCallback = cb;

    initOutputThread();

    if (mOutputThread == nullptr) {
        ALOGE("%s: init OutputThread failed!", __FUNCTION__);
    }
    return Void();
}

Return<void> ExternalCameraOfflineSession::getCaptureResultMetadataQueue(
        V3_3::ICameraDeviceSession::getCaptureResultMetadataQueue_cb _hidl_cb) {
    Mutex::Autolock _il(mInterfaceLock);
    _hidl_cb(*mResultMetadataQueue->getDesc());
    return Void();
}

void ExternalCameraOfflineSession::cleanupBuffersLocked(int id) {
    for (auto& pair : mCirculatingBuffers.at(id)) {
        sHandleImporter.freeBuffer(pair.second);
    }
    mCirculatingBuffers[id].clear();
    mCirculatingBuffers.erase(id);
}

Return<void> ExternalCameraOfflineSession::close() {
    Mutex::Autolock _il(mInterfaceLock);
    {
        Mutex::Autolock _l(mLock);
        if (mClosed) {
            ALOGW("%s: offline session already closed!", __FUNCTION__);
            return Void();
        }
    }
    if (mBufferRequestThread) {
        mBufferRequestThread->requestExit();
        mBufferRequestThread->join();
        mBufferRequestThread.clear();
    }
    if (mOutputThread) {
        mOutputThread->flush();
        mOutputThread->requestExit();
        mOutputThread->join();
        mOutputThread.clear();
    }

    Mutex::Autolock _l(mLock);
    // free all buffers
    {
        Mutex::Autolock _cbl(mCbsLock);
        for(auto stream : mOfflineStreams) {
            cleanupBuffersLocked(stream.id);
        }
    }
    mCallback.clear();
    mClosed = true;
    return Void();
}

} // namespace implementation
}  // namespace V3_6
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
