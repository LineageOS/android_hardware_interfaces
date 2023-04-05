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

#include "device_cb.h"

#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <grallocusage/GrallocUsageConversion.h>
#include <cinttypes>

using ::aidl::android::hardware::camera::device::BufferStatus;
using ::aidl::android::hardware::camera::device::ErrorMsg;
using ::aidl::android::hardware::camera::device::StreamBufferRequestError;
using ::aidl::android::hardware::camera::device::StreamBuffersVal;
using ::aidl::android::hardware::graphics::common::PixelFormat;

const int64_t kBufferReturnTimeoutSec = 1;

DeviceCb::DeviceCb(CameraAidlTest* parent, camera_metadata_t* staticMeta) : mParent(parent) {
    mStaticMetadata = staticMeta;
}

ScopedAStatus DeviceCb::notify(const std::vector<NotifyMsg>& msgs) {
    std::vector<std::pair<bool, nsecs_t>> readoutTimestamps;

    size_t count = msgs.size();
    readoutTimestamps.resize(count);

    for (size_t i = 0; i < count; i++) {
        const NotifyMsg& msg = msgs[i];
        switch (msg.getTag()) {
            case NotifyMsg::Tag::error:
                readoutTimestamps[i] = {false, 0};
                break;
            case NotifyMsg::Tag::shutter:
                const auto& shutter = msg.get<NotifyMsg::Tag::shutter>();
                readoutTimestamps[i] = {true, shutter.readoutTimestamp};
                break;
        }
    }

    return notifyHelper(msgs, readoutTimestamps);
}

ScopedAStatus DeviceCb::processCaptureResult(const std::vector<CaptureResult>& results) {
    if (nullptr == mParent) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    bool notify = false;
    std::unique_lock<std::mutex> l(mParent->mLock);
    for (const auto& result : results) {
        notify = processCaptureResultLocked(result, result.physicalCameraMetadata);
    }

    l.unlock();
    if (notify) {
        mParent->mResultCondition.notify_one();
    }

    return ndk::ScopedAStatus::ok();
}

ScopedAStatus DeviceCb::requestStreamBuffers(const std::vector<BufferRequest>& bufReqs,
                                             std::vector<StreamBufferRet>* buffers,
                                             BufferRequestStatus* _aidl_return) {
    std::vector<StreamBufferRet>& bufRets = *buffers;
    std::unique_lock<std::mutex> l(mLock);

    if (!mUseHalBufManager) {
        ALOGE("%s: Camera does not support HAL buffer management", __FUNCTION__);
        ADD_FAILURE();
        *_aidl_return = BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS;
        return ScopedAStatus::ok();
    }

    if (bufReqs.size() > mStreams.size()) {
        ALOGE("%s: illegal buffer request: too many requests!", __FUNCTION__);
        ADD_FAILURE();
        *_aidl_return = BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS;
        return ndk::ScopedAStatus::ok();
    }

    std::vector<size_t> indexes(bufReqs.size());
    for (size_t i = 0; i < bufReqs.size(); i++) {
        bool found = false;
        for (size_t idx = 0; idx < mStreams.size(); idx++) {
            if (bufReqs[i].streamId == mStreams[idx].id) {
                found = true;
                indexes[i] = idx;
                break;
            }
        }
        if (!found) {
            ALOGE("%s: illegal buffer request: unknown streamId %d!", __FUNCTION__,
                  bufReqs[i].streamId);
            ADD_FAILURE();
            *_aidl_return = BufferRequestStatus::FAILED_ILLEGAL_ARGUMENTS;
            return ScopedAStatus::ok();
        }
    }

    bool allStreamOk = true;
    bool atLeastOneStreamOk = false;
    bufRets.resize(bufReqs.size());

    for (size_t i = 0; i < bufReqs.size(); i++) {
        size_t idx = indexes[i];
        const auto& stream = mStreams[idx];
        const auto& halStream = mHalStreams[idx];
        const BufferRequest& bufReq = bufReqs[i];

        if (mOutstandingBufferIds[idx].size() + bufReq.numBuffersRequested > halStream.maxBuffers) {
            bufRets[i].streamId = stream.id;
            bufRets[i].val.set<StreamBuffersVal::Tag::error>(
                    StreamBufferRequestError::MAX_BUFFER_EXCEEDED);
            allStreamOk = false;
            continue;
        }

        std::vector<StreamBuffer> tmpRetBuffers(bufReq.numBuffersRequested);
        for (size_t j = 0; j < bufReq.numBuffersRequested; j++) {
            buffer_handle_t handle;
            uint32_t w = stream.width;
            uint32_t h = stream.height;
            if (stream.format == PixelFormat::BLOB) {
                w = stream.bufferSize;
                h = 1;
            }

            CameraAidlTest::allocateGraphicBuffer(
                    w, h,
                    android_convertGralloc1To0Usage(static_cast<uint64_t>(halStream.producerUsage),
                                                    static_cast<uint64_t>(halStream.consumerUsage)),
                    halStream.overrideFormat, &handle);

            StreamBuffer streamBuffer = StreamBuffer();
            StreamBuffer& sb = tmpRetBuffers[j];
            sb = {
                    stream.id,        mNextBufferId,  ::android::dupToAidl(handle),
                    BufferStatus::OK, NativeHandle(), NativeHandle(),
            };

            mOutstandingBufferIds[idx][mNextBufferId++] = handle;
        }
        atLeastOneStreamOk = true;
        bufRets[i].streamId = stream.id;
        bufRets[i].val.set<StreamBuffersVal::Tag::buffers>(std::move(tmpRetBuffers));
    }

    if (allStreamOk) {
        *_aidl_return = BufferRequestStatus::OK;
    } else if (atLeastOneStreamOk) {
        *_aidl_return = BufferRequestStatus::FAILED_PARTIAL;
    } else {
        *_aidl_return = BufferRequestStatus::FAILED_UNKNOWN;
    }

    if (!hasOutstandingBuffersLocked()) {
        l.unlock();
        mFlushedCondition.notify_one();
    }

    return ndk::ScopedAStatus::ok();
}

