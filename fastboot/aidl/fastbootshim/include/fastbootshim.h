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

#include <aidl/android/hardware/fastboot/BnFastboot.h>
#include <android/hardware/fastboot/1.1/IFastboot.h>

namespace aidl {
namespace android {
namespace hardware {
namespace fastboot {
// Shim that wraps HIDL IFastboot with AIDL BnFastboot
class FastbootShim : public BnFastboot {
    using HidlFastboot = ::android::hardware::fastboot::V1_1::IFastboot;

  public:
    explicit FastbootShim(const ::android::sp<HidlFastboot>& service);
    ::ndk::ScopedAStatus doOemCommand(const std::string& in_oemCmd,
                                      std::string* _aidl_return) override;
    ::ndk::ScopedAStatus doOemSpecificErase() override;
    ::ndk::ScopedAStatus getBatteryVoltageFlashingThreshold(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getOffModeChargeState(bool* _aidl_return) override;
    ::ndk::ScopedAStatus getPartitionType(
            const std::string& in_partitionName,
            ::aidl::android::hardware::fastboot::FileSystemType* _aidl_return) override;
    ::ndk::ScopedAStatus getVariant(std::string* _aidl_return) override;

  private:
    ::android::sp<HidlFastboot> service_;
};

}  // namespace fastboot
}  // namespace hardware
}  // namespace android
}  // namespace aidl
