// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GRPCVehicleHardware.h"
#include "GRPCVehicleProxyServer.h"
#include "IVehicleHardware.h"
#include "VehicleServer.grpc.pb.h"
#include "VehicleServer.pb.h"

#include <gmock/gmock.h>
#include <grpc++/grpc++.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace android::hardware::automotive::vehicle::virtualization {

namespace aidlvhal = ::aidl::android::hardware::automotive::vehicle;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;

const std::string kFakeServerAddr = "0.0.0.0:54321";

class VehicleHardwareForTest : public IVehicleHardware {
  public:
    void registerOnPropertyChangeEvent(
            std::unique_ptr<const PropertyChangeCallback> callback) override {
        mOnProp = std::move(callback);
    }

    void onPropertyEvent(std::vector<aidlvhal::VehiclePropValue> values) {
        if (mOnProp) {
            (*mOnProp)(std::move(values));
        }
    }

    void registerSupportedValueChangeCallback(
            std::unique_ptr<const SupportedValueChangeCallback> callback) override {
        mOnSupportedValuesChange = std::move(callback);
    }

    void onSupportedValuesChange(std::vector<PropIdAreaId> propIdAreaIds) {
        if (mOnSupportedValuesChange) {
            (*mOnSupportedValuesChange)(propIdAreaIds);
        }
    }

    // Functions that we do not care.
    std::vector<aidlvhal::VehiclePropConfig> getAllPropertyConfigs() const override { return {}; }

    aidlvhal::StatusCode setValues(
            std::shared_ptr<const SetValuesCallback> callback,
            const std::vector<aidlvhal::SetValueRequest>& requests) override {
        return aidlvhal::StatusCode::OK;
    }

    aidlvhal::StatusCode getValues(
            std::shared_ptr<const GetValuesCallback> callback,
            const std::vector<aidlvhal::GetValueRequest>& requests) const override {
        return aidlvhal::StatusCode::OK;
    }

    DumpResult dump(const std::vector<std::string>& options) override { return {}; }

    aidlvhal::StatusCode checkHealth() override { return aidlvhal::StatusCode::OK; }

    void registerOnPropertySetErrorEvent(
            std::unique_ptr<const PropertySetErrorCallback> callback) override {}

  private:
    std::unique_ptr<const PropertyChangeCallback> mOnProp;
    std::unique_ptr<const SupportedValueChangeCallback> mOnSupportedValuesChange;
};

class MockVehicleHardware : public IVehicleHardware {
  public:
    // Mock methods from IVehicleHardware
    MOCK_METHOD(std::vector<aidlvhal::VehiclePropConfig>, getAllPropertyConfigs, (),
                (const, override));

    MOCK_METHOD((aidlvhal::StatusCode), setValues,
                (std::shared_ptr<const SetValuesCallback> callback,
                 const std::vector<aidlvhal::SetValueRequest>& requests),
                (override));

    MOCK_METHOD((aidlvhal::StatusCode), getValues,
                (std::shared_ptr<const GetValuesCallback> callback,
                 const std::vector<aidlvhal::GetValueRequest>& requests),
                (const, override));

    MOCK_METHOD(DumpResult, dump, (const std::vector<std::string>& options), (override));
    MOCK_METHOD(aidlvhal::StatusCode, checkHealth, (), (override));
    MOCK_METHOD(void, registerOnPropertyChangeEvent,
                (std::unique_ptr<const PropertyChangeCallback> callback), (override));
    MOCK_METHOD(void, registerOnPropertySetErrorEvent,
                (std::unique_ptr<const PropertySetErrorCallback> callback), (override));
    MOCK_METHOD(std::chrono::nanoseconds, getPropertyOnChangeEventBatchingWindow, (), (override));
    MOCK_METHOD(aidlvhal::StatusCode, subscribe, (aidlvhal::SubscribeOptions options), (override));
    MOCK_METHOD(aidlvhal::StatusCode, unsubscribe, (int32_t propId, int32_t areaId), (override));
    MOCK_METHOD(aidlvhal::StatusCode, updateSampleRate,
                (int32_t propId, int32_t areaId, float sampleRate), (override));
    MOCK_METHOD(std::vector<aidlvhal::MinMaxSupportedValueResult>, getMinMaxSupportedValues,
                (const std::vector<PropIdAreaId>& propIdAreaIds), (override));
    MOCK_METHOD(std::vector<aidlvhal::SupportedValuesListResult>, getSupportedValuesLists,
                (const std::vector<PropIdAreaId>& propIdAreaIds), (override));
};

