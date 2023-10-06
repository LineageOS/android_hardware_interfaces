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

#include <AidlHalPropValue.h>
#include <IVhalClient.h>
#include <aidl/android/hardware/automotive/remoteaccess/ApState.h>
#include <aidl/android/hardware/automotive/remoteaccess/BnRemoteTaskCallback.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropValue.h>
#include <gmock/gmock.h>
#include <grpcpp/test/mock_stream.h>
#include <gtest/gtest.h>
#include <wakeup_client.grpc.pb.h>
#include <chrono>
#include <thread>

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

namespace {

using ::android::base::ScopedLockAssertion;
using ::android::frameworks::automotive::vhal::AidlHalPropValue;
using ::android::frameworks::automotive::vhal::IHalPropConfig;
using ::android::frameworks::automotive::vhal::IHalPropValue;
using ::android::frameworks::automotive::vhal::ISubscriptionCallback;
using ::android::frameworks::automotive::vhal::ISubscriptionClient;
using ::android::frameworks::automotive::vhal::IVhalClient;
using ::android::frameworks::automotive::vhal::VhalClientResult;

using ::aidl::android::hardware::automotive::remoteaccess::ApState;
using ::aidl::android::hardware::automotive::remoteaccess::BnRemoteTaskCallback;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

using ::grpc::ClientAsyncReaderInterface;
using ::grpc::ClientAsyncResponseReaderInterface;
using ::grpc::ClientContext;
using ::grpc::ClientReader;
using ::grpc::ClientReaderInterface;
using ::grpc::CompletionQueue;
using ::grpc::Status;
using ::grpc::testing::MockClientReader;
using ::ndk::ScopedAStatus;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

constexpr char kTestVin[] = "test_VIN";

}  // namespace

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

class FakeVhalClient final : public android::frameworks::automotive::vhal::IVhalClient {
  public:
    inline bool isAidlVhal() { return true; }

    VhalClientResult<std::unique_ptr<IHalPropValue>> getValueSync(
            const IHalPropValue& requestValue) override {
        auto propValue = std::make_unique<AidlHalPropValue>(requestValue.getPropId());
        propValue->setStringValue(kTestVin);
        return propValue;
    }

    std::unique_ptr<IHalPropValue> createHalPropValue(int32_t propId) override {
        return std::make_unique<AidlHalPropValue>(propId);
    }

    // Functions we do not care.
    std::unique_ptr<IHalPropValue> createHalPropValue([[maybe_unused]] int32_t propId,
                                                      [[maybe_unused]] int32_t areaId) override {
        return nullptr;
    }

    void getValue([[maybe_unused]] const IHalPropValue& requestValue,
                  [[maybe_unused]] std::shared_ptr<GetValueCallbackFunc> callback) override {}

    void setValue([[maybe_unused]] const IHalPropValue& requestValue,
                  [[maybe_unused]] std::shared_ptr<SetValueCallbackFunc> callback) override {}

    VhalClientResult<void> setValueSync([[maybe_unused]] const IHalPropValue& requestValue) {
        return {};
    }

    VhalClientResult<void> addOnBinderDiedCallback(
            [[maybe_unused]] std::shared_ptr<OnBinderDiedCallbackFunc> callback) override {
        return {};
    }

    VhalClientResult<void> removeOnBinderDiedCallback(
            [[maybe_unused]] std::shared_ptr<OnBinderDiedCallbackFunc> callback) override {
        return {};
    }

    VhalClientResult<std::vector<std::unique_ptr<IHalPropConfig>>> getAllPropConfigs() override {
        return std::vector<std::unique_ptr<IHalPropConfig>>();
    }

    VhalClientResult<std::vector<std::unique_ptr<IHalPropConfig>>> getPropConfigs(
            [[maybe_unused]] std::vector<int32_t> propIds) override {
        return std::vector<std::unique_ptr<IHalPropConfig>>();
    }

    std::unique_ptr<ISubscriptionClient> getSubscriptionClient(
            [[maybe_unused]] std::shared_ptr<ISubscriptionCallback> callback) override {
        return nullptr;
    }
};

class FakeRemoteTaskCallback : public BnRemoteTaskCallback {
  public:
    ScopedAStatus onRemoteTaskRequested(const std::string& clientId,
                                        const std::vector<uint8_t>& data) override {
        std::lock_guard<std::mutex> lockGuard(mLock);
        mDataByClientId[clientId] = data;
        mTaskCount++;
        mCv.notify_all();
        return ScopedAStatus::ok();
    }

    std::vector<uint8_t> getData(const std::string& clientId) { return mDataByClientId[clientId]; }

