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

#define LOG_TAG "VtsHalEvsTest"

#include "FrameHandler.h"
#include "FormatConvert.h"

#include <aidl/android/hardware/graphics/common/HardwareBuffer.h>
#include <aidl/android/hardware/graphics/common/HardwareBufferDescription.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>

namespace {

using ::aidl::android::hardware::automotive::evs::BufferDesc;
using ::aidl::android::hardware::automotive::evs::CameraDesc;
using ::aidl::android::hardware::automotive::evs::EvsEventDesc;
using ::aidl::android::hardware::automotive::evs::EvsEventType;
using ::aidl::android::hardware::automotive::evs::IEvsCamera;
using ::aidl::android::hardware::automotive::evs::IEvsDisplay;
using ::aidl::android::hardware::common::NativeHandle;
using ::aidl::android::hardware::graphics::common::HardwareBuffer;
using ::aidl::android::hardware::graphics::common::HardwareBufferDescription;
using ::ndk::ScopedAStatus;
using std::chrono_literals::operator""s;

NativeHandle dupNativeHandle(const NativeHandle& handle, bool doDup) {
    NativeHandle dup;

    dup.fds = std::vector<::ndk::ScopedFileDescriptor>(handle.fds.size());
    if (!doDup) {
        for (auto i = 0; i < handle.fds.size(); ++i) {
            dup.fds.at(i).set(handle.fds[i].get());
        }
    } else {
        for (auto i = 0; i < handle.fds.size(); ++i) {
            dup.fds[i] = std::move(handle.fds[i].dup());
        }
    }
    dup.ints = handle.ints;

    return std::move(dup);
}

HardwareBuffer dupHardwareBuffer(const HardwareBuffer& buffer, bool doDup) {
    HardwareBuffer dup = {
            .description = buffer.description,
            .handle = dupNativeHandle(buffer.handle, doDup),
    };

    return std::move(dup);
}

BufferDesc dupBufferDesc(const BufferDesc& src, bool doDup) {
    BufferDesc dup = {
            .buffer = dupHardwareBuffer(src.buffer, doDup),
            .pixelSizeBytes = src.pixelSizeBytes,
            .bufferId = src.bufferId,
            .deviceId = src.deviceId,
            .timestamp = src.timestamp,
            .metadata = src.metadata,
    };

    return std::move(dup);
}

bool comparePayload(const EvsEventDesc& l, const EvsEventDesc& r) {
    return std::equal(l.payload.begin(), l.payload.end(), r.payload.begin());
}

} // namespace

FrameHandler::FrameHandler(const std::shared_ptr<IEvsCamera>& pCamera, const CameraDesc& cameraInfo,
                           const std::shared_ptr<IEvsDisplay>& pDisplay, BufferControlFlag mode)
    : mCamera(pCamera), mCameraInfo(cameraInfo), mDisplay(pDisplay), mReturnMode(mode) {
    // Nothing but member initialization here.
}

void FrameHandler::shutdown() {
    // Make sure we're not still streaming
    blockingStopStream();

    // At this point, the receiver thread is no longer running, so we can safely drop
    // our remote object references so they can be freed
    mCamera = nullptr;
    mDisplay = nullptr;
}

bool FrameHandler::startStream() {
    // Tell the camera to start streaming
    auto status = mCamera->startVideoStream(ref<FrameHandler>());
    if (!status.isOk()) {
        return false;
    }

    // Mark ourselves as running
    mLock.lock();
    mRunning = true;
    mLock.unlock();

    return true;
}

void FrameHandler::asyncStopStream() {
    // Tell the camera to stop streaming.
    // This will result in a null frame being delivered when the stream actually stops.
    mCamera->stopVideoStream();
}

void FrameHandler::blockingStopStream() {
    // Tell the stream to stop
    asyncStopStream();

    // Wait until the stream has actually stopped
    std::unique_lock<std::mutex> lock(mEventLock);
    if (mRunning) {
        mEventSignal.wait(lock, [this]() { return !mRunning; });
    }
}

