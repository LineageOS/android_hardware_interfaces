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

#ifndef ANDROID_HARDWARE_CONTEXTHUB_V1_X_CONTEXTHUB_H
#define ANDROID_HARDWARE_CONTEXTHUB_V1_X_CONTEXTHUB_H

#include <android/hardware/contexthub/1.0/IContexthub.h>
#include <android/hardware/contexthub/1.0/types.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace contexthub {
namespace V1_X {
namespace implementation {

template <class IContextHubInterface>
struct ContextHub : public IContextHubInterface {
    using ContextHubMsg = ::android::hardware::contexthub::V1_0::ContextHubMsg;
    using HubAppInfo = ::android::hardware::contexthub::V1_0::HubAppInfo;
    using IContexthubCallback = ::android::hardware::contexthub::V1_0::IContexthubCallback;
    using NanoAppBinary = ::android::hardware::contexthub::V1_0::NanoAppBinary;
    using Result = ::android::hardware::contexthub::V1_0::Result;
    using getHubs_cb = ::android::hardware::contexthub::V1_0::IContexthub::getHubs_cb;

  public:
    Return<void> getHubs(getHubs_cb _hidl_cb) override {
        ::android::hardware::contexthub::V1_0::ContextHub hub = {};
        hub.name = "Mock Context Hub";
        hub.vendor = "AOSP";
        hub.toolchain = "n/a";
        hub.platformVersion = 1;
        hub.toolchainVersion = 1;
        hub.hubId = kMockHubId;
        hub.peakMips = 1;
        hub.peakPowerDrawMw = 1;
        hub.maxSupportedMsgLen = 4096;
        hub.chrePlatformId = UINT64_C(0x476f6f6754000000);
        hub.chreApiMajorVersion = 1;
        hub.chreApiMinorVersion = 4;

        // Report a single mock hub
        std::vector<::android::hardware::contexthub::V1_0::ContextHub> hubs;
        hubs.push_back(hub);

        _hidl_cb(hubs);
        return Void();
    }

    // We don't expose any nanoapps, therefore all nanoapp-related API calls return with BAD_PARAMS
    Return<Result> sendMessageToHub(uint32_t /*hubId*/, const ContextHubMsg& /*msg*/) override {
        return Result::BAD_PARAMS;
    }

    Return<Result> loadNanoApp(uint32_t /*hubId*/, const NanoAppBinary& /*appBinary*/,
                               uint32_t /*transactionId*/) override {
        return Result::BAD_PARAMS;
    }

    Return<Result> unloadNanoApp(uint32_t /*hubId*/, uint64_t /*appId*/,
                                 uint32_t /*transactionId*/) override {
        return Result::BAD_PARAMS;
    }

    Return<Result> enableNanoApp(uint32_t /*hubId*/, uint64_t /*appId*/,
                                 uint32_t /*transactionId*/) override {
        return Result::BAD_PARAMS;
    }

    Return<Result> disableNanoApp(uint32_t /*hubId*/, uint64_t /*appId*/,
                                  uint32_t /*transactionId*/) override {
        return Result::BAD_PARAMS;
    }

  protected:
    static constexpr uint32_t kMockHubId = 0;
};

}  // namespace implementation
}  // namespace V1_X
}  // namespace contexthub
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONTEXTHUB_V1_X_CONTEXTHUB_H
