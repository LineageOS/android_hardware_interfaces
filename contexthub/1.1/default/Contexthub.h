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

#include <android/hardware/contexthub/1.1/IContexthub.h>

namespace android {
namespace hardware {
namespace contexthub {
namespace V1_1 {
namespace implementation {

class Contexthub : public V1_1::IContexthub {
    using ContextHubMsg = ::android::hardware::contexthub::V1_0::ContextHubMsg;
    using IContexthubCallback = ::android::hardware::contexthub::V1_0::IContexthubCallback;
    using NanoAppBinary = ::android::hardware::contexthub::V1_0::NanoAppBinary;
    using Result = ::android::hardware::contexthub::V1_0::Result;

  public:
    // Methods from V1_0::IContexthub
    Return<void> getHubs(getHubs_cb _hidl_cb) override;
    Return<Result> registerCallback(uint32_t hubId,
                                    const ::android::sp<IContexthubCallback>& cb) override;
    Return<Result> sendMessageToHub(uint32_t hubId, const ContextHubMsg& msg) override;
    Return<Result> loadNanoApp(uint32_t hubId, const NanoAppBinary& appBinary,
                               uint32_t transactionId) override;
    Return<Result> unloadNanoApp(uint32_t hubId, uint64_t appId, uint32_t transactionId) override;
    Return<Result> enableNanoApp(uint32_t hubId, uint64_t appId, uint32_t transactionId) override;
    Return<Result> disableNanoApp(uint32_t hubId, uint64_t appId, uint32_t transactionId) override;
    Return<Result> queryApps(uint32_t hubId) override;

    // Methods from V1_1::IContexthub
    Return<void> onSettingChanged(Setting setting, SettingValue newValue) override;

  private:
    sp<IContexthubCallback> mCallback;
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
