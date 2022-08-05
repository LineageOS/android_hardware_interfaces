/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "RemoteAccessService.h"

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

using ::aidl::android::hardware::automotive::remoteaccess::ApState;
using ::aidl::android::hardware::automotive::remoteaccess::IRemoteTaskCallback;
using ::ndk::ScopedAStatus;

ScopedAStatus RemoteAccessService::getDeviceId([[maybe_unused]] std::string* deviceId) {
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::getWakeupServiceName(
        [[maybe_unused]] std::string* wakeupServiceName) {
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::setRemoteTaskCallback(
        [[maybe_unused]] const std::shared_ptr<IRemoteTaskCallback>& callback) {
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::clearRemoteTaskCallback() {
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::notifyApStateChange([[maybe_unused]] const ApState& newState) {
    return ScopedAStatus::ok();
}

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
