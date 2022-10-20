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

#pragma once

#include <android-base/thread_annotations.h>
#include <wakeup_client.grpc.pb.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

// A class to generate fake task for testing. Not required for real implementation. In real
// implementation, the task should come from remote task server. This class is thread-safe.
class FakeTaskGenerator final {
  public:
    GetRemoteTasksResponse generateTask();

  private:
    // Simulates the client ID for each task.
    std::atomic<int> mCurrentClientId = 0;
    constexpr static uint8_t DATA[] = {0xde, 0xad, 0xbe, 0xef};
};

// TaskQueue is thread-safe.
class TaskQueue final {
  public:
    void add(const GetRemoteTasksResponse& response);
    std::optional<GetRemoteTasksResponse> maybePopOne();
    void waitForTask();
    void stopWait();

  private:
    std::mutex mLock;
    std::queue<GetRemoteTasksResponse> mTasks GUARDED_BY(mLock);
    // A variable to notify mTasks is not empty.
    std::condition_variable mTasksNotEmptyCv;
    bool mStopped GUARDED_BY(mLock);
};

class TestWakeupClientServiceImpl final : public WakeupClient::Service {
  public:
    TestWakeupClientServiceImpl();

    ~TestWakeupClientServiceImpl();

    grpc::Status GetRemoteTasks(grpc::ServerContext* context, const GetRemoteTasksRequest* request,
                                grpc::ServerWriter<GetRemoteTasksResponse>* writer) override;

    grpc::Status NotifyWakeupRequired(grpc::ServerContext* context,
                                      const NotifyWakeupRequiredRequest* request,
                                      NotifyWakeupRequiredResponse* response) override;

  private:
    // This is a thread for communicating with remote wakeup server (via network) and receive tasks
    // from it.
    std::thread mThread;
    // A variable to notify server is stopping.
    std::condition_variable mServerStoppedCv;
    std::mutex mLock;
    bool mServerStopped GUARDED_BY(mLock);

    // Thread-safe. For test impl only.
    FakeTaskGenerator mFakeTaskGenerator;
    // Thread-sfae.
    TaskQueue mTaskQueue;

    void fakeTaskGenerateLoop();
};

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
