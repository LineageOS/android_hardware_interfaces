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

#pragma once

#include "IVehicleHardware.h"

#include "VehicleServer.grpc.pb.h"
#include "VehicleServer.pb.h"

#include <grpc++/grpc++.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <utility>

namespace android::hardware::automotive::vehicle::virtualization {

namespace aidlvhal = ::aidl::android::hardware::automotive::vehicle;

// Connect other GRPC vehicle hardware(s) to the underlying vehicle hardware.
class GrpcVehicleProxyServer : public proto::VehicleServer::Service {
  public:
    GrpcVehicleProxyServer(std::string serverAddr, std::unique_ptr<IVehicleHardware>&& hardware);

    GrpcVehicleProxyServer(std::vector<std::string> serverAddrs,
                           std::unique_ptr<IVehicleHardware>&& hardware);

    ::grpc::Status GetAllPropertyConfig(
            ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
            ::grpc::ServerWriter<proto::VehiclePropConfig>* stream) override;

    ::grpc::Status SetValues(::grpc::ServerContext* context,
                             const proto::VehiclePropValueRequests* requests,
                             proto::SetValueResults* results) override;

    ::grpc::Status GetValues(::grpc::ServerContext* context,
                             const proto::VehiclePropValueRequests* requests,
                             proto::GetValueResults* results) override;

    ::grpc::Status UpdateSampleRate(::grpc::ServerContext* context,
                                    const proto::UpdateSampleRateRequest* request,
                                    proto::VehicleHalCallStatus* status) override;

    ::grpc::Status Subscribe(::grpc::ServerContext* context, const proto::SubscribeRequest* request,
                             proto::VehicleHalCallStatus* status) override;

    ::grpc::Status Unsubscribe(::grpc::ServerContext* context,
                               const proto::UnsubscribeRequest* request,
                               proto::VehicleHalCallStatus* status) override;

    ::grpc::Status CheckHealth(::grpc::ServerContext* context, const ::google::protobuf::Empty*,
                               proto::VehicleHalCallStatus* status) override;

    ::grpc::Status Dump(::grpc::ServerContext* context, const proto::DumpOptions* options,
                        proto::DumpResult* result) override;

    ::grpc::Status StartPropertyValuesStream(
            ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
            ::grpc::ServerWriter<proto::VehiclePropValues>* stream) override;

    ::grpc::Status GetMinMaxSupportedValues(
            ::grpc::ServerContext* context, const proto::GetMinMaxSupportedValuesRequest* requests,
            proto::GetMinMaxSupportedValuesResult* results) override;

    ::grpc::Status GetSupportedValuesLists(::grpc::ServerContext* context,
                                           const proto::GetSupportedValuesListsRequest* requests,
                                           proto::GetSupportedValuesListsResult* results) override;

    ::grpc::Status StartSupportedValuesChangeStream(
            ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
            ::grpc::ServerWriter<proto::SupportedValuesChange>* stream) override;

    GrpcVehicleProxyServer& Start();

    GrpcVehicleProxyServer& Shutdown();

    void Wait();

  private:
    void OnVehiclePropChange(const std::vector<aidlvhal::VehiclePropValue>& values);

    void OnSupportedValuesChange(const std::vector<PropIdAreaId>& propIdAreaIds);

    // We keep long-lasting connection for streaming the prop values.
    template <typename ValueType>
    struct ConnectionDescriptor {
        explicit ConnectionDescriptor(::grpc::ServerWriter<ValueType>* stream)
            : mStream(stream),
              mConnectionID(connection_id_counter_.fetch_add(1) + 1),
              mMtx(std::make_unique<std::mutex>()),
              mCV(std::make_unique<std::condition_variable>()) {}

        ConnectionDescriptor(const ConnectionDescriptor&) = delete;
        ConnectionDescriptor(ConnectionDescriptor&& cd) = default;
        ConnectionDescriptor& operator=(const ConnectionDescriptor&) = delete;
        ConnectionDescriptor& operator=(ConnectionDescriptor&& cd) = default;

        ~ConnectionDescriptor();

        uint64_t ID() const { return mConnectionID; }

        bool Write(const ValueType& values);

        void Wait();

        void Shutdown();

      private:
        ::grpc::ServerWriter<ValueType>* mStream;
        uint64_t mConnectionID{0};
        std::unique_ptr<std::mutex> mMtx;
        std::unique_ptr<std::condition_variable> mCV;
        bool mShutdownFlag{false};

        static std::atomic<uint64_t> connection_id_counter_;
    };

    std::vector<std::string> mServiceAddrs;
    std::unique_ptr<::grpc::Server> mServer{nullptr};
    std::unique_ptr<IVehicleHardware> mHardware;

    std::shared_mutex mConnectionMutex;
    std::vector<std::shared_ptr<ConnectionDescriptor<proto::VehiclePropValues>>>
            mValueStreamingConnections;
    std::vector<std::shared_ptr<ConnectionDescriptor<proto::SupportedValuesChange>>>
            mSupportedValuesChangeConnections;

    template <typename ValueType>
    void writeToStream(std::vector<std::shared_ptr<ConnectionDescriptor<ValueType>>>& connections,
                       const ValueType& protoValues);

    static constexpr auto kHardwareOpTimeout = std::chrono::seconds(1);
};

}  // namespace android::hardware::automotive::vehicle::virtualization