bool FrameHandler::returnHeldBuffer() {
    std::lock_guard<std::mutex> lock(mLock);

    // Return the oldest buffer we're holding
    if (mHeldBuffers.empty()) {
        // No buffers are currently held
        return false;
    }

    std::vector<BufferDesc> buffers = std::move(mHeldBuffers.front());
    mHeldBuffers.pop();
    mCamera->doneWithFrame(buffers);

    return true;
}

bool FrameHandler::isRunning() {
    std::lock_guard<std::mutex> lock(mLock);
    return mRunning;
}

void FrameHandler::waitForFrameCount(unsigned frameCount) {
    // Wait until we've seen at least the requested number of frames (could be more)
    std::unique_lock<std::mutex> lock(mLock);
    mFrameSignal.wait(lock, [this, frameCount]() { return mFramesReceived >= frameCount; });
}

void FrameHandler::getFramesCounters(unsigned* received, unsigned* displayed) {
    std::lock_guard<std::mutex> lock(mLock);

    if (received) {
        *received = mFramesReceived;
    }
    if (displayed) {
        *displayed = mFramesDisplayed;
    }
}

ScopedAStatus FrameHandler::deliverFrame(const std::vector<BufferDesc>& buffers) {
    mLock.lock();
    // For VTS tests, FrameHandler uses a single frame among delivered frames.
    auto bufferIdx = mFramesDisplayed % buffers.size();
    auto& buffer = buffers[bufferIdx];
    mLock.unlock();

    // Store a dimension of a received frame.
    mFrameWidth = buffer.buffer.description.width;
    mFrameHeight = buffer.buffer.description.height;

    // If we were given an opened display at construction time, then send the received
    // image back down the camera.
    bool displayed = false;
    if (mDisplay) {
        // Get the output buffer we'll use to display the imagery
        BufferDesc tgtBuffer;
        auto status = mDisplay->getTargetBuffer(&tgtBuffer);
        if (!status.isOk()) {
            printf("Didn't get target buffer - frame lost\n");
            LOG(ERROR) << "Didn't get requested output buffer -- skipping this frame.";
        } else {
            // Copy the contents of the of buffer.memHandle into tgtBuffer
            copyBufferContents(tgtBuffer, buffer);

            // Send the target buffer back for display
            auto status = mDisplay->returnTargetBufferForDisplay(tgtBuffer);
            if (!status.isOk()) {
                printf("AIDL error on display buffer (%d)- frame lost\n",
                       status.getServiceSpecificError());
                LOG(ERROR) << "Error making the remote function call.  AIDL said "
                           << status.getServiceSpecificError();
            } else {
                // Everything looks good!
                // Keep track so tests or watch dogs can monitor progress
                displayed = true;
            }
        }
    }

    mLock.lock();
    // increases counters
    ++mFramesReceived;
    mFramesDisplayed += (int)displayed;
    mLock.unlock();
    mFrameSignal.notify_all();

    switch (mReturnMode) {
        case eAutoReturn: {
            // Send the camera buffer back now that the client has seen it
            LOG(DEBUG) << "Calling doneWithFrame";
            if (!mCamera->doneWithFrame(buffers).isOk()) {
                LOG(WARNING) << "Failed to return buffers";
            }
            break;
        }

        case eNoAutoReturn: {
            // Hang onto the buffer handles for now -- the client will return it explicitly later
            std::vector<BufferDesc> buffersToHold;
            for (const auto& buffer : buffers) {
                buffersToHold.push_back(dupBufferDesc(buffer, /* doDup = */ true));
            }
            mHeldBuffers.push(std::move(buffersToHold));
            break;
        }
    }

    LOG(DEBUG) << "Frame handling complete";
    return ScopedAStatus::ok();
}

