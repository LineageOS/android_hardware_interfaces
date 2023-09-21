/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "EvsGlDisplay.h"

#include <aidl/android/hardware/automotive/evs/EvsResult.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/thread_annotations.h>
#include <linux/time.h>
#include <ui/DisplayMode.h>
#include <ui/DisplayState.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>
#include <utils/SystemClock.h>

#include <chrono>

namespace {

using ::aidl::android::frameworks::automotive::display::ICarDisplayProxy;
using ::aidl::android::hardware::graphics::common::BufferUsage;
using ::aidl::android::hardware::graphics::common::PixelFormat;
using ::android::base::ScopedLockAssertion;
using ::ndk::ScopedAStatus;

constexpr auto kTimeout = std::chrono::seconds(1);

bool debugFirstFrameDisplayed = false;

int generateFingerPrint(buffer_handle_t handle) {
    return static_cast<int>(reinterpret_cast<long>(handle) & 0xFFFFFFFF);
}

}  // namespace

namespace aidl::android::hardware::automotive::evs::implementation {

EvsGlDisplay::EvsGlDisplay(const std::shared_ptr<ICarDisplayProxy>& pDisplayProxy,
                           uint64_t displayId)
    : mDisplayId(displayId), mDisplayProxy(pDisplayProxy) {
    LOG(DEBUG) << "EvsGlDisplay instantiated";

    // Set up our self description
    // NOTE:  These are arbitrary values chosen for testing
    mInfo.id = std::to_string(displayId);
    mInfo.vendorFlags = 3870;

    // Start a thread to render images on this display
    {
        std::lock_guard lock(mLock);
        mState = RUN;
    }
    mRenderThread = std::thread([this]() { renderFrames(); });
}

EvsGlDisplay::~EvsGlDisplay() {
    LOG(DEBUG) << "EvsGlDisplay being destroyed";
    forceShutdown();
}

/**
 * This gets called if another caller "steals" ownership of the display
 */
void EvsGlDisplay::forceShutdown() {
    LOG(DEBUG) << "EvsGlDisplay forceShutdown";
    {
        std::lock_guard lock(mLock);

        // If the buffer isn't being held by a remote client, release it now as an
        // optimization to release the resources more quickly than the destructor might
        // get called.
        if (mBuffer.handle != nullptr) {
            // Report if we're going away while a buffer is outstanding
            if (mBufferBusy || mState == RUN) {
                LOG(ERROR) << "EvsGlDisplay going down while client is holding a buffer";
            }
            mState = STOPPING;
        }

        // Put this object into an unrecoverable error state since somebody else
        // is going to own the display now.
        mRequestedState = DisplayState::DEAD;
    }
    mBufferReadyToRender.notify_all();

    if (mRenderThread.joinable()) {
        mRenderThread.join();
    }
}

/**
 * Initialize GL in the context of a caller's thread and prepare a graphic
 * buffer to use.
 */
bool EvsGlDisplay::initializeGlContextLocked() {
    // Initialize our display window
    // NOTE:  This will cause the display to become "VISIBLE" before a frame is actually
    // returned, which is contrary to the spec and will likely result in a black frame being
    // (briefly) shown.
    if (!mGlWrapper.initialize(mDisplayProxy, mDisplayId)) {
        // Report the failure
        LOG(ERROR) << "Failed to initialize GL display";
        return false;
    }

    // Assemble the buffer description we'll use for our render target
    static_assert(::aidl::android::hardware::graphics::common::PixelFormat::RGBA_8888 ==
                  static_cast<::aidl::android::hardware::graphics::common::PixelFormat>(
                          HAL_PIXEL_FORMAT_RGBA_8888));
    mBuffer.description = {
            .width = static_cast<int>(mGlWrapper.getWidth()),
            .height = static_cast<int>(mGlWrapper.getHeight()),
            .layers = 1,
            .format = PixelFormat::RGBA_8888,
            // FIXME: Below line is not using
            // ::aidl::android::hardware::graphics::common::BufferUsage because
            // BufferUsage enum does not support a bitwise-OR operation; they
            // should be BufferUsage::GPU_RENDER_TARGET |
            // BufferUsage::COMPOSER_OVERLAY
            .usage = static_cast<BufferUsage>(GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER),
    };

    ::android::GraphicBufferAllocator& alloc(::android::GraphicBufferAllocator::get());
    uint32_t stride = static_cast<uint32_t>(mBuffer.description.stride);
    buffer_handle_t handle = nullptr;
    const ::android::status_t result =
            alloc.allocate(mBuffer.description.width, mBuffer.description.height,
                           static_cast<::android::PixelFormat>(mBuffer.description.format),
                           mBuffer.description.layers,
                           static_cast<uint64_t>(mBuffer.description.usage), &handle, &stride,
                           /* requestorName= */ "EvsGlDisplay");
    mBuffer.description.stride = stride;
    mBuffer.fingerprint = generateFingerPrint(mBuffer.handle);
    if (result != ::android::NO_ERROR) {
        LOG(ERROR) << "Error " << result << " allocating " << mBuffer.description.width << " x "
                   << mBuffer.description.height << " graphics buffer.";
        mGlWrapper.shutdown();
        return false;
    }

    mBuffer.handle = handle;
    if (mBuffer.handle == nullptr) {
        LOG(ERROR) << "We didn't get a buffer handle back from the allocator";
        mGlWrapper.shutdown();
        return false;
    }

    LOG(DEBUG) << "Allocated new buffer " << mBuffer.handle << " with stride "
               << mBuffer.description.stride;
    return true;
}

/**
 * This method runs in a separate thread and renders the contents of the buffer.
 */
void EvsGlDisplay::renderFrames() {
    {
        std::lock_guard lock(mLock);

        if (!initializeGlContextLocked()) {
            LOG(ERROR) << "Failed to initialize GL context";
            return;
        }

        // Display buffer is ready.
        mBufferBusy = false;
    }
    mBufferReadyToUse.notify_all();

    while (true) {
        {
            std::unique_lock lock(mLock);
            ScopedLockAssertion lock_assertion(mLock);
            mBufferReadyToRender.wait(
                    lock, [this]() REQUIRES(mLock) { return mBufferReady || mState != RUN; });
            if (mState != RUN) {
                LOG(DEBUG) << "A rendering thread is stopping";
                break;
            }
            mBufferReady = false;
        }

        // Update the texture contents with the provided data
        if (!mGlWrapper.updateImageTexture(mBuffer.handle, mBuffer.description)) {
            LOG(WARNING) << "Failed to update the image texture";
            continue;
        }

        // Put the image on the screen
        mGlWrapper.renderImageToScreen();
        if (!debugFirstFrameDisplayed) {
            LOG(DEBUG) << "EvsFirstFrameDisplayTiming start time: " << ::android::elapsedRealtime()
                       << " ms.";
            debugFirstFrameDisplayed = true;
        }

        // Mark current frame is consumed.
        {
            std::lock_guard lock(mLock);
            mBufferBusy = false;
        }
        mBufferDone.notify_all();
    }

    LOG(DEBUG) << "A rendering thread is stopped.";

    // Drop the graphics buffer we've been using
    ::android::GraphicBufferAllocator& alloc(::android::GraphicBufferAllocator::get());
    alloc.free(mBuffer.handle);
    mBuffer.handle = nullptr;

    mGlWrapper.hideWindow(mDisplayProxy, mDisplayId);
    mGlWrapper.shutdown();

    std::lock_guard lock(mLock);
    mState = STOPPED;
}

/**
 * Returns basic information about the EVS display provided by the system.
 * See the description of the DisplayDesc structure for details.
 */
ScopedAStatus EvsGlDisplay::getDisplayInfo(DisplayDesc* _aidl_return) {
    if (!mDisplayProxy) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::UNDERLYING_SERVICE_ERROR));
    }

    ::aidl::android::frameworks::automotive::display::DisplayDesc proxyDisplay;
    auto status = mDisplayProxy->getDisplayInfo(mDisplayId, &proxyDisplay);
    if (!status.isOk()) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::UNDERLYING_SERVICE_ERROR));
    }

    _aidl_return->width = proxyDisplay.width;
    _aidl_return->height = proxyDisplay.height;
    _aidl_return->orientation = static_cast<Rotation>(proxyDisplay.orientation);
    _aidl_return->id = mInfo.id;  // FIXME: what should be ID here?
    _aidl_return->vendorFlags = mInfo.vendorFlags;
    return ::ndk::ScopedAStatus::ok();
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
ScopedAStatus EvsGlDisplay::setDisplayState(DisplayState state) {
    LOG(DEBUG) << __FUNCTION__;
    std::lock_guard lock(mLock);

    if (mRequestedState == DisplayState::DEAD) {
        // This object no longer owns the display -- it's been superceeded!
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::OWNERSHIP_LOST));
    }

    // Ensure we recognize the requested state so we don't go off the rails
    static constexpr ::ndk::enum_range<DisplayState> kDisplayStateRange;
    if (std::find(kDisplayStateRange.begin(), kDisplayStateRange.end(), state) ==
        kDisplayStateRange.end()) {
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }

    switch (state) {
        case DisplayState::NOT_VISIBLE:
            mGlWrapper.hideWindow(mDisplayProxy, mDisplayId);
            break;
        case DisplayState::VISIBLE:
            mGlWrapper.showWindow(mDisplayProxy, mDisplayId);
            break;
        default:
            break;
    }

    // Record the requested state
    mRequestedState = state;

    return ScopedAStatus::ok();
}

