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

namespace {
std::shared_ptr<::grpc::ServerCredentials> getServerCredentials() {
    return ::grpc::InsecureServerCredentials();
}

template <class ProtoRequestType>
std::vector<PropIdAreaId> getPropIdAreaIdsFromProtoRequest(const ProtoRequestType* request) {
    std::vector<PropIdAreaId> propIdAreaIds;
    for (const proto::PropIdAreaId& protoPropIdAreaId : request->prop_id_area_id()) {
        PropIdAreaId aidlPropIdAreaId = {};
        proto_msg_converter::protoToAidl(protoPropIdAreaId, &aidlPropIdAreaId);
        propIdAreaIds.push_back(aidlPropIdAreaId);
    }
    return propIdAreaIds;
}

template <class AidlResultType, class ProtoResultType>
void aidlResultsToProtoResults(const AidlResultType& aidlResults, ProtoResultType* result) {
    for (const auto& aidlResult : aidlResults) {
        auto* protoResult = result->add_result();
        proto_msg_converter::aidlToProto(aidlResult, protoResult);
    }
}
}  // namespace

template <typename ValueType>
std::atomic<uint64_t>
        GrpcVehicleProxyServer::ConnectionDescriptor<ValueType>::connection_id_counter_{0};

GrpcVehicleProxyServer::GrpcVehicleProxyServer(std::string serverAddr,
                                               std::unique_ptr<IVehicleHardware>&& hardware)
    : GrpcVehicleProxyServer(std::vector<std::string>({serverAddr}), std::move(hardware)) {};