TEST(GRPCVehicleProxyServerUnitTest, ClientConnectDisconnect) {
    auto testHardware = std::make_unique<VehicleHardwareForTest>();
    // HACK: manipulate the underlying hardware via raw pointer for testing.
    auto* testHardwareRaw = testHardware.get();
    auto vehicleServer =
            std::make_unique<GrpcVehicleProxyServer>(kFakeServerAddr, std::move(testHardware));
    vehicleServer->Start();

    constexpr auto kWaitForConnectionMaxTime = std::chrono::seconds(5);
    constexpr auto kWaitForStreamStartTime = std::chrono::seconds(1);
    constexpr auto kWaitForUpdateDeliveryTime = std::chrono::seconds(1);

    auto updateReceived1 = std::make_shared<bool>(false);
    auto vehicleHardware1 = std::make_unique<GRPCVehicleHardware>(kFakeServerAddr);
    vehicleHardware1->registerOnPropertyChangeEvent(
            std::make_unique<const IVehicleHardware::PropertyChangeCallback>(
                    [updateReceived1](const auto&) { *updateReceived1 = true; }));
    EXPECT_TRUE(vehicleHardware1->waitForConnected(kWaitForConnectionMaxTime));
    std::this_thread::sleep_for(kWaitForStreamStartTime);

    // Client hardware 1 received update from the server.
    EXPECT_FALSE(*updateReceived1);
    testHardwareRaw->onPropertyEvent({aidlvhal::VehiclePropValue{.prop = 1}});
    // Wait for the update delivery.
    std::this_thread::sleep_for(kWaitForUpdateDeliveryTime);
    EXPECT_TRUE(*updateReceived1);

    // Reset.
    *updateReceived1 = false;

    auto updateReceived2 = std::make_shared<bool>(false);
    auto vehicleHardware2 = std::make_unique<GRPCVehicleHardware>(kFakeServerAddr);
    vehicleHardware2->registerOnPropertyChangeEvent(
            std::make_unique<const IVehicleHardware::PropertyChangeCallback>(
                    [updateReceived2](const auto&) { *updateReceived2 = true; }));
    EXPECT_TRUE(vehicleHardware2->waitForConnected(kWaitForConnectionMaxTime));
    std::this_thread::sleep_for(kWaitForStreamStartTime);

    // Both client hardware 1 and 2 received update from the server.
    EXPECT_FALSE(*updateReceived1);
    EXPECT_FALSE(*updateReceived2);
    testHardwareRaw->onPropertyEvent({aidlvhal::VehiclePropValue{.prop = 1}});
    // Wait for the update delivery.
    std::this_thread::sleep_for(kWaitForUpdateDeliveryTime);
    EXPECT_TRUE(*updateReceived1);
    EXPECT_TRUE(*updateReceived2);

    // Reset.
    *updateReceived1 = false;
    *updateReceived2 = false;

    vehicleHardware1.reset();

    // Client 1 exited, only client hardware 2 received update from the server.
    EXPECT_FALSE(*updateReceived1);
    EXPECT_FALSE(*updateReceived2);
    testHardwareRaw->onPropertyEvent({aidlvhal::VehiclePropValue{.prop = 1}});
    // Wait for the update delivery.
    std::this_thread::sleep_for(kWaitForUpdateDeliveryTime);
    EXPECT_FALSE(*updateReceived1);
    EXPECT_TRUE(*updateReceived2);

    vehicleServer->Shutdown().Wait();
}

TEST(GRPCVehicleProxyServerUnitTest, RegisterSupportedValuesChange) {
    int32_t testPropId1 = 123;
    int32_t testAreaId1 = 234;
    int32_t testPropId2 = 345;
    int32_t testAreaId2 = 456;

    auto testHardware = std::make_unique<VehicleHardwareForTest>();
    // HACK: manipulate the underlying hardware via raw pointer for testing.
    auto* testHardwareRaw = testHardware.get();
    auto vehicleServer =
            std::make_unique<GrpcVehicleProxyServer>(kFakeServerAddr, std::move(testHardware));
    vehicleServer->Start();

    constexpr auto kWaitForConnectionMaxTime = std::chrono::seconds(5);
    constexpr auto kWaitForStreamStartTime = std::chrono::seconds(1);
    constexpr auto kWaitForUpdateDeliveryTime = std::chrono::seconds(1);

    std::vector<PropIdAreaId> changedPropIdAreaIds;
    std::mutex m;
    std::condition_variable cv;
    auto vehicleHardware1 = std::make_unique<GRPCVehicleHardware>(kFakeServerAddr);
    vehicleHardware1->registerSupportedValueChangeCallback(
            std::make_unique<const IVehicleHardware::SupportedValueChangeCallback>(
                    [&changedPropIdAreaIds, &m, &cv](std::vector<PropIdAreaId> propIdAreaIds) {
                        {
                            std::lock_guard lk(m);
                            changedPropIdAreaIds = propIdAreaIds;
                        }
                        cv.notify_one();
                    }));

    EXPECT_TRUE(vehicleHardware1->waitForConnected(kWaitForConnectionMaxTime));
    std::this_thread::sleep_for(kWaitForStreamStartTime);

    testHardwareRaw->onSupportedValuesChange({PropIdAreaId{
                                                      .propId = testPropId1,
                                                      .areaId = testAreaId1,
                                              },
                                              PropIdAreaId{
                                                      .propId = testPropId2,
                                                      .areaId = testAreaId2,
                                              }});

    {
        std::unique_lock lk(m);
        cv.wait_for(lk, std::chrono::seconds(1),
                    [&changedPropIdAreaIds] { return !changedPropIdAreaIds.empty(); });
    }

    // Must make sure we always stop the server even if the test failed.
    vehicleServer->Shutdown().Wait();

    {
        std::unique_lock lk(m);
        ASSERT_FALSE(changedPropIdAreaIds.empty())
                << "Not received onSupportedValuesChange event before timeout";
    }
}

