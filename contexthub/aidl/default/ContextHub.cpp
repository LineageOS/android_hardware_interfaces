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

::ndk::ScopedAStatus ContextHub::getContextHubs(std::vector<ContextHubInfo>* out_contextHubInfos) {
    ContextHubInfo hub = {};
    hub.name = "Mock Context Hub";
    hub.vendor = "AOSP";
    hub.toolchain = "n/a";
    hub.id = kMockHubId;
    hub.peakMips = 1;
    hub.maxSupportedMessageLengthBytes = 4096;
    hub.chrePlatformId = UINT64_C(0x476f6f6754000000);
    hub.chreApiMajorVersion = 1;
    hub.chreApiMinorVersion = 6;

    out_contextHubInfos->push_back(hub);

    return ndk::ScopedAStatus::ok();
}

// We don't expose any nanoapps for the default impl, therefore all nanoapp-related APIs fail.
::ndk::ScopedAStatus ContextHub::loadNanoapp(int32_t /* in_contextHubId */,
                                             const NanoappBinary& /* in_appBinary */,
                                             int32_t /* in_transactionId */, bool* _aidl_return) {
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::unloadNanoapp(int32_t /* in_contextHubId */,
                                               int64_t /* in_appId */,
                                               int32_t /* in_transactionId */, bool* _aidl_return) {
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::disableNanoapp(int32_t /* in_contextHubId */,
                                                int64_t /* in_appId */,
                                                int32_t /* in_transactionId */,
                                                bool* _aidl_return) {
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::enableNanoapp(int32_t /* in_contextHubId */,
                                               int64_t /* in_appId */,
                                               int32_t /* in_transactionId */, bool* _aidl_return) {
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::onSettingChanged(Setting /* in_setting */, bool /*in_enabled */) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::queryNanoapps(int32_t in_contextHubId, bool* _aidl_return) {
    if (in_contextHubId == kMockHubId && mCallback != nullptr) {
        std::vector<NanoappInfo> nanoapps;
        mCallback->handleNanoappInfo(nanoapps);
        *_aidl_return = true;
    } else {
        *_aidl_return = false;
    }

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::registerCallback(int32_t in_contextHubId,
                                                  const std::shared_ptr<IContextHubCallback>& in_cb,
                                                  bool* _aidl_return) {
    if (in_contextHubId == kMockHubId) {
        mCallback = in_cb;
        *_aidl_return = true;
    } else {
        *_aidl_return = false;
    }
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::sendMessageToHub(int32_t in_contextHubId,
                                                  const ContextHubMessage& /* in_message */,
                                                  bool* _aidl_return) {
    if (in_contextHubId == kMockHubId) {
        // Return true here to indicate that the HAL has accepted the message.
        // Successful delivery of the message to a nanoapp should be handled at
        // a higher level protocol.
        *_aidl_return = true;
    } else {
        *_aidl_return = false;
    }

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::onHostEndpointConnected(const HostEndpointInfo& in_info) {
    mConnectedHostEndpoints.insert(in_info.hostEndpointId);

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::onHostEndpointDisconnected(char16_t in_hostEndpointId) {
    if (mConnectedHostEndpoints.count(in_hostEndpointId) > 0) {
        mConnectedHostEndpoints.erase(in_hostEndpointId);
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    }
}

}  // namespace contexthub
}  // namespace hardware
}  // namespace android
}  // namespace aidl
