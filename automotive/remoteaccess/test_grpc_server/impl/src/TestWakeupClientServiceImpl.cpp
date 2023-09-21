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

constexpr int kTaskIntervalInMs = 5'000;
constexpr int64_t KTaskTimeoutInMs = 20'000;

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

TaskQueue::TaskQueue() {
    mTaskTimeoutMessageHandler = android::sp<TaskTimeoutMessageHandler>::make(this);
    mLooper = Looper::prepare(/*opts=*/0);
    mCheckTaskTimeoutThread = std::thread([this] { checkForTestTimeoutLoop(); });
}

TaskQueue::~TaskQueue() {
    {
        std::lock_guard<std::mutex> lockGuard(mLock);
        mStopped = true;
    }
    while (true) {
        // Remove all pending timeout handlers from queue.
        if (!maybePopOne().has_value()) {
            break;
        }
    }
    if (mCheckTaskTimeoutThread.joinable()) {
        mCheckTaskTimeoutThread.join();
    }
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
    mLooper->sendMessageDelayed(KTaskTimeoutInMs * 1000, mTaskTimeoutMessageHandler, message);
    mTasksNotEmptyCv.notify_all();
}

void TaskQueue::waitForTask() {
    std::unique_lock<std::mutex> lock(mLock);
    waitForTaskWithLock(lock);
}

void TaskQueue::waitForTaskWithLock(std::unique_lock<std::mutex>& lock) {
    mTasksNotEmptyCv.wait(lock, [this] {
        ScopedLockAssertion lockAssertion(mLock);
        return mTasks.size() > 0 || mStopped;
    });
}

void TaskQueue::stopWait() {
    std::lock_guard<std::mutex> lockGuard(mLock);
    mStopped = true;
    mTasksNotEmptyCv.notify_all();
}

bool TaskQueue::isEmpty() {
    std::lock_guard<std::mutex> lockGuard(mLock);
    return mTasks.size() == 0 || mStopped;
}

void TaskQueue::checkForTestTimeoutLoop() {
    Looper::setForThread(mLooper);

    while (true) {
        {
            std::unique_lock<std::mutex> lock(mLock);
            if (mStopped) {
                return;
            }
        }

        mLooper->pollAll(/*timeoutMillis=*/-1);
    }
}

void TaskQueue::handleTaskTimeout() {
    // We know which task timed-out from the taskId in the message. However, there is no easy way
    // to remove a specific task with the task ID from the priority_queue, so we just check from
    // the top of the queue (which have the oldest tasks).
    std::lock_guard<std::mutex> lockGuard(mLock);
    int64_t now = uptimeMillis();
    while (mTasks.size() > 0) {
        const TaskInfo& taskInfo = mTasks.top();
        if (taskInfo.timestampInMs + KTaskTimeoutInMs > now) {
            break;
        }
        // In real implementation, this should report task failure to remote wakeup server.
        printf("Task for client ID: %s timed-out, added at %" PRId64 " ms, now %" PRId64 " ms",
               taskInfo.taskData.clientid().c_str(), taskInfo.timestampInMs, now);
        mTasks.pop();
    }
}

TestWakeupClientServiceImpl::TestWakeupClientServiceImpl() {}

TestWakeupClientServiceImpl::~TestWakeupClientServiceImpl() {
    { std::lock_guard<std::mutex> lockGuard(mLock); }
    mTaskQueue.stopWait();
    stopGeneratingFakeTask();
}

void TestWakeupClientServiceImpl::injectTask(const std::string& taskData,
                                             const std::string& clientId) {
    GetRemoteTasksResponse response;
    response.set_data(taskData);
    response.set_clientid(clientId);
    injectTaskResponse(response);
}

void TestWakeupClientServiceImpl::injectTaskResponse(const GetRemoteTasksResponse& response) {
    printf("Received a new task\n");
    mTaskQueue.add(response);
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
    mThread = std::thread([this, clientId] { fakeTaskGenerateLoop(clientId); });
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
    if (mThread.joinable()) {
        mThread.join();
    }
    printf("Stopped generating fake tasks\n");
}

void TestWakeupClientServiceImpl::fakeTaskGenerateLoop(const std::string& clientId) {
    // In actual implementation, this should communicate with the remote server and receives tasks
    // from it. Here we simulate receiving one remote task every {kTaskIntervalInMs}ms.
    while (true) {
        injectTaskResponse(mFakeTaskGenerator.generateTask(clientId));
        printf("Sleeping for %d seconds until next task\n", kTaskIntervalInMs);

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
        mTaskQueue.waitForTask();

        while (true) {
            auto maybeTask = mTaskQueue.maybePopOne();
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
                mTaskQueue.add(response);
                mRemoteTaskConnectionAlive = false;
                return Status::CANCELLED;
            }
        }
    }
    mRemoteTaskConnectionAlive = false;
    return Status::OK;
}

Status TestWakeupClientServiceImpl::NotifyWakeupRequired(ServerContext* context,
                                                         const NotifyWakeupRequiredRequest* request,
                                                         NotifyWakeupRequiredResponse* response) {
    printf("NotifyWakeupRequired called\n");
    if (request->iswakeuprequired() && !mWakeupRequired && !mTaskQueue.isEmpty()) {
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
