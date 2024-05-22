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
#include "VehicleServer.grpc.pb.h"
#include "VehicleServer.pb.h"
#include "VehicleServer_mock.grpc.pb.h"

#include <gmock/gmock.h>
#include <grpc++/grpc++.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>

namespace android::hardware::automotive::vehicle::virtualization {

namespace aidlvhal = ::aidl::android::hardware::automotive::vehicle;

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;

using proto::MockVehicleServerStub;

const std::string kFakeServerAddr = "0.0.0.0:54321";

class FakeVehicleServer : public proto::VehicleServer::Service {
  public:
    ::grpc::Status StartPropertyValuesStream(
            ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
            ::grpc::ServerWriter<proto::VehiclePropValues>* stream) override {
        stream->Write(proto::VehiclePropValues());
        // A fake disconnection.
        return ::grpc::Status(::grpc::StatusCode::ABORTED, "Connection lost.");
    }

    // Functions that we do not care.
    ::grpc::Status GetAllPropertyConfig(
            ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
            ::grpc::ServerWriter<proto::VehiclePropConfig>* stream) override {
        return ::grpc::Status::OK;
    }

    ::grpc::Status SetValues(::grpc::ServerContext* context,
                             const proto::VehiclePropValueRequests* requests,
                             proto::SetValueResults* results) override {
        return ::grpc::Status::OK;
    }

    ::grpc::Status GetValues(::grpc::ServerContext* context,
                             const proto::VehiclePropValueRequests* requests,
                             proto::GetValueResults* results) override {
        return ::grpc::Status::OK;
    }
};

TEST(GRPCVehicleHardwareUnitTest, Reconnect) {
    auto receivedUpdate = std::make_shared<std::atomic<int>>(0);
    auto vehicleHardware = std::make_unique<GRPCVehicleHardware>(kFakeServerAddr);
    vehicleHardware->registerOnPropertyChangeEvent(
            std::make_unique<const IVehicleHardware::PropertyChangeCallback>(
                    [receivedUpdate](const auto&) { receivedUpdate->fetch_add(1); }));

    constexpr size_t kServerRestartTimes = 5;
    for (size_t serverStart = 0; serverStart < kServerRestartTimes; ++serverStart) {
        EXPECT_EQ(receivedUpdate->load(), 0);
        auto fakeServer = std::make_unique<FakeVehicleServer>();
        ::grpc::ServerBuilder builder;
        builder.RegisterService(fakeServer.get());
        builder.AddListeningPort(kFakeServerAddr, ::grpc::InsecureServerCredentials());
        auto grpcServer = builder.BuildAndStart();

        // Wait until the vehicle hardware received the second update (after one fake
        // disconnection).
        constexpr auto kMaxWaitTime = std::chrono::seconds(5);
        auto startTime = std::chrono::steady_clock::now();
        while (receivedUpdate->load() <= 1 &&
               std::chrono::steady_clock::now() - startTime < kMaxWaitTime)
            ;

        grpcServer->Shutdown();
        grpcServer->Wait();
        EXPECT_GT(receivedUpdate->load(), 1);

        // Reset for the next round.
        receivedUpdate->store(0);
    }
}

class GRPCVehicleHardwareMockServerUnitTest : public ::testing::Test {
  protected:
    NiceMock<MockVehicleServerStub>* mGrpcStub;
    std::unique_ptr<GRPCVehicleHardware> mHardware;

    void SetUp() override {
        auto stub = std::make_unique<NiceMock<MockVehicleServerStub>>();
        ;
        mGrpcStub = stub.get();
        mHardware = std::make_unique<GRPCVehicleHardware>(std::move(stub));
    }

