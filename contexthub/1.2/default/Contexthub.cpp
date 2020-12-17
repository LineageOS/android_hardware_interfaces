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

using ::android::hardware::contexthub::V1_0::Result;
using ::android::hardware::contexthub::V1_X::implementation::IContextHubCallbackWrapperV1_0;
using ::android::hardware::contexthub::V1_X::implementation::IContextHubCallbackWrapperV1_2;

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

// We don't expose any nanoapps, therefore all nanoapp-related API calls return with BAD_PARAMS
Return<Result> Contexthub::sendMessageToHub_1_2(uint32_t /* hubId */,
                                                const ContextHubMsg& /* msg */) {
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