GrpcVehicleProxyServer::GrpcVehicleProxyServer(std::vector<std::string> serverAddrs,
                                               std::unique_ptr<IVehicleHardware>&& hardware)
    : mServiceAddrs(std::move(serverAddrs)), mHardware(std::move(hardware)) {
    mHardware->registerOnPropertyChangeEvent(
            std::make_unique<const IVehicleHardware::PropertyChangeCallback>(
                    [this](std::vector<aidlvhal::VehiclePropValue> values) {
                        OnVehiclePropChange(values);
                    }));
    mHardware->registerSupportedValueChangeCallback(
            std::make_unique<const IVehicleHardware::SupportedValueChangeCallback>(
                    [this](std::vector<PropIdAreaId> propIdAreaIds) {
                        OnSupportedValuesChange(propIdAreaIds);
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
    std::unordered_set<int64_t> requestIds;
    for (const auto& protoRequest : requests->requests()) {
        auto& aidlRequest = aidlRequests.emplace_back();
        int64_t requestId = protoRequest.request_id();
        aidlRequest.requestId = requestId;
        proto_msg_converter::protoToAidl(protoRequest.value(), &aidlRequest.value);
        requestIds.insert(requestId);
    }
    auto waitMtx = std::make_shared<std::mutex>();
    auto waitCV = std::make_shared<std::condition_variable>();
    auto complete = std::make_shared<bool>(false);
    auto tmpResults = std::make_shared<proto::SetValueResults>();
    auto aidlStatus = mHardware->setValues(
            std::make_shared<const IVehicleHardware::SetValuesCallback>(
                    [waitMtx, waitCV, complete, tmpResults,
                     &requestIds](std::vector<aidlvhal::SetValueResult> setValueResults) {
                        bool receivedAllResults = false;
                        {
                            std::lock_guard lck(*waitMtx);
                            for (const auto& aidlResult : setValueResults) {
                                auto& protoResult = *tmpResults->add_results();
                                int64_t requestIdForResult = aidlResult.requestId;
                                protoResult.set_request_id(requestIdForResult);
                                protoResult.set_status(
                                        static_cast<proto::StatusCode>(aidlResult.status));
                                requestIds.erase(requestIdForResult);
                            }
                            if (requestIds.empty()) {
                                receivedAllResults = true;
                                *complete = true;
                            }
                        }
                        if (receivedAllResults) {
                            waitCV->notify_all();
                        }
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
    std::unordered_set<int64_t> requestIds;
    for (const auto& protoRequest : requests->requests()) {
        auto& aidlRequest = aidlRequests.emplace_back();
        int64_t requestId = protoRequest.request_id();
        aidlRequest.requestId = requestId;
        proto_msg_converter::protoToAidl(protoRequest.value(), &aidlRequest.prop);
        requestIds.insert(requestId);
    }
    auto waitMtx = std::make_shared<std::mutex>();
    auto waitCV = std::make_shared<std::condition_variable>();
    auto complete = std::make_shared<bool>(false);
    auto tmpResults = std::make_shared<proto::GetValueResults>();
    auto aidlStatus = mHardware->getValues(
            std::make_shared<const IVehicleHardware::GetValuesCallback>(
                    [waitMtx, waitCV, complete, tmpResults,
                     &requestIds](std::vector<aidlvhal::GetValueResult> getValueResults) {
                        bool receivedAllResults = false;
                        {
                            std::lock_guard lck(*waitMtx);
                            for (const auto& aidlResult : getValueResults) {
                                auto& protoResult = *tmpResults->add_results();
                                int64_t requestIdForResult = aidlResult.requestId;
                                protoResult.set_request_id(requestIdForResult);
                                protoResult.set_status(
                                        static_cast<proto::StatusCode>(aidlResult.status));
                                if (aidlResult.prop) {
                                    auto* valuePtr = protoResult.mutable_value();
                                    proto_msg_converter::aidlToProto(*aidlResult.prop, valuePtr);
                                }
                                requestIds.erase(requestIdForResult);
                            }
                            if (requestIds.empty()) {
                                receivedAllResults = true;
                                *complete = true;
                            }
                        }
                        if (receivedAllResults) {
                            waitCV->notify_all();
                        }
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

::grpc::Status GrpcVehicleProxyServer::Subscribe(::grpc::ServerContext* context,
                                                 const proto::SubscribeRequest* request,
                                                 proto::VehicleHalCallStatus* status) {
    const auto& protoSubscribeOptions = request->options();
    aidlvhal::SubscribeOptions aidlSubscribeOptions = {};
    proto_msg_converter::protoToAidl(protoSubscribeOptions, &aidlSubscribeOptions);
    const auto status_code = mHardware->subscribe(aidlSubscribeOptions);
    status->set_status_code(static_cast<proto::StatusCode>(status_code));
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::Unsubscribe(::grpc::ServerContext* context,
                                                   const proto::UnsubscribeRequest* request,
                                                   proto::VehicleHalCallStatus* status) {
    int32_t propId = request->prop_id();
    int32_t areaId = request->area_id();
    const auto status_code = mHardware->unsubscribe(propId, areaId);
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
    result->set_refresh_property_configs(dumpResult.refreshPropertyConfigs);
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::StartPropertyValuesStream(
        ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
        ::grpc::ServerWriter<proto::VehiclePropValues>* stream) {
    auto conn = std::make_shared<ConnectionDescriptor<proto::VehiclePropValues>>(stream);
    {
        std::lock_guard lck(mConnectionMutex);
        mValueStreamingConnections.push_back(conn);
    }
    conn->Wait();
    LOG(ERROR) << __func__ << ": Stream lost, ID : " << conn->ID();
    return ::grpc::Status(::grpc::StatusCode::ABORTED, "Connection lost.");
}

::grpc::Status GrpcVehicleProxyServer::GetMinMaxSupportedValues(
        ::grpc::ServerContext* context, const proto::GetMinMaxSupportedValuesRequest* request,
        proto::GetMinMaxSupportedValuesResult* result) {
    std::vector<PropIdAreaId> propIdAreaIds = getPropIdAreaIdsFromProtoRequest(request);
    std::vector<aidlvhal::MinMaxSupportedValueResult> minMaxSupportedValueResults =
            mHardware->getMinMaxSupportedValues(propIdAreaIds);
    aidlResultsToProtoResults(minMaxSupportedValueResults, result);
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::GetSupportedValuesLists(
        ::grpc::ServerContext* context, const proto::GetSupportedValuesListsRequest* request,
        proto::GetSupportedValuesListsResult* result) {
    std::vector<PropIdAreaId> propIdAreaIds = getPropIdAreaIdsFromProtoRequest(request);
    std::vector<aidlvhal::SupportedValuesListResult> supportedValuesListResults =
            mHardware->getSupportedValuesLists(propIdAreaIds);
    aidlResultsToProtoResults(supportedValuesListResults, result);
    return ::grpc::Status::OK;
}

::grpc::Status GrpcVehicleProxyServer::StartSupportedValuesChangeStream(
        ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
        ::grpc::ServerWriter<proto::SupportedValuesChange>* stream) {
    auto conn = std::make_shared<ConnectionDescriptor<proto::SupportedValuesChange>>(stream);
    {
        std::lock_guard lck(mConnectionMutex);
        mSupportedValuesChangeConnections.push_back(conn);
    }
    conn->Wait();
    LOG(ERROR) << __func__ << ": Stream lost, ID : " << conn->ID();
    return ::grpc::Status(::grpc::StatusCode::ABORTED, "Connection lost.");
}

void GrpcVehicleProxyServer::OnVehiclePropChange(
        const std::vector<aidlvhal::VehiclePropValue>& values) {
    proto::VehiclePropValues protoValues;
    for (const auto& value : values) {
        auto* protoValuePtr = protoValues.add_values();
        proto_msg_converter::aidlToProto(value, protoValuePtr);
    }
    writeToStream(mValueStreamingConnections, protoValues);
}

void GrpcVehicleProxyServer::OnSupportedValuesChange(
        const std::vector<PropIdAreaId>& propIdAreaIds) {
    proto::SupportedValuesChange protoValues;
    for (const auto& propIdAreaId : propIdAreaIds) {
        auto* protoValuePtr = protoValues.add_prop_id_area_ids();
        proto_msg_converter::aidlToProto(propIdAreaId, protoValuePtr);
    }
    writeToStream(mSupportedValuesChangeConnections, protoValues);
}

template <typename ValueType>
void GrpcVehicleProxyServer::writeToStream(
        std::vector<std::shared_ptr<ConnectionDescriptor<ValueType>>>& connections,
        const ValueType& protoValues) {
    std::unordered_set<uint64_t> brokenConn;
    {
        std::shared_lock read_lock(mConnectionMutex);
        for (auto& connection : connections) {
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
    connections.erase(std::remove_if(connections.begin(), connections.end(),
                                     [&brokenConn](const auto& conn) {
                                         return brokenConn.find(conn->ID()) != brokenConn.end();
                                     }),
                      connections.end());
}

GrpcVehicleProxyServer& GrpcVehicleProxyServer::Start() {
    if (mServer) {
        LOG(WARNING) << __func__ << ": GrpcVehicleProxyServer has already started.";
        return *this;
    }
    ::grpc::ServerBuilder builder;
    builder.RegisterService(this);
    for (const std::string& serviceAddr : mServiceAddrs) {
        builder.AddListeningPort(serviceAddr, getServerCredentials());
    }
    mServer = builder.BuildAndStart();
    CHECK(mServer) << __func__ << ": failed to create the GRPC server, "
                   << "please make sure the configuration and permissions are correct";
    return *this;
}

GrpcVehicleProxyServer& GrpcVehicleProxyServer::Shutdown() {
    LOG(INFO) << __func__ << ": Start shutting down GrpcVehicleProxyServer";
    std::shared_lock read_lock(mConnectionMutex);
    LOG(INFO) << __func__ << ": Waiting for value stream connection to shutdown";
    for (auto& conn : mValueStreamingConnections) {
        conn->Shutdown();
    }
    LOG(INFO) << __func__ << ": Waiting for supported values change stream connection to shutdown";
    for (auto& conn : mSupportedValuesChangeConnections) {
        conn->Shutdown();
    }
    LOG(INFO) << __func__ << ": Requesting server to shutdown";
    if (mServer) {
        mServer->Shutdown();
    }
    return *this;
}

void GrpcVehicleProxyServer::Wait() {
    LOG(INFO) << __func__ << ": Waiting for server to shutdown";
    if (mServer) {
        mServer->Wait();
    }
    LOG(INFO) << __func__ << ": Server shutdown complete";
    mServer.reset();
}

template <typename ValueType>
GrpcVehicleProxyServer::ConnectionDescriptor<ValueType>::~ConnectionDescriptor() {
    Shutdown();
}

template <typename ValueType>
bool GrpcVehicleProxyServer::ConnectionDescriptor<ValueType>::Write(const ValueType& values) {
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

template <typename ValueType>
void GrpcVehicleProxyServer::ConnectionDescriptor<ValueType>::Wait() {
    std::unique_lock lck(*mMtx);
    mCV->wait(lck, [this] { return mShutdownFlag; });
}

template <typename ValueType>
void GrpcVehicleProxyServer::ConnectionDescriptor<ValueType>::Shutdown() {
    {
        std::lock_guard lck(*mMtx);
        mShutdownFlag = true;
    }
    mCV->notify_all();
}

}  // namespace android::hardware::automotive::vehicle::virtualization
