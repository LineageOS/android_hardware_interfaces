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

using ::android::hardware::contexthub::V1_0::Result;

Return<void> Contexthub::onSettingChanged(Setting /*setting*/, SettingValue /*newValue*/) {
    return Void();
}

Return<Result> Contexthub::registerCallback(uint32_t hubId, const sp<IContexthubCallback>& cb) {
    if (hubId == kMockHubId) {
        mCallback = cb;
        return Result::OK;
    }
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

}  // namespace implementation
}  // namespace V1_1
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
