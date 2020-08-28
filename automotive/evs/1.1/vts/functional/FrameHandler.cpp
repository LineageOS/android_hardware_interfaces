/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <stdio.h>
#include <string.h>
#include <chrono>

#include <android/log.h>
#include <cutils/native_handle.h>
#include <ui/GraphicBuffer.h>

using namespace std::chrono_literals;

FrameHandler::FrameHandler(android::sp <IEvsCamera> pCamera, CameraDesc cameraInfo,
                           android::sp <IEvsDisplay> pDisplay,
                           BufferControlFlag mode) :
    mCamera(pCamera),
    mCameraInfo(cameraInfo),
    mDisplay(pDisplay),
    mReturnMode(mode) {
    // Nothing but member initialization here...
}


void FrameHandler::shutdown()
{
    // Make sure we're not still streaming
    blockingStopStream();

    // At this point, the receiver thread is no longer running, so we can safely drop
    // our remote object references so they can be freed
    mCamera = nullptr;
    mDisplay = nullptr;
}


bool FrameHandler::startStream() {
    // Tell the camera to start streaming
    Return<EvsResult> result = mCamera->startVideoStream(this);
    if (result != EvsResult::OK) {
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

    hidl_vec<BufferDesc_1_1> buffers = mHeldBuffers.front();
    mHeldBuffers.pop();
    mCamera->doneWithFrame_1_1(buffers);

    return true;
}


bool FrameHandler::isRunning() {
    std::lock_guard<std::mutex> lock(mLock);
    return mRunning;
}


void FrameHandler::waitForFrameCount(unsigned frameCount) {
    // Wait until we've seen at least the requested number of frames (could be more)
    std::unique_lock<std::mutex> lock(mLock);
    mFrameSignal.wait(lock, [this, frameCount](){
                                return mFramesReceived >= frameCount;
                            });
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


Return<void> FrameHandler::deliverFrame(const BufferDesc_1_0& bufferArg) {
    ALOGW("A frame delivered via v1.0 method is rejected.");
    mCamera->doneWithFrame(bufferArg);
    return Void();
}


Return<void> FrameHandler::deliverFrame_1_1(const hidl_vec<BufferDesc_1_1>& buffers) {
    mLock.lock();
    // For VTS tests, FrameHandler uses a single frame among delivered frames.
    auto bufferIdx = mFramesDisplayed % buffers.size();
    auto buffer = buffers[bufferIdx];
    mLock.unlock();

    const AHardwareBuffer_Desc* pDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&buffer.buffer.description);
    ALOGD("Received a frame from the camera (%p)",
          buffer.buffer.nativeHandle.getNativeHandle());

    // Store a dimension of a received frame.
    mFrameWidth = pDesc->width;
    mFrameHeight = pDesc->height;

    // If we were given an opened display at construction time, then send the received
    // image back down the camera.
    bool displayed = false;
    if (mDisplay.get()) {
        // Get the output buffer we'll use to display the imagery
        BufferDesc_1_0 tgtBuffer = {};
        mDisplay->getTargetBuffer([&tgtBuffer](const BufferDesc_1_0& buff) {
                                      tgtBuffer = buff;
                                  }
        );

        if (tgtBuffer.memHandle == nullptr) {
            printf("Didn't get target buffer - frame lost\n");
            ALOGE("Didn't get requested output buffer -- skipping this frame.");
        } else {
            // Copy the contents of the of buffer.memHandle into tgtBuffer
            copyBufferContents(tgtBuffer, buffer);

            // Send the target buffer back for display
            Return<EvsResult> result = mDisplay->returnTargetBufferForDisplay(tgtBuffer);
            if (!result.isOk()) {
                printf("HIDL error on display buffer (%s)- frame lost\n",
                       result.description().c_str());
                ALOGE("Error making the remote function call.  HIDL said %s",
                      result.description().c_str());
            } else if (result != EvsResult::OK) {
                printf("Display reported error - frame lost\n");
                ALOGE("We encountered error %d when returning a buffer to the display!",
                      (EvsResult) result);
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
    case eAutoReturn:
        // Send the camera buffer back now that the client has seen it
        ALOGD("Calling doneWithFrame");
        mCamera->doneWithFrame_1_1(buffers);
        break;
    case eNoAutoReturn:
        // Hang onto the buffer handles for now -- the client will return it explicitly later
        mHeldBuffers.push(buffers);
        break;
    }

    ALOGD("Frame handling complete");

    return Void();
}


Return<void> FrameHandler::notify(const EvsEventDesc& event) {
    // Local flag we use to keep track of when the stream is stopping
    std::unique_lock<std::mutex> lock(mEventLock);
    mLatestEventDesc.aType = event.aType;
    mLatestEventDesc.payload[0] = event.payload[0];
    mLatestEventDesc.payload[1] = event.payload[1];
    if (mLatestEventDesc.aType == EvsEventType::STREAM_STOPPED) {
        // Signal that the last frame has been received and the stream is stopped
        mRunning = false;
    } else if (mLatestEventDesc.aType == EvsEventType::PARAMETER_CHANGED) {
        ALOGD("Camera parameter 0x%X is changed to 0x%X",
              mLatestEventDesc.payload[0], mLatestEventDesc.payload[1]);
    } else {
        ALOGD("Received an event %s", eventToString(mLatestEventDesc.aType));
    }
    lock.unlock();
    mEventSignal.notify_one();

    return Void();
}


bool FrameHandler::copyBufferContents(const BufferDesc_1_0& tgtBuffer,
                                      const BufferDesc_1_1& srcBuffer) {
    bool success = true;
    const AHardwareBuffer_Desc* pSrcDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&srcBuffer.buffer.description);

    // Make sure we don't run off the end of either buffer
    const unsigned width  = std::min(tgtBuffer.width,
                                     pSrcDesc->width);
    const unsigned height = std::min(tgtBuffer.height,
                                     pSrcDesc->height);

    sp<android::GraphicBuffer> tgt = new android::GraphicBuffer(tgtBuffer.memHandle,
                                                                android::GraphicBuffer::CLONE_HANDLE,
                                                                tgtBuffer.width,
                                                                tgtBuffer.height,
                                                                tgtBuffer.format,
                                                                1,
                                                                tgtBuffer.usage,
                                                                tgtBuffer.stride);
    sp<android::GraphicBuffer> src = new android::GraphicBuffer(srcBuffer.buffer.nativeHandle,
                                                                android::GraphicBuffer::CLONE_HANDLE,
                                                                pSrcDesc->width,
                                                                pSrcDesc->height,
                                                                pSrcDesc->format,
                                                                pSrcDesc->layers,
                                                                pSrcDesc->usage,
                                                                pSrcDesc->stride);

    // Lock our source buffer for reading (current expectation are for this to be NV21 format)
    uint8_t* srcPixels = nullptr;
    src->lock(GRALLOC_USAGE_SW_READ_OFTEN, (void**)&srcPixels);

    // Lock our target buffer for writing (should be either RGBA8888 or BGRA8888 format)
    uint32_t* tgtPixels = nullptr;
    tgt->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)&tgtPixels);

    if (srcPixels && tgtPixels) {
        using namespace ::android::hardware::automotive::evs::common;
        if (tgtBuffer.format == HAL_PIXEL_FORMAT_RGBA_8888) {
            if (pSrcDesc->format == HAL_PIXEL_FORMAT_YCRCB_420_SP) {   // 420SP == NV21
                Utils::copyNV21toRGB32(width, height,
                                       srcPixels,
                                       tgtPixels, tgtBuffer.stride);
            } else if (pSrcDesc->format == HAL_PIXEL_FORMAT_YV12) { // YUV_420P == YV12
                Utils::copyYV12toRGB32(width, height,
                                       srcPixels,
                                       tgtPixels, tgtBuffer.stride);
            } else if (pSrcDesc->format == HAL_PIXEL_FORMAT_YCBCR_422_I) { // YUYV
                Utils::copyYUYVtoRGB32(width, height,
                                       srcPixels, pSrcDesc->stride,
                                       tgtPixels, tgtBuffer.stride);
            } else if (pSrcDesc->format == tgtBuffer.format) {  // 32bit RGBA
                Utils::copyMatchedInterleavedFormats(width, height,
                                                     srcPixels, pSrcDesc->stride,
                                                     tgtPixels, tgtBuffer.stride,
                                                     tgtBuffer.pixelSize);
            } else {
                ALOGE("Camera buffer format is not supported");
                success = false;
            }
        } else if (tgtBuffer.format == HAL_PIXEL_FORMAT_BGRA_8888) {
            if (pSrcDesc->format == HAL_PIXEL_FORMAT_YCRCB_420_SP) {   // 420SP == NV21
                Utils::copyNV21toBGR32(width, height,
                                       srcPixels,
                                       tgtPixels, tgtBuffer.stride);
            } else if (pSrcDesc->format == HAL_PIXEL_FORMAT_YV12) { // YUV_420P == YV12
                Utils::copyYV12toBGR32(width, height,
                                       srcPixels,
                                       tgtPixels, tgtBuffer.stride);
            } else if (pSrcDesc->format == HAL_PIXEL_FORMAT_YCBCR_422_I) { // YUYV
                Utils::copyYUYVtoBGR32(width, height,
                                       srcPixels, pSrcDesc->stride,
                                       tgtPixels, tgtBuffer.stride);
            } else if (pSrcDesc->format == tgtBuffer.format) {  // 32bit RGBA
                Utils::copyMatchedInterleavedFormats(width, height,
                                                     srcPixels, pSrcDesc->stride,
                                                     tgtPixels, tgtBuffer.stride,
                                                     tgtBuffer.pixelSize);
            } else {
                ALOGE("Camera buffer format is not supported");
                success = false;
            }
        } else {
            // We always expect 32 bit RGB for the display output for now.  Is there a need for 565?
            ALOGE("Diplay buffer is always expected to be 32bit RGBA");
            success = false;
        }
    } else {
        ALOGE("Failed to lock buffer contents for contents transfer");
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

bool FrameHandler::waitForEvent(const EvsEventDesc& aTargetEvent,
                                      EvsEventDesc& aReceivedEvent,
                                      bool ignorePayload) {
    // Wait until we get an expected parameter change event.
    std::unique_lock<std::mutex> lock(mEventLock);
    auto now = std::chrono::system_clock::now();
    bool found = false;
    while (!found) {
        bool result = mEventSignal.wait_until(lock, now + 5s,
            [this, aTargetEvent, ignorePayload, &aReceivedEvent, &found](){
                found = (mLatestEventDesc.aType == aTargetEvent.aType) &&
                        (ignorePayload || (mLatestEventDesc.payload[0] == aTargetEvent.payload[0] &&
                                           mLatestEventDesc.payload[1] == aTargetEvent.payload[1]));

                aReceivedEvent.aType = mLatestEventDesc.aType;
                aReceivedEvent.payload[0] = mLatestEventDesc.payload[0];
                aReceivedEvent.payload[1] = mLatestEventDesc.payload[1];
                return found;
            }
        );

        if (!result) {
            ALOGW("A timer is expired before a target event has happened.");
            break;
        }
    }

    return found;
}

const char *FrameHandler::eventToString(const EvsEventType aType) {
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

