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
#include <android-base/parseint.h>
#include <android-base/stringprintf.h>
#include <android/binder_status.h>
#include <grpc++/grpc++.h>
#include <private/android_filesystem_config.h>
#include <sys/stat.h>
#include <utils/Log.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

namespace {

using ::aidl::android::hardware::automotive::remoteaccess::ApState;
using ::aidl::android::hardware::automotive::remoteaccess::IRemoteTaskCallback;
using ::aidl::android::hardware::automotive::remoteaccess::ScheduleInfo;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::android::base::Error;
using ::android::base::ParseInt;
using ::android::base::Result;
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
const std::string PROCESSOR_ID = "application_processor";
constexpr char COMMAND_SET_AP_STATE[] = "--set-ap-state";
constexpr char COMMAND_START_DEBUG_CALLBACK[] = "--start-debug-callback";
constexpr char COMMAND_STOP_DEBUG_CALLBACK[] = "--stop-debug-callback";
constexpr char COMMAND_SHOW_TASK[] = "--show-task";
constexpr char COMMAND_GET_VEHICLE_ID[] = "--get-vehicle-id";
constexpr char COMMAND_INJECT_TASK[] = "--inject-task";
constexpr char COMMAND_INJECT_TASK_NEXT_REBOOT[] = "--inject-task-next-reboot";
constexpr char COMMAND_STATUS[] = "--status";

constexpr char DEBUG_TASK_FILE[] = "/data/vendor/remoteaccess/debugTask";

std::vector<uint8_t> stringToBytes(std::string_view s) {
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

std::string boolToString(bool x) {
    return x ? "true" : "false";
}

}  // namespace

RemoteAccessService::RemoteAccessService(WakeupClient::StubInterface* grpcStub)
    : mGrpcStub(grpcStub) {
    std::ifstream debugTaskFile;
    debugTaskFile.open(DEBUG_TASK_FILE, std::ios::in);
    if (!debugTaskFile.is_open()) {
        ALOGD("No debug task available");
        return;
    }

    char buffer[1024] = {};
    debugTaskFile.getline(buffer, sizeof(buffer));
    std::string clientId = std::string(buffer);
    debugTaskFile.getline(buffer, sizeof(buffer));
    std::string taskData = std::string(buffer);
    int latencyInSec;
    debugTaskFile >> latencyInSec;
    debugTaskFile.close();

    ALOGD("Task for client: %s, data: [%s], latency: %d\n", clientId.c_str(), taskData.c_str(),
          latencyInSec);

    mInjectDebugTaskThread = std::thread([this, clientId, taskData, latencyInSec] {
        std::this_thread::sleep_for(std::chrono::seconds(latencyInSec));
        if (auto result = deliverRemoteTaskThroughCallback(clientId, taskData); !result.ok()) {
            ALOGE("Failed to inject debug task, clientID: %s, taskData: %s, error: %s",
                  clientId.c_str(), taskData.c_str(), result.error().message().c_str());
            return;
        }
        ALOGD("Task for client: %s, data: [%s] successfully injected\n", clientId.c_str(),
              taskData.c_str());
    });
}

RemoteAccessService::~RemoteAccessService() {
    maybeStopTaskLoop();
    if (mInjectDebugTaskThread.joinable()) {
        mInjectDebugTaskThread.join();
    }
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
            // Don't reset mGetRemoteTaskContext here since the read stream might still be affective
            // and might still be using it. This will cause reader->Read to return false and
            // mGetRemoteTasksContext will be cleared after reader->Finish() is called.
        }
        mTaskWaitStopped = true;
        mCv.notify_all();
    }
    if (mThread.joinable()) {
        mThread.join();
    }

    mTaskLoopRunning = false;
}

void RemoteAccessService::updateGrpcConnected(bool connected) {
    std::lock_guard<std::mutex> lockGuard(mLock);
    mGrpcConnected = connected;
}

