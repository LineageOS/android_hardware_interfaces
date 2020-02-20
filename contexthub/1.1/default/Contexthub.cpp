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
namespace V1_1 {
namespace implementation {

using ::android::hardware::contexthub::V1_0::ContextHub;
using ::android::hardware::contexthub::V1_0::HubAppInfo;
using ::android::hardware::contexthub::V1_0::Result;

namespace {

constexpr uint32_t kMockHubId = 0;

}  // anonymous namespace

Return<void> Contexthub::getHubs(getHubs_cb _hidl_cb) {
    ContextHub hub = {};
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
    std::vector<ContextHub> hubs;
    hubs.push_back(hub);

    _hidl_cb(hubs);
    return Void();
}

Return<Result> Contexthub::registerCallback(uint32_t hubId, const sp<IContexthubCallback>& cb) {
    if (hubId == kMockHubId) {
        mCallback = cb;
        return Result::OK;
    }
    return Result::BAD_PARAMS;
}

// We don't expose any nanoapps, therefore all nanoapp-related API calls return with BAD_PARAMS
Return<Result> Contexthub::sendMessageToHub(uint32_t /*hubId*/, const ContextHubMsg& /*msg*/) {
    return Result::BAD_PARAMS;
}

Return<Result> Contexthub::loadNanoApp(uint32_t /*hubId*/, const NanoAppBinary& /*appBinary*/,
                                       uint32_t /*transactionId*/) {
    return Result::BAD_PARAMS;
}

Return<Result> Contexthub::unloadNanoApp(uint32_t /*hubId*/, uint64_t /*appId*/,
                                         uint32_t /*transactionId*/) {
    return Result::BAD_PARAMS;
}

Return<Result> Contexthub::enableNanoApp(uint32_t /*hubId*/, uint64_t /*appId*/,
                                         uint32_t /*transactionId*/) {
    return Result::BAD_PARAMS;
}

Return<Result> Contexthub::disableNanoApp(uint32_t /*hubId*/, uint64_t /*appId*/,
                                          uint32_t /*transactionId*/) {
    return Result::BAD_PARAMS;
}

Return<Result> Contexthub::queryApps(uint32_t hubId) {
    if (hubId == kMockHubId && mCallback != nullptr) {
        std::vector<HubAppInfo> nanoapps;
        mCallback->handleAppsInfo(nanoapps);
        return Result::OK;
    }
    return Result::BAD_PARAMS;
}

Return<void> Contexthub::onSettingChanged(Setting /*setting*/, SettingValue /*newValue*/) {
    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
