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

#include "TestWakeupClientServiceImpl.h"

#include <android-base/stringprintf.h>
#include <inttypes.h>
#include <utils/Looper.h>
#include <utils/SystemClock.h>
#include <chrono>
#include <thread>

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

namespace {

using ::android::uptimeMillis;
using ::android::base::ScopedLockAssertion;
using ::android::base::StringPrintf;
using ::grpc::ServerContext;
using ::grpc::ServerWriter;
using ::grpc::Status;

constexpr int64_t kTaskIntervalInMs = 5'000;
constexpr int64_t kTaskTimeoutInMs = 20'000;

int64_t msToNs(int64_t ms) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(ms))
            .count();
}

int64_t sToNs(int64_t s) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(s)).count();
}

}  // namespace

GetRemoteTasksResponse FakeTaskGenerator::generateTask(const std::string& clientId) {
    GetRemoteTasksResponse response;
    response.set_data(reinterpret_cast<const char*>(DATA), sizeof(DATA));
    response.set_clientid(clientId);
    return response;
}

TaskTimeoutMessageHandler::TaskTimeoutMessageHandler(TaskQueue* taskQueue)
    : mTaskQueue(taskQueue) {}

void TaskTimeoutMessageHandler::handleMessage(const android::Message& message) {
    mTaskQueue->handleTaskTimeout();
}

TaskQueue::TaskQueue(android::sp<Looper> looper) {
    mTaskTimeoutMessageHandler = android::sp<TaskTimeoutMessageHandler>::make(this);
    mLooper = looper;
}

std::optional<GetRemoteTasksResponse> TaskQueue::maybePopOne() {
    std::lock_guard<std::mutex> lockGuard(mLock);
    if (mTasks.size() == 0) {
        return std::nullopt;
    }
    TaskInfo response = std::move(mTasks.top());
    mTasks.pop();
    mLooper->removeMessages(mTaskTimeoutMessageHandler, response.taskId);
    return std::move(response.taskData);
}

void TaskQueue::add(const GetRemoteTasksResponse& task) {
    std::lock_guard<std::mutex> lockGuard(mLock);
    if (mStopped) {
        return;
    }
    int taskId = mTaskIdCounter++;
    mTasks.push(TaskInfo{
            .taskId = taskId,
            .timestampInMs = uptimeMillis(),
            .taskData = task,
    });
    android::Message message(taskId);
    mLooper->sendMessageDelayed(msToNs(kTaskTimeoutInMs), mTaskTimeoutMessageHandler, message);
    mTasksNotEmptyCv.notify_all();
}

void TaskQueue::waitForTask() {
    std::unique_lock<std::mutex> lock(mLock);
    mTasksNotEmptyCv.wait(lock, [this] {
        ScopedLockAssertion lockAssertion(mLock);
        return mTasks.size() > 0 || mStopped;
    });
}

void TaskQueue::stopWait() {
    mStopped = true;
    {
        std::lock_guard<std::mutex> lockGuard(mLock);
        mTasksNotEmptyCv.notify_all();
    }
}

bool TaskQueue::isEmpty() {
    std::lock_guard<std::mutex> lockGuard(mLock);
    return mTasks.size() == 0 || mStopped;
}

void TaskQueue::handleTaskTimeout() {
    // We know which task timed-out from the taskId in the message. However, there is no easy way
    // to remove a specific task with the task ID from the priority_queue, so we just check from
    // the top of the queue (which have the oldest tasks).
    std::lock_guard<std::mutex> lockGuard(mLock);
    int64_t now = uptimeMillis();
    while (mTasks.size() > 0) {
        const TaskInfo& taskInfo = mTasks.top();
        if (taskInfo.timestampInMs + kTaskTimeoutInMs > now) {
            break;
        }
        // In real implementation, this should report task failure to remote wakeup server.
        printf("Task for client ID: %s timed-out, added at %" PRId64 " ms, now %" PRId64 " ms\n",
               taskInfo.taskData.clientid().c_str(), taskInfo.timestampInMs, now);
        mTasks.pop();
    }
}

TestWakeupClientServiceImpl::TestWakeupClientServiceImpl() {
    mTaskScheduleMsgHandler = android::sp<TaskScheduleMsgHandler>::make(this);
    mLooper = android::sp<Looper>::make(/*opts=*/0);
    mLooperThread = std::thread([this] { loop(); });
    mTaskQueue = std::make_unique<TaskQueue>(mLooper);
}

TestWakeupClientServiceImpl::~TestWakeupClientServiceImpl() {
    if (mServerStopped) {
        return;
    }
    stopServer();
}

void TestWakeupClientServiceImpl::stopServer() {
    mTaskQueue->stopWait();
    stopGeneratingFakeTask();
    // Set the flag so that the loop thread will exit.
    mServerStopped = true;
    mLooper->wake();
    if (mLooperThread.joinable()) {
        mLooperThread.join();
    }
}