Result<void> RemoteAccessService::deliverRemoteTaskThroughCallback(const std::string& clientId,
                                                                   std::string_view taskData) {
    std::shared_ptr<IRemoteTaskCallback> callback;
    {
        std::lock_guard<std::mutex> lockGuard(mLock);
        callback = mRemoteTaskCallback;
        mClientIdToTaskCount[clientId] += 1;
    }
    if (callback == nullptr) {
        return Error() << "No callback registered, task ignored";
    }
    ALOGD("Calling onRemoteTaskRequested callback for client ID: %s", clientId.c_str());
    ScopedAStatus callbackStatus =
            callback->onRemoteTaskRequested(clientId, stringToBytes(taskData));
    if (!callbackStatus.isOk()) {
        return Error() << "Failed to call onRemoteTaskRequested callback, status: "
                       << callbackStatus.getStatus()
                       << ", message: " << callbackStatus.getMessage();
    }
    return {};
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
        updateGrpcConnected(true);
        GetRemoteTasksResponse response;
        while (reader->Read(&response)) {
            ALOGI("Receiving one task from remote task client");

            if (auto result =
                        deliverRemoteTaskThroughCallback(response.clientid(), response.data());
                !result.ok()) {
                ALOGE("%s", result.error().message().c_str());
                continue;
            }
        }
        updateGrpcConnected(false);
        Status status = reader->Finish();
        mGetRemoteTasksContext.reset();

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

ScopedAStatus RemoteAccessService::getVehicleId(std::string* vehicleId) {
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    auto vhalClient = IVhalClient::tryCreate();
    if (vhalClient == nullptr) {
        ALOGE("Failed to connect to VHAL");
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                /*errorCode=*/0, "Failed to connect to VHAL to get vehicle ID");
    }
    return getVehicleIdWithClient(*vhalClient.get(), vehicleId);
#else
    // Don't use VHAL client in fuzzing since IPC is not allowed.
    return ScopedAStatus::ok();
#endif
}