    void TearDown() override { mHardware.reset(); }
};

MATCHER_P(RepeatedInt32Eq, expected_values, "") {
    return std::vector<int32_t>(arg.begin(), arg.end()) == expected_values;
}

TEST_F(GRPCVehicleHardwareMockServerUnitTest, Subscribe) {
    proto::VehicleHalCallStatus protoStatus;
    protoStatus.set_status_code(proto::StatusCode::OK);
    proto::SubscribeRequest actualRequest;

    EXPECT_CALL(*mGrpcStub, Subscribe(_, _, _))
            .WillOnce(DoAll(SaveArg<1>(&actualRequest), SetArgPointee<2>(protoStatus),
                            Return(::grpc::Status::OK)));

    aidlvhal::SubscribeOptions options = {.propId = 1,
                                          .areaIds = {1, 2, 3, 4},
                                          .sampleRate = 1.234,
                                          .resolution = 0.01,
                                          .enableVariableUpdateRate = true};
    auto status = mHardware->subscribe(options);

    EXPECT_EQ(status, aidlvhal::StatusCode::OK);
    const auto& protoOptions = actualRequest.options();
    EXPECT_EQ(protoOptions.prop_id(), 1);
    EXPECT_THAT(protoOptions.area_ids(), RepeatedInt32Eq(std::vector<int32_t>({1, 2, 3, 4})));
    EXPECT_FLOAT_EQ(protoOptions.sample_rate(), 1.234);
    EXPECT_FLOAT_EQ(protoOptions.resolution(), 0.01);
    EXPECT_EQ(protoOptions.enable_variable_update_rate(), true);
}

TEST_F(GRPCVehicleHardwareMockServerUnitTest, SubscribeLegacyServer) {
    EXPECT_CALL(*mGrpcStub, Subscribe(_, _, _))
            .WillOnce(Return(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")));

    aidlvhal::SubscribeOptions options;
    auto status = mHardware->subscribe(options);

    EXPECT_EQ(status, aidlvhal::StatusCode::OK);
}

TEST_F(GRPCVehicleHardwareMockServerUnitTest, SubscribeGrpcFailure) {
    EXPECT_CALL(*mGrpcStub, Subscribe(_, _, _))
            .WillOnce(Return(::grpc::Status(::grpc::StatusCode::INTERNAL, "GRPC Error")));

    aidlvhal::SubscribeOptions options;
    auto status = mHardware->subscribe(options);

    EXPECT_EQ(status, aidlvhal::StatusCode::INTERNAL_ERROR);
}

TEST_F(GRPCVehicleHardwareMockServerUnitTest, SubscribeProtoFailure) {
    proto::VehicleHalCallStatus protoStatus;
    protoStatus.set_status_code(proto::StatusCode::NOT_AVAILABLE_SPEED_LOW);

    EXPECT_CALL(*mGrpcStub, Subscribe(_, _, _))
            .WillOnce(DoAll(SetArgPointee<2>(protoStatus),  // Set the output status
                            Return(::grpc::Status::OK)));

    aidlvhal::SubscribeOptions options;
    auto status = mHardware->subscribe(options);

    EXPECT_EQ(status, aidlvhal::StatusCode::NOT_AVAILABLE_SPEED_LOW);
}

TEST_F(GRPCVehicleHardwareMockServerUnitTest, Unsubscribe) {
    proto::VehicleHalCallStatus protoStatus;
    protoStatus.set_status_code(proto::StatusCode::OK);
    proto::UnsubscribeRequest actualRequest;

    EXPECT_CALL(*mGrpcStub, Unsubscribe(_, _, _))
            .WillOnce(DoAll(SaveArg<1>(&actualRequest), SetArgPointee<2>(protoStatus),
                            Return(::grpc::Status::OK)));

    int32_t propId = 1;
    int32_t areaId = 2;
    auto status = mHardware->unsubscribe(propId, areaId);

    EXPECT_EQ(status, aidlvhal::StatusCode::OK);
    EXPECT_EQ(actualRequest.prop_id(), propId);
    EXPECT_EQ(actualRequest.area_id(), areaId);
}

TEST_F(GRPCVehicleHardwareMockServerUnitTest, UnsubscribeLegacyServer) {
    EXPECT_CALL(*mGrpcStub, Unsubscribe(_, _, _))
            .WillOnce(Return(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")));

    auto status = mHardware->unsubscribe(1, 2);

    EXPECT_EQ(status, aidlvhal::StatusCode::OK);
}

TEST_F(GRPCVehicleHardwareMockServerUnitTest, UnsubscribeGrpcFailure) {
    EXPECT_CALL(*mGrpcStub, Unsubscribe(_, _, _))
            .WillOnce(Return(::grpc::Status(::grpc::StatusCode::INTERNAL, "GRPC Error")));

    auto status = mHardware->unsubscribe(1, 2);

    EXPECT_EQ(status, aidlvhal::StatusCode::INTERNAL_ERROR);
}

TEST_F(GRPCVehicleHardwareMockServerUnitTest, UnsubscribeProtoFailure) {
    proto::VehicleHalCallStatus protoStatus;
    protoStatus.set_status_code(proto::StatusCode::NOT_AVAILABLE_SPEED_LOW);

    EXPECT_CALL(*mGrpcStub, Unsubscribe(_, _, _))
            .WillOnce(DoAll(SetArgPointee<2>(protoStatus),  // Set the output status
                            Return(::grpc::Status::OK)));

    auto status = mHardware->unsubscribe(1, 2);

    EXPECT_EQ(status, aidlvhal::StatusCode::NOT_AVAILABLE_SPEED_LOW);
}

}  // namespace android::hardware::automotive::vehicle::virtualization
