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

#define LOG_TAG "ExtCamDevSsn@3.5"
#include <android/log.h>

#include <utils/Trace.h>
#include "ExternalCameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_5 {
namespace implementation {

ExternalCameraDeviceSession::ExternalCameraDeviceSession(
        const sp<V3_2::ICameraDeviceCallback>& callback,
        const ExternalCameraConfig& cfg,
        const std::vector<SupportedV4L2Format>& sortedFormats,
        const CroppingType& croppingType,
        const common::V1_0::helper::CameraMetadata& chars,
        const std::string& cameraId,
        unique_fd v4l2Fd) :
        V3_4::implementation::ExternalCameraDeviceSession(
                callback, cfg, sortedFormats, croppingType, chars, cameraId, std::move(v4l2Fd)) {

    mCallback_3_5 = nullptr;

    auto castResult = V3_5::ICameraDeviceCallback::castFrom(callback);
    if (castResult.isOk()) {
        sp<V3_5::ICameraDeviceCallback> callback3_5 = castResult;
        if (callback3_5 != nullptr) {
            mCallback_3_5 = callback3_5;
        }
    }

    if (mCallback_3_5 != nullptr) {
        mSupportBufMgr = true;
    }
}

ExternalCameraDeviceSession::~ExternalCameraDeviceSession() {
    closeOutputThreadImpl();
}

Return<void> ExternalCameraDeviceSession::configureStreams_3_5(
        const StreamConfiguration& requestedConfiguration,
        ICameraDeviceSession::configureStreams_3_5_cb _hidl_cb)  {
    return configureStreams_3_4(requestedConfiguration.v3_4, _hidl_cb);
}

Return<void> ExternalCameraDeviceSession::signalStreamFlush(
        const hidl_vec<int32_t>& /*streamIds*/, uint32_t /*streamConfigCounter*/) {
    return Void();
}

Status ExternalCameraDeviceSession::importRequestLocked(
        const CaptureRequest& request,
        hidl_vec<buffer_handle_t*>& allBufPtrs,
        hidl_vec<int>& allFences) {
    if (mSupportBufMgr) {
        return importRequestLockedImpl(request, allBufPtrs, allFences, /*allowEmptyBuf*/ true);
    }
    return importRequestLockedImpl(request, allBufPtrs, allFences, /*allowEmptyBuf*/ false);
}


ExternalCameraDeviceSession::BufferRequestThread::BufferRequestThread(
        wp<OutputThreadInterface> parent,
        sp<V3_5::ICameraDeviceCallback> callbacks) :
        mParent(parent),
        mCallbacks(callbacks) {}

int ExternalCameraDeviceSession::BufferRequestThread::requestBufferStart(
        const std::vector<HalStreamBuffer>& bufReqs) {
    if (bufReqs.empty()) {
        ALOGE("%s: bufReqs is empty!", __FUNCTION__);
        return -1;
    }

    {
        std::lock_guard<std::mutex> lk(mLock);
        if (mRequestingBuffer) {
            ALOGE("%s: BufferRequestThread does not support more than one concurrent request!",
                    __FUNCTION__);
            return -1;
        }

        mBufferReqs = bufReqs;
        mRequestingBuffer = true;
    }
    mRequestCond.notify_one();
    return 0;
}

int ExternalCameraDeviceSession::BufferRequestThread::waitForBufferRequestDone(
        std::vector<HalStreamBuffer>* outBufReq) {
    std::unique_lock<std::mutex> lk(mLock);
    if (!mRequestingBuffer) {
        ALOGE("%s: no pending buffer request!", __FUNCTION__);
        return -1;
    }

    if (mPendingReturnBufferReqs.empty()) {
        std::chrono::milliseconds timeout = std::chrono::milliseconds(kReqProcTimeoutMs);
        auto st = mRequestDoneCond.wait_for(lk, timeout);
        if (st == std::cv_status::timeout) {
            ALOGE("%s: wait for buffer request finish timeout!", __FUNCTION__);
            return -1;
        }
    }
    mRequestingBuffer = false;
    *outBufReq = std::move(mPendingReturnBufferReqs);
    mPendingReturnBufferReqs.clear();
    return 0;
}

void ExternalCameraDeviceSession::BufferRequestThread::waitForNextRequest() {
    ATRACE_CALL();
    std::unique_lock<std::mutex> lk(mLock);
    int waitTimes = 0;
    while (mBufferReqs.empty()) {
        if (exitPending()) {
            return;
        }
        std::chrono::milliseconds timeout = std::chrono::milliseconds(kReqWaitTimeoutMs);
        auto st = mRequestCond.wait_for(lk, timeout);
        if (st == std::cv_status::timeout) {
            waitTimes++;
            if (waitTimes == kReqWaitTimesWarn) {
                // BufferRequestThread just wait forever for new buffer request
                // But it will print some periodic warning indicating it's waiting
                ALOGV("%s: still waiting for new buffer request", __FUNCTION__);
                waitTimes = 0;
            }
        }
    }

    // Fill in hidl BufferRequest
    mHalBufferReqs.resize(mBufferReqs.size());
    for (size_t i = 0; i < mHalBufferReqs.size(); i++) {
        mHalBufferReqs[i].streamId = mBufferReqs[i].streamId;
        mHalBufferReqs[i].numBuffersRequested = 1;
    }
}

bool ExternalCameraDeviceSession::BufferRequestThread::threadLoop() {
    waitForNextRequest();
    if (exitPending()) {
        return false;
    }

    ATRACE_BEGIN("HIDL requestStreamBuffers");
    BufferRequestStatus status;
    hidl_vec<StreamBufferRet> bufRets;
    auto err = mCallbacks->requestStreamBuffers(mHalBufferReqs,
            [&status, &bufRets]
            (BufferRequestStatus s, const hidl_vec<StreamBufferRet>& rets) {
                status = s;
                bufRets = std::move(rets);
            });
    ATRACE_END();
    if (!err.isOk()) {
        ALOGE("%s: Transaction error: %s", __FUNCTION__, err.description().c_str());
        return false;
    }

    std::unique_lock<std::mutex> lk(mLock);
    if (status == BufferRequestStatus::OK || status == BufferRequestStatus::FAILED_PARTIAL) {
        if (bufRets.size() != mHalBufferReqs.size()) {
            ALOGE("%s: expect %zu buffer requests returned, only got %zu",
                    __FUNCTION__, mHalBufferReqs.size(), bufRets.size());
            return false;
        }

        auto parent = mParent.promote();
        if (parent == nullptr) {
            ALOGE("%s: session has been disconnected!", __FUNCTION__);
            return false;
        }

        hidl_vec<int> importedFences;
        importedFences.resize(bufRets.size());
        for (size_t i = 0; i < bufRets.size(); i++) {
            int streamId = bufRets[i].streamId;
            switch (bufRets[i].val.getDiscriminator()) {
                case StreamBuffersVal::hidl_discriminator::error:
                    continue;
                case StreamBuffersVal::hidl_discriminator::buffers: {
                    const hidl_vec<V3_2::StreamBuffer>& hBufs = bufRets[i].val.buffers();
                    if (hBufs.size() != 1) {
                        ALOGE("%s: expect 1 buffer returned, got %zu!", __FUNCTION__, hBufs.size());
                        return false;
                    }
                    const V3_2::StreamBuffer& hBuf = hBufs[0];

                    mBufferReqs[i].bufferId = hBuf.bufferId;
                    // TODO: create a batch import API so we don't need to lock/unlock mCbsLock
                    // repeatedly?
                    lk.unlock();
                    Status s = parent->importBuffer(streamId,
                            hBuf.bufferId, hBuf.buffer.getNativeHandle(),
                            /*out*/&mBufferReqs[i].bufPtr,
                            /*allowEmptyBuf*/false);
                    lk.lock();

                    if (s != Status::OK) {
                        ALOGE("%s: stream %d import buffer failed!", __FUNCTION__, streamId);
                        cleanupInflightFences(importedFences, i - 1);
                        return false;
                    }
                    if (!sHandleImporter.importFence(hBuf.acquireFence,
                            mBufferReqs[i].acquireFence)) {
                        ALOGE("%s: stream %d import fence failed!", __FUNCTION__, streamId);
                        cleanupInflightFences(importedFences, i - 1);
                        return false;
                    }
                    importedFences[i] = mBufferReqs[i].acquireFence;
                }
                break;
                default:
                    ALOGE("%s: unkown StreamBuffersVal discrimator!", __FUNCTION__);
                    return false;
            }
        }
    } else {
        ALOGE("%s: requestStreamBuffers call failed!", __FUNCTION__);
    }

    mPendingReturnBufferReqs = std::move(mBufferReqs);
    mBufferReqs.clear();

    lk.unlock();
    mRequestDoneCond.notify_one();
    return true;
}

void ExternalCameraDeviceSession::initOutputThread() {
    if (mSupportBufMgr) {
        mBufferRequestThread = new BufferRequestThread(this, mCallback_3_5);
        mBufferRequestThread->run("ExtCamBufReq", PRIORITY_DISPLAY);
    }
    mOutputThread = new OutputThread(
            this, mCroppingType, mCameraCharacteristics, mBufferRequestThread);
}

void ExternalCameraDeviceSession::closeOutputThreadImpl() {
    if (mBufferRequestThread) {
        mBufferRequestThread->requestExit();
        mBufferRequestThread->join();
        mBufferRequestThread.clear();
    }
}

void ExternalCameraDeviceSession::closeOutputThread() {
    closeOutputThreadImpl();
    V3_4::implementation::ExternalCameraDeviceSession::closeOutputThread();
}

ExternalCameraDeviceSession::OutputThread::OutputThread(
        wp<OutputThreadInterface> parent,
        CroppingType ct,
        const common::V1_0::helper::CameraMetadata& chars,
        sp<BufferRequestThread> bufReqThread) :
        V3_4::implementation::ExternalCameraDeviceSession::OutputThread(parent, ct, chars),
        mBufferRequestThread(bufReqThread) {}

ExternalCameraDeviceSession::OutputThread::~OutputThread() {}

int ExternalCameraDeviceSession::OutputThread::requestBufferStart(
        const std::vector<HalStreamBuffer>& bufs) {
    if (mBufferRequestThread != nullptr) {
        return mBufferRequestThread->requestBufferStart(bufs);
    }
    return 0;
}

int ExternalCameraDeviceSession::OutputThread::waitForBufferRequestDone(
        /*out*/std::vector<HalStreamBuffer>* outBufs) {
    if (mBufferRequestThread != nullptr) {
        return mBufferRequestThread->waitForBufferRequestDone(outBufs);
    }
    return 0;
}

Return<void> ExternalCameraDeviceSession::isReconfigurationRequired(
        const V3_2::CameraMetadata& /*oldSessionParams*/,
        const V3_2::CameraMetadata& /*newSessionParams*/,
        ICameraDeviceSession::isReconfigurationRequired_cb _hidl_cb) {
    //Stub implementation
    _hidl_cb(Status::OK, true);
    return Void();
}

} // namespace implementation
}  // namespace V3_5
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