void TestWakeupClientServiceImpl::loop() {
    Looper::setForThread(mLooper);

    while (true) {
        mLooper->pollAll(/*timeoutMillis=*/-1);
        if (mServerStopped) {
            return;
        }
    }
}

void TestWakeupClientServiceImpl::injectTask(const std::string& taskData,
                                             const std::string& clientId) {
    GetRemoteTasksResponse response;
    response.set_data(taskData);
    response.set_clientid(clientId);
    injectTaskResponse(response);
}

void TestWakeupClientServiceImpl::injectTaskResponse(const GetRemoteTasksResponse& response) {
    printf("Receive a new task\n");
    mTaskQueue->add(response);
    if (mWakeupRequired) {
        wakeupApplicationProcessor();
    }
}

void TestWakeupClientServiceImpl::startGeneratingFakeTask(const std::string& clientId) {
    std::lock_guard<std::mutex> lockGuard(mLock);
    if (mGeneratingFakeTask) {
        printf("Fake task is already being generated\n");
        return;
    }
    mGeneratingFakeTask = true;
    mFakeTaskThread = std::thread([this, clientId] { fakeTaskGenerateLoop(clientId); });
    printf("Started generating fake tasks\n");
}

void TestWakeupClientServiceImpl::stopGeneratingFakeTask() {
    {
        std::lock_guard<std::mutex> lockGuard(mLock);
        if (!mGeneratingFakeTask) {
            printf("Fake task is not being generated, do nothing\n");
            return;
        }
        mTaskLoopStoppedCv.notify_all();
        mGeneratingFakeTask = false;
    }
    if (mFakeTaskThread.joinable()) {
        mFakeTaskThread.join();
    }
    printf("Stopped generating fake tasks\n");
}

void TestWakeupClientServiceImpl::fakeTaskGenerateLoop(const std::string& clientId) {
    // In actual implementation, this should communicate with the remote server and receives tasks
    // from it. Here we simulate receiving one remote task every {kTaskIntervalInMs}ms.
    while (true) {
        injectTaskResponse(mFakeTaskGenerator.generateTask(clientId));
        printf("Sleeping for %" PRId64 " seconds until next task\n", kTaskIntervalInMs);

        std::unique_lock lk(mLock);
        if (mTaskLoopStoppedCv.wait_for(lk, std::chrono::milliseconds(kTaskIntervalInMs), [this] {
                ScopedLockAssertion lockAssertion(mLock);
                return !mGeneratingFakeTask;
            })) {
            // If the stopped flag is set, we are quitting, exit the loop.
            return;
        }
    }
}

Status TestWakeupClientServiceImpl::GetRemoteTasks(ServerContext* context,
                                                   const GetRemoteTasksRequest* request,
                                                   ServerWriter<GetRemoteTasksResponse>* writer) {
    printf("GetRemoteTasks called\n");
    mRemoteTaskConnectionAlive = true;
    while (true) {
        mTaskQueue->waitForTask();

        if (mServerStopped) {
            // Server stopped, exit the loop.
            printf("Server stopped exit loop\n");
            break;
        }

        while (true) {
            auto maybeTask = mTaskQueue->maybePopOne();
            if (!maybeTask.has_value()) {
                // No task left, loop again and wait for another task(s).
                break;
            }
            // Loop through all the task in the queue but obtain lock for each element so we don't
            // hold lock while writing the response.
            const GetRemoteTasksResponse& response = maybeTask.value();
            if (!writer->Write(response)) {
                // Broken stream, maybe the client is shutting down.
                printf("Failed to deliver remote task to remote access HAL\n");
                // The task failed to be sent, add it back to the queue. The order might change, but
                // it is okay.
                mTaskQueue->add(response);
                mRemoteTaskConnectionAlive = false;
                return Status::CANCELLED;
            }
        }
    }
    // Server stopped, exit the loop.
    return Status::CANCELLED;
}

Status TestWakeupClientServiceImpl::NotifyWakeupRequired(ServerContext* context,
                                                         const NotifyWakeupRequiredRequest* request,
                                                         NotifyWakeupRequiredResponse* response) {
    printf("NotifyWakeupRequired called\n");
    if (request->iswakeuprequired() && !mWakeupRequired && !mTaskQueue->isEmpty()) {
        // If wakeup is now required and previously not required, this means we have finished
        // shutting down the device. If there are still pending tasks, try waking up AP again
        // to finish executing those tasks.
        wakeupApplicationProcessor();
    }
    mWakeupRequired = request->iswakeuprequired();
    if (mWakeupRequired) {
        // We won't know the connection is down unless we try to send a task over. If wakeup is
        // required, the connection is very likely already down.
        mRemoteTaskConnectionAlive = false;
    }
    return Status::OK;
}

