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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

using ::ndk::ScopedAStatus;

class MockGrpcClientStub : public WakeupClient::StubInterface {
  public:
    MOCK_METHOD(ClientReaderInterface<GetRemoteTasksResponse>*, GetRemoteTasksRaw,
                (ClientContext * context, const GetRemoteTasksRequest& request));
    MOCK_METHOD(Status, NotifyWakeupRequired,
                (ClientContext * context, const NotifyWakeupRequiredRequest& request,
                 NotifyWakeupRequiredResponse* response));
    // Async methods which we do not care.
    MOCK_METHOD(ClientAsyncReaderInterface<GetRemoteTasksResponse>*, AsyncGetRemoteTasksRaw,
                (ClientContext * context, const GetRemoteTasksRequest& request, CompletionQueue* cq,
                 void* tag));
    MOCK_METHOD(ClientAsyncReaderInterface<GetRemoteTasksResponse>*, PrepareAsyncGetRemoteTasksRaw,
                (ClientContext * context, const GetRemoteTasksRequest& request,
                 CompletionQueue* cq));
    MOCK_METHOD(ClientAsyncResponseReaderInterface<NotifyWakeupRequiredResponse>*,
                AsyncNotifyWakeupRequiredRaw,
                (ClientContext * context, const NotifyWakeupRequiredRequest& request,
                 CompletionQueue* cq));
    MOCK_METHOD(ClientAsyncResponseReaderInterface<NotifyWakeupRequiredResponse>*,
                PrepareAsyncNotifyWakeupRequiredRaw,
                (ClientContext * context, const NotifyWakeupRequiredRequest& request,
                 CompletionQueue* cq));
};

class RemoteAccessServiceUnitTest : public ::testing::Test {
  public:
    RemoteAccessServiceUnitTest() {
        mGrpcWakeupClientStub = std::make_unique<MockGrpcClientStub>();
        mService = ndk::SharedRefBase::make<RemoteAccessService>(mGrpcWakeupClientStub.get());
    }

    MockGrpcClientStub* getGrpcWakeupClientStub() { return mGrpcWakeupClientStub.get(); }

    RemoteAccessService* getService() { return mService.get(); }

  private:
    std::unique_ptr<MockGrpcClientStub> mGrpcWakeupClientStub;
    std::shared_ptr<RemoteAccessService> mService;
};

TEST_F(RemoteAccessServiceUnitTest, TestGetWakeupServiceName) {
    std::string serviceName;

    ScopedAStatus status = getService()->getWakeupServiceName(&serviceName);

    EXPECT_TRUE(status.isOk());
    EXPECT_EQ(serviceName, "com.google.vehicle.wakeup");
}

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
