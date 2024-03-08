/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <gtest/gtest.h>
#include <chrono>

namespace android::hardware::automotive::remoteaccess::test {

using ::android::base::ScopedLockAssertion;

using ::grpc::Channel;
using ::grpc::ClientContext;
using ::grpc::Server;
using ::grpc::ServerBuilder;
using ::grpc::Status;

const std::string kTestClientId = "test client id";
const std::string kTestScheduleId = "test schedule id";
const std::vector<uint8_t> kTestData = {0xde, 0xad, 0xbe, 0xef};
constexpr int32_t kTestCount = 1234;
constexpr int64_t kTestStartTimeInEpochSeconds = 2345;
constexpr int64_t kTestPeriodicInSeconds = 123;
const std::string kTestGrpcAddr = "localhost:50051";

class MyTestWakeupClientServiceImpl final : public TestWakeupClientServiceImpl {
  public:
    void wakeupApplicationProcessor() override {
        // Do nothing.
    }
};

class TestWakeupClientServiceImplUnitTest : public ::testing::Test {
  public:
    virtual void SetUp() override {
        mServerThread = std::thread([this] {
            {
                std::unique_lock<std::mutex> lock(mLock);
                mService = std::make_unique<MyTestWakeupClientServiceImpl>();
                ServerBuilder builder;
                builder.AddListeningPort(kTestGrpcAddr, grpc::InsecureServerCredentials());
                builder.RegisterService(mService.get());
                mServer = builder.BuildAndStart();
                mServerStartCv.notify_one();
            }
            mServer->Wait();
        });
        {
            std::unique_lock<std::mutex> lock(mLock);
            mServerStartCv.wait(lock, [this] {
                ScopedLockAssertion lockAssertion(mLock);
                return mServer != nullptr;
            });
        }
        mChannel = grpc::CreateChannel(kTestGrpcAddr, grpc::InsecureChannelCredentials());
        mStub = WakeupClient::NewStub(mChannel);
    }

    virtual void TearDown() override {
        printf("Start server shutdown\n");
        mService->stopServer();
        mServer->Shutdown();
        printf("Server shutdown complete\n");
        mServerThread.join();
        printf("Server thread exits\n");
        mServer.reset();
        mService.reset();
        printf("Server and service classes reset\n");
    }

    WakeupClient::Stub* getStub() { return mStub.get(); }

    size_t waitForRemoteTasks(size_t count) {
        ClientContext context = {};
        GetRemoteTasksResponse response;
        auto reader = mStub->GetRemoteTasks(&context, GetRemoteTasksRequest{});
        size_t got = 0;
        while (reader->Read(&response)) {
            got++;
            mRemoteTaskResponses.push_back(response);
            if (got == count) {
                break;
            }
        }
        // If there is more messages to be read in the reader, cancel them all so that we can
        // finish.
        context.TryCancel();
        reader->Finish();
        return got;
    }

    std::vector<GetRemoteTasksResponse> getRemoteTaskResponses() { return mRemoteTaskResponses; }

    Status scheduleTask(int32_t count, int64_t startTimeInEpochSeconds, int64_t periodicInSeconds) {
        return scheduleTask(kTestScheduleId, count, startTimeInEpochSeconds, periodicInSeconds);
    }

    Status scheduleTask(const std::string& scheduleId, int32_t count,
                        int64_t startTimeInEpochSeconds, int64_t periodicInSeconds) {
        ClientContext context;
        ScheduleTaskRequest request;
        ScheduleTaskResponse response;
        int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
        request.mutable_scheduleinfo()->set_clientid(kTestClientId);
        request.mutable_scheduleinfo()->set_scheduleid(scheduleId);
        request.mutable_scheduleinfo()->set_data(kTestData.data(), kTestData.size());
        request.mutable_scheduleinfo()->set_count(count);
        request.mutable_scheduleinfo()->set_starttimeinepochseconds(startTimeInEpochSeconds);
        request.mutable_scheduleinfo()->set_periodicinseconds(periodicInSeconds);

        return getStub()->ScheduleTask(&context, request, &response);
    }

    int64_t getNow() {
        return std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                .count();
    }

