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

#include <VehicleUtils.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>
#include <android-base/stringprintf.h>
#include <android/binder_status.h>
#include <grpc++/grpc++.h>
#include <private/android_filesystem_config.h>
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
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::android::base::ScopedLockAssertion;
using ::android::base::StringAppendF;
using ::android::base::StringPrintf;
using ::android::frameworks::automotive::vhal::IVhalClient;
using ::android::hardware::automotive::vehicle::toInt;
using ::grpc::ClientContext;
using ::grpc::ClientReaderInterface;
using ::grpc::Status;
using ::grpc::StatusCode;
using ::ndk::ScopedAStatus;

const std::string WAKEUP_SERVICE_NAME = "com.google.vehicle.wakeup";
constexpr char COMMAND_SET_AP_STATE[] = "--set-ap-state";
constexpr char COMMAND_START_DEBUG_CALLBACK[] = "--start-debug-callback";
constexpr char COMMAND_STOP_DEBUG_CALLBACK[] = "--stop-debug-callback";
constexpr char COMMAND_SHOW_TASK[] = "--show-task";
constexpr char COMMAND_GET_DEVICE_ID[] = "--get-device-id";

std::vector<uint8_t> stringToBytes(const std::string& s) {
    const char* data = s.data();
    return std::vector<uint8_t>(data, data + s.size());
}

ScopedAStatus rpcStatusToScopedAStatus(const Status& status, const std::string& errorMsg) {
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
            status.error_code(), (errorMsg + ", error: " + status.error_message()).c_str());
}

std::string printBytes(const std::vector<uint8_t>& bytes) {
    std::string s;
    for (size_t i = 0; i < bytes.size(); i++) {
        StringAppendF(&s, "%02x", bytes[i]);
    }
    return s;
}

bool checkBoolFlag(const char* flag) {
    return !strcmp(flag, "1") || !strcmp(flag, "0");
}

void dprintErrorStatus(int fd, const char* detail, const ScopedAStatus& status) {
    dprintf(fd, "%s, code: %d, error: %s\n", detail, status.getStatus(), status.getMessage());
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
            ALOGI("Receiving one task from remote task client");

            std::shared_ptr<IRemoteTaskCallback> callback;
            {
                std::lock_guard<std::mutex> lockGuard(mLock);
                callback = mRemoteTaskCallback;
            }
            if (callback == nullptr) {
                ALOGD("No callback registered, task ignored");
                continue;
            }
            ALOGD("Calling onRemoteTaskRequested callback for client ID: %s",
                  response.clientid().c_str());
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
    auto vhalClient = IVhalClient::tryCreate();
    if (vhalClient == nullptr) {
        ALOGE("Failed to connect to VHAL");
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                /*errorCode=*/0, "Failed to connect to VHAL to get device ID");
    }
    return getDeviceIdWithClient(*vhalClient.get(), deviceId);
}