ScopedAStatus FrameHandler::notify(const EvsEventDesc& event) {
    // Local flag we use to keep track of when the stream is stopping
    std::unique_lock<std::mutex> lock(mEventLock);
    mLatestEventDesc.aType = event.aType;
    mLatestEventDesc.payload = event.payload;
    if (mLatestEventDesc.aType == EvsEventType::STREAM_STOPPED) {
        // Signal that the last frame has been received and the stream is stopped
        mRunning = false;
    } else if (mLatestEventDesc.aType == EvsEventType::PARAMETER_CHANGED) {
        LOG(DEBUG) << "Camera parameter " << mLatestEventDesc.payload[0] << " is changed to "
                   << mLatestEventDesc.payload[1];
    } else {
        LOG(DEBUG) << "Received an event " << eventToString(mLatestEventDesc.aType);
    }
    lock.unlock();
    mEventSignal.notify_one();

    return ScopedAStatus::ok();
}

bool FrameHandler::copyBufferContents(const BufferDesc& tgtBuffer, const BufferDesc& srcBuffer) {
    bool success = true;
    const HardwareBufferDescription* pSrcDesc =
            reinterpret_cast<const HardwareBufferDescription*>(&srcBuffer.buffer.description);
    const HardwareBufferDescription* pTgtDesc =
            reinterpret_cast<const HardwareBufferDescription*>(&tgtBuffer.buffer.description);

    // Make sure we don't run off the end of either buffer
    const unsigned width = std::min(pTgtDesc->width, pSrcDesc->width);
    const unsigned height = std::min(pTgtDesc->height, pSrcDesc->height);

    // FIXME: We duplicate file descriptors twice below; consider using TAKE_HANDLE
    // instead of CLONE_HANDLE.
    buffer_handle_t target = ::android::dupFromAidl(tgtBuffer.buffer.handle);
    ::android::sp<android::GraphicBuffer> tgt = new android::GraphicBuffer(
            target, android::GraphicBuffer::CLONE_HANDLE, pTgtDesc->width, pTgtDesc->height,
            static_cast<android::PixelFormat>(pTgtDesc->format), pTgtDesc->layers,
            static_cast<uint64_t>(pTgtDesc->usage), pTgtDesc->stride);

    buffer_handle_t source = ::android::dupFromAidl(srcBuffer.buffer.handle);
    ::android::sp<android::GraphicBuffer> src = new android::GraphicBuffer(
            source, android::GraphicBuffer::CLONE_HANDLE, pSrcDesc->width, pSrcDesc->height,
            static_cast<android::PixelFormat>(pSrcDesc->format), pSrcDesc->layers,
            static_cast<uint64_t>(pSrcDesc->usage), pSrcDesc->stride);

    // Lock our source buffer for reading (current expectation are for this to be NV21 format)
    uint8_t* srcPixels = nullptr;
    src->lock(GRALLOC_USAGE_SW_READ_OFTEN, (void**)&srcPixels);

    // Lock our target buffer for writing (should be either RGBA8888 or BGRA8888 format)
    uint32_t* tgtPixels = nullptr;
    tgt->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)&tgtPixels);

    if (srcPixels && tgtPixels) {
        using namespace ::android::hardware::automotive::evs::common;
        if (static_cast<android_pixel_format_t>(pTgtDesc->format) == HAL_PIXEL_FORMAT_RGBA_8888) {
            if (static_cast<android_pixel_format_t>(pSrcDesc->format) ==
                HAL_PIXEL_FORMAT_YCRCB_420_SP) {  // 420SP == NV21
                Utils::copyNV21toRGB32(width, height, srcPixels, tgtPixels, pTgtDesc->stride);
            } else if (static_cast<android_pixel_format_t>(pSrcDesc->format) ==
                       HAL_PIXEL_FORMAT_YV12) {  // YUV_420P == YV12
                Utils::copyYV12toRGB32(width, height, srcPixels, tgtPixels, pTgtDesc->stride);
            } else if (static_cast<android_pixel_format_t>(pSrcDesc->format) ==
                       HAL_PIXEL_FORMAT_YCBCR_422_I) {  // YUYV
                Utils::copyYUYVtoRGB32(width, height, srcPixels, pSrcDesc->stride, tgtPixels,
                                       pTgtDesc->stride);
            } else if (pSrcDesc->format == pTgtDesc->format) {  // 32bit RGBA
                Utils::copyMatchedInterleavedFormats(width, height, srcPixels, pSrcDesc->stride,
                                                     tgtPixels, pTgtDesc->stride,
                                                     tgtBuffer.pixelSizeBytes);
            } else {
                LOG(ERROR) << "Camera buffer format is not supported";
                success = false;
            }
        } else if (static_cast<android_pixel_format_t>(pTgtDesc->format) ==
                   HAL_PIXEL_FORMAT_BGRA_8888) {
            if (static_cast<android_pixel_format_t>(pSrcDesc->format) ==
                HAL_PIXEL_FORMAT_YCRCB_420_SP) {  // 420SP == NV21
                Utils::copyNV21toBGR32(width, height, srcPixels, tgtPixels, pTgtDesc->stride);
            } else if (static_cast<android_pixel_format_t>(pSrcDesc->format) ==
                       HAL_PIXEL_FORMAT_YV12) {  // YUV_420P == YV12
                Utils::copyYV12toBGR32(width, height, srcPixels, tgtPixels, pTgtDesc->stride);
            } else if (static_cast<android_pixel_format_t>(pSrcDesc->format) ==
                       HAL_PIXEL_FORMAT_YCBCR_422_I) {  // YUYV
                Utils::copyYUYVtoBGR32(width, height, srcPixels, pSrcDesc->stride, tgtPixels,
                                       pTgtDesc->stride);
            } else if (pSrcDesc->format == pTgtDesc->format) {  // 32bit RGBA
                Utils::copyMatchedInterleavedFormats(width, height, srcPixels, pSrcDesc->stride,
                                                     tgtPixels, pTgtDesc->stride,
                                                     tgtBuffer.pixelSizeBytes);
            } else {
                LOG(ERROR) << "Camera buffer format is not supported";
                success = false;
            }
        } else {
            // We always expect 32 bit RGB for the display output for now.  Is there a need for 565?
            LOG(ERROR) << "Diplay buffer is always expected to be 32bit RGBA";
            success = false;
        }
    } else {
        LOG(ERROR) << "Failed to lock buffer contents for contents transfer";
        success = false;
    }

    if (srcPixels) {
        src->unlock();
    }
    if (tgtPixels) {
        tgt->unlock();
    }

    return success;
}