ScopedAStatus DeviceCb::returnStreamBuffers(const std::vector<StreamBuffer>& buffers) {
    if (!mUseHalBufManager) {
        ALOGE("%s: Camera does not support HAL buffer management", __FUNCTION__);
        ADD_FAILURE();
    }

    std::unique_lock<std::mutex> l(mLock);
    for (const auto& buf : buffers) {
        if (buf.bufferId == 0) {
            // Don't return buffers of bufId 0 (empty buffer)
            continue;
        }
        bool found = false;
        for (size_t idx = 0; idx < mOutstandingBufferIds.size(); idx++) {
            if (mStreams[idx].id == buf.streamId &&
                mOutstandingBufferIds[idx].count(buf.bufferId) == 1) {
                mOutstandingBufferIds[idx].erase(buf.bufferId);
                // TODO: check do we need to close/delete native handle or assume we have enough
                // memory to run till the test finish? since we do not capture much requests (and
                // most of time one buffer is sufficient)
                found = true;
                break;
            }
        }
        if (found) {
            continue;
        }
        ALOGE("%s: unknown buffer ID %" PRIu64, __FUNCTION__, buf.bufferId);
        ADD_FAILURE();
    }
    if (!hasOutstandingBuffersLocked()) {
        l.unlock();
        mFlushedCondition.notify_one();
    }

    return ndk::ScopedAStatus::ok();
}

void DeviceCb::setCurrentStreamConfig(const std::vector<Stream>& streams,
                                      const std::vector<HalStream>& halStreams) {
    ASSERT_EQ(streams.size(), halStreams.size());
    ASSERT_NE(streams.size(), 0);
    for (size_t i = 0; i < streams.size(); i++) {
        ASSERT_EQ(streams[i].id, halStreams[i].id);
    }
    std::lock_guard<std::mutex> l(mLock);
    mUseHalBufManager = true;
    mStreams = streams;
    mHalStreams = halStreams;
    mOutstandingBufferIds.clear();
    for (size_t i = 0; i < streams.size(); i++) {
        mOutstandingBufferIds.emplace_back();
    }
}

void DeviceCb::waitForBuffersReturned() {
    std::unique_lock<std::mutex> lk(mLock);
    if (hasOutstandingBuffersLocked()) {
        auto timeout = std::chrono::seconds(kBufferReturnTimeoutSec);
        auto st = mFlushedCondition.wait_for(lk, timeout);
        ASSERT_NE(std::cv_status::timeout, st);
    }
}

