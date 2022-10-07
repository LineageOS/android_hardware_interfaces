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
#include <utils/Log.h>
#include <chrono>
#include <thread>

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

namespace {

using ::android::base::ScopedLockAssertion;
using ::android::base::StringPrintf;
using ::grpc::ServerContext;
using ::grpc::ServerWriter;
using ::grpc::Status;

constexpr int kTaskIntervalInSec = 5;

}  // namespace

GetRemoteTasksResponse FakeTaskGenerator::generateTask() {
    int clientId = mCurrentClientId++;
    GetRemoteTasksResponse response;
    response.set_data(std::string(reinterpret_cast<const char*>(DATA), sizeof(DATA)));
    std::string clientIdStr = StringPrintf("%d", clientId);
    response.set_clientid(clientIdStr);
    return response;
}

std::optional<GetRemoteTasksResponse> TaskQueue::maybePopOne() {
    std::lock_guard<std::mutex> lockGuard(mLock);
    if (mTasks.size() == 0) {
        return std::nullopt;
    }
    GetRemoteTasksResponse response = mTasks.front();
    mTasks.pop();
    return std::move(response);
}
void TaskQueue::add(const GetRemoteTasksResponse& task) {
    // TODO (b/246841306): add timeout to tasks.
    std::lock_guard<std::mutex> lockGuard(mLock);
    mTasks.push(task);
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
    std::lock_guard<std::mutex> lockGuard(mLock);
    mStopped = true;
    mTasksNotEmptyCv.notify_all();
}

TestWakeupClientServiceImpl::TestWakeupClientServiceImpl() {
    mThread = std::thread([this] { fakeTaskGenerateLoop(); });
}

TestWakeupClientServiceImpl::~TestWakeupClientServiceImpl() {
    {
        std::lock_guard<std::mutex> lockGuard(mLock);
        mServerStopped = true;
        mServerStoppedCv.notify_all();
    }
    mTaskQueue.stopWait();
    if (mThread.joinable()) {
        mThread.join();
    }
}

void TestWakeupClientServiceImpl::fakeTaskGenerateLoop() {
    // In actual implementation, this should communicate with the remote server and receives tasks
    // from it. Here we simulate receiving one remote task every {kTaskIntervalInSec}s.
    while (true) {
        mTaskQueue.add(mFakeTaskGenerator.generateTask());
        ALOGI("Sleeping for %d seconds until next task", kTaskIntervalInSec);

        std::unique_lock lk(mLock);
        if (mServerStoppedCv.wait_for(lk, std::chrono::seconds(kTaskIntervalInSec), [this] {
                ScopedLockAssertion lockAssertion(mLock);
                return mServerStopped;
            })) {
            // If the stopped flag is set, we are quitting, exit the loop.
            return;
        }
    }
}

Status TestWakeupClientServiceImpl::GetRemoteTasks(ServerContext* context,
                                                   const GetRemoteTasksRequest* request,
                                                   ServerWriter<GetRemoteTasksResponse>* writer) {
    ALOGD("GetRemoteTasks called");
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
                ALOGW("Failed to deliver remote task to remote access HAL");
                // The task failed to be sent, add it back to the queue. The order might change, but
                // it is okay.
                mTaskQueue.add(response);
                return Status::CANCELLED;
            }
        }
    }
    return Status::OK;
}

Status TestWakeupClientServiceImpl::NotifyWakeupRequired(ServerContext* context,
                                                         const NotifyWakeupRequiredRequest* request,
                                                         NotifyWakeupRequiredResponse* response) {
    return Status::OK;
}

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
