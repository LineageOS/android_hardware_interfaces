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

#define LOG_TAG "android.hardware.evs@1.0-service"

#include "EvsCamera.h"

#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>


namespace android {
namespace hardware {
namespace evs {
namespace V1_0 {
namespace implementation {


// These are the special camera names for which we'll initialize custom test data
const char EvsCamera::kCameraName_Backup[]    = "backup";
const char EvsCamera::kCameraName_RightTurn[] = "Right Turn";


// TODO(b/31632518):  Need to get notification when our client dies so we can close the camera.
// As it stands, if the client dies suddently, the buffer may be stranded.
// As possible work around would be to give the client a HIDL object to exclusively hold
// and use it's destructor to perform some work in the server side.

EvsCamera::EvsCamera(const char *id) {
    ALOGD("EvsCamera instantiated");

    mDescription.cameraId = id;
    mFrameBusy = false;
    mStreamState = STOPPED;

    // Set up dummy data for testing
    if (mDescription.cameraId == kCameraName_Backup) {
        mDescription.hints                  = UsageHint::USAGE_HINT_REVERSE;
        mDescription.vendorFlags            = 0xFFFFFFFF;   // Arbitrary value
        mDescription.defaultHorResolution   = 320;          // 1/2 NTSC/VGA
        mDescription.defaultVerResolution   = 240;          // 1/2 NTSC/VGA
    }
    else if (mDescription.cameraId == kCameraName_RightTurn) {
        // Nothing but the name and the usage hint
        mDescription.hints                  = UsageHint::USAGE_HINT_RIGHT_TURN;
    }
    else {
        // Leave empty for a minimalist camera description without even a hint
    }
}

EvsCamera::~EvsCamera() {
    ALOGD("EvsCamera being destroyed");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // Make sure our output stream is cleaned up
    // (It really should be already)
    stopVideoStream();

    // Drop the graphics buffer we've been using
    if (mBuffer) {
        // Drop the graphics buffer we've been using
        GraphicBufferAllocator& alloc(GraphicBufferAllocator::get());
        alloc.free(mBuffer);
    }

    ALOGD("EvsCamera destroyed");
}


// Methods from ::android::hardware::evs::V1_0::IEvsCamera follow.
Return<void> EvsCamera::getId(getId_cb id_cb) {
    ALOGD("getId");

    id_cb(mDescription.cameraId);

    return Void();
}


Return<EvsResult> EvsCamera::setMaxFramesInFlight(uint32_t bufferCount) {
    ALOGD("setMaxFramesInFlight");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // TODO:  Update our stored value

    // TODO:  Adjust our buffer count right now if we can.  Otherwise, it'll adjust in doneWithFrame

    // For now we support only one!
    if (bufferCount != 1) {
        return EvsResult::BUFFER_NOT_AVAILABLE;
    }

    return EvsResult::OK;
}

Return<EvsResult> EvsCamera::startVideoStream(const ::android::sp<IEvsCameraStream>& stream)  {
    ALOGD("startVideoStream");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // We only support a single stream at a time
    if (mStreamState != STOPPED) {
        ALOGE("ignoring startVideoStream call when a stream is already running.");
        return EvsResult::STREAM_ALREADY_RUNNING;
    }

    // Record the user's callback for use when we have a frame ready
    mStream = stream;

    // Allocate a graphics buffer into which we'll put our test images
    if (!mBuffer) {
        mWidth  = (mDescription.defaultHorResolution) ? mDescription.defaultHorResolution : 640;
        mHeight = (mDescription.defaultVerResolution) ? mDescription.defaultVerResolution : 480;
        // TODO:  What about stride?  Assume no padding for now...
        mStride = 4* mWidth;    // Special cased to assume 4 byte pixels with no padding for now

        ALOGD("Allocating buffer for camera frame");
        GraphicBufferAllocator &alloc(GraphicBufferAllocator::get());
        status_t result = alloc.allocate(mWidth, mHeight,
                                         HAL_PIXEL_FORMAT_RGBA_8888, 1, GRALLOC_USAGE_HW_TEXTURE,
                                         &mBuffer, &mStride, 0, "EvsCamera");
        if (result != NO_ERROR) {
            ALOGE("Error %d allocating %d x %d graphics buffer", result, mWidth, mHeight);
            return EvsResult::BUFFER_NOT_AVAILABLE;
        }
        if (!mBuffer) {
            ALOGE("We didn't get a buffer handle back from the allocator");
            return EvsResult::BUFFER_NOT_AVAILABLE;
        }
    }

    // Start the frame generation thread
    mStreamState = RUNNING;
    mCaptureThread = std::thread([this](){GenerateFrames();});

    return EvsResult::OK;
}

Return<EvsResult> EvsCamera::doneWithFrame(uint32_t frameId, const hidl_handle& bufferHandle)  {
    ALOGD("doneWithFrame");
    std::lock_guard<std::mutex> lock(mAccessLock);

    if (!bufferHandle)
    {
        ALOGE("ignoring doneWithFrame called with invalid handle");
        return EvsResult::INVALID_ARG;
    }

    // TODO:  Track which frames we've delivered and validate this is one of them

    // Mark the frame buffer as available for a new frame
    mFrameBusy = false;

    // TODO:  If we currently have too many buffers, drop this one

    return EvsResult::OK;
}

Return<void> EvsCamera::stopVideoStream()  {
    ALOGD("stopVideoStream");

    bool waitForJoin = false;
    // Lock scope
    {
        std::lock_guard <std::mutex> lock(mAccessLock);

        if (mStreamState == RUNNING) {
            // Tell the GenerateFrames loop we want it to stop
            mStreamState = STOPPING;

            // Note that we asked the thread to stop and should wait for it do so
            waitForJoin = true;
        }
    }

    if (waitForJoin) {
        // Block outside the mutex until the "stop" flag has been acknowledged
        // NOTE:  We won't send any more frames, but the client might still get one already in flight
        ALOGD("Waiting for stream thread to end...");
        mCaptureThread.join();

        // Lock scope
        {
            std::lock_guard <std::mutex> lock(mAccessLock);
            mStreamState = STOPPED;
        }
    }

    return Void();
}

Return<int32_t> EvsCamera::getExtendedInfo(uint32_t opaqueIdentifier)  {
    ALOGD("getExtendedInfo");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // For any single digit value, return the index itself as a test value
    if (opaqueIdentifier <= 9) {
        return opaqueIdentifier;
    }

    // Return zero by default as required by the spec
    return 0;
}

Return<EvsResult> EvsCamera::setExtendedInfo(uint32_t /*opaqueIdentifier*/, int32_t /*opaqueValue*/)  {
    ALOGD("setExtendedInfo");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // We don't store any device specific information in this implementation
    return EvsResult::INVALID_ARG;
}


void EvsCamera::GenerateFrames() {
    ALOGD("Frame generate loop started");

    uint32_t frameNumber;

    while (true) {
        bool timeForFrame = false;
        // Lock scope
        {
            std::lock_guard<std::mutex> lock(mAccessLock);

            // Tick the frame counter -- rollover is tolerated
            frameNumber = mFrameId++;

            if (mStreamState != RUNNING) {
                // Break out of our main thread loop
                break;
            }

            if (mFrameBusy) {
                // Can't do anything right now -- skip this frame
                ALOGW("Skipped a frame because client hasn't returned a buffer\n");
            }
            else {
                // We're going to make the frame busy
                mFrameBusy = true;
                timeForFrame = true;
            }
        }

        if (timeForFrame) {
            // Lock our output buffer for writing
            uint32_t *pixels = nullptr;
            GraphicBufferMapper &mapper = GraphicBufferMapper::get();
            mapper.lock(mBuffer,
                        GRALLOC_USAGE_SW_WRITE_OFTEN,
                        android::Rect(mWidth, mHeight),
                        (void **) &pixels);

            // If we failed to lock the pixel buffer, we're about to crash, but log it first
            if (!pixels) {
                ALOGE("Camera failed to gain access to image buffer for writing");
            }

            // Fill in the test pixels
            for (unsigned row = 0; row < mHeight; row++) {
                for (unsigned col = 0; col < mWidth; col++) {
                    // Index into the row to set the pixel at this column
                    // (We're making vertical gradient in the green channel, and
                    // horitzontal gradient in the blue channel)
                    pixels[col] = 0xFF0000FF | ((row & 0xFF) << 16) | ((col & 0xFF) << 8);
                }
                // Point to the next row
                pixels = pixels + (mStride / sizeof(*pixels));
            }

            // Release our output buffer
            mapper.unlock(mBuffer);

            // Issue the (asynchronous) callback to the client
            mStream->deliverFrame(frameNumber, mBuffer);
            ALOGD("Delivered %p as frame %d", mBuffer, frameNumber);
        }

        // We arbitrarily choose to generate frames at 10 fps (1/10 * uSecPerSec)
        usleep(100000);
    }

    // If we've been asked to stop, send one last NULL frame to signal the actual end of stream
    mStream->deliverFrame(frameNumber, nullptr);

    return;
}

} // namespace implementation
} // namespace V1_0
} // namespace evs
} // namespace hardware
} // namespace android
