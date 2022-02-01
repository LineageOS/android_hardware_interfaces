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

#ifndef ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSDISPLAY_H
#define ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSDISPLAY_H

#include "GlWrapper.h"

#include <android/frameworks/automotive/display/1.0/IAutomotiveDisplayProxyService.h>
#include <android/hardware/automotive/evs/1.1/IEvsDisplay.h>
#include <ui/GraphicBuffer.h>

namespace android::hardware::automotive::evs::V1_1::implementation {

class EvsDisplay : public IEvsDisplay {
  public:
    // Methods from ::android::hardware::automotive::evs::V1_0::IEvsDisplay follow.
    Return<void> getDisplayInfo(getDisplayInfo_cb _hidl_cb) override;
    Return<V1_0::EvsResult> setDisplayState(V1_0::DisplayState state) override;
    Return<V1_0::DisplayState> getDisplayState() override;
    Return<void> getTargetBuffer(getTargetBuffer_cb _hidl_cb) override;
    Return<V1_0::EvsResult> returnTargetBufferForDisplay(const V1_0::BufferDesc& buffer) override;

    // Methods from ::android::hardware::automotive::evs::V1_1::IEvsDisplay follow.
    Return<void> getDisplayInfo_1_1(getDisplayInfo_1_1_cb _info_cb) override;

    // Implementation details
    EvsDisplay();
    EvsDisplay(
            sp<frameworks::automotive::display::V1_0::IAutomotiveDisplayProxyService> pDisplayProxy,
            uint64_t displayId);
    virtual ~EvsDisplay() override;

    void forceShutdown();  // This gets called if another caller "steals" ownership of the display
    Return<V1_0::EvsResult> returnTargetBufferForDisplayImpl(const uint32_t bufferId,
                                                             const buffer_handle_t memHandle);

  private:
    V1_0::DisplayDesc mInfo = {};
    V1_0::BufferDesc mBuffer = {};  // A graphics buffer into which we'll store images
    V1_0::DisplayState mRequestedState = V1_0::DisplayState::NOT_VISIBLE;
    bool mFrameBusy = false;  // A flag telling us our buffer is in use
    std::mutex mAccessLock;
    sp<frameworks::automotive::display::V1_0::IAutomotiveDisplayProxyService> mDisplayProxy;
    uint64_t mDisplayId;
    std::unique_ptr<GlWrapper> mGlWrapper;
};

}  // namespace android::hardware::automotive::evs::V1_1::implementation

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSDISPLAY_H