ScopedAStatus RemoteAccessService::getDeviceIdWithClient(IVhalClient& vhalClient,
                                                         std::string* deviceId) {
    auto result = vhalClient.getValueSync(
            *vhalClient.createHalPropValue(toInt(VehicleProperty::INFO_VIN)));
    if (!result.ok()) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                /*errorCode=*/0,
                ("failed to get INFO_VIN from VHAL: " + result.error().message()).c_str());
    }
    *deviceId = (*result)->getStringValue();
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::getWakeupServiceName(std::string* wakeupServiceName) {
    *wakeupServiceName = WAKEUP_SERVICE_NAME;
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::setRemoteTaskCallback(
        const std::shared_ptr<IRemoteTaskCallback>& callback) {
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

bool RemoteAccessService::checkDumpPermission() {
    uid_t uid = AIBinder_getCallingUid();
    return uid == AID_ROOT || uid == AID_SHELL || uid == AID_SYSTEM;
}

void RemoteAccessService::dumpHelp(int fd) {
    dprintf(fd, "%s",
            (std::string("RemoteAccess HAL debug interface, Usage: \n") + COMMAND_SET_AP_STATE +
             " [0/1](isReadyForRemoteTask) [0/1](isWakeupRequired)  Set the new AP state\n" +
             COMMAND_START_DEBUG_CALLBACK +
             " Start a debug callback that will record the received tasks\n" +
             COMMAND_STOP_DEBUG_CALLBACK + " Stop the debug callback\n" + COMMAND_SHOW_TASK +
             " Show tasks received by debug callback\n" + COMMAND_GET_DEVICE_ID +
             " Get device id\n")
                    .c_str());
}

binder_status_t RemoteAccessService::dump(int fd, const char** args, uint32_t numArgs) {
    if (!checkDumpPermission()) {
        dprintf(fd, "Caller must be root, system or shell\n");
        return STATUS_PERMISSION_DENIED;
    }

    if (numArgs == 0) {
        dumpHelp(fd);
        return STATUS_OK;
    }

    if (!strcmp(args[0], COMMAND_SET_AP_STATE)) {
        if (numArgs < 3) {
            dumpHelp(fd);
            return STATUS_OK;
        }
        ApState apState = {};
        const char* remoteTaskFlag = args[1];
        if (!strcmp(remoteTaskFlag, "1") && !strcmp(remoteTaskFlag, "0")) {
            dumpHelp(fd);
            return STATUS_OK;
        }
        if (!checkBoolFlag(args[1])) {
            dumpHelp(fd);
            return STATUS_OK;
        }
        if (!strcmp(args[1], "1")) {
            apState.isReadyForRemoteTask = true;
        }
        if (!checkBoolFlag(args[2])) {
            dumpHelp(fd);
            return STATUS_OK;
        }
        if (!strcmp(args[2], "1")) {
            apState.isWakeupRequired = true;
        }
        auto status = notifyApStateChange(apState);
        if (!status.isOk()) {
            dprintErrorStatus(fd, "Failed to set AP state", status);
        } else {
            dprintf(fd, "successfully set the new AP state\n");
        }
    } else if (!strcmp(args[0], COMMAND_START_DEBUG_CALLBACK)) {
        mDebugCallback = ndk::SharedRefBase::make<DebugRemoteTaskCallback>();
        setRemoteTaskCallback(mDebugCallback);
        dprintf(fd, "Debug callback registered\n");
    } else if (!strcmp(args[0], COMMAND_STOP_DEBUG_CALLBACK)) {
        if (mDebugCallback) {
            mDebugCallback.reset();
        }
        clearRemoteTaskCallback();
        dprintf(fd, "Debug callback unregistered\n");
    } else if (!strcmp(args[0], COMMAND_SHOW_TASK)) {
        if (mDebugCallback) {
            dprintf(fd, "%s", mDebugCallback->printTasks().c_str());
        } else {
            dprintf(fd, "Debug callback is not currently used, use \"%s\" first.\n",
                    COMMAND_START_DEBUG_CALLBACK);
        }
    } else if (!strcmp(args[0], COMMAND_GET_DEVICE_ID)) {
        std::string deviceId;
        auto status = getDeviceId(&deviceId);
        if (!status.isOk()) {
            dprintErrorStatus(fd, "Failed to get device ID", status);
        } else {
            dprintf(fd, "Device Id: %s\n", deviceId.c_str());
        }
    } else {
        dumpHelp(fd);
    }

    return STATUS_OK;
}

ScopedAStatus DebugRemoteTaskCallback::onRemoteTaskRequested(const std::string& clientId,
                                                             const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lockGuard(mLock);
    mTasks.push_back({
            .clientId = clientId,
            .data = data,
    });
    return ScopedAStatus::ok();
}

std::string DebugRemoteTaskCallback::printTasks() {
    std::lock_guard<std::mutex> lockGuard(mLock);
    std::string s = StringPrintf("Received %zu tasks in %f seconds", mTasks.size(),
                                 (android::uptimeMillis() - mStartTimeMillis) / 1000.);
    for (size_t i = 0; i < mTasks.size(); i++) {
        StringAppendF(&s, "Client Id: %s, Data: %s\n", mTasks[i].clientId.c_str(),
                      printBytes(mTasks[i].data).c_str());
    }
    return s;
}

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
