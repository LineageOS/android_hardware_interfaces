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
using ::android::base::ScopedLockAssertion;
using ::grpc::ClientContext;
using ::grpc::ClientReaderInterface;
using ::grpc::Status;
using ::grpc::StatusCode;
using ::ndk::ScopedAStatus;

const std::string WAKEUP_SERVICE_NAME = "com.google.vehicle.wakeup";

std::vector<uint8_t> stringToBytes(const std::string& s) {
    const char* data = s.data();
    return std::vector<uint8_t>(data, data + s.size());
}

ScopedAStatus rpcStatusToScopedAStatus(const Status& status, const std::string& errorMsg) {
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
            status.error_code(), (errorMsg + ", error: " + status.error_message()).c_str());
}

}  // namespace

RemoteAccessService::RemoteAccessService(WakeupClient::StubInterface* grpcStub)
    : mGrpcStub(grpcStub){};

RemoteAccessService::~RemoteAccessService() {
    maybeStopTaskLoop();
}

void RemoteAccessService::maybeStartTaskLoop() {
    std::lock_guard<std::mutex> lockGuard(mStartStopTaskLoopLock);
    if (mTaskLoopRunning) {
        return;
    }

    mThread = std::thread([this]() { runTaskLoop(); });

    mTaskLoopRunning = true;
}

void RemoteAccessService::maybeStopTaskLoop() {
    std::lock_guard<std::mutex> lockGuard(mStartStopTaskLoopLock);
    if (!mTaskLoopRunning) {
        return;
    }

    {
        std::lock_guard<std::mutex> lockGuard(mLock);
        // Try to stop the reading stream.
        if (mGetRemoteTasksContext) {
            mGetRemoteTasksContext->TryCancel();
            mGetRemoteTasksContext.reset();
        }
        mTaskWaitStopped = true;
        mCv.notify_all();
    }
    if (mThread.joinable()) {
        mThread.join();
    }

    mTaskLoopRunning = false;
}

void RemoteAccessService::runTaskLoop() {
    GetRemoteTasksRequest request = {};
    std::unique_ptr<ClientReaderInterface<GetRemoteTasksResponse>> reader;
    while (true) {
        {
            std::lock_guard<std::mutex> lockGuard(mLock);
            mGetRemoteTasksContext.reset(new ClientContext());
            reader = mGrpcStub->GetRemoteTasks(mGetRemoteTasksContext.get(), request);
        }
        GetRemoteTasksResponse response;
        while (reader->Read(&response)) {
            std::shared_ptr<IRemoteTaskCallback> callback;
            {
                std::lock_guard<std::mutex> lockGuard(mLock);
                callback = mRemoteTaskCallback;
            }
            if (callback == nullptr) {
                continue;
            }
            ScopedAStatus callbackStatus = callback->onRemoteTaskRequested(
                    response.clientid(), stringToBytes(response.data()));
            if (!callbackStatus.isOk()) {
                ALOGE("Failed to call onRemoteTaskRequested callback, status: %d, message: %s",
                      callbackStatus.getStatus(), callbackStatus.getMessage());
            }
        }
        Status status = reader->Finish();

        ALOGE("GetRemoteTasks stream breaks, code: %d, message: %s, sleeping for 10s and retry",
              status.error_code(), status.error_message().c_str());
        // The long lasting connection should not return. But if the server returns, retry after
        // 10s.
        {
            std::unique_lock lk(mLock);
            if (mCv.wait_for(lk, std::chrono::milliseconds(mRetryWaitInMs), [this] {
                    ScopedLockAssertion lockAssertion(mLock);
                    return mTaskWaitStopped;
                })) {
                // If the stopped flag is set, we are quitting, exit the loop.
                break;
            }
        }
    }
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
    std::lock_guard<std::mutex> lockGuard(mLock);
    mRemoteTaskCallback = callback;
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::clearRemoteTaskCallback() {
    std::lock_guard<std::mutex> lockGuard(mLock);
    mRemoteTaskCallback.reset();
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::notifyApStateChange(const ApState& newState) {
    ClientContext context;
    NotifyWakeupRequiredRequest request = {};
    request.set_iswakeuprequired(newState.isWakeupRequired);
    NotifyWakeupRequiredResponse response = {};
    Status status = mGrpcStub->NotifyWakeupRequired(&context, request, &response);
    if (!status.ok()) {
        return rpcStatusToScopedAStatus(status, "Failed to notify isWakeupRequired");
    }

    if (newState.isReadyForRemoteTask) {
        maybeStartTaskLoop();
    } else {
        maybeStopTaskLoop();
    }
    return ScopedAStatus::ok();
}

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