void TestWakeupClientServiceImpl::cleanupScheduledTaskLocked(const std::string& clientId,
                                                             const std::string& scheduleId) {
    mInfoByScheduleIdByClientId[clientId].erase(scheduleId);
    if (mInfoByScheduleIdByClientId[clientId].size() == 0) {
        mInfoByScheduleIdByClientId.erase(clientId);
    }
}

TaskScheduleMsgHandler::TaskScheduleMsgHandler(TestWakeupClientServiceImpl* impl) : mImpl(impl) {}

void TaskScheduleMsgHandler::handleMessage(const android::Message& message) {
    mImpl->handleAddTask(message.what);
}

Status TestWakeupClientServiceImpl::ScheduleTask(ServerContext* context,
                                                 const ScheduleTaskRequest* request,
                                                 ScheduleTaskResponse* response) {
    std::lock_guard<std::mutex> lockGuard(mLock);

    const GrpcScheduleInfo& grpcScheduleInfo = request->scheduleinfo();
    const std::string& scheduleId = grpcScheduleInfo.scheduleid();
    const std::string& clientId = grpcScheduleInfo.clientid();
    response->set_errorcode(ErrorCode::OK);

    if (mInfoByScheduleIdByClientId.find(clientId) != mInfoByScheduleIdByClientId.end() &&
        mInfoByScheduleIdByClientId[clientId].find(scheduleId) !=
                mInfoByScheduleIdByClientId[clientId].end()) {
        printf("Duplicate schedule Id: %s for client Id: %s\n", scheduleId.c_str(),
               clientId.c_str());
        response->set_errorcode(ErrorCode::INVALID_ARG);
        return Status::OK;
    }

    int64_t startTimeInEpochSeconds = grpcScheduleInfo.starttimeinepochseconds();
    int64_t periodicInSeconds = grpcScheduleInfo.periodicinseconds();
    int32_t count = grpcScheduleInfo.count();

    int scheduleMsgId = mScheduleMsgCounter++;
    mInfoByScheduleIdByClientId[clientId][scheduleId] = {
            .grpcScheduleInfo = std::make_unique<GrpcScheduleInfo>(grpcScheduleInfo),
            .scheduleMsgId = scheduleMsgId,
            .periodicInSeconds = periodicInSeconds,
            .currentCount = 0,
            .totalCount = count,
    };

    int64_t delayInSeconds =
            startTimeInEpochSeconds - std::chrono::duration_cast<std::chrono::seconds>(
                                              std::chrono::system_clock::now().time_since_epoch())
                                              .count();
    if (delayInSeconds < 0) {
        delayInSeconds = 0;
    }

    printf("ScheduleTask called with client Id: %s, schedule Id: %s, delay: %" PRId64 " s\n",
           clientId.c_str(), scheduleId.c_str(), delayInSeconds);

    mLooper->sendMessageDelayed(sToNs(delayInSeconds), mTaskScheduleMsgHandler,
                                android::Message(scheduleMsgId));

    return Status::OK;
}

bool TestWakeupClientServiceImpl::getScheduleInfoLocked(int scheduleMsgId,
                                                        ScheduleInfo** outScheduleInfoPtr) {
    for (auto& [_, infoByScheduleId] : mInfoByScheduleIdByClientId) {
        for (auto& [_, scheduleInfo] : infoByScheduleId) {
            if (scheduleInfo.scheduleMsgId == scheduleMsgId) {
                *outScheduleInfoPtr = &scheduleInfo;
                return true;
            }
        }
    }
    return false;
}

void TestWakeupClientServiceImpl::handleAddTask(int scheduleMsgId) {
    std::lock_guard<std::mutex> lockGuard(mLock);

    ScheduleInfo* scheduleInfoPtr;
    bool found = getScheduleInfoLocked(scheduleMsgId, &scheduleInfoPtr);
    if (!found) {
        printf("The schedule msg Id: %d is not found\n", scheduleMsgId);
        return;
    }

    const GrpcScheduleInfo& grpcScheduleInfo = *scheduleInfoPtr->grpcScheduleInfo;
    const std::string scheduleId = grpcScheduleInfo.scheduleid();
    const std::string clientId = grpcScheduleInfo.clientid();

    GetRemoteTasksResponse injectResponse;
    injectResponse.set_data(grpcScheduleInfo.data().data(), grpcScheduleInfo.data().size());
    injectResponse.set_clientid(clientId);
    injectTaskResponse(injectResponse);
    scheduleInfoPtr->currentCount++;

    printf("Sending scheduled tasks for scheduleId: %s, clientId: %s, taskCount: %d\n",
           scheduleId.c_str(), clientId.c_str(), scheduleInfoPtr->currentCount);

    if (scheduleInfoPtr->totalCount != 0 &&
        scheduleInfoPtr->currentCount == scheduleInfoPtr->totalCount) {
        // This schedule is finished.
        cleanupScheduledTaskLocked(clientId, scheduleId);
        return;
    }

    // Schedule the task for the next period.
    mLooper->sendMessageDelayed(sToNs(scheduleInfoPtr->periodicInSeconds), mTaskScheduleMsgHandler,
                                android::Message(scheduleMsgId));
}

