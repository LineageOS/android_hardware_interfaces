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

#include <aidl/android/hardware/tv/hdmi/earc/BnEArc.h>
#include <aidl/android/hardware/tv/hdmi/earc/Result.h>
#include <algorithm>
#include <vector>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace hdmi {
namespace earc {
namespace implementation {

using ::aidl::android::hardware::tv::hdmi::earc::BnEArc;
using ::aidl::android::hardware::tv::hdmi::earc::IEArc;
using ::aidl::android::hardware::tv::hdmi::earc::IEArcCallback;
using ::aidl::android::hardware::tv::hdmi::earc::IEArcStatus;
using ::aidl::android::hardware::tv::hdmi::earc::Result;

struct EArcMock : public BnEArc {
    EArcMock();

    ::ndk::ScopedAStatus setEArcEnabled(bool in_enabled) override;
    ::ndk::ScopedAStatus isEArcEnabled(bool* _aidl_return) override;
    ::ndk::ScopedAStatus setCallback(const std::shared_ptr<IEArcCallback>& in_callback) override;
    ::ndk::ScopedAStatus getState(int32_t in_portId, IEArcStatus* _aidl_return) override;
    ::ndk::ScopedAStatus getLastReportedAudioCapabilities(
            int32_t in_portId, std::vector<uint8_t>* _aidl_return) override;
    ::ndk::ScopedAStatus reportCapabilities(const std::vector<uint8_t>& capabilities,
                                            int32_t portId);
    ::ndk::ScopedAStatus changeState(const IEArcStatus status, int32_t portId);

  private:
    static void* __threadLoop(void* data);
    void threadLoop();

  private:
    static void serviceDied(void* cookie);
    std::shared_ptr<IEArcCallback> mCallback;

    // Variables for the virtual EARC hal impl
    std::vector<std::vector<uint8_t>> mCapabilities;
    std::vector<IEArcStatus> mPortStatus;
    bool mEArcEnabled = true;

    // Port configuration
    int mTotalPorts = 1;

    // Testing variables
    pthread_t mThreadId = 0;

    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
};
}  // namespace implementation
}  // namespace earc
}  // Namespace hdmi
}  // Namespace tv
}  // namespace hardware
}  // namespace android
