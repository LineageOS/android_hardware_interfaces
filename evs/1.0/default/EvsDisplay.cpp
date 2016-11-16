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

#include "EvsDisplay.h"

#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>


namespace android {
namespace hardware {
namespace evs {
namespace V1_0 {
namespace implementation {


// TODO(b/31632518):  Need to get notification when our client dies so we can close the camera.
// As it stands, if the client dies suddently, the buffer may be stranded.
// As possible work around would be to give the client a HIDL object to exclusively hold
// and use it's destructor to perform some work in the server side.


EvsDisplay::EvsDisplay() {
    ALOGD("EvsDisplay instantiated");

    // Set up our self description
    mInfo.displayId             = "Mock Display";
    mInfo.vendorFlags           = 3870;
    mInfo.defaultHorResolution  = 320;
    mInfo.defaultVerResolution  = 240;
}


EvsDisplay::~EvsDisplay() {
    ALOGD("EvsDisplay being destroyed");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // Report if we're going away while a buffer is outstanding.  This could be bad.
    if (mFrameBusy) {
        ALOGE("EvsDisplay going down while client is holding a buffer\n");
    }

    // Make sure we release our frame buffer
    if (mBuffer) {
        // Drop the graphics buffer we've been using
        GraphicBufferAllocator& alloc(GraphicBufferAllocator::get());
        alloc.free(mBuffer);
    }
    ALOGD("EvsDisplay destroyed");
}


/**
 * Returns basic information about the EVS display provided by the system.
 * See the description of the DisplayDesc structure below for details.
 */
Return<void> EvsDisplay::getDisplayInfo(getDisplayInfo_cb _hidl_cb)  {
    ALOGD("getDisplayInfo");

    // Send back our self description
    _hidl_cb(mInfo);
    return Void();
}


/**
 * Clients may set the display state to express their desired state.
 * The HAL implementation must gracefully accept a request for any state
 * while in any other state, although the response may be to ignore the request.
 * The display is defined to start in the NOT_VISIBLE state upon initialization.
 * The client is then expected to request the VISIBLE_ON_NEXT_FRAME state, and
 * then begin providing video.  When the display is no longer required, the client
 * is expected to request the NOT_VISIBLE state after passing the last video frame.
 */
Return<EvsResult> EvsDisplay::setDisplayState(DisplayState state) {
    ALOGD("setDisplayState");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // Ensure we recognize the requested state so we don't go off the rails
    if (state < DisplayState::NUM_STATES) {
        // Record the requested state
        mRequestedState = state;
        return EvsResult::OK;
    }
    else {
        // Turn off the display if asked for an unrecognized state
        mRequestedState = DisplayState::NOT_VISIBLE;
        return EvsResult::INVALID_ARG;
    }
}


/**
 * The HAL implementation should report the actual current state, which might
 * transiently differ from the most recently requested state.  Note, however, that
 * the logic responsible for changing display states should generally live above
 * the device layer, making it undesirable for the HAL implementation to
 * spontaneously change display states.
 */
Return<DisplayState> EvsDisplay::getDisplayState()  {
    ALOGD("getDisplayState");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // At the moment, we treat the requested state as immediately active
    DisplayState currentState = mRequestedState;

    return currentState;
}


/**
 * This call returns a handle to a frame buffer associated with the display.
 * This buffer may be locked and written to by software and/or GL.  This buffer
 * must be returned via a call to returnTargetBufferForDisplay() even if the
 * display is no longer visible.
 */
// TODO: We need to know if/when our client dies so we can get the buffer back! (blocked b/31632518)
Return<void> EvsDisplay::getTargetBuffer(getTargetBuffer_cb _hidl_cb)  {
    ALOGD("getTargetBuffer");
    std::lock_guard<std::mutex> lock(mAccessLock);

    // If we don't already have a buffer, allocate one now
    if (!mBuffer) {
        GraphicBufferAllocator& alloc(GraphicBufferAllocator::get());
        status_t result = alloc.allocate(mInfo.defaultHorResolution, mInfo.defaultVerResolution,
                                         HAL_PIXEL_FORMAT_RGBA_8888, 1,
                                         GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_COMPOSER,
                                         &mBuffer, &mStride, 0, "EvsDisplay");
        mFrameBusy = false;
        ALOGD("Allocated new buffer %p with stride %u", mBuffer, mStride);
    }

    // Do we have a frame available?
    if (mFrameBusy) {
        // This means either we have a 2nd client trying to compete for buffers
        // (an unsupported mode of operation) or else the client hasn't returned
        // a previously issues buffer yet (they're behaving badly).
        // NOTE:  We have to make callback even if we have nothing to provide
        ALOGE("getTargetBuffer called while no buffers available.");
        _hidl_cb(nullptr);
    }
    else {
        // Mark our buffer as busy
        mFrameBusy = true;

        // Send the buffer to the client
        ALOGD("Providing display buffer %p", mBuffer);
        _hidl_cb(mBuffer);
    }

    // All done
    return Void();
}


/**
 * This call tells the display that the buffer is ready for display.
 * The buffer is no longer valid for use by the client after this call.
 */
Return<EvsResult> EvsDisplay::returnTargetBufferForDisplay(const hidl_handle& bufferHandle)  {
    ALOGD("returnTargetBufferForDisplay %p", bufferHandle.getNativeHandle());
    std::lock_guard<std::mutex> lock(mAccessLock);

    // This shouldn't happen if we haven't issued the buffer!
    if (!bufferHandle) {
        ALOGE ("returnTargetBufferForDisplay called without a valid buffer handle.\n");
        return EvsResult::INVALID_ARG;
    }
    /* TODO(b/33492405): It would be nice to validate we got back the buffer we expect,
     * but HIDL doesn't support that (yet?)
    if (bufferHandle != mBuffer) {
        ALOGE ("Got an unrecognized frame returned.\n");
        return EvsResult::INVALID_ARG;
    }
    */
    if (!mFrameBusy) {
        ALOGE ("A frame was returned with no outstanding frames.\n");
        return EvsResult::BUFFER_NOT_AVAILABLE;
    }

    mFrameBusy = false;

    // If we were waiting for a new frame, this is it!
    if (mRequestedState == DisplayState::VISIBLE_ON_NEXT_FRAME) {
        mRequestedState = DisplayState::VISIBLE;
    }

    // Validate we're in an expected state
    if (mRequestedState != DisplayState::VISIBLE) {
        // We shouldn't get frames back when we're not visible.
        ALOGE ("Got an unexpected frame returned while not visible - ignoring.\n");
    }
    else {
        // Make this buffer visible
        // TODO:  Add code to put this image on the screen (or validate it somehow?)
    }

    return EvsResult::OK;
}

} // namespace implementation
} // namespace V1_0
} // namespace evs
} // namespace hardware
} // namespace android