void FrameHandler::getFrameDimension(unsigned* width, unsigned* height) {
    if (width) {
        *width = mFrameWidth;
    }

    if (height) {
        *height = mFrameHeight;
    }
}

bool FrameHandler::waitForEvent(const EvsEventDesc& aTargetEvent, EvsEventDesc& aReceivedEvent,
                                bool ignorePayload) {
    // Wait until we get an expected parameter change event.
    std::unique_lock<std::mutex> lock(mEventLock);
    auto now = std::chrono::system_clock::now();
    bool found = false;
    while (!found) {
        bool result = mEventSignal.wait_until(
                lock, now + 5s, [this, aTargetEvent, ignorePayload, &aReceivedEvent, &found]() {
                    found = (mLatestEventDesc.aType == aTargetEvent.aType) &&
                            (ignorePayload || comparePayload(mLatestEventDesc, aTargetEvent));
                    aReceivedEvent.aType = mLatestEventDesc.aType;
                    aReceivedEvent.payload = mLatestEventDesc.payload;
                    return found;
                });

        if (!result) {
            LOG(WARNING) << "A timer is expired before a target event has happened.";
            break;
        }
    }

    return found;
}

const char* FrameHandler::eventToString(const EvsEventType aType) {
    switch (aType) {
        case EvsEventType::STREAM_STARTED:
            return "STREAM_STARTED";
        case EvsEventType::STREAM_STOPPED:
            return "STREAM_STOPPED";
        case EvsEventType::FRAME_DROPPED:
            return "FRAME_DROPPED";
        case EvsEventType::TIMEOUT:
            return "TIMEOUT";
        case EvsEventType::PARAMETER_CHANGED:
            return "PARAMETER_CHANGED";
        case EvsEventType::MASTER_RELEASED:
            return "MASTER_RELEASED";
        default:
            return "Unknown";
    }
}
