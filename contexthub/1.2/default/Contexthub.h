/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "ContextHub.h"
#include "IContextHubCallbackWrapper.h"

#include <android/hardware/contexthub/1.2/IContexthub.h>

namespace android {
namespace hardware {
namespace contexthub {
namespace V1_2 {
namespace implementation {

class Contexthub
    : public ::android::hardware::contexthub::V1_X::implementation::ContextHub<IContexthub> {
    using ContextHubMsg = ::android::hardware::contexthub::V1_2::ContextHubMsg;
    using IContexthubCallback = ::android::hardware::contexthub::V1_2::IContexthubCallback;
    using IContextHubCallbackWrapperBase =
            ::android::hardware::contexthub::V1_X::implementation::IContextHubCallbackWrapperBase;
    using Result = ::android::hardware::contexthub::V1_0::Result;
    using SettingValue = ::android::hardware::contexthub::V1_1::SettingValue;
    using SettingV1_1 = ::android::hardware::contexthub::V1_1::Setting;
    using getHubs_1_2_cb = ::android::hardware::contexthub::V1_2::IContexthub::getHubs_1_2_cb;

  public:
    // Methods from V1_0::IContexthub
    Return<Result> registerCallback(uint32_t hubId,
                                    const sp<V1_0::IContexthubCallback>& cb) override;

    Return<Result> queryApps(uint32_t hubId) override;

    // Methods from V1_1::IContexthub
    Return<void> onSettingChanged(SettingV1_1 setting, SettingValue newValue) override;

    // Methods from V1_2::IContexthub
    Return<void> getHubs_1_2(getHubs_1_2_cb _hidl_cb) override;

    Return<void> onSettingChanged_1_2(Setting setting, SettingValue newValue) override;

    Return<Result> registerCallback_1_2(uint32_t hubId,
                                        const sp<V1_2::IContexthubCallback>& cb) override;

  private:
    sp<IContextHubCallbackWrapperBase> mCallback;
};

}  // namespace implementation
}  // namespace V1_2
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
