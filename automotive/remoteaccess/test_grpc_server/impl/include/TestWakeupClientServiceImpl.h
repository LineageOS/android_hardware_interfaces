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
#include <utils/Looper.h>
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
    GetRemoteTasksResponse generateTask(const std::string& clientId);

  private:
    constexpr static uint8_t DATA[] = {0xde, 0xad, 0xbe, 0xef};
};

struct TaskInfo {
    // This is unique per-task. Note that a task might be popped and put back into the task queue,
    // it will have a new task ID but the same clientId in the task data.
    int taskId;
    int64_t timestampInMs;
    GetRemoteTasksResponse taskData;
};

struct TaskInfoComparator {
    // We want the smallest timestamp and smallest task ID on top.
    bool operator()(const TaskInfo& l, const TaskInfo& r) {
        return l.timestampInMs > r.timestampInMs ||
               (l.timestampInMs == r.timestampInMs && l.taskId > r.taskId);
    }
};

// forward-declaration.
class TaskQueue;

class TaskTimeoutMessageHandler final : public android::MessageHandler {
  public:
    TaskTimeoutMessageHandler(TaskQueue* taskQueue);
    void handleMessage(const android::Message& message) override;

  private:
    TaskQueue* mTaskQueue;
};

// TaskQueue is thread-safe.
class TaskQueue final {
  public:
    TaskQueue(android::sp<Looper> looper);

    void add(const GetRemoteTasksResponse& response);
    std::optional<GetRemoteTasksResponse> maybePopOne();
    void waitForTask();
    void stopWait();
    bool isEmpty();

  private:
    friend class TaskTimeoutMessageHandler;

    std::mutex mLock;
    std::priority_queue<TaskInfo, std::vector<TaskInfo>, TaskInfoComparator> mTasks
            GUARDED_BY(mLock);
    // A variable to notify mTasks is not empty.
    std::condition_variable mTasksNotEmptyCv;
    std::atomic<bool> mStopped;
    android::sp<Looper> mLooper;
    android::sp<TaskTimeoutMessageHandler> mTaskTimeoutMessageHandler;
    std::atomic<int> mTaskIdCounter = 0;

    void loop();
    void handleTaskTimeout();
};

// forward-declaration
class TestWakeupClientServiceImpl;

class TaskScheduleMsgHandler final : public android::MessageHandler {
  public:
    TaskScheduleMsgHandler(TestWakeupClientServiceImpl* mImpl);
    void handleMessage(const android::Message& message) override;

  private:
    TestWakeupClientServiceImpl* mImpl;
};

class TestWakeupClientServiceImpl : public WakeupClient::Service {
  public:
    TestWakeupClientServiceImpl();

    ~TestWakeupClientServiceImpl();

    // Stop the handling for all income requests. Prepare for shutdown.
    void stopServer();

    grpc::Status GetRemoteTasks(grpc::ServerContext* context, const GetRemoteTasksRequest* request,
                                grpc::ServerWriter<GetRemoteTasksResponse>* writer) override;

    grpc::Status NotifyWakeupRequired(grpc::ServerContext* context,
                                      const NotifyWakeupRequiredRequest* request,
                                      NotifyWakeupRequiredResponse* response) override;

    grpc::Status ScheduleTask(grpc::ServerContext* context, const ScheduleTaskRequest* request,
                              ScheduleTaskResponse* response) override;

    grpc::Status UnscheduleTask(grpc::ServerContext* context, const UnscheduleTaskRequest* request,
                                UnscheduleTaskResponse* response) override;

    grpc::Status UnscheduleAllTasks(grpc::ServerContext* context,
                                    const UnscheduleAllTasksRequest* request,
                                    UnscheduleAllTasksResponse* response) override;

    grpc::Status IsTaskScheduled(grpc::ServerContext* context,
                                 const IsTaskScheduledRequest* request,
                                 IsTaskScheduledResponse* response) override;

    grpc::Status GetAllScheduledTasks(grpc::ServerContext* context,
                                      const GetAllScheduledTasksRequest* request,
                                      GetAllScheduledTasksResponse* response) override;

    /**
     * Starts generating fake tasks for the specific client repeatedly.
     *
     * The fake task will have {0xDE 0xAD 0xBE 0xEF} as payload. A new fake task will be sent
     * to the client every 5s.
     */
    void startGeneratingFakeTask(const std::string& clientId);

    /**
     * stops generating fake tasks.
     */
    void stopGeneratingFakeTask();

    /**
     * Returns whether we need to wakeup the target device to send remote tasks.
     */
    bool isWakeupRequired();

    /**
     * Returns whether we have an active connection with the target device.
     */
    bool isRemoteTaskConnectionAlive();

    /**
     * Injects a fake task with taskData to be sent to the specific client.
     */
    void injectTask(const std::string& taskData, const std::string& clientId);

    /**
     * Wakes up the target device.
     *
     * This must be implemented by child class and contains device specific logic. E.g. this might
     * be sending QEMU commands for the emulator device.
     */
    virtual void wakeupApplicationProcessor() = 0;

    /**
     * Cleans up a scheduled task info.
     */
    void cleanupScheduledTaskLocked(const std::string& clientId, const std::string& scheduleId)
            REQUIRES(mLock);

  private:
    friend class TaskScheduleMsgHandler;

    struct ScheduleInfo {
        std::unique_ptr<GrpcScheduleInfo> grpcScheduleInfo;
        // This is a unique ID to represent this schedule. Each repeated tasks will have different
        // task ID but will have the same scheduleMsgId so that we can use to unschedule. This has
        // to be an int so we cannot use the scheduleId provided by the client.
        int scheduleMsgId;
        int64_t periodicInSeconds;
        int32_t currentCount;
        int32_t totalCount;
    };

    std::atomic<int> mScheduleMsgCounter = 0;
    // This is a looper for scheduling tasks to be executed in the future.
    android::sp<Looper> mLooper;
    android::sp<TaskScheduleMsgHandler> mTaskScheduleMsgHandler;
    // This is a thread for generating fake tasks.
    std::thread mFakeTaskThread;
    // This is a thread for the looper.
    std::thread mLooperThread;
    // A variable to notify server is stopping.
    std::condition_variable mTaskLoopStoppedCv;
    // Whether wakeup AP is required for executing tasks.
    std::atomic<bool> mWakeupRequired = true;
    // Whether we currently have an active long-live connection to deliver remote tasks.
    std::atomic<bool> mRemoteTaskConnectionAlive = false;
    std::mutex mLock;
    bool mGeneratingFakeTask GUARDED_BY(mLock);
    std::atomic<bool> mServerStopped;
    std::unordered_map<std::string, std::unordered_map<std::string, ScheduleInfo>>
            mInfoByScheduleIdByClientId GUARDED_BY(mLock);

    // Thread-safe. For test impl only.
    FakeTaskGenerator mFakeTaskGenerator;
    // Thread-safe.
    std::unique_ptr<TaskQueue> mTaskQueue;

    void fakeTaskGenerateLoop(const std::string& clientId);
    void injectTaskResponse(const GetRemoteTasksResponse& response);
    bool getScheduleInfoLocked(int scheduleMsgId, ScheduleInfo** outScheduleInfoPtr)
            REQUIRES(mLock);
    void handleAddTask(int scheduleMsgId);
    void loop();
};

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