Status TestWakeupClientServiceImpl::UnscheduleTask(ServerContext* context,
                                                   const UnscheduleTaskRequest* request,
                                                   UnscheduleTaskResponse* response) {
    std::lock_guard<std::mutex> lockGuard(mLock);

    const std::string& clientId = request->clientid();
    const std::string& scheduleId = request->scheduleid();
    printf("UnscheduleTask called with client Id: %s, schedule Id: %s\n", clientId.c_str(),
           scheduleId.c_str());

    if (mInfoByScheduleIdByClientId.find(clientId) == mInfoByScheduleIdByClientId.end() ||
        mInfoByScheduleIdByClientId[clientId].find(scheduleId) ==
                mInfoByScheduleIdByClientId[clientId].end()) {
        printf("UnscheduleTask: no task associated with clientId: %s, scheduleId: %s\n",
               clientId.c_str(), scheduleId.c_str());
        return Status::OK;
    }

    mLooper->removeMessages(mTaskScheduleMsgHandler,
                            mInfoByScheduleIdByClientId[clientId][scheduleId].scheduleMsgId);
    cleanupScheduledTaskLocked(clientId, scheduleId);
    return Status::OK;
}

Status TestWakeupClientServiceImpl::UnscheduleAllTasks(ServerContext* context,
                                                       const UnscheduleAllTasksRequest* request,
                                                       UnscheduleAllTasksResponse* response) {
    std::lock_guard<std::mutex> lockGuard(mLock);

    const std::string& clientId = request->clientid();
    printf("UnscheduleAllTasks called with client Id: %s\n", clientId.c_str());
    if (mInfoByScheduleIdByClientId.find(clientId) == mInfoByScheduleIdByClientId.end()) {
        printf("UnscheduleTask: no task associated with clientId: %s\n", clientId.c_str());
        return Status::OK;
    }
    const auto& infoByScheduleId = mInfoByScheduleIdByClientId[clientId];
    std::vector<int> scheduleMsgIds;
    for (const auto& [_, scheduleInfo] : infoByScheduleId) {
        mLooper->removeMessages(mTaskScheduleMsgHandler, /*what=*/scheduleInfo.scheduleMsgId);
    }

    mInfoByScheduleIdByClientId.erase(clientId);
    return Status::OK;
}

Status TestWakeupClientServiceImpl::IsTaskScheduled(ServerContext* context,
                                                    const IsTaskScheduledRequest* request,
                                                    IsTaskScheduledResponse* response) {
    std::lock_guard<std::mutex> lockGuard(mLock);

    const std::string& clientId = request->clientid();
    const std::string& scheduleId = request->scheduleid();
    printf("IsTaskScheduled called with client Id: %s, scheduleId: %s\n", clientId.c_str(),
           scheduleId.c_str());

    if (mInfoByScheduleIdByClientId.find(clientId) == mInfoByScheduleIdByClientId.end()) {
        response->set_istaskscheduled(false);
        return Status::OK;
    }
    if (mInfoByScheduleIdByClientId[clientId].find(scheduleId) ==
        mInfoByScheduleIdByClientId[clientId].end()) {
        response->set_istaskscheduled(false);
        return Status::OK;
    }
    response->set_istaskscheduled(true);
    return Status::OK;
}

Status TestWakeupClientServiceImpl::GetAllScheduledTasks(ServerContext* context,
                                                         const GetAllScheduledTasksRequest* request,
                                                         GetAllScheduledTasksResponse* response) {
    const std::string& clientId = request->clientid();
    printf("GetAllScheduledTasks called with client Id: %s\n", clientId.c_str());
    response->clear_allscheduledtasks();
    {
        std::unique_lock lk(mLock);
        if (mInfoByScheduleIdByClientId.find(clientId) == mInfoByScheduleIdByClientId.end()) {
            return Status::OK;
        }
        for (const auto& [_, scheduleInfo] : mInfoByScheduleIdByClientId[clientId]) {
            (*response->add_allscheduledtasks()) = *scheduleInfo.grpcScheduleInfo;
        }
    }
    return Status::OK;
}

bool TestWakeupClientServiceImpl::isWakeupRequired() {
    return mWakeupRequired;
}

bool TestWakeupClientServiceImpl::isRemoteTaskConnectionAlive() {
    return mRemoteTaskConnectionAlive;
}

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