TEST(GRPCVehicleProxyServerUnitTest, Subscribe) {
    auto mockHardware = std::make_unique<MockVehicleHardware>();
    // We make sure this is alive inside the function scope.
    MockVehicleHardware* mockHardwarePtr = mockHardware.get();
    GrpcVehicleProxyServer server = GrpcVehicleProxyServer("", std::move(mockHardware));
    ::grpc::ServerContext context;
    proto::SubscribeRequest request;
    proto::VehicleHalCallStatus returnStatus;
    aidlvhal::SubscribeOptions aidlOptions;
    request.mutable_options()->set_prop_id(1);
    request.mutable_options()->add_area_ids(2);
    request.mutable_options()->set_sample_rate(1.234);
    request.mutable_options()->set_resolution(0.01);
    request.mutable_options()->set_enable_variable_update_rate(true);

    EXPECT_CALL(*mockHardwarePtr, subscribe(_))
            .WillOnce(DoAll(SaveArg<0>(&aidlOptions), Return(aidlvhal::StatusCode::OK)));

    auto grpcStatus = server.Subscribe(&context, &request, &returnStatus);

    EXPECT_TRUE(grpcStatus.ok());
    EXPECT_EQ(returnStatus.status_code(), proto::StatusCode::OK);
    EXPECT_EQ(aidlOptions.propId, 1);
    EXPECT_EQ(aidlOptions.areaIds, std::vector<int32_t>{2});
    EXPECT_FLOAT_EQ(aidlOptions.sampleRate, 1.234);
    EXPECT_FLOAT_EQ(aidlOptions.resolution, 0.01);
    EXPECT_TRUE(aidlOptions.enableVariableUpdateRate);
}

TEST(GRPCVehicleProxyServerUnitTest, SubscribeNotAvailable) {
    auto mockHardware = std::make_unique<MockVehicleHardware>();
    // We make sure this is alive inside the function scope.
    MockVehicleHardware* mockHardwarePtr = mockHardware.get();
    GrpcVehicleProxyServer server = GrpcVehicleProxyServer("", std::move(mockHardware));
    ::grpc::ServerContext context;
    proto::SubscribeRequest request;
    proto::VehicleHalCallStatus returnStatus;

    EXPECT_CALL(*mockHardwarePtr, subscribe(_))
            .WillOnce(Return(aidlvhal::StatusCode::NOT_AVAILABLE));

    auto grpcStatus = server.Subscribe(&context, &request, &returnStatus);

    EXPECT_TRUE(grpcStatus.ok());
    EXPECT_EQ(returnStatus.status_code(), proto::StatusCode::NOT_AVAILABLE);
}

TEST(GRPCVehicleProxyServerUnitTest, Unsubscribe) {
    auto mockHardware = std::make_unique<MockVehicleHardware>();
    // We make sure this is alive inside the function scope.
    MockVehicleHardware* mockHardwarePtr = mockHardware.get();
    GrpcVehicleProxyServer server = GrpcVehicleProxyServer("", std::move(mockHardware));
    ::grpc::ServerContext context;
    proto::UnsubscribeRequest request;
    proto::VehicleHalCallStatus returnStatus;
    request.set_prop_id(1);
    request.set_area_id(2);

    EXPECT_CALL(*mockHardwarePtr, unsubscribe(1, 2)).WillOnce(Return(aidlvhal::StatusCode::OK));

    auto grpcStatus = server.Unsubscribe(&context, &request, &returnStatus);

    EXPECT_TRUE(grpcStatus.ok());
    EXPECT_EQ(returnStatus.status_code(), proto::StatusCode::OK);
}