/**
 * The HAL implementation should report the actual current state, which might
 * transiently differ from the most recently requested state.  Note, however, that
 * the logic responsible for changing display states should generally live above
 * the device layer, making it undesirable for the HAL implementation to
 * spontaneously change display states.
 */
ScopedAStatus EvsGlDisplay::getDisplayState(DisplayState* _aidl_return) {
    LOG(DEBUG) << __FUNCTION__;
    std::lock_guard lock(mLock);
    *_aidl_return = mRequestedState;
    return ScopedAStatus::ok();
}

/**
 * This call returns a handle to a frame buffer associated with the display.
 * This buffer may be locked and written to by software and/or GL.  This buffer
 * must be returned via a call to returnTargetBufferForDisplay() even if the
 * display is no longer visible.
 */
ScopedAStatus EvsGlDisplay::getTargetBuffer(BufferDesc* _aidl_return) {
    LOG(DEBUG) << __FUNCTION__;
    std::unique_lock lock(mLock);
    ScopedLockAssertion lock_assertion(mLock);
    if (mRequestedState == DisplayState::DEAD) {
        LOG(ERROR) << "Rejecting buffer request from object that lost ownership of the display.";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::OWNERSHIP_LOST));
    }

    // If we don't already have a buffer, allocate one now
    // mBuffer.memHandle is a type of buffer_handle_t, which is equal to
    // native_handle_t*.
    mBufferReadyToUse.wait(lock, [this]() REQUIRES(mLock) { return !mBufferBusy; });

    // Do we have a frame available?
    if (mBufferBusy) {
        // This means either we have a 2nd client trying to compete for buffers
        // (an unsupported mode of operation) or else the client hasn't returned
        // a previously issued buffer yet (they're behaving badly).
        // NOTE:  We have to make the callback even if we have nothing to provide
        LOG(ERROR) << "getTargetBuffer called while no buffers available.";
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::BUFFER_NOT_AVAILABLE));
    }

    // Mark our buffer as busy
    mBufferBusy = true;

    // Send the buffer to the client
    LOG(VERBOSE) << "Providing display buffer handle " << mBuffer.handle;

    BufferDesc bufferDescToSend = {
            .buffer =
                    {
                            .description = mBuffer.description,
                            .handle = std::move(::android::dupToAidl(mBuffer.handle)),
                    },
            .pixelSizeBytes = 4,  // RGBA_8888 is 4-byte-per-pixel format
            .bufferId = mBuffer.fingerprint,
    };
    *_aidl_return = std::move(bufferDescToSend);

    return ScopedAStatus::ok();
}

