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

#include <RemoteAccessService.h>
#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <gmock/gmock.h>
#include <grpcpp/test/mock_stream.h>
#include <wakeup_client.grpc.pb.h>

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

using ::grpc::ClientAsyncReaderInterface;
using ::grpc::ClientAsyncResponseReaderInterface;
using ::grpc::ClientContext;
using ::grpc::ClientReader;
using ::grpc::ClientReaderInterface;
using ::grpc::CompletionQueue;
using ::grpc::Status;
using ::grpc::testing::MockClientReader;
using ::testing::_;
using ::testing::Return;

class MockGrpcClientStub : public WakeupClient::StubInterface {
  public:
    ClientReaderInterface<GetRemoteTasksResponse>* GetRemoteTasksRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const GetRemoteTasksRequest& request) override {
        MockClientReader<GetRemoteTasksResponse>* mockClientReader =
                new MockClientReader<GetRemoteTasksResponse>();
        ON_CALL(*mockClientReader, Finish()).WillByDefault(Return(Status::OK));
        ON_CALL(*mockClientReader, Read(_)).WillByDefault(Return(false));
        return mockClientReader;
    }

    Status NotifyWakeupRequired([[maybe_unused]] ClientContext* context,
                                [[maybe_unused]] const NotifyWakeupRequiredRequest& request,
                                [[maybe_unused]] NotifyWakeupRequiredResponse* response) {
        return Status::OK;
    }

    Status ScheduleTask(ClientContext* context, const ScheduleTaskRequest& request,
                        ScheduleTaskResponse* response) {
        return Status::OK;
    }

    Status UnscheduleTask(ClientContext* context, const UnscheduleTaskRequest& request,
                          UnscheduleTaskResponse* response) {
        return Status::OK;
    }

    Status UnscheduleAllTasks(ClientContext* context, const UnscheduleAllTasksRequest& request,
                              UnscheduleAllTasksResponse* response) {
        return Status::OK;
    }

    Status IsTaskScheduled(ClientContext* context, const IsTaskScheduledRequest& request,
                           IsTaskScheduledResponse* response) {
        return Status::OK;
    }

    Status GetAllScheduledTasks(ClientContext* context, const GetAllScheduledTasksRequest& request,
                                GetAllScheduledTasksResponse* response) {
        return Status::OK;
    }

    // Async methods which we do not care.
    ClientAsyncReaderInterface<GetRemoteTasksResponse>* AsyncGetRemoteTasksRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const GetRemoteTasksRequest& request,
            [[maybe_unused]] CompletionQueue* cq, [[maybe_unused]] void* tag) {
        return nullptr;
    }

    ClientAsyncReaderInterface<GetRemoteTasksResponse>* PrepareAsyncGetRemoteTasksRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const GetRemoteTasksRequest& request,
            [[maybe_unused]] CompletionQueue* cq) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<NotifyWakeupRequiredResponse>* AsyncNotifyWakeupRequiredRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const NotifyWakeupRequiredRequest& request,
            [[maybe_unused]] CompletionQueue* cq) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<NotifyWakeupRequiredResponse>*
    PrepareAsyncNotifyWakeupRequiredRaw([[maybe_unused]] ClientContext* context,
                                        [[maybe_unused]] const NotifyWakeupRequiredRequest& request,
                                        [[maybe_unused]] CompletionQueue* c) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<ScheduleTaskResponse>* AsyncScheduleTaskRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const ScheduleTaskRequest& request,
            [[maybe_unused]] CompletionQueue* cq) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<ScheduleTaskResponse>* PrepareAsyncScheduleTaskRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const ScheduleTaskRequest& request,
            [[maybe_unused]] CompletionQueue* c) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<UnscheduleTaskResponse>* AsyncUnscheduleTaskRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const UnscheduleTaskRequest& request,
            [[maybe_unused]] CompletionQueue* cq) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<UnscheduleTaskResponse>* PrepareAsyncUnscheduleTaskRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const UnscheduleTaskRequest& request,
            [[maybe_unused]] CompletionQueue* c) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<UnscheduleAllTasksResponse>* AsyncUnscheduleAllTasksRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const UnscheduleAllTasksRequest& request,
            [[maybe_unused]] CompletionQueue* cq) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<UnscheduleAllTasksResponse>*
    PrepareAsyncUnscheduleAllTasksRaw([[maybe_unused]] ClientContext* context,
                                      [[maybe_unused]] const UnscheduleAllTasksRequest& request,
                                      [[maybe_unused]] CompletionQueue* c) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<IsTaskScheduledResponse>* AsyncIsTaskScheduledRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const IsTaskScheduledRequest& request,
            [[maybe_unused]] CompletionQueue* cq) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<IsTaskScheduledResponse>* PrepareAsyncIsTaskScheduledRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const IsTaskScheduledRequest& request,
            [[maybe_unused]] CompletionQueue* c) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<GetAllScheduledTasksResponse>* AsyncGetAllScheduledTasksRaw(
            [[maybe_unused]] ClientContext* context,
            [[maybe_unused]] const GetAllScheduledTasksRequest& request,
            [[maybe_unused]] CompletionQueue* cq) {
        return nullptr;
    }

    ClientAsyncResponseReaderInterface<GetAllScheduledTasksResponse>*
    PrepareAsyncGetAllScheduledTasksRaw([[maybe_unused]] ClientContext* context,
                                        [[maybe_unused]] const GetAllScheduledTasksRequest& request,
                                        [[maybe_unused]] CompletionQueue* c) {
        return nullptr;
    }
};

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    android::hardware::automotive::remoteaccess::MockGrpcClientStub stub;
    std::shared_ptr<android::hardware::automotive::remoteaccess::RemoteAccessService> service =
            ndk::SharedRefBase::make<
                    android::hardware::automotive::remoteaccess::RemoteAccessService>(&stub);
    android::fuzzService(service->asBinder().get(), FuzzedDataProvider(data, size));

    return 0;
}
