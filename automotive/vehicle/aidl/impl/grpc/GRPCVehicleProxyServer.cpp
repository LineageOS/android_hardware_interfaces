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

#include "GRPCVehicleProxyServer.h"

#include "ProtoMessageConverter.h"

#include <grpc++/grpc++.h>

#include <android-base/logging.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <unordered_set>
#include <utility>
#include <vector>

namespace android::hardware::automotive::vehicle::virtualization {

std::atomic<uint64_t> GrpcVehicleProxyServer::ConnectionDescriptor::connection_id_counter_{0};

static std::shared_ptr<::grpc::ServerCredentials> getServerCredentials() {
    // TODO(chenhaosjtuacm): get secured credentials here
    return ::grpc::InsecureServerCredentials();
}

GrpcVehicleProxyServer::GrpcVehicleProxyServer(std::string serverAddr,
                                               std::unique_ptr<IVehicleHardware>&& hardware)
    : mServiceAddr(std::move(serverAddr)), mHardware(std::move(hardware)) {
    mHardware->registerOnPropertyChangeEvent(
            std::make_unique<const IVehicleHardware::PropertyChangeCallback>(
                    [this](std::vector<aidlvhal::VehiclePropValue> values) {
                        OnVehiclePropChange(values);
                    }));
}

::grpc::Status GrpcVehicleProxyServer::GetAllPropertyConfig(
        ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
        ::grpc::ServerWriter<proto::VehiclePropConfig>* stream) {
    for (const auto& config : mHardware->getAllPropertyConfigs()) {
        proto::VehiclePropConfig protoConfig;
        proto_msg_converter::aidlToProto(config, &protoConfig);
        if (!stream->Write(protoConfig)) {
            return ::grpc::Status(::grpc::StatusCode::ABORTED, "Connection lost.");
        }
    }
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::SetValues(::grpc::ServerContext* context,
                                                 const proto::VehiclePropValueRequests* requests,
                                                 proto::SetValueResults* results) {
    std::vector<aidlvhal::SetValueRequest> aidlRequests;
    for (const auto& protoRequest : requests->requests()) {
        auto& aidlRequest = aidlRequests.emplace_back();
        aidlRequest.requestId = protoRequest.request_id();
        proto_msg_converter::protoToAidl(protoRequest.value(), &aidlRequest.value);
    }
    auto waitMtx = std::make_shared<std::mutex>();
    auto waitCV = std::make_shared<std::condition_variable>();
    auto complete = std::make_shared<bool>(false);
    auto tmpResults = std::make_shared<proto::SetValueResults>();
    auto aidlStatus = mHardware->setValues(
            std::make_shared<const IVehicleHardware::SetValuesCallback>(
                    [waitMtx, waitCV, complete,
                     tmpResults](std::vector<aidlvhal::SetValueResult> setValueResults) {
                        for (const auto& aidlResult : setValueResults) {
                            auto& protoResult = *tmpResults->add_results();
                            protoResult.set_request_id(aidlResult.requestId);
                            protoResult.set_status(
                                    static_cast<proto::StatusCode>(aidlResult.status));
                        }
                        {
                            std::lock_guard lck(*waitMtx);
                            *complete = true;
                        }
                        waitCV->notify_all();
                    }),
            aidlRequests);
    if (aidlStatus != aidlvhal::StatusCode::OK) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                              "The underlying hardware fails to set values, VHAL status: " +
                                      toString(aidlStatus));
    }
    std::unique_lock lck(*waitMtx);
    bool success = waitCV->wait_for(lck, kHardwareOpTimeout, [complete] { return *complete; });
    if (!success) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                              "The underlying hardware set values timeout.");
    }
    *results = std::move(*tmpResults);
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::GetValues(::grpc::ServerContext* context,
                                                 const proto::VehiclePropValueRequests* requests,
                                                 proto::GetValueResults* results) {
    std::vector<aidlvhal::GetValueRequest> aidlRequests;
    for (const auto& protoRequest : requests->requests()) {
        auto& aidlRequest = aidlRequests.emplace_back();
        aidlRequest.requestId = protoRequest.request_id();
        proto_msg_converter::protoToAidl(protoRequest.value(), &aidlRequest.prop);
    }
    auto waitMtx = std::make_shared<std::mutex>();
    auto waitCV = std::make_shared<std::condition_variable>();
    auto complete = std::make_shared<bool>(false);
    auto tmpResults = std::make_shared<proto::GetValueResults>();
    auto aidlStatus = mHardware->getValues(
            std::make_shared<const IVehicleHardware::GetValuesCallback>(
                    [waitMtx, waitCV, complete,
                     tmpResults](std::vector<aidlvhal::GetValueResult> getValueResults) {
                        for (const auto& aidlResult : getValueResults) {
                            auto& protoResult = *tmpResults->add_results();
                            protoResult.set_request_id(aidlResult.requestId);
                            protoResult.set_status(
                                    static_cast<proto::StatusCode>(aidlResult.status));
                            if (aidlResult.prop) {
                                auto* valuePtr = protoResult.mutable_value();
                                proto_msg_converter::aidlToProto(*aidlResult.prop, valuePtr);
                            }
                        }
                        {
                            std::lock_guard lck(*waitMtx);
                            *complete = true;
                        }
                        waitCV->notify_all();
                    }),
            aidlRequests);
    if (aidlStatus != aidlvhal::StatusCode::OK) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                              "The underlying hardware fails to get values, VHAL status: " +
                                      toString(aidlStatus));
    }
    std::unique_lock lck(*waitMtx);
    bool success = waitCV->wait_for(lck, kHardwareOpTimeout, [complete] { return *complete; });
    if (!success) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                              "The underlying hardware get values timeout.");
    }
    *results = std::move(*tmpResults);
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::UpdateSampleRate(
        ::grpc::ServerContext* context, const proto::UpdateSampleRateRequest* request,
        proto::VehicleHalCallStatus* status) {
    const auto status_code = mHardware->updateSampleRate(request->prop(), request->area_id(),
                                                         request->sample_rate());
    status->set_status_code(static_cast<proto::StatusCode>(status_code));
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::CheckHealth(::grpc::ServerContext* context,
                                                   const ::google::protobuf::Empty*,
                                                   proto::VehicleHalCallStatus* status) {
    status->set_status_code(static_cast<proto::StatusCode>(mHardware->checkHealth()));
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::Dump(::grpc::ServerContext* context,
                                            const proto::DumpOptions* options,
                                            proto::DumpResult* result) {
    std::vector<std::string> dumpOptionStrings(options->options().begin(),
                                               options->options().end());
    auto dumpResult = mHardware->dump(dumpOptionStrings);
    result->set_caller_should_dump_state(dumpResult.callerShouldDumpState);
    result->set_buffer(dumpResult.buffer);
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::StartPropertyValuesStream(
        ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
        ::grpc::ServerWriter<proto::VehiclePropValues>* stream) {
    auto conn = std::make_shared<ConnectionDescriptor>(stream);
    {
        std::lock_guard lck(mConnectionMutex);
        mValueStreamingConnections.push_back(conn);
    }
    conn->Wait();
    LOG(ERROR) << __func__ << ": Stream lost, ID : " << conn->ID();
    return ::grpc::Status(::grpc::StatusCode::ABORTED, "Connection lost.");
}

void GrpcVehicleProxyServer::OnVehiclePropChange(
        const std::vector<aidlvhal::VehiclePropValue>& values) {
    std::unordered_set<uint64_t> brokenConn;
    proto::VehiclePropValues protoValues;
    for (const auto& value : values) {
        auto* protoValuePtr = protoValues.add_values();
        proto_msg_converter::aidlToProto(value, protoValuePtr);
    }
    {
        std::shared_lock read_lock(mConnectionMutex);
        for (auto& connection : mValueStreamingConnections) {
            auto writeOK = connection->Write(protoValues);
            if (!writeOK) {
                LOG(ERROR) << __func__
                           << ": Server Write failed, connection lost. ID: " << connection->ID();
                brokenConn.insert(connection->ID());
            }
        }
    }
    if (brokenConn.empty()) {
        return;
    }
    std::unique_lock write_lock(mConnectionMutex);
    mValueStreamingConnections.erase(
            std::remove_if(mValueStreamingConnections.begin(), mValueStreamingConnections.end(),
                           [&brokenConn](const auto& conn) {
                               return brokenConn.find(conn->ID()) != brokenConn.end();
                           }),
            mValueStreamingConnections.end());
}

GrpcVehicleProxyServer& GrpcVehicleProxyServer::Start() {
    if (mServer) {
        LOG(WARNING) << __func__ << ": GrpcVehicleProxyServer has already started.";
        return *this;
    }
    ::grpc::ServerBuilder builder;
    builder.RegisterService(this);
    builder.AddListeningPort(mServiceAddr, getServerCredentials());
    mServer = builder.BuildAndStart();
    CHECK(mServer) << __func__ << ": failed to create the GRPC server, "
                   << "please make sure the configuration and permissions are correct";
    return *this;
}

GrpcVehicleProxyServer& GrpcVehicleProxyServer::Shutdown() {
    std::shared_lock read_lock(mConnectionMutex);
    for (auto& conn : mValueStreamingConnections) {
        conn->Shutdown();
    }
    if (mServer) {
        mServer->Shutdown();
    }
    return *this;
}

void GrpcVehicleProxyServer::Wait() {
    if (mServer) {
        mServer->Wait();
    }
    mServer.reset();
}

GrpcVehicleProxyServer::ConnectionDescriptor::~ConnectionDescriptor() {
    Shutdown();
}

bool GrpcVehicleProxyServer::ConnectionDescriptor::Write(const proto::VehiclePropValues& values) {
    if (!mStream) {
        LOG(ERROR) << __func__ << ": Empty stream. ID: " << ID();
        Shutdown();
        return false;
    }
    {
        std::lock_guard lck(*mMtx);
        if (!mShutdownFlag && mStream->Write(values)) {
            return true;
        } else {
            LOG(ERROR) << __func__ << ": Server Write failed, connection lost. ID: " << ID();
        }
    }
    Shutdown();
    return false;
}

void GrpcVehicleProxyServer::ConnectionDescriptor::Wait() {
    std::unique_lock lck(*mMtx);
    mCV->wait(lck, [this] { return mShutdownFlag; });
}

void GrpcVehicleProxyServer::ConnectionDescriptor::Shutdown() {
    {
        std::lock_guard lck(*mMtx);
        mShutdownFlag = true;
    }
    mCV->notify_all();
}

}  // namespace android::hardware::automotive::vehicle::virtualization
