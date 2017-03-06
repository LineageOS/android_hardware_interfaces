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

#include <set>
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

HandleImporter& CameraDeviceSession::sHandleImporter = HandleImporter::getInstance();
const int CameraDeviceSession::ResultBatcher::NOT_BATCHED;

CameraDeviceSession::CameraDeviceSession(
    camera3_device_t* device,
    const camera_metadata_t* deviceInfo,
    const sp<ICameraDeviceCallback>& callback) :
        camera3_callback_ops({&sProcessCaptureResult, &sNotify}),
        mDevice(device),
        mResultBatcher(callback) {

    mDeviceInfo = deviceInfo;
    uint32_t numPartialResults = 1;
    camera_metadata_entry partialResultsCount =
            mDeviceInfo.find(ANDROID_REQUEST_PARTIAL_RESULT_COUNT);
    if (partialResultsCount.count > 0) {
        numPartialResults = partialResultsCount.data.i32[0];
    }
    mResultBatcher.setNumPartialResults(numPartialResults);

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

CameraDeviceSession::ResultBatcher::ResultBatcher(
        const sp<ICameraDeviceCallback>& callback) : mCallback(callback) {};

bool CameraDeviceSession::ResultBatcher::InflightBatch::allDelivered() const {
    if (!mShutterDelivered) return false;

    if (mPartialResultProgress < mNumPartialResults) {
        return false;
    }

    for (const auto& pair : mBatchBufs) {
        if (!pair.second.mDelivered) {
            return false;
        }
    }
    return true;
}

void CameraDeviceSession::ResultBatcher::setNumPartialResults(uint32_t n) {
    Mutex::Autolock _l(mLock);
    mNumPartialResults = n;
}

void CameraDeviceSession::ResultBatcher::setBatchedStreams(
        const std::vector<int>& streamsToBatch) {
    Mutex::Autolock _l(mLock);
    mStreamsToBatch = streamsToBatch;
}

void CameraDeviceSession::ResultBatcher::registerBatch(
        const hidl_vec<CaptureRequest>& requests) {
    auto batch = std::make_shared<InflightBatch>();
    batch->mFirstFrame = requests[0].frameNumber;
    batch->mBatchSize = requests.size();
    batch->mLastFrame = batch->mFirstFrame + batch->mBatchSize - 1;
    batch->mNumPartialResults = mNumPartialResults;
    for (int id : mStreamsToBatch) {
        batch->mBatchBufs[id] = InflightBatch::BufferBatch();
    }
    Mutex::Autolock _l(mLock);
    mInflightBatches.push_back(batch);
}

std::pair<int, std::shared_ptr<CameraDeviceSession::ResultBatcher::InflightBatch>>
CameraDeviceSession::ResultBatcher::getBatch(
        uint32_t frameNumber) {
    Mutex::Autolock _l(mLock);
    int numBatches = mInflightBatches.size();
    if (numBatches == 0) {
        return std::make_pair(NOT_BATCHED, nullptr);
    }
    uint32_t frameMin = mInflightBatches[0]->mFirstFrame;
    uint32_t frameMax = mInflightBatches[numBatches - 1]->mLastFrame;
    if (frameNumber < frameMin || frameNumber > frameMax) {
        return std::make_pair(NOT_BATCHED, nullptr);
    }
    for (int i = 0; i < numBatches; i++) {
        if (frameNumber >= mInflightBatches[i]->mFirstFrame &&
                frameNumber <= mInflightBatches[i]->mLastFrame) {
            return std::make_pair(i, mInflightBatches[i]);
        }
    }
    return std::make_pair(NOT_BATCHED, nullptr);
}

void CameraDeviceSession::ResultBatcher::checkAndRemoveFirstBatch() {
    Mutex::Autolock _l(mLock);
    if (mInflightBatches.size() > 0) {
        std::shared_ptr<InflightBatch> batch = mInflightBatches[0];
        bool shouldRemove = false;
        {
            Mutex::Autolock _l(batch->mLock);
            if (batch->allDelivered()) {
                batch->mRemoved = true;
                shouldRemove = true;
            }
        }
        if (shouldRemove) {
            mInflightBatches.pop_front();
        }
    }
}

void CameraDeviceSession::ResultBatcher::sendBatchShutterCbsLocked(std::shared_ptr<InflightBatch> batch) {
    if (batch->mShutterDelivered) {
        ALOGW("%s: batch shutter callback already sent!", __FUNCTION__);
        return;
    }

    mCallback->notify(batch->mShutterMsgs);
    batch->mShutterDelivered = true;
    batch->mShutterMsgs.clear();
}

void CameraDeviceSession::ResultBatcher::freeReleaseFences(hidl_vec<CaptureResult>& results) {
    for (auto& result : results) {
        if (result.inputBuffer.releaseFence.getNativeHandle() != nullptr) {
            native_handle_t* handle = const_cast<native_handle_t*>(
                    result.inputBuffer.releaseFence.getNativeHandle());
            native_handle_delete(handle);
        }
        for (auto& buf : result.outputBuffers) {
            if (buf.releaseFence.getNativeHandle() != nullptr) {
                native_handle_t* handle = const_cast<native_handle_t*>(
                        buf.releaseFence.getNativeHandle());
                native_handle_delete(handle);
            }
        }
    }
    return;
}

void CameraDeviceSession::ResultBatcher::sendBatchBuffersLocked(std::shared_ptr<InflightBatch> batch) {
    sendBatchBuffersLocked(batch, mStreamsToBatch);
}

void CameraDeviceSession::ResultBatcher::sendBatchBuffersLocked(
        std::shared_ptr<InflightBatch> batch, const std::vector<int>& streams) {
    size_t batchSize = 0;
    for (int streamId : streams) {
        auto it = batch->mBatchBufs.find(streamId);
        if (it != batch->mBatchBufs.end()) {
            InflightBatch::BufferBatch& bb = it->second;
            if (bb.mDelivered) {
                continue;
            }
            if (bb.mBuffers.size() > batchSize) {
                batchSize = bb.mBuffers.size();
            }
        } else {
            ALOGE("%s: stream ID %d is not batched!", __FUNCTION__, streamId);
            return;
        }
    }

    if (batchSize == 0) {
        ALOGW("%s: there is no buffer to be delivered for this batch.", __FUNCTION__);
        for (int streamId : streams) {
            InflightBatch::BufferBatch& bb = batch->mBatchBufs[streamId];
            bb.mDelivered = true;
        }
        return;
    }

    hidl_vec<CaptureResult> results;
    results.resize(batchSize);
    for (size_t i = 0; i < batchSize; i++) {
        results[i].frameNumber = batch->mFirstFrame + i;
        results[i].partialResult = 0; // 0 for buffer only results
        results[i].inputBuffer.streamId = -1;
        results[i].inputBuffer.bufferId = 0;
        results[i].inputBuffer.buffer = nullptr;
        std::vector<StreamBuffer> outBufs;
        for (int streamId : streams) {
            InflightBatch::BufferBatch& bb = batch->mBatchBufs[streamId];
            if (bb.mDelivered) {
                continue;
            }
            if (i < bb.mBuffers.size()) {
                outBufs.push_back(bb.mBuffers[i]);
            }
        }
        results[i].outputBuffers = outBufs;
    }
    mCallback->processCaptureResult(results);
    freeReleaseFences(results);
    for (int streamId : streams) {
        InflightBatch::BufferBatch& bb = batch->mBatchBufs[streamId];
        bb.mDelivered = true;
        bb.mBuffers.clear();
    }
}

void CameraDeviceSession::ResultBatcher::sendBatchMetadataLocked(
    std::shared_ptr<InflightBatch> batch, uint32_t lastPartialResultIdx) {
    if (lastPartialResultIdx <= batch->mPartialResultProgress) {
        // Result has been delivered. Return
        ALOGW("%s: partial result %u has been delivered", __FUNCTION__, lastPartialResultIdx);
        return;
    }

    std::vector<CaptureResult> results;
    std::vector<uint32_t> toBeRemovedIdxes;
    for (auto& pair : batch->mResultMds) {
        uint32_t partialIdx = pair.first;
        if (partialIdx > lastPartialResultIdx) {
            continue;
        }
        toBeRemovedIdxes.push_back(partialIdx);
        InflightBatch::MetadataBatch& mb = pair.second;
        for (const auto& p : mb.mMds) {
            CaptureResult result;
            result.frameNumber = p.first;
            result.result = std::move(p.second);
            result.inputBuffer.streamId = -1;
            result.inputBuffer.bufferId = 0;
            result.inputBuffer.buffer = nullptr;
            result.partialResult = partialIdx;
            results.push_back(std::move(result));
        }
        mb.mMds.clear();
    }
    mCallback->processCaptureResult(results);
    batch->mPartialResultProgress = lastPartialResultIdx;
    for (uint32_t partialIdx : toBeRemovedIdxes) {
        batch->mResultMds.erase(partialIdx);
    }
}

void CameraDeviceSession::ResultBatcher::notifySingleMsg(NotifyMsg& msg) {
    mCallback->notify({msg});
    return;
}

void CameraDeviceSession::ResultBatcher::notify(NotifyMsg& msg) {
    uint32_t frameNumber;
    if (CC_LIKELY(msg.type == MsgType::SHUTTER)) {
        frameNumber = msg.msg.shutter.frameNumber;
    } else {
        frameNumber = msg.msg.error.frameNumber;
    }

    auto pair = getBatch(frameNumber);
    int batchIdx = pair.first;
    if (batchIdx == NOT_BATCHED) {
        notifySingleMsg(msg);
        return;
    }

    // When error happened, stop batching for all batches earlier
    if (CC_UNLIKELY(msg.type == MsgType::ERROR)) {
        Mutex::Autolock _l(mLock);
        for (int i = 0; i <= batchIdx; i++) {
            // Send batched data up
            std::shared_ptr<InflightBatch> batch = mInflightBatches[0];
            {
                Mutex::Autolock _l(batch->mLock);
                sendBatchShutterCbsLocked(batch);
                sendBatchBuffersLocked(batch);
                sendBatchMetadataLocked(batch, mNumPartialResults);
                if (!batch->allDelivered()) {
                    ALOGE("%s: error: some batch data not sent back to framework!",
                            __FUNCTION__);
                }
                batch->mRemoved = true;
            }
            mInflightBatches.pop_front();
        }
        // Send the error up
        notifySingleMsg(msg);
        return;
    }
    // Queue shutter callbacks for future delivery
    std::shared_ptr<InflightBatch> batch = pair.second;
    {
        Mutex::Autolock _l(batch->mLock);
        // Check if the batch is removed (mostly by notify error) before lock was acquired
        if (batch->mRemoved) {
            // Fall back to non-batch path
            notifySingleMsg(msg);
            return;
        }

        batch->mShutterMsgs.push_back(msg);
        if (frameNumber == batch->mLastFrame) {
            sendBatchShutterCbsLocked(batch);
        }
    } // end of batch lock scope

    // see if the batch is complete
    if (frameNumber == batch->mLastFrame) {
        checkAndRemoveFirstBatch();
    }
}

void CameraDeviceSession::ResultBatcher::processOneCaptureResult(CaptureResult& result) {
    hidl_vec<CaptureResult> results = {result};
    mCallback->processCaptureResult(results);
    freeReleaseFences(results);
    return;
}

void CameraDeviceSession::ResultBatcher::processCaptureResult(CaptureResult& result) {
    auto pair = getBatch(result.frameNumber);
    int batchIdx = pair.first;
    if (batchIdx == NOT_BATCHED) {
        processOneCaptureResult(result);
        return;
    }
    std::shared_ptr<InflightBatch> batch = pair.second;
    {
        Mutex::Autolock _l(batch->mLock);
        // Check if the batch is removed (mostly by notify error) before lock was acquired
        if (batch->mRemoved) {
            // Fall back to non-batch path
            processOneCaptureResult(result);
            return;
        }

        // queue metadata
        if (result.result.size() != 0) {
            // Save a copy of metadata
            batch->mResultMds[result.partialResult].mMds.push_back(
                    std::make_pair(result.frameNumber, result.result));
        }

        // queue buffer
        std::vector<int> filledStreams;
        std::vector<StreamBuffer> nonBatchedBuffers;
        for (auto& buffer : result.outputBuffers) {
            auto it = batch->mBatchBufs.find(buffer.streamId);
            if (it != batch->mBatchBufs.end()) {
                InflightBatch::BufferBatch& bb = it->second;
                bb.mBuffers.push_back(buffer);
                filledStreams.push_back(buffer.streamId);
            } else {
                nonBatchedBuffers.push_back(buffer);
            }
        }

        // send non-batched buffers up
        if (nonBatchedBuffers.size() > 0 || result.inputBuffer.streamId != -1) {
            CaptureResult nonBatchedResult;
            nonBatchedResult.frameNumber = result.frameNumber;
            nonBatchedResult.outputBuffers = nonBatchedBuffers;
            nonBatchedResult.inputBuffer = result.inputBuffer;
            nonBatchedResult.partialResult = 0; // 0 for buffer only results
            processOneCaptureResult(nonBatchedResult);
        }

        if (result.frameNumber == batch->mLastFrame) {
            // Send data up
            if (result.partialResult > 0) {
                sendBatchMetadataLocked(batch, result.partialResult);
            }
            // send buffer up
            if (filledStreams.size() > 0) {
                sendBatchBuffersLocked(batch, filledStreams);
            }
        }
    } // end of batch lock scope

    // see if the batch is complete
    if (result.frameNumber == batch->mLastFrame) {
        checkAndRemoveFirstBatch();
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

        // Track video streams
        mVideoStreamIds.clear();
        for (const auto& stream : requestedConfiguration.streams) {
            if (stream.streamType == StreamType::OUTPUT &&
                    stream.usage & graphics::allocator::V2_0::ConsumerUsage::VIDEO_ENCODER) {
                mVideoStreamIds.push_back(stream.id);
            }
        }
        mResultBatcher.setBatchedStreams(mVideoStreamIds);
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

Return<void> CameraDeviceSession::processCaptureRequest(
        const hidl_vec<CaptureRequest>& requests, processCaptureRequest_cb _hidl_cb)  {
    uint32_t numRequestProcessed = 0;
    Status s = Status::OK;
    for (size_t i = 0; i < requests.size(); i++, numRequestProcessed++) {
        s = processOneCaptureRequest(requests[i]);
        if (s != Status::OK) {
            break;
        }
    }

    if (s == Status::OK && requests.size() > 1) {
        mResultBatcher.registerBatch(requests);
    }

    _hidl_cb(s, numRequestProcessed);
    return Void();
}

Status CameraDeviceSession::processOneCaptureRequest(const CaptureRequest& request)  {
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
    if (numBufs > 0) {
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
            native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
            handle->data[0] = hal_result->input_buffer->release_fence;
            result.inputBuffer.releaseFence = handle;
        } else {
            result.inputBuffer.releaseFence = nullptr;
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
            native_handle_t* handle = native_handle_create(/*numFds*/1, /*numInts*/0);
            handle->data[0] = hal_result->output_buffers[i].release_fence;
            result.outputBuffers[i].releaseFence = handle;
        } else {
            result.outputBuffers[i].releaseFence = nullptr;
        }
    }

    // Free inflight record/fences.
    // Do this before call back to camera service because camera service might jump to
    // configure_streams right after the processCaptureResult call so we need to finish
    // updating inflight queues first
    if (numBufs > 0) {
        Mutex::Autolock _l(d->mInflightLock);
        if (hasInputBuf) {
            int streamId = static_cast<Camera3Stream*>(hal_result->input_buffer->stream)->mId;
            auto key = std::make_pair(streamId, frameNumber);
            d->mInflightBuffers.erase(key);
        }

        for (size_t i = 0; i < numOutputBufs; i++) {
            int streamId = static_cast<Camera3Stream*>(hal_result->output_buffers[i].stream)->mId;
            auto key = std::make_pair(streamId, frameNumber);
            d->mInflightBuffers.erase(key);
        }

        if (d->mInflightBuffers.empty()) {
            ALOGV("%s: inflight buffer queue is now empty!", __FUNCTION__);
        }
    }

    d->mResultBatcher.processCaptureResult(result);
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
            return;
        }
    }
    d->mResultBatcher.notify(hidlMsg);
}

} // namespace implementation
}  // namespace V3_2
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