ScopedAStatus RemoteAccessService::getVehicleIdWithClient(IVhalClient& vhalClient,
                                                          std::string* vehicleId) {
    auto result = vhalClient.getValueSync(
            *vhalClient.createHalPropValue(toInt(VehicleProperty::INFO_VIN)));
    if (!result.ok()) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                /*errorCode=*/0,
                ("failed to get INFO_VIN from VHAL: " + result.error().message()).c_str());
    }
    *vehicleId = (*result)->getStringValue();
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::getProcessorId(std::string* processorId) {
    *processorId = PROCESSOR_ID;
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

ScopedAStatus RemoteAccessService::isTaskScheduleSupported(bool* out) {
    *out = true;
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::scheduleTask(const ScheduleInfo& scheduleInfo) {
    ClientContext context;
    ScheduleTaskRequest request = {};
    ScheduleTaskResponse response = {};
    request.mutable_scheduleinfo()->set_clientid(scheduleInfo.clientId);
    request.mutable_scheduleinfo()->set_scheduleid(scheduleInfo.scheduleId);
    request.mutable_scheduleinfo()->set_data(scheduleInfo.taskData.data(),
                                             scheduleInfo.taskData.size());
    request.mutable_scheduleinfo()->set_count(scheduleInfo.count);
    request.mutable_scheduleinfo()->set_starttimeinepochseconds(
            scheduleInfo.startTimeInEpochSeconds);
    request.mutable_scheduleinfo()->set_periodicinseconds(scheduleInfo.periodicInSeconds);
    Status status = mGrpcStub->ScheduleTask(&context, request, &response);
    if (!status.ok()) {
        return rpcStatusToScopedAStatus(status, "Failed to call ScheduleTask");
    }
    int errorCode = response.errorcode();
    switch (errorCode) {
        case ErrorCode::OK:
            return ScopedAStatus::ok();
        case ErrorCode::INVALID_ARG:
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        default:
            // Should not happen.
            return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    -1, ("Got unknown error code: " + ErrorCode_Name(errorCode) +
                         " from remote access HAL")
                                .c_str());
    }
}

ScopedAStatus RemoteAccessService::unscheduleTask(const std::string& clientId,
                                                  const std::string& scheduleId) {
    ClientContext context;
    UnscheduleTaskRequest request = {};
    UnscheduleTaskResponse response = {};
    request.set_clientid(clientId);
    request.set_scheduleid(scheduleId);
    Status status = mGrpcStub->UnscheduleTask(&context, request, &response);
    if (!status.ok()) {
        return rpcStatusToScopedAStatus(status, "Failed to call UnscheduleTask");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::unscheduleAllTasks(const std::string& clientId) {
    ClientContext context;
    UnscheduleAllTasksRequest request = {};
    UnscheduleAllTasksResponse response = {};
    request.set_clientid(clientId);
    Status status = mGrpcStub->UnscheduleAllTasks(&context, request, &response);
    if (!status.ok()) {
        return rpcStatusToScopedAStatus(status, "Failed to call UnscheduleAllTasks");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::isTaskScheduled(const std::string& clientId,
                                                   const std::string& scheduleId, bool* out) {
    ClientContext context;
    IsTaskScheduledRequest request = {};
    IsTaskScheduledResponse response = {};
    request.set_clientid(clientId);
    request.set_scheduleid(scheduleId);
    Status status = mGrpcStub->IsTaskScheduled(&context, request, &response);
    if (!status.ok()) {
        return rpcStatusToScopedAStatus(status, "Failed to call isTaskScheduled");
    }
    *out = response.istaskscheduled();
    return ScopedAStatus::ok();
}

ScopedAStatus RemoteAccessService::getAllScheduledTasks(const std::string& clientId,
                                                        std::vector<ScheduleInfo>* out) {
    ClientContext context;
    GetAllScheduledTasksRequest request = {};
    GetAllScheduledTasksResponse response = {};
    request.set_clientid(clientId);
    Status status = mGrpcStub->GetAllScheduledTasks(&context, request, &response);
    if (!status.ok()) {
        return rpcStatusToScopedAStatus(status, "Failed to call isTaskScheduled");
    }
    out->clear();
    for (int i = 0; i < response.allscheduledtasks_size(); i++) {
        const GrpcScheduleInfo& rpcScheduleInfo = response.allscheduledtasks(i);
        ScheduleInfo scheduleInfo = {
                .clientId = rpcScheduleInfo.clientid(),
                .scheduleId = rpcScheduleInfo.scheduleid(),
                .taskData = stringToBytes(rpcScheduleInfo.data()),
                .count = rpcScheduleInfo.count(),
                .startTimeInEpochSeconds = rpcScheduleInfo.starttimeinepochseconds(),
                .periodicInSeconds = rpcScheduleInfo.periodicinseconds(),
        };
        out->push_back(std::move(scheduleInfo));
    }
    return ScopedAStatus::ok();
}

bool RemoteAccessService::checkDumpPermission() {
    uid_t uid = AIBinder_getCallingUid();
    return uid == AID_ROOT || uid == AID_SHELL || uid == AID_SYSTEM;
}

void RemoteAccessService::dumpHelp(int fd) {
    dprintf(fd,
            "RemoteAccess HAL debug interface, Usage: \n"
            "%s [0/1](isReadyForRemoteTask) [0/1](isWakeupRequired): Set the new AP state\n"
            "%s: Start a debug callback that will record the received tasks\n"
            "%s: Stop the debug callback\n"
            "%s: Show tasks received by debug callback\n"
            "%s: Get vehicle id\n"
            "%s [client_id] [task_data]: Inject a task\n"
            "%s [client_id] [task_data] [latencyInSec]: "
            "Inject a task on next reboot after latencyInSec seconds\n"
            "%s: Show status\n",
            COMMAND_SET_AP_STATE, COMMAND_START_DEBUG_CALLBACK, COMMAND_STOP_DEBUG_CALLBACK,
            COMMAND_SHOW_TASK, COMMAND_GET_VEHICLE_ID, COMMAND_INJECT_TASK,
            COMMAND_INJECT_TASK_NEXT_REBOOT, COMMAND_STATUS);
}

binder_status_t RemoteAccessService::dump(int fd, const char** args, uint32_t numArgs) {
    if (!checkDumpPermission()) {
        dprintf(fd, "Caller must be root, system or shell\n");
        return STATUS_PERMISSION_DENIED;
    }

    if (numArgs == 0) {
        dumpHelp(fd);
        printCurrentStatus(fd);
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
    } else if (!strcmp(args[0], COMMAND_GET_VEHICLE_ID)) {
        std::string vehicleId;
        auto status = getVehicleId(&vehicleId);
        if (!status.isOk()) {
            dprintErrorStatus(fd, "Failed to get vehicle ID", status);
        } else {
            dprintf(fd, "Vehicle Id: %s\n", vehicleId.c_str());
        }
    } else if (!strcmp(args[0], COMMAND_INJECT_TASK)) {
        if (numArgs < 3) {
            dumpHelp(fd);
            return STATUS_OK;
        }
        debugInjectTask(fd, args[1], args[2]);
    } else if (!strcmp(args[0], COMMAND_INJECT_TASK_NEXT_REBOOT)) {
        if (numArgs < 4) {
            dumpHelp(fd);
            return STATUS_OK;
        }
        debugInjectTaskNextReboot(fd, args[1], args[2], args[3]);
    } else if (!strcmp(args[0], COMMAND_STATUS)) {
        printCurrentStatus(fd);
    } else {
        dumpHelp(fd);
    }

    return STATUS_OK;
}

void RemoteAccessService::printCurrentStatus(int fd) {
    std::lock_guard<std::mutex> lockGuard(mLock);
    dprintf(fd,
            "\nRemoteAccess HAL status \n"
            "Remote task callback registered: %s\n"
            "Task receiving GRPC connection established: %s\n"
            "Received task count by clientId: \n%s\n",
            boolToString(mRemoteTaskCallback.get()).c_str(), boolToString(mGrpcConnected).c_str(),
            clientIdToTaskCountToStringLocked().c_str());
}

void RemoteAccessService::debugInjectTask(int fd, std::string_view clientId,
                                          std::string_view taskData) {
    std::string clientIdCopy = std::string(clientId);
    if (auto result = deliverRemoteTaskThroughCallback(clientIdCopy, taskData); !result.ok()) {
        dprintf(fd, "Failed to inject task: %s\n", result.error().message().c_str());
        return;
    }
    dprintf(fd, "Task for client: %s, data: [%s] successfully injected\n", clientId.data(),
            taskData.data());
}

void RemoteAccessService::debugInjectTaskNextReboot(int fd, std::string_view clientId,
                                                    std::string_view taskData,
                                                    const char* latencyInSecStr) {
    int latencyInSec;
    if (!ParseInt(latencyInSecStr, &latencyInSec)) {
        dprintf(fd, "The input latency in second is not a valid integer");
        return;
    }
    std::ofstream debugTaskFile;
    debugTaskFile.open(DEBUG_TASK_FILE, std::ios::out);
    if (!debugTaskFile.is_open()) {
        dprintf(fd,
                "Failed to open debug task file, please run the command: "
                "'adb shell touch %s' first\n",
                DEBUG_TASK_FILE);
        return;
    }
    if (taskData.find("\n") != std::string::npos) {
        dprintf(fd, "Task data must not contain newline\n");
        return;
    }
    debugTaskFile << clientId << "\n" << taskData << "\n" << latencyInSec;
    debugTaskFile.close();
    dprintf(fd,
            "Task with clientId: %s, task data: %s, latency: %d sec scheduled for next reboot\n",
            clientId.data(), taskData.data(), latencyInSec);
}

std::string RemoteAccessService::clientIdToTaskCountToStringLocked() {
    // Print the table header
    std::string output = "| ClientId | Count |\n";
    for (const auto& [clientId, taskCount] : mClientIdToTaskCount) {
        output += StringPrintf("  %-9s  %-6zu\n", clientId.c_str(), taskCount);
    }
    return output;
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
