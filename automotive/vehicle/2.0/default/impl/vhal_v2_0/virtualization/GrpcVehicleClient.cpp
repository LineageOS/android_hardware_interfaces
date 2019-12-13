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
#include "GrpcVehicleClient.h"

#include <condition_variable>
#include <mutex>

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

static std::shared_ptr<::grpc::ChannelCredentials> getChannelCredentials() {
    // TODO(chenhaosjtuacm): get secured credentials here
    return ::grpc::InsecureChannelCredentials();
}

class GrpcVehicleClientImpl : public EmulatedVehicleClient {
  public:
    GrpcVehicleClientImpl(const std::string& addr)
        : mServiceAddr(addr),
          mGrpcChannel(::grpc::CreateChannel(mServiceAddr, getChannelCredentials())),
          mGrpcStub(vhal_proto::VehicleServer::NewStub(mGrpcChannel)) {
        StartValuePollingThread();
    }

    ~GrpcVehicleClientImpl() {
        mShuttingDownFlag.store(true);
        mShutdownCV.notify_all();
        if (mPollingThread.joinable()) {
            mPollingThread.join();
        }
    }

    // methods from IVehicleClient

    std::vector<VehiclePropConfig> getAllPropertyConfig() const override;

    StatusCode setProperty(const VehiclePropValue& value, bool updateStatus) override;

  private:
    void StartValuePollingThread();

    // private data members

    std::string mServiceAddr;
    std::shared_ptr<::grpc::Channel> mGrpcChannel;
    std::unique_ptr<vhal_proto::VehicleServer::Stub> mGrpcStub;
    std::thread mPollingThread;

    std::mutex mShutdownMutex;
    std::condition_variable mShutdownCV;
    std::atomic<bool> mShuttingDownFlag{false};
};

std::unique_ptr<EmulatedVehicleClient> makeGrpcVehicleClient(const std::string& addr) {
    return std::make_unique<GrpcVehicleClientImpl>(addr);
}

std::vector<VehiclePropConfig> GrpcVehicleClientImpl::getAllPropertyConfig() const {
    std::vector<VehiclePropConfig> configs;
    ::grpc::ClientContext context;
    auto config_stream = mGrpcStub->GetAllPropertyConfig(&context, ::google::protobuf::Empty());
    vhal_proto::VehiclePropConfig protoConfig;
    while (config_stream->Read(&protoConfig)) {
        VehiclePropConfig config;
        proto_msg_converter::fromProto(&config, protoConfig);
        configs.emplace_back(std::move(config));
    }
    auto grpc_status = config_stream->Finish();
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__
                   << ": GRPC GetAllPropertyConfig Failed: " << grpc_status.error_message();
        configs.clear();
    }

    return configs;
}

StatusCode GrpcVehicleClientImpl::setProperty(const VehiclePropValue& value, bool updateStatus) {
    ::grpc::ClientContext context;
    vhal_proto::WrappedVehiclePropValue wrappedProtoValue;
    vhal_proto::VehicleHalCallStatus vhal_status;
    proto_msg_converter::toProto(wrappedProtoValue.mutable_value(), value);
    wrappedProtoValue.set_update_status(updateStatus);

    auto grpc_status = mGrpcStub->SetProperty(&context, wrappedProtoValue, &vhal_status);
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__ << ": GRPC SetProperty Failed: " << grpc_status.error_message();
        return StatusCode::INTERNAL_ERROR;
    }

    return static_cast<StatusCode>(vhal_status.status_code());
}

void GrpcVehicleClientImpl::StartValuePollingThread() {
    mPollingThread = std::thread([this]() {
        while (!mShuttingDownFlag.load()) {
            ::grpc::ClientContext context;

            std::atomic<bool> rpc_ok{true};
            std::thread shuttingdown_watcher([this, &rpc_ok, &context]() {
                std::unique_lock<std::mutex> shutdownLock(mShutdownMutex);
                mShutdownCV.wait(shutdownLock, [this, &rpc_ok]() {
                    return !rpc_ok.load() || mShuttingDownFlag.load();
                });
                context.TryCancel();
            });

            auto value_stream =
                    mGrpcStub->StartPropertyValuesStream(&context, ::google::protobuf::Empty());
            vhal_proto::WrappedVehiclePropValue wrappedProtoValue;
            while (!mShuttingDownFlag.load() && value_stream->Read(&wrappedProtoValue)) {
                VehiclePropValue value;
                proto_msg_converter::fromProto(&value, wrappedProtoValue.value());
                onPropertyValue(value, wrappedProtoValue.update_status());
            }

            rpc_ok.store(false);
            mShutdownCV.notify_all();
            shuttingdown_watcher.join();

            auto grpc_status = value_stream->Finish();
            // never reach here until connection lost
            LOG(ERROR) << __func__
                       << ": GRPC Value Streaming Failed: " << grpc_status.error_message();

            // try to reconnect
        }
    });
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
