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
#include "Contexthub.h"

#include <vector>

namespace android {
namespace hardware {
namespace contexthub {
namespace V1_2 {
namespace implementation {

using ::android::hardware::hidl_string;
using ::android::hardware::contexthub::V1_0::Result;
using ::android::hardware::contexthub::V1_X::implementation::IContextHubCallbackWrapperV1_0;
using ::android::hardware::contexthub::V1_X::implementation::IContextHubCallbackWrapperV1_2;

Return<void> Contexthub::getHubs_1_2(getHubs_1_2_cb _hidl_cb) {
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

    std::vector<hidl_string> hubPermissionList;

    _hidl_cb(hubs, hubPermissionList);
    return Void();
}

Return<Result> Contexthub::registerCallback(uint32_t hubId,
                                            const sp<V1_0::IContexthubCallback>& cb) {
    if (hubId == kMockHubId) {
        mCallback = new IContextHubCallbackWrapperV1_0(cb);
        return Result::OK;
    }
    return Result::BAD_PARAMS;
}

Return<Result> Contexthub::queryApps(uint32_t hubId) {
    if (hubId == kMockHubId && mCallback != nullptr) {
        std::vector<V1_2::HubAppInfo> nanoapps;
        mCallback->handleAppsInfo(nanoapps);
        return Result::OK;
    }
    return Result::BAD_PARAMS;
}

Return<Result> Contexthub::registerCallback_1_2(uint32_t hubId,
                                                const sp<V1_2::IContexthubCallback>& cb) {
    if (hubId == kMockHubId) {
        mCallback = new IContextHubCallbackWrapperV1_2(cb);
        return Result::OK;
    }
    return Result::BAD_PARAMS;
}

Return<void> Contexthub::onSettingChanged(SettingV1_1 /*setting*/, SettingValue /*newValue*/) {
    return Void();
}

Return<void> Contexthub::onSettingChanged_1_2(Setting /*setting*/, SettingValue /*newValue*/) {
    return Void();
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