    bool wait(size_t taskCount, size_t timeoutInSec) {
        std::unique_lock<std::mutex> lock(mLock);
        return mCv.wait_for(lock, std::chrono::seconds(timeoutInSec), [taskCount, this] {
            ScopedLockAssertion lockAssertion(mLock);
            return mTaskCount >= taskCount;
        });
    }

  private:
    std::mutex mLock;
    std::unordered_map<std::string, std::vector<uint8_t>> mDataByClientId GUARDED_BY(mLock);
    size_t mTaskCount GUARDED_BY(mLock) = 0;
    std::condition_variable mCv;
};

class RemoteAccessServiceUnitTest : public ::testing::Test {
  public:
    virtual void SetUp() override {
        mGrpcWakeupClientStub = std::make_unique<MockGrpcClientStub>();
        mService = ndk::SharedRefBase::make<RemoteAccessService>(mGrpcWakeupClientStub.get());
    }

    MockGrpcClientStub* getGrpcWakeupClientStub() { return mGrpcWakeupClientStub.get(); }

    RemoteAccessService* getService() { return mService.get(); }

    void setRetryWaitInMs(size_t retryWaitInMs) { mService->setRetryWaitInMs(retryWaitInMs); }

    ScopedAStatus getVehicleIdWithClient(IVhalClient& vhalClient, std::string* vehicleId) {
        return mService->getVehicleIdWithClient(vhalClient, vehicleId);
    }

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

TEST_F(RemoteAccessServiceUnitTest, TestNotifyApStateChangeWakeupRequired) {
    bool isWakeupRequired = false;
    EXPECT_CALL(*getGrpcWakeupClientStub(), NotifyWakeupRequired)
            .WillOnce([&isWakeupRequired]([[maybe_unused]] ClientContext* context,
                                          const NotifyWakeupRequiredRequest& request,
                                          [[maybe_unused]] NotifyWakeupRequiredResponse* response) {
                isWakeupRequired = request.iswakeuprequired();
                return Status();
            });

    ApState newState = {
            .isWakeupRequired = true,
    };
    ScopedAStatus status = getService()->notifyApStateChange(newState);

    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(isWakeupRequired);
}

TEST_F(RemoteAccessServiceUnitTest, TestGetRemoteTasks) {
    GetRemoteTasksResponse response1;
    std::vector<uint8_t> testData = {0xde, 0xad, 0xbe, 0xef};
    response1.set_clientid("1");
    response1.set_data(testData.data(), testData.size());
    GetRemoteTasksResponse response2;
    response2.set_clientid("2");
    std::shared_ptr<FakeRemoteTaskCallback> callback =
            ndk::SharedRefBase::make<FakeRemoteTaskCallback>();

    ON_CALL(*getGrpcWakeupClientStub(), GetRemoteTasksRaw)
            .WillByDefault(
                    [response1, response2]([[maybe_unused]] ClientContext* context,
                                           [[maybe_unused]] const GetRemoteTasksRequest& request) {
                        // mockReader ownership will be transferred to the client so we don't own it
                        // here.
                        MockClientReader<GetRemoteTasksResponse>* mockClientReader =
                                new MockClientReader<GetRemoteTasksResponse>();
                        EXPECT_CALL(*mockClientReader, Finish()).WillOnce(Return(Status::OK));
                        EXPECT_CALL(*mockClientReader, Read(_))
                                .WillOnce(DoAll(SetArgPointee<0>(response1), Return(true)))
                                .WillOnce(DoAll(SetArgPointee<0>(response2), Return(true)))
                                .WillRepeatedly(Return(false));
                        return mockClientReader;
                    });

    getService()->setRemoteTaskCallback(callback);
    // Start the long live connection to receive tasks.
    ApState newState = {
            .isReadyForRemoteTask = true,
    };
    ASSERT_TRUE(getService()->notifyApStateChange(newState).isOk());

    ASSERT_TRUE(callback->wait(/*taskCount=*/2, /*timeoutInSec=*/10))
            << "Did not receive enough tasks";
    EXPECT_EQ(callback->getData("1"), testData);
    EXPECT_EQ(callback->getData("2"), std::vector<uint8_t>());
}

TEST_F(RemoteAccessServiceUnitTest, TestGetRemoteTasksRetryConnection) {
    GetRemoteTasksResponse response;
    std::shared_ptr<FakeRemoteTaskCallback> callback =
            ndk::SharedRefBase::make<FakeRemoteTaskCallback>();

    ON_CALL(*getGrpcWakeupClientStub(), GetRemoteTasksRaw)
            .WillByDefault([response]([[maybe_unused]] ClientContext* context,
                                      [[maybe_unused]] const GetRemoteTasksRequest& request) {
                // mockReader ownership will be transferred to the client so we don't own it here.
                MockClientReader<GetRemoteTasksResponse>* mockClientReader =
                        new MockClientReader<GetRemoteTasksResponse>();
                EXPECT_CALL(*mockClientReader, Finish()).WillOnce(Return(Status::OK));
                // Connection fails after receiving one task. Should retry after some time.
                EXPECT_CALL(*mockClientReader, Read(_))
                        .WillOnce(DoAll(SetArgPointee<0>(response), Return(true)))
                        .WillRepeatedly(Return(false));
                return mockClientReader;
            });

    getService()->setRemoteTaskCallback(callback);
    setRetryWaitInMs(100);
    // Start the long live connection to receive tasks.
    ApState newState = {
            .isReadyForRemoteTask = true,
    };
    ASSERT_TRUE(getService()->notifyApStateChange(newState).isOk());

    ASSERT_TRUE(callback->wait(/*taskCount=*/2, /*timeoutInSec=*/10))
            << "Did not receive enough tasks";
}

TEST_F(RemoteAccessServiceUnitTest, TestGetRemoteTasksDefaultNotReady) {
    GetRemoteTasksResponse response1;
    std::vector<uint8_t> testData = {0xde, 0xad, 0xbe, 0xef};
    response1.set_clientid("1");
    response1.set_data(testData.data(), testData.size());
    GetRemoteTasksResponse response2;
    response2.set_clientid("2");
    std::shared_ptr<FakeRemoteTaskCallback> callback =
            ndk::SharedRefBase::make<FakeRemoteTaskCallback>();

    EXPECT_CALL(*getGrpcWakeupClientStub(), GetRemoteTasksRaw).Times(0);

    // Default state is not ready for remote tasks, so no callback will be called.
    getService()->setRemoteTaskCallback(callback);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(RemoteAccessServiceUnitTest, TestGetRemoteTasksNotReadyAfterReady) {
    GetRemoteTasksResponse response1;
    std::vector<uint8_t> testData = {0xde, 0xad, 0xbe, 0xef};
    response1.set_clientid("1");
    response1.set_data(testData.data(), testData.size());
    GetRemoteTasksResponse response2;
    response2.set_clientid("2");
    std::shared_ptr<FakeRemoteTaskCallback> callback =
            ndk::SharedRefBase::make<FakeRemoteTaskCallback>();

    ON_CALL(*getGrpcWakeupClientStub(), GetRemoteTasksRaw)
            .WillByDefault(
                    [response1, response2]([[maybe_unused]] ClientContext* context,
                                           [[maybe_unused]] const GetRemoteTasksRequest& request) {
                        // mockReader ownership will be transferred to the client so we don't own it
                        // here.
                        MockClientReader<GetRemoteTasksResponse>* mockClientReader =
                                new MockClientReader<GetRemoteTasksResponse>();
                        EXPECT_CALL(*mockClientReader, Finish()).WillOnce(Return(Status::OK));
                        EXPECT_CALL(*mockClientReader, Read(_))
                                .WillOnce(DoAll(SetArgPointee<0>(response1), Return(true)))
                                .WillOnce(DoAll(SetArgPointee<0>(response2), Return(true)))
                                .WillRepeatedly(Return(false));
                        return mockClientReader;
                    });
    // Should only be called once when is is ready for remote task.
    EXPECT_CALL(*getGrpcWakeupClientStub(), GetRemoteTasksRaw).Times(1);

    getService()->setRemoteTaskCallback(callback);
    setRetryWaitInMs(100);
    // Start the long live connection to receive tasks.
    ApState newState = {
            .isReadyForRemoteTask = true,
    };
    ASSERT_TRUE(getService()->notifyApStateChange(newState).isOk());
    ASSERT_TRUE(callback->wait(/*taskCount=*/2, /*timeoutInSec=*/10))
            << "Did not receive enough tasks";

    // Stop the long live connection.
    newState.isReadyForRemoteTask = false;
    ASSERT_TRUE(getService()->notifyApStateChange(newState).isOk());

    // Wait for the retry delay, but the loop should already exit.
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
}

TEST_F(RemoteAccessServiceUnitTest, testGetVehicleId) {
    std::string vehicleId;

    FakeVhalClient vhalClient;

    ASSERT_TRUE(getVehicleIdWithClient(vhalClient, &vehicleId).isOk());
    ASSERT_EQ(vehicleId, kTestVin);
}

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