TEST(GRPCVehicleProxyServerUnitTest, testGetMinMaxSupportedValues) {
    int32_t testPropId = 1234;
    int32_t testAreaId = 4321;
    int32_t testValue1 = 12345;
    int32_t testValue2 = 54321;
    auto mockHardware = std::make_unique<MockVehicleHardware>();
    // We make sure this is alive inside the function scope.
    MockVehicleHardware* mockHardwarePtr = mockHardware.get();
    GrpcVehicleProxyServer server = GrpcVehicleProxyServer("", std::move(mockHardware));
    ::grpc::ServerContext context;
    proto::GetMinMaxSupportedValuesRequest request;
    proto::GetMinMaxSupportedValuesResult result;
    auto* requestPropIdAreaId = request.add_prop_id_area_id();
    requestPropIdAreaId->set_prop_id(testPropId);
    requestPropIdAreaId->set_area_id(testAreaId);
    std::vector<PropIdAreaId> propIdAreaIds;
    std::vector<aidlvhal::MinMaxSupportedValueResult> resultFromHardware = {{
            .status = aidlvhal::StatusCode::OK,
            .minSupportedValue = aidlvhal::RawPropValues{.int32Values = {testValue1}},
            .maxSupportedValue = aidlvhal::RawPropValues{.int32Values = {testValue2}},
    }};

    EXPECT_CALL(*mockHardwarePtr, getMinMaxSupportedValues(_))
            .WillOnce(DoAll(SaveArg<0>(&propIdAreaIds), Return(resultFromHardware)));

    auto grpcStatus = server.GetMinMaxSupportedValues(&context, &request, &result);

    ASSERT_THAT(propIdAreaIds, ::testing::SizeIs(1));
    EXPECT_EQ(propIdAreaIds[0], PropIdAreaId({.propId = testPropId, .areaId = testAreaId}));

    ASSERT_TRUE(grpcStatus.ok());
    ASSERT_THAT(result.result(), ::testing::SizeIs(1));
    EXPECT_EQ(result.result()[0].status(), proto::StatusCode::OK);
    ASSERT_THAT(result.result()[0].min_supported_value().int32_values(), ::testing::SizeIs(1));
    EXPECT_EQ(result.result()[0].min_supported_value().int32_values()[0], testValue1);
    ASSERT_THAT(result.result()[0].max_supported_value().int32_values(), ::testing::SizeIs(1));
    EXPECT_EQ(result.result()[0].max_supported_value().int32_values()[0], testValue2);
}

TEST(GRPCVehicleProxyServerUnitTest, testGetSupportedValuesLists) {
    int32_t testPropId = 1234;
    int32_t testAreaId = 4321;
    int32_t testValue1 = 12345;
    int32_t testValue2 = 54321;
    auto mockHardware = std::make_unique<MockVehicleHardware>();
    // We make sure this is alive inside the function scope.
    MockVehicleHardware* mockHardwarePtr = mockHardware.get();
    GrpcVehicleProxyServer server = GrpcVehicleProxyServer("", std::move(mockHardware));
    ::grpc::ServerContext context;
    proto::GetSupportedValuesListsRequest request;
    proto::GetSupportedValuesListsResult result;
    auto* requestPropIdAreaId = request.add_prop_id_area_id();
    requestPropIdAreaId->set_prop_id(testPropId);
    requestPropIdAreaId->set_area_id(testAreaId);
    std::vector<PropIdAreaId> propIdAreaIds;
    std::vector<aidlvhal::SupportedValuesListResult> resultFromHardware = {{
            .status = aidlvhal::StatusCode::OK,
            .supportedValuesList = std::vector<std::optional<aidlvhal::RawPropValues>>({
                    aidlvhal::RawPropValues{.int32Values = {testValue1}},
                    aidlvhal::RawPropValues{.int32Values = {testValue2}},
            }),
    }};

    EXPECT_CALL(*mockHardwarePtr, getSupportedValuesLists(_))
            .WillOnce(DoAll(SaveArg<0>(&propIdAreaIds), Return(resultFromHardware)));

    auto grpcStatus = server.GetSupportedValuesLists(&context, &request, &result);

    ASSERT_THAT(propIdAreaIds, ::testing::SizeIs(1));
    EXPECT_EQ(propIdAreaIds[0], PropIdAreaId({.propId = testPropId, .areaId = testAreaId}));

    ASSERT_TRUE(grpcStatus.ok());
    ASSERT_THAT(result.result(), ::testing::SizeIs(1));
    EXPECT_EQ(result.result()[0].status(), proto::StatusCode::OK);
    ASSERT_THAT(result.result()[0].supported_values_list(), ::testing::SizeIs(2));
    ASSERT_THAT(result.result()[0].supported_values_list()[0].int32_values(), ::testing::SizeIs(1));
    EXPECT_THAT(result.result()[0].supported_values_list()[0].int32_values()[0], testValue1);
    ASSERT_THAT(result.result()[0].supported_values_list()[1].int32_values(), ::testing::SizeIs(1));
    EXPECT_THAT(result.result()[0].supported_values_list()[1].int32_values()[0], testValue2);
}

}  // namespace android::hardware::automotive::vehicle::virtualization
