/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include "GrpcVehicleServer.h"

#include <condition_variable>
#include <mutex>
#include <shared_mutex>

#include <android-base/logging.h>
#include <grpc++/grpc++.h>

#include "VehicleServer.grpc.pb.h"
#include "VehicleServer.pb.h"
#include "vhal_v2_0/ProtoMessageConverter.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class GrpcVehicleServerImpl : public GrpcVehicleServer, public vhal_proto::VehicleServer::Service {
  public:
    GrpcVehicleServerImpl(const std::string& addr) : mServiceAddr(addr) {
        setValuePool(&mValueObjectPool);
    }

    // method from GrpcVehicleServer
    void Start() override;

    // method from IVehicleServer
    void onPropertyValueFromCar(const VehiclePropValue& value, bool updateStatus) override;

    // methods from vhal_proto::VehicleServer::Service

    ::grpc::Status GetAllPropertyConfig(
            ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
            ::grpc::ServerWriter<vhal_proto::VehiclePropConfig>* stream) override;

    ::grpc::Status SetProperty(::grpc::ServerContext* context,
                               const vhal_proto::WrappedVehiclePropValue* wrappedPropValue,
                               vhal_proto::VehicleHalCallStatus* status) override;

    ::grpc::Status StartPropertyValuesStream(
            ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
            ::grpc::ServerWriter<vhal_proto::WrappedVehiclePropValue>* stream) override;

  private:
    // We keep long-lasting connection for streaming the prop values.
    // For us, each connection can be represented as a function to send the new value, and
    // an ID to identify this connection
    struct ConnectionDescriptor {
        using ValueWriterType = std::function<bool(const vhal_proto::WrappedVehiclePropValue&)>;

        ConnectionDescriptor(ValueWriterType&& value_writer)
            : mValueWriter(std::move(value_writer)),
              mConnectionID(CONNECTION_ID_COUNTER.fetch_add(1)) {}

        ConnectionDescriptor(const ConnectionDescriptor&) = delete;

        ConnectionDescriptor& operator=(const ConnectionDescriptor&) = delete;

        // This move constructor is NOT THREAD-SAFE, which means it cannot be moved
        // while using. Since the connection descriptors are pretected by mConnectionMutex
        // then we are fine here
        ConnectionDescriptor(ConnectionDescriptor&& cd)
            : mValueWriter(std::move(cd.mValueWriter)),
              mConnectionID(cd.mConnectionID),
              mIsAlive(cd.mIsAlive.load()) {
            cd.mIsAlive.store(false);
        }

        ValueWriterType mValueWriter;
        uint64_t mConnectionID;
        std::atomic<bool> mIsAlive{true};

        static std::atomic<uint64_t> CONNECTION_ID_COUNTER;
    };

    std::string mServiceAddr;
    VehiclePropValuePool mValueObjectPool;
    mutable std::shared_mutex mConnectionMutex;
    mutable std::shared_mutex mWriterMutex;
    std::list<ConnectionDescriptor> mValueStreamingConnections;
};

std::atomic<uint64_t> GrpcVehicleServerImpl::ConnectionDescriptor::CONNECTION_ID_COUNTER = 0;

static std::shared_ptr<::grpc::ServerCredentials> getServerCredentials() {
    // TODO(chenhaosjtuacm): get secured credentials here
    return ::grpc::InsecureServerCredentials();
}

GrpcVehicleServerPtr makeGrpcVehicleServer(const std::string& addr) {
    return std::make_unique<GrpcVehicleServerImpl>(addr);
}

void GrpcVehicleServerImpl::Start() {
    ::grpc::ServerBuilder builder;
    builder.RegisterService(this);
    builder.AddListeningPort(mServiceAddr, getServerCredentials());
    std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());

    server->Wait();
}

void GrpcVehicleServerImpl::onPropertyValueFromCar(const VehiclePropValue& value,
                                                   bool updateStatus) {
    vhal_proto::WrappedVehiclePropValue wrappedPropValue;
    proto_msg_converter::toProto(wrappedPropValue.mutable_value(), value);
    wrappedPropValue.set_update_status(updateStatus);
    std::shared_lock read_lock(mConnectionMutex);

    bool has_terminated_connections = 0;

    for (auto& connection : mValueStreamingConnections) {
        auto writeOK = connection.mValueWriter(wrappedPropValue);
        if (!writeOK) {
            LOG(ERROR) << __func__ << ": Server Write failed, connection lost. ID: "
                       << connection.mConnectionID;
            has_terminated_connections = true;
            connection.mIsAlive.store(false);
        }
    }

    if (!has_terminated_connections) {
        return;
    }

    read_lock.unlock();

    std::unique_lock write_lock(mConnectionMutex);

    for (auto itr = mValueStreamingConnections.begin(); itr != mValueStreamingConnections.end();) {
        if (!itr->mIsAlive.load()) {
            itr = mValueStreamingConnections.erase(itr);
        } else {
            ++itr;
        }
    }
}

::grpc::Status GrpcVehicleServerImpl::GetAllPropertyConfig(
        ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
        ::grpc::ServerWriter<vhal_proto::VehiclePropConfig>* stream) {
    auto configs = onGetAllPropertyConfig();
    for (auto& config : configs) {
        vhal_proto::VehiclePropConfig protoConfig;
        proto_msg_converter::toProto(&protoConfig, config);
        if (!stream->Write(protoConfig)) {
            return ::grpc::Status(::grpc::StatusCode::ABORTED, "Connection lost.");
        }
    }

    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleServerImpl::SetProperty(
        ::grpc::ServerContext* context, const vhal_proto::WrappedVehiclePropValue* wrappedPropValue,
        vhal_proto::VehicleHalCallStatus* status) {
    VehiclePropValue value;
    proto_msg_converter::fromProto(&value, wrappedPropValue->value());

    auto set_status = static_cast<int32_t>(onSetProperty(value, wrappedPropValue->update_status()));
    if (!vhal_proto::VehicleHalStatusCode_IsValid(set_status)) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, "Unknown status code");
    }

    status->set_status_code(static_cast<vhal_proto::VehicleHalStatusCode>(set_status));

    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleServerImpl::StartPropertyValuesStream(
        ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
        ::grpc::ServerWriter<vhal_proto::WrappedVehiclePropValue>* stream) {
    std::mutex terminateMutex;
    std::condition_variable terminateCV;
    std::unique_lock<std::mutex> terminateLock(terminateMutex);
    bool terminated{false};

    auto callBack = [stream, &terminateMutex, &terminateCV, &terminated,
                     this](const vhal_proto::WrappedVehiclePropValue& value) {
        std::unique_lock lock(mWriterMutex);
        if (!stream->Write(value)) {
            std::unique_lock<std::mutex> terminateLock(terminateMutex);
            terminated = true;
            terminateLock.unlock();
            terminateCV.notify_all();
            return false;
        }
        return true;
    };

    // Register connection
    std::unique_lock lock(mConnectionMutex);
    auto& conn = mValueStreamingConnections.emplace_back(std::move(callBack));
    lock.unlock();

    // Never stop until connection lost
    terminateCV.wait(terminateLock, [&terminated]() { return terminated; });

    LOG(ERROR) << __func__ << ": Stream lost, ID : " << conn.mConnectionID;

    return ::grpc::Status(::grpc::StatusCode::ABORTED, "Connection lost.");
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
