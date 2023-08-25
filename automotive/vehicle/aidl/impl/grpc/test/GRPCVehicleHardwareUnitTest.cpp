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

#include <gmock/gmock.h>
#include <grpc++/grpc++.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>

namespace android::hardware::automotive::vehicle::virtualization {

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

}  // namespace android::hardware::automotive::vehicle::virtualization
