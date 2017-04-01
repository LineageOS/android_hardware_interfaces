/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <stdio.h>
#include <string.h>

#include <android/log.h>
#include <cutils/native_handle.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicBuffer.h>

#include <algorithm>    // std::min


// For the moment, we're assuming that the underlying EVS driver we're working with
// is providing 4 byte RGBx data.  This is fine for loopback testing, although
// real hardware is expected to provide YUV data -- most likly formatted as YV12
static const unsigned kBytesPerPixel = 4;   // assuming 4 byte RGBx pixels


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
    // Mark ourselves as running
    mLock.lock();
    mRunning = true;
    mLock.unlock();

    // Tell the camera to start streaming
    Return<EvsResult> result = mCamera->startVideoStream(this);
    return (result == EvsResult::OK);
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
    std::unique_lock<std::mutex> lock(mLock);
    mSignal.wait(lock, [this](){ return !mRunning; });
}


bool FrameHandler::returnHeldBuffer() {
    std::unique_lock<std::mutex> lock(mLock);

    // Return the oldest buffer we're holding
    if (mHeldBuffers.empty()) {
        // No buffers are currently held
        return false;
    }

    BufferDesc buffer = mHeldBuffers.front();
    mHeldBuffers.pop();
    mCamera->doneWithFrame(buffer);

    return true;
}


bool FrameHandler::isRunning() {
    std::unique_lock<std::mutex> lock(mLock);
    return mRunning;
}


void FrameHandler::waitForFrameCount(unsigned frameCount) {
    // Wait until we've seen at least the requested number of frames (could be more)
    std::unique_lock<std::mutex> lock(mLock);
    mSignal.wait(lock, [this, frameCount](){ return mFramesReceived >= frameCount; });
}


void FrameHandler::getFramesCounters(unsigned* received, unsigned* displayed) {
    std::unique_lock<std::mutex> lock(mLock);

    if (received) {
        *received = mFramesReceived;
    }
    if (displayed) {
        *displayed = mFramesDisplayed;
    }
}


Return<void> FrameHandler::deliverFrame(const BufferDesc& bufferArg) {
    ALOGD("Received a frame from the camera (%p)", bufferArg.memHandle.getNativeHandle());

    // Local flag we use to keep track of when the stream is stopping
    bool timeToStop = false;

    // TODO:  Why do we get a gralloc crash if we don't clone the buffer here?
    BufferDesc buffer(bufferArg);
    ALOGD("Clone the received frame as %p", buffer.memHandle.getNativeHandle());

    if (buffer.memHandle.getNativeHandle() == nullptr) {
        // Signal that the last frame has been received and the stream is stopped
        timeToStop = true;
    } else {
        // If we were given an opened display at construction time, then send the received
        // image back down the camera.
        if (mDisplay.get()) {
            // Get the output buffer we'll use to display the imagery
            BufferDesc tgtBuffer = {};
            mDisplay->getTargetBuffer([&tgtBuffer](const BufferDesc& buff) {
                                          tgtBuffer = buff;
                                      }
            );

            if (tgtBuffer.memHandle == nullptr) {
                printf("Didn't get target buffer - frame lost\n");
                ALOGE("Didn't get requested output buffer -- skipping this frame.");
            } else {
                // In order for the handles passed through HIDL and stored in the BufferDesc to
                // be lockable, we must register them with GraphicBufferMapper
                registerBufferHelper(tgtBuffer);
                registerBufferHelper(buffer);

                // Copy the contents of the of buffer.memHandle into tgtBuffer
                copyBufferContents(tgtBuffer, buffer);

                // Send the target buffer back for display
                Return <EvsResult> result = mDisplay->returnTargetBufferForDisplay(tgtBuffer);
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
                    mLock.lock();
                    mFramesDisplayed++;
                    mLock.unlock();
                }

                // Now tell GraphicBufferMapper we won't be using these handles anymore
                unregisterBufferHelper(tgtBuffer);
                unregisterBufferHelper(buffer);
            }
        }


        switch (mReturnMode) {
        case eAutoReturn:
            // Send the camera buffer back now that we're done with it
            ALOGD("Calling doneWithFrame");
            // TODO:  Why is it that we get a HIDL crash if we pass back the cloned buffer?
            mCamera->doneWithFrame(bufferArg);
            break;
        case eNoAutoReturn:
            // Hang onto the buffer handle for now -- we'll return it explicitly later
            mHeldBuffers.push(bufferArg);
        }


        ALOGD("Frame handling complete");
    }


    // Update our received frame count and notify anybody who cares that things have changed
    mLock.lock();
    if (timeToStop) {
        mRunning = false;
    } else {
        mFramesReceived++;
    }
    mLock.unlock();
    mSignal.notify_all();


    return Void();
}


