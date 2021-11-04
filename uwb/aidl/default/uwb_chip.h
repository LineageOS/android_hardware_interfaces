/*
 * Copyright 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_UWB_UWBCHIP
#define ANDROID_HARDWARE_UWB_UWBCHIP

#include <vector>

#include <aidl/android/hardware/uwb/BnUwbChip.h>
#include <aidl/android/hardware/uwb/IUwbClientCallback.h>

namespace android {
namespace hardware {
namespace uwb {
namespace impl {
using namespace ::aidl::android::hardware::uwb;
// Default implementation mean't to be used on simulator targets.
class UwbChip : public BnUwbChip {
  public:
    UwbChip(const std::string& name);
    virtual ~UwbChip();

    ::ndk::ScopedAStatus getName(std::string* name) override;
    ::ndk::ScopedAStatus open(const std::shared_ptr<IUwbClientCallback>& clientCallback) override;
    ::ndk::ScopedAStatus close() override;
    ::ndk::ScopedAStatus coreInit() override;
    ::ndk::ScopedAStatus getSupportedAndroidUciVersion(int32_t* version) override;
    ::ndk::ScopedAStatus getSupportedAndroidCapabilities(int64_t* capabilities) override;
    ::ndk::ScopedAStatus sendUciMessage(const std::vector<uint8_t>& data,
                                        int32_t* bytes_written) override;

  private:
    std::string name_;
    std::shared_ptr<IUwbClientCallback> mClientCallback;
};
}  // namespace impl
}  // namespace uwb
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_UWB_UWBCHIP
