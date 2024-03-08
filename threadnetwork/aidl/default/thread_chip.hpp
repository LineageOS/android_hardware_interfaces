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

#pragma once

#include <aidl/android/hardware/threadnetwork/BnThreadChip.h>
#include <aidl/android/hardware/threadnetwork/IThreadChipCallback.h>

#include "lib/spinel/spinel_interface.hpp"
#include "lib/url/url.hpp"
#include "mainloop.hpp"

#include <android/binder_auto_utils.h>
#include <android/binder_ibinder.h>
#include <utils/Mutex.h>

namespace aidl {
namespace android {
namespace hardware {
namespace threadnetwork {

class ThreadChip : public BnThreadChip, ot::Posix::Mainloop::Source {
  public:
    ThreadChip(char* url);
    ~ThreadChip() {}

    ndk::ScopedAStatus open(const std::shared_ptr<IThreadChipCallback>& in_callback) override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus sendSpinelFrame(const std::vector<uint8_t>& in_frame) override;
    ndk::ScopedAStatus hardwareReset() override;
    void Update(otSysMainloopContext& context) override;
    void Process(const otSysMainloopContext& context) override;

  private:
    static void onBinderDiedJump(void* context);
    void onBinderDied(void);
    static void onBinderUnlinkedJump(void* context);
    void onBinderUnlinked(void);
    static void handleReceivedFrameJump(void* context);
    void handleReceivedFrame(void);
    ndk::ScopedAStatus errorStatus(int32_t error, const char* message);
    ndk::ScopedAStatus initChip(const std::shared_ptr<IThreadChipCallback>& in_callback);
    ndk::ScopedAStatus deinitChip();

    ot::Url::Url mUrl;
    std::shared_ptr<ot::Spinel::SpinelInterface> mSpinelInterface;
    ot::Spinel::SpinelInterface::RxFrameBuffer mRxFrameBuffer;
    std::shared_ptr<IThreadChipCallback> mCallback;
    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
};

}  // namespace threadnetwork
}  // namespace hardware
}  // namespace android
}  // namespace aidl
