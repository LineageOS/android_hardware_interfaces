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

using ::android::hardware::contexthub::V1_0::Result;

namespace android {
namespace hardware {
namespace contexthub {
namespace V1_2 {
namespace implementation {

// TODO(b/166846988): Implement new methods.
Return<Result> Contexthub::registerCallback_1_2(uint32_t /* hubId */,
                                                const sp<IContexthubCallback>& /* cb */) {
    return Result::UNKNOWN_FAILURE;
}

Return<Result> Contexthub::sendMessageToHub_1_2(uint32_t /* hubId */,
                                                const ContextHubMsg& /* msg */) {
    return Result::UNKNOWN_FAILURE;
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