  private:
    std::condition_variable mServerStartCv;
    std::mutex mLock;
    std::thread mServerThread;
    std::unique_ptr<MyTestWakeupClientServiceImpl> mService;
    std::unique_ptr<Server> mServer;
    std::shared_ptr<Channel> mChannel;
    std::unique_ptr<WakeupClient::Stub> mStub;
    std::vector<GetRemoteTasksResponse> mRemoteTaskResponses;
};

TEST_F(TestWakeupClientServiceImplUnitTest, TestScheduleTask) {
    ClientContext context = {};
    ScheduleTaskRequest request = {};
    ScheduleTaskResponse response = {};

    request.mutable_scheduleinfo()->set_clientid(kTestClientId);
    request.mutable_scheduleinfo()->set_scheduleid(kTestScheduleId);
    request.mutable_scheduleinfo()->set_data(kTestData.data(), kTestData.size());
    request.mutable_scheduleinfo()->set_count(2);
    // Schedule the task to be executed 1s later.
    request.mutable_scheduleinfo()->set_starttimeinepochseconds(getNow() + 1);
    request.mutable_scheduleinfo()->set_periodicinseconds(1);

    Status status = getStub()->ScheduleTask(&context, request, &response);

    ASSERT_TRUE(status.ok());
    ASSERT_EQ(response.errorcode(), ErrorCode::OK);

    size_t gotTaskCount = waitForRemoteTasks(/*count=*/2);

    EXPECT_EQ(gotTaskCount, 2);
    auto responses = getRemoteTaskResponses();
    for (const auto& response : responses) {
        EXPECT_EQ(response.clientid(), kTestClientId);
        EXPECT_EQ(response.data(), std::string(kTestData.begin(), kTestData.end()));
    }
}

TEST_F(TestWakeupClientServiceImplUnitTest, TestScheduleTask_conflictScheduleId) {
    Status status = scheduleTask(/*count=*/2, /*startTimeInEpochSeconds=*/getNow() + 1,
                                 /*periodicInSeconds=*/1);

    ASSERT_TRUE(status.ok());

    // Schedule the same task again.
    ClientContext context = {};
    ScheduleTaskRequest request = {};
    ScheduleTaskResponse response = {};

    request.mutable_scheduleinfo()->set_clientid(kTestClientId);
    request.mutable_scheduleinfo()->set_scheduleid(kTestScheduleId);
    request.mutable_scheduleinfo()->set_data(kTestData.data(), kTestData.size());
    request.mutable_scheduleinfo()->set_count(2);
    request.mutable_scheduleinfo()->set_starttimeinepochseconds(getNow() + 1);
    request.mutable_scheduleinfo()->set_periodicinseconds(1);

    status = getStub()->ScheduleTask(&context, request, &response);

    ASSERT_TRUE(status.ok());
    ASSERT_EQ(response.errorcode(), ErrorCode::INVALID_ARG);
}

TEST_F(TestWakeupClientServiceImplUnitTest, TestUnscheduleTask) {
    Status status = scheduleTask(/*count=*/2, /*startTimeInEpochSeconds=*/getNow() + 1,
                                 /*periodicInSeconds=*/1);

    ASSERT_TRUE(status.ok());

    ClientContext context;
    UnscheduleTaskRequest request;
    UnscheduleTaskResponse response;
    request.set_clientid(kTestClientId);
    request.set_scheduleid(kTestScheduleId);
    status = getStub()->UnscheduleTask(&context, request, &response);

    ASSERT_TRUE(status.ok());

    sleep(2);

    // There should be no remote tasks received after 2s because the task was unscheduled.
    EXPECT_EQ(getRemoteTaskResponses().size(), 0);
}

TEST_F(TestWakeupClientServiceImplUnitTest, TestIsTaskScheduled) {
    int64_t startTimeInEpochSeconds = getNow() + 1;
    int64_t periodicInSeconds = 1234;

    Status status = scheduleTask(/*count=*/2, startTimeInEpochSeconds, periodicInSeconds);

    ASSERT_TRUE(status.ok());

    ClientContext context;
    IsTaskScheduledRequest request;
    IsTaskScheduledResponse response;
    request.set_clientid(kTestClientId);
    request.set_scheduleid(kTestScheduleId);
    status = getStub()->IsTaskScheduled(&context, request, &response);

    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(response.istaskscheduled());

    ClientContext context2;
    IsTaskScheduledRequest request2;
    IsTaskScheduledResponse response2;
    request.set_clientid(kTestClientId);
    request.set_scheduleid("invalid id");
    status = getStub()->IsTaskScheduled(&context2, request2, &response2);

    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(response2.istaskscheduled());
}

TEST_F(TestWakeupClientServiceImplUnitTest, TestUnscheduleAllTasks) {
    std::string scheduleId1 = "scheduleId1";
    std::string scheduleId2 = "scheduleId2";
    int64_t time1 = getNow();
    int64_t time2 = getNow() + 1;
    int64_t periodicInSeconds1 = 1;
    int64_t periodicInSeconds2 = 1;
    int32_t count1 = 2;
    int64_t count2 = 5;

    Status status = scheduleTask(scheduleId1, count1, time1, periodicInSeconds1);
    ASSERT_TRUE(status.ok());
    status = scheduleTask(scheduleId2, count2, time2, periodicInSeconds2);
    ASSERT_TRUE(status.ok());

    ClientContext context;
    UnscheduleAllTasksRequest request;
    UnscheduleAllTasksResponse response;
    request.set_clientid(kTestClientId);
    status = getStub()->UnscheduleAllTasks(&context, request, &response);
    ASSERT_TRUE(status.ok());

    sleep(2);

    // There should be no remote tasks received after 2s because the tasks were unscheduled.
    EXPECT_EQ(getRemoteTaskResponses().size(), 0);
}

TEST_F(TestWakeupClientServiceImplUnitTest, TestGetAllScheduledTasks) {
    std::string scheduleId1 = "scheduleId1";
    std::string scheduleId2 = "scheduleId2";
    int64_t time1 = getNow();
    int64_t time2 = getNow() + 1;
    int64_t periodicInSeconds1 = 1;
    int64_t periodicInSeconds2 = 1;
    int32_t count1 = 2;
    int64_t count2 = 5;

    Status status = scheduleTask(scheduleId1, count1, time1, periodicInSeconds1);
    ASSERT_TRUE(status.ok());
    status = scheduleTask(scheduleId2, count2, time2, periodicInSeconds2);
    ASSERT_TRUE(status.ok());

    ClientContext context;
    GetAllScheduledTasksRequest request;
    GetAllScheduledTasksResponse response;
    request.set_clientid("invalid client Id");
    status = getStub()->GetAllScheduledTasks(&context, request, &response);

    ASSERT_TRUE(status.ok());
    EXPECT_EQ(response.allscheduledtasks_size(), 0);

    ClientContext context2;
    GetAllScheduledTasksRequest request2;
    GetAllScheduledTasksResponse response2;
    request2.set_clientid(kTestClientId);
    status = getStub()->GetAllScheduledTasks(&context2, request2, &response2);

    ASSERT_TRUE(status.ok());
    ASSERT_EQ(response2.allscheduledtasks_size(), 2);
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(response2.allscheduledtasks(i).clientid(), kTestClientId);
        if (response2.allscheduledtasks(i).scheduleid() == scheduleId1) {
            EXPECT_EQ(response2.allscheduledtasks(i).data(),
                      std::string(kTestData.begin(), kTestData.end()));
            EXPECT_EQ(response2.allscheduledtasks(i).count(), count1);
            EXPECT_EQ(response2.allscheduledtasks(i).starttimeinepochseconds(), time1);
            EXPECT_EQ(response2.allscheduledtasks(i).periodicinseconds(), periodicInSeconds1);
        } else {
            EXPECT_EQ(response2.allscheduledtasks(i).scheduleid(), scheduleId2);
            EXPECT_EQ(response2.allscheduledtasks(i).data(),
                      std::string(kTestData.begin(), kTestData.end()));
            EXPECT_EQ(response2.allscheduledtasks(i).count(), count2);
            EXPECT_EQ(response2.allscheduledtasks(i).starttimeinepochseconds(), time2);
            EXPECT_EQ(response2.allscheduledtasks(i).periodicinseconds(), periodicInSeconds2);
        }
    }
}

}  // namespace android::hardware::automotive::remoteaccess::test

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
