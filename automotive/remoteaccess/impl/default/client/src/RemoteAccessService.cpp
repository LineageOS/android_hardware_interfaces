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

#include <android/binder_status.h>
#include <grpc++/grpc++.h>
#include <utils/Log.h>
#include <chrono>
#include <thread>

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

namespace {

using ::aidl::android::hardware::automotive::remoteaccess::ApState;
using ::aidl::android::hardware::automotive::remoteaccess::IRemoteTaskCallback;
using ::grpc::ClientContext;
using ::grpc::ClientReader;
using ::grpc::Status;
using ::grpc::StatusCode;
using ::ndk::ScopedAStatus;

const std::string WAKEUP_SERVICE_NAME = "com.google.vehicle.wakeup";

}  // namespace

RemoteAccessService::RemoteAccessService(WakeupClient::StubInterface* grpcStub)
    : mGrpcStub(grpcStub) {
    // mThread = std::thread([this]() { taskLoop(); });
}

void RemoteAccessService::taskLoop() {
    // TODO(b/241483300): handle remote tasks.
}

ScopedAStatus RemoteAccessService::getDeviceId(std::string* deviceId) {
    // TODO(b/241483300): Call VHAL to get VIN.
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::getWakeupServiceName(std::string* wakeupServiceName) {
    *wakeupServiceName = WAKEUP_SERVICE_NAME;
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::setRemoteTaskCallback(
        [[maybe_unused]] const std::shared_ptr<IRemoteTaskCallback>& callback) {
    mRemoteTaskCallback = callback;
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::clearRemoteTaskCallback() {
    mRemoteTaskCallback.reset();
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::notifyApStateChange([[maybe_unused]] const ApState& newState) {
    return ScopedAStatus::ok();
}

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
