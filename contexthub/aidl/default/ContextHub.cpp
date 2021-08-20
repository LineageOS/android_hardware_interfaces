/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "contexthub-impl/ContextHub.h"

namespace aidl {
namespace android {
namespace hardware {
namespace contexthub {

// TODO(b/194285834): Implement AIDL HAL

::ndk::ScopedAStatus ContextHub::getContextHubs(
        std::vector<ContextHubInfo>* /* out_contextHubInfos */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::loadNanoapp(int32_t /* in_contextHubId */,
                                             const NanoappBinary& /* in_appBinary */,
                                             int32_t /* in_transactionId */,
                                             bool* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::unloadNanoapp(int32_t /* in_contextHubId */,
                                               int64_t /* in_appId */,
                                               int32_t /* in_transactionId */,
                                               bool* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::disableNanoapp(int32_t /* in_contextHubId */,
                                                int64_t /* in_appId */,
                                                int32_t /* in_transactionId */,
                                                bool* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::enableNanoapp(int32_t /* in_contextHubId */,
                                               int64_t /* in_appId */,
                                               int32_t /* in_transactionId */,
                                               bool* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::onSettingChanged(Setting /* in_setting */, bool /*in_enabled */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::queryNanoapps(int32_t /* in_contextHubId */,
                                               bool* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::registerCallback(
        int32_t /* in_contextHubId */, const std::shared_ptr<IContextHubCallback>& /* in_cb */,
        bool* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::sendMessageToHub(int32_t /* in_contextHubId */,
                                                  const ContextHubMessage& /* in_message */,
                                                  bool* /* _aidl_return */) {
    return ndk::ScopedAStatus::ok();
}

}  // namespace contexthub
}  // namespace hardware
}  // namespace android
}  // namespace aidl