bool DeviceCb::processCaptureResultLocked(
        const CaptureResult& results, std::vector<PhysicalCameraMetadata> physicalCameraMetadata) {
    bool notify = false;
    uint32_t frameNumber = results.frameNumber;

    if ((results.result.metadata.empty()) && (results.outputBuffers.empty()) &&
        (results.inputBuffer.buffer.fds.empty()) && (results.fmqResultSize == 0)) {
        ALOGE("%s: No result data provided by HAL for frame %d result count: %d", __func__,
              frameNumber, (int)results.fmqResultSize);
        ADD_FAILURE();
        return notify;
    }

    auto requestEntry = mParent->mInflightMap.find(frameNumber);
    if (requestEntry == mParent->mInflightMap.end()) {
        ALOGE("%s: Unexpected frame number! received: %u", __func__, frameNumber);
        ADD_FAILURE();
        return notify;
    }

    bool isPartialResult = false;
    bool hasInputBufferInRequest = false;
    auto& request = requestEntry->second;

    CameraMetadata resultMetadata;
    size_t resultSize = 0;
    if (results.fmqResultSize > 0) {
        resultMetadata.metadata.resize(results.fmqResultSize);
        if (request->resultQueue == nullptr) {
            ADD_FAILURE();
            return notify;
        }

        if (!request->resultQueue->read(reinterpret_cast<int8_t*>(resultMetadata.metadata.data()),
                                        results.fmqResultSize)) {
            ALOGE("%s: Frame %d: Cannot read camera metadata from fmq,"
                  "size = %" PRIu64,
                  __func__, frameNumber, results.fmqResultSize);
            ADD_FAILURE();
            return notify;
        }

        // Physical device results are only expected in the last/final
        // partial result notification.
        bool expectPhysicalResults = !(request->usePartialResult &&
                                       (results.partialResult < request->numPartialResults));
        if (expectPhysicalResults &&
            (physicalCameraMetadata.size() != request->expectedPhysicalResults.size())) {
            ALOGE("%s: Frame %d: Returned physical metadata count %zu "
                  "must be equal to expected count %zu",
                  __func__, frameNumber, physicalCameraMetadata.size(),
                  request->expectedPhysicalResults.size());
            ADD_FAILURE();
            return notify;
        }
        std::vector<std::vector<uint8_t>> physResultMetadata;
        physResultMetadata.resize(physicalCameraMetadata.size());
        for (size_t i = 0; i < physicalCameraMetadata.size(); i++) {
            physResultMetadata[i].resize(physicalCameraMetadata[i].fmqMetadataSize);
            if (!request->resultQueue->read(reinterpret_cast<int8_t*>(physResultMetadata[i].data()),
                                            physicalCameraMetadata[i].fmqMetadataSize)) {
                ALOGE("%s: Frame %d: Cannot read physical camera metadata from fmq,"
                      "size = %" PRIu64,
                      __func__, frameNumber, physicalCameraMetadata[i].fmqMetadataSize);
                ADD_FAILURE();
                return notify;
            }
        }
        resultSize = resultMetadata.metadata.size();
    } else if (!results.result.metadata.empty()) {
        resultMetadata = results.result;
        resultSize = resultMetadata.metadata.size();
    }

    if (!request->usePartialResult && (resultSize > 0) && (results.partialResult != 1)) {
        ALOGE("%s: Result is malformed for frame %d: partial_result %u "
              "must be 1  if partial result is not supported",
              __func__, frameNumber, results.partialResult);
        ADD_FAILURE();
        return notify;
    }

    if (results.partialResult != 0) {
        request->partialResultCount = results.partialResult;
    }

    // Check if this result carries only partial metadata
    if (request->usePartialResult && (resultSize > 0)) {
        if ((results.partialResult > request->numPartialResults) || (results.partialResult < 1)) {
            ALOGE("%s: Result is malformed for frame %d: partial_result %u"
                  " must be  in the range of [1, %d] when metadata is "
                  "included in the result",
                  __func__, frameNumber, results.partialResult, request->numPartialResults);
            ADD_FAILURE();
            return notify;
        }

        // Verify no duplicate tags between partial results
        const camera_metadata_t* partialMetadata =
                reinterpret_cast<const camera_metadata_t*>(resultMetadata.metadata.data());
        const camera_metadata_t* collectedMetadata = request->collectedResult.getAndLock();
        camera_metadata_ro_entry_t searchEntry, foundEntry;
        for (size_t i = 0; i < get_camera_metadata_entry_count(partialMetadata); i++) {
            if (0 != get_camera_metadata_ro_entry(partialMetadata, i, &searchEntry)) {
                ADD_FAILURE();
                request->collectedResult.unlock(collectedMetadata);
                return notify;
            }
            if (-ENOENT !=
                find_camera_metadata_ro_entry(collectedMetadata, searchEntry.tag, &foundEntry)) {
                ADD_FAILURE();
                request->collectedResult.unlock(collectedMetadata);
                return notify;
            }
        }
        request->collectedResult.unlock(collectedMetadata);
        request->collectedResult.append(partialMetadata);

        isPartialResult = (results.partialResult < request->numPartialResults);
    } else if (resultSize > 0) {
        request->collectedResult.append(
                reinterpret_cast<const camera_metadata_t*>(resultMetadata.metadata.data()));
        isPartialResult = false;
    }

    hasInputBufferInRequest = request->hasInputBuffer;

    // Did we get the (final) result metadata for this capture?
    if ((resultSize > 0) && !isPartialResult) {
        if (request->haveResultMetadata) {
            ALOGE("%s: Called multiple times with metadata for frame %d", __func__, frameNumber);
            ADD_FAILURE();
            return notify;
        }
        request->haveResultMetadata = true;
        request->collectedResult.sort();

        // Verify final result metadata
        camera_metadata_t* staticMetadataBuffer = mStaticMetadata;
        bool isMonochrome = Status::OK == CameraAidlTest::isMonochromeCamera(staticMetadataBuffer);
        if (isMonochrome) {
            CameraAidlTest::verifyMonochromeCameraResult(request->collectedResult);
        }

        // Verify logical camera result metadata
        bool isLogicalCamera =
                Status::OK == CameraAidlTest::isLogicalMultiCamera(staticMetadataBuffer);
        if (isLogicalCamera) {
            camera_metadata_t* collectedMetadata =
                    const_cast<camera_metadata_t*>(request->collectedResult.getAndLock());
            uint8_t* rawMetadata = reinterpret_cast<uint8_t*>(collectedMetadata);
            std::vector metadata = std::vector(
                    rawMetadata, rawMetadata + get_camera_metadata_size(collectedMetadata));
            CameraAidlTest::verifyLogicalCameraResult(staticMetadataBuffer, metadata);
            request->collectedResult.unlock(collectedMetadata);
        }
    }

    uint32_t numBuffersReturned = results.outputBuffers.size();
    auto& inputBuffer = results.inputBuffer.buffer;
    if (!inputBuffer.fds.empty() && !inputBuffer.ints.empty()) {
        if (hasInputBufferInRequest) {
            numBuffersReturned += 1;
        } else {
            ALOGW("%s: Input buffer should be NULL if there is no input"
                  " buffer sent in the request",
                  __func__);
        }
    }
    request->numBuffersLeft -= numBuffersReturned;
    if (request->numBuffersLeft < 0) {
        ALOGE("%s: Too many buffers returned for frame %d", __func__, frameNumber);
        ADD_FAILURE();
        return notify;
    }

    for (const auto& buffer : results.outputBuffers) {
        CameraAidlTest::InFlightRequest::StreamBufferAndTimestamp streamBufferAndTimestamp;
        auto outstandingBuffers = mUseHalBufManager ? mOutstandingBufferIds :
            request->mOutstandingBufferIds;
        auto bufferId = mUseHalBufManager ? buffer.bufferId : results.frameNumber;
        auto outputBuffer = outstandingBuffers.empty() ? ::android::makeFromAidl(buffer.buffer) :
            outstandingBuffers[buffer.streamId][bufferId];
        streamBufferAndTimestamp.buffer = {buffer.streamId,
                                           bufferId,
                                           outputBuffer,
                                           buffer.status,
                                           ::android::makeFromAidl(buffer.acquireFence),
                                           ::android::makeFromAidl(buffer.releaseFence)};
        streamBufferAndTimestamp.timeStamp = systemTime();
        request->resultOutputBuffers.push_back(streamBufferAndTimestamp);
    }
    // If shutter event is received notify the pending threads.
    if (request->shutterTimestamp != 0) {
        notify = true;
    }

    if (mUseHalBufManager) {
        returnStreamBuffers(results.outputBuffers);
    }
    return notify;
}