/**
 * This call tells the display that the buffer is ready for display.
 * The buffer is no longer valid for use by the client after this call.
 */
ScopedAStatus EvsGlDisplay::returnTargetBufferForDisplay(const BufferDesc& buffer) {
    LOG(VERBOSE) << __FUNCTION__;
    std::unique_lock lock(mLock);
    ScopedLockAssertion lock_assertion(mLock);

    // Nobody should call us with a null handle
    if (buffer.buffer.handle.fds.size() < 1) {
        LOG(ERROR) << __FUNCTION__ << " called without a valid buffer handle.";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }
    if (buffer.bufferId != mBuffer.fingerprint) {
        LOG(ERROR) << "Got an unrecognized frame returned.";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }
    if (!mBufferBusy) {
        LOG(ERROR) << "A frame was returned with no outstanding frames.";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }

    // If we've been displaced by another owner of the display, then we can't do anything else
    if (mRequestedState == DisplayState::DEAD) {
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::OWNERSHIP_LOST));
    }

    // If we were waiting for a new frame, this is it!
    if (mRequestedState == DisplayState::VISIBLE_ON_NEXT_FRAME) {
        mRequestedState = DisplayState::VISIBLE;
        mGlWrapper.showWindow(mDisplayProxy, mDisplayId);
    }

    // Validate we're in an expected state
    if (mRequestedState != DisplayState::VISIBLE) {
        // Not sure why a client would send frames back when we're not visible.
        LOG(WARNING) << "Got a frame returned while not visible - ignoring.";
        return ScopedAStatus::ok();
    }
    mBufferReady = true;
    mBufferReadyToRender.notify_all();

    if (!mBufferDone.wait_for(lock, kTimeout, [this]() REQUIRES(mLock) { return !mBufferBusy; })) {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::UNDERLYING_SERVICE_ERROR));
    }

    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::automotive::evs::implementation
