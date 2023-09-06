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

#pragma once

#include "GlWrapper.h"

#include <aidl/android/frameworks/automotive/display/ICarDisplayProxy.h>
#include <aidl/android/hardware/automotive/evs/BnEvsDisplay.h>
#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/DisplayDesc.h>
#include <aidl/android/hardware/automotive/evs/DisplayState.h>
#include <android-base/thread_annotations.h>

#include <thread>

namespace aidl::android::hardware::automotive::evs::implementation {

class EvsGlDisplay final : public BnEvsDisplay {
  public:
    // Methods from ::aidl::android::hardware::automotive::evs::IEvsDisplay
    // follow.
    ndk::ScopedAStatus getDisplayInfo(evs::DisplayDesc* _aidl_return) override;
    ndk::ScopedAStatus getDisplayState(evs::DisplayState* _aidl_return) override;
    ndk::ScopedAStatus getTargetBuffer(evs::BufferDesc* _aidl_return) override;
    ndk::ScopedAStatus returnTargetBufferForDisplay(const evs::BufferDesc& buffer) override;
    ndk::ScopedAStatus setDisplayState(evs::DisplayState state) override;

    // Implementation details
    EvsGlDisplay(const std::shared_ptr<automotivedisplay::ICarDisplayProxy>& service,
                 uint64_t displayId);
    virtual ~EvsGlDisplay() override;

    // This gets called if another caller "steals" ownership of the display
    void forceShutdown();

  private:
    // A graphics buffer into which we'll store images.  This member variable
    // will be protected by semaphores.
    struct BufferRecord {
        ::aidl::android::hardware::graphics::common::HardwareBufferDescription description;
        buffer_handle_t handle;
        int fingerprint;
    } mBuffer;

    // State of a rendering thread
    enum RenderThreadStates {
        STOPPED = 0,
        STOPPING = 1,
        RUN = 2,
    };

    uint64_t mDisplayId;
    evs::DisplayDesc mInfo;
    evs::DisplayState mRequestedState GUARDED_BY(mLock) = evs::DisplayState::NOT_VISIBLE;
    std::shared_ptr<automotivedisplay::ICarDisplayProxy> mDisplayProxy;

    GlWrapper mGlWrapper;
    mutable std::mutex mLock;

    // This tells us whether or not our buffer is in use.  Protected by
    // semaphores.
    bool mBufferBusy = false;

    // Variables to synchronize a rendering thread w/ main and binder threads
    std::thread mRenderThread;
    RenderThreadStates mState GUARDED_BY(mLock) = STOPPED;
    bool mBufferReady = false;
    void renderFrames();
    bool initializeGlContextLocked() REQUIRES(mLock);

    std::condition_variable mBufferReadyToUse;
    std::condition_variable mBufferReadyToRender;
    std::condition_variable mBufferDone;
};

}  // namespace aidl::android::hardware::automotive::evs::implementation