ScopedAStatus DeviceCb::notifyHelper(
        const std::vector<NotifyMsg>& msgs,
        const std::vector<std::pair<bool, nsecs_t>>& readoutTimestamps) {
    std::lock_guard<std::mutex> l(mParent->mLock);

    for (size_t i = 0; i < msgs.size(); i++) {
        const NotifyMsg& msg = msgs[i];
        NotifyMsg::Tag msgTag = msgs[i].getTag();
        switch (msgTag) {
            case NotifyMsg::Tag::error:
                if (ErrorCode::ERROR_DEVICE == msg.get<NotifyMsg::Tag::error>().errorCode) {
                    ALOGE("%s: Camera reported serious device error", __func__);
                    ADD_FAILURE();
                } else {
                    auto itr = mParent->mInflightMap.find(
                            msg.get<NotifyMsg::Tag::error>().frameNumber);
                    if (itr == mParent->mInflightMap.end()) {
                        ALOGE("%s: Unexpected error frame number! received: %u", __func__,
                              msg.get<NotifyMsg::Tag::error>().frameNumber);
                        ADD_FAILURE();
                        break;
                    }

                    auto r = itr->second;
                    if (ErrorCode::ERROR_RESULT == msg.get<NotifyMsg::Tag::error>().errorCode &&
                        msg.get<NotifyMsg::Tag::error>().errorStreamId != -1) {
                        if (r->haveResultMetadata) {
                            ALOGE("%s: Camera must report physical camera result error before "
                                  "the final capture result!",
                                  __func__);
                            ADD_FAILURE();
                        } else {
                            for (auto& mStream : mStreams) {
                                if (mStream.id == msg.get<NotifyMsg::Tag::error>().errorStreamId) {
                                    std::string physicalCameraId = mStream.physicalCameraId;
                                    bool idExpected =
                                            r->expectedPhysicalResults.find(physicalCameraId) !=
                                            r->expectedPhysicalResults.end();
                                    if (!idExpected) {
                                        ALOGE("%s: ERROR_RESULT's error stream's physicalCameraId "
                                              "%s must be expected",
                                              __func__, physicalCameraId.c_str());
                                        ADD_FAILURE();
                                    } else {
                                        r->expectedPhysicalResults.erase(physicalCameraId);
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        r->errorCodeValid = true;
                        r->errorCode = msg.get<NotifyMsg::Tag::error>().errorCode;
                        r->errorStreamId = msg.get<NotifyMsg::Tag::error>().errorStreamId;
                    }
                }
                break;
            case NotifyMsg::Tag::shutter:
                auto itr =
                        mParent->mInflightMap.find(msg.get<NotifyMsg::Tag::shutter>().frameNumber);
                if (itr == mParent->mInflightMap.end()) {
                    ALOGE("%s: Unexpected shutter frame number! received: %u", __func__,
                          msg.get<NotifyMsg::Tag::shutter>().frameNumber);
                    ADD_FAILURE();
                    break;
                }
                auto& r = itr->second;
                r->shutterTimestamp = msg.get<NotifyMsg::Tag::shutter>().timestamp;
                r->shutterReadoutTimestampValid = readoutTimestamps[i].first;
                r->shutterReadoutTimestamp = readoutTimestamps[i].second;
                break;
        }
    }

    mParent->mResultCondition.notify_one();
    return ScopedAStatus::ok();
}

bool DeviceCb::hasOutstandingBuffersLocked() {
    if (!mUseHalBufManager) {
        return false;
    }
    for (const auto& outstandingBuffers : mOutstandingBufferIds) {
        if (!outstandingBuffers.empty()) {
            return true;
        }
    }
    return false;
}