bool FrameHandler::copyBufferContents(const BufferDesc& tgtBuffer,
                                      const BufferDesc& srcBuffer) {
    bool success = true;

    // Make sure we don't run off the end of either buffer
    const unsigned width     = std::min(tgtBuffer.width,
                                        srcBuffer.width);
    const unsigned height    = std::min(tgtBuffer.height,
                                        srcBuffer.height);

    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();


    // Lock our source buffer for reading
    unsigned char* srcPixels = nullptr;
    mapper.registerBuffer(srcBuffer.memHandle);
    mapper.lock(srcBuffer.memHandle,
                GRALLOC_USAGE_SW_READ_OFTEN,
                android::Rect(width, height),
                (void **) &srcPixels);

    // Lock our target buffer for writing
    unsigned char* tgtPixels = nullptr;
    mapper.registerBuffer(tgtBuffer.memHandle);
    mapper.lock(tgtBuffer.memHandle,
                GRALLOC_USAGE_SW_WRITE_OFTEN,
                android::Rect(width, height),
                (void **) &tgtPixels);

    if (srcPixels && tgtPixels) {
        for (unsigned row = 0; row < height; row++) {
            // Copy the entire row of pixel data
            memcpy(tgtPixels, srcPixels, width * kBytesPerPixel);

            // Advance to the next row (keeping in mind that stride here is in units of pixels)
            tgtPixels += tgtBuffer.stride * kBytesPerPixel;
            srcPixels += srcBuffer.stride * kBytesPerPixel;
        }
    } else {
        ALOGE("Failed to copy buffer contents");
        success = false;
    }

    if (srcPixels) {
        mapper.unlock(srcBuffer.memHandle);
    }
    if (tgtPixels) {
        mapper.unlock(tgtBuffer.memHandle);
    }
    mapper.unregisterBuffer(srcBuffer.memHandle);
    mapper.unregisterBuffer(tgtBuffer.memHandle);

    return success;
}


void FrameHandler::registerBufferHelper(const BufferDesc& buffer)
{
    // In order for the handles passed through HIDL and stored in the BufferDesc to
    // be lockable, we must register them with GraphicBufferMapper.
    // If the device upon which we're running supports gralloc1, we could just call
    // registerBuffer directly with the handle.  But that call  is broken for gralloc0 devices
    // (which we care about, at least for now).  As a result, we have to synthesize a GraphicBuffer
    // object around the buffer handle in order to make a call to the overloaded alternate
    // version of the registerBuffer call that does happen to work on gralloc0 devices.
#if REGISTER_BUFFER_ALWAYS_WORKS
    android::GraphicBufferMapper::get().registerBuffer(buffer.memHandle);
#else
    android::sp<android::GraphicBuffer> pGfxBuff = new android::GraphicBuffer(
            buffer.width, buffer.height, buffer.format,
            1, /* we always use exactly one layer */
            buffer.usage, buffer.stride,
            const_cast<native_handle_t*>(buffer.memHandle.getNativeHandle()),
            false /* GraphicBuffer should not try to free the handle */
    );

    android::GraphicBufferMapper::get().registerBuffer(pGfxBuff.get());
#endif
}


void FrameHandler::unregisterBufferHelper(const BufferDesc& buffer)
{
    // Now tell GraphicBufferMapper we won't be using these handles anymore
    android::GraphicBufferMapper::get().unregisterBuffer(buffer.memHandle);
}
