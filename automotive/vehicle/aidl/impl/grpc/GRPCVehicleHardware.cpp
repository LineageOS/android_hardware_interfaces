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

#include <GRPCVehicleHardware.h>

#include "ProtoMessageConverter.h"

#include <android-base/logging.h>
#include <grpc++/grpc++.h>

#include <cstdlib>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace android::hardware::automotive::vehicle::virtualization {

static std::shared_ptr<::grpc::ChannelCredentials> getChannelCredentials() {
    // TODO(chenhaosjtuacm): get secured credentials here
    return ::grpc::InsecureChannelCredentials();
}

GRPCVehicleHardware::GRPCVehicleHardware(std::string service_addr)
    : mServiceAddr(std::move(service_addr)),
      mGrpcChannel(::grpc::CreateChannel(mServiceAddr, getChannelCredentials())),
      mGrpcStub(proto::VehicleServer::NewStub(mGrpcChannel)),
      mValuePollingThread([this] { ValuePollingLoop(); }) {}

GRPCVehicleHardware::~GRPCVehicleHardware() {
    {
        std::lock_guard lck(mShutdownMutex);
        mShuttingDownFlag.store(true);
    }
    mShutdownCV.notify_all();
    mValuePollingThread.join();
}

std::vector<aidlvhal::VehiclePropConfig> GRPCVehicleHardware::getAllPropertyConfigs() const {
    std::vector<aidlvhal::VehiclePropConfig> configs;
    ::grpc::ClientContext context;
    auto config_stream = mGrpcStub->GetAllPropertyConfig(&context, ::google::protobuf::Empty());
    proto::VehiclePropConfig protoConfig;
    while (config_stream->Read(&protoConfig)) {
        aidlvhal::VehiclePropConfig config;
        proto_msg_converter::protoToAidl(protoConfig, &config);
        configs.push_back(std::move(config));
    }
    auto grpc_status = config_stream->Finish();
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__
                   << ": GRPC GetAllPropertyConfig Failed: " << grpc_status.error_message();
    }
    return configs;
}

aidlvhal::StatusCode GRPCVehicleHardware::setValues(
        std::shared_ptr<const SetValuesCallback> callback,
        const std::vector<aidlvhal::SetValueRequest>& requests) {
    ::grpc::ClientContext context;
    proto::VehiclePropValueRequests protoRequests;
    proto::SetValueResults protoResults;
    for (const auto& request : requests) {
        auto& protoRequest = *protoRequests.add_requests();
        protoRequest.set_request_id(request.requestId);
        proto_msg_converter::aidlToProto(request.value, protoRequest.mutable_value());
    }
    // TODO(chenhaosjtuacm): Make it Async.
    auto grpc_status = mGrpcStub->SetValues(&context, protoRequests, &protoResults);
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__ << ": GRPC SetValues Failed: " << grpc_status.error_message();
        {
            std::shared_lock lck(mCallbackMutex);
            // TODO(chenhaosjtuacm): call on-set-error callback.
        }
        return aidlvhal::StatusCode::INTERNAL_ERROR;
    }
    std::vector<aidlvhal::SetValueResult> results;
    for (const auto& protoResult : protoResults.results()) {
        auto& result = results.emplace_back();
        result.requestId = protoResult.request_id();
        result.status = static_cast<aidlvhal::StatusCode>(protoResult.status());
        // TODO(chenhaosjtuacm): call on-set-error callback.
    }
    (*callback)(std::move(results));

    return aidlvhal::StatusCode::OK;
}

aidlvhal::StatusCode GRPCVehicleHardware::getValues(
        std::shared_ptr<const GetValuesCallback> callback,
        const std::vector<aidlvhal::GetValueRequest>& requests) const {
    ::grpc::ClientContext context;
    proto::VehiclePropValueRequests protoRequests;
    proto::GetValueResults protoResults;
    for (const auto& request : requests) {
        auto& protoRequest = *protoRequests.add_requests();
        protoRequest.set_request_id(request.requestId);
        proto_msg_converter::aidlToProto(request.prop, protoRequest.mutable_value());
    }
    // TODO(chenhaosjtuacm): Make it Async.
    auto grpc_status = mGrpcStub->GetValues(&context, protoRequests, &protoResults);
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__ << ": GRPC GetValues Failed: " << grpc_status.error_message();
        return aidlvhal::StatusCode::INTERNAL_ERROR;
    }
    std::vector<aidlvhal::GetValueResult> results;
    for (const auto& protoResult : protoResults.results()) {
        auto& result = results.emplace_back();
        result.requestId = protoResult.request_id();
        result.status = static_cast<aidlvhal::StatusCode>(protoResult.status());
        if (protoResult.has_value()) {
            aidlvhal::VehiclePropValue value;
            proto_msg_converter::protoToAidl(protoResult.value(), &value);
            result.prop = std::move(value);
        }
    }
    (*callback)(std::move(results));

    return aidlvhal::StatusCode::OK;
}

void GRPCVehicleHardware::registerOnPropertyChangeEvent(
        std::unique_ptr<const PropertyChangeCallback> callback) {
    std::lock_guard lck(mCallbackMutex);
    if (mOnPropChange) {
        LOG(ERROR) << __func__ << " must only be called once.";
        return;
    }
    mOnPropChange = std::move(callback);
}

void GRPCVehicleHardware::registerOnPropertySetErrorEvent(
        std::unique_ptr<const PropertySetErrorCallback> callback) {
    std::lock_guard lck(mCallbackMutex);
    if (mOnSetErr) {
        LOG(ERROR) << __func__ << " must only be called once.";
        return;
    }
    mOnSetErr = std::move(callback);
}

DumpResult GRPCVehicleHardware::dump(const std::vector<std::string>& options) {
    ::grpc::ClientContext context;
    proto::DumpOptions protoDumpOptions;
    proto::DumpResult protoDumpResult;
    for (const auto& option : options) {
        protoDumpOptions.add_options(option);
    }
    auto grpc_status = mGrpcStub->Dump(&context, protoDumpOptions, &protoDumpResult);
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__ << ": GRPC Dump Failed: " << grpc_status.error_message();
        return {};
    }
    return {
            .callerShouldDumpState = protoDumpResult.caller_should_dump_state(),
            .buffer = protoDumpResult.buffer(),
    };
}

aidlvhal::StatusCode GRPCVehicleHardware::checkHealth() {
    ::grpc::ClientContext context;
    proto::VehicleHalCallStatus protoStatus;
    auto grpc_status = mGrpcStub->CheckHealth(&context, ::google::protobuf::Empty(), &protoStatus);
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__ << ": GRPC CheckHealth Failed: " << grpc_status.error_message();
        return aidlvhal::StatusCode::INTERNAL_ERROR;
    }
    return static_cast<aidlvhal::StatusCode>(protoStatus.status_code());
}

aidlvhal::StatusCode GRPCVehicleHardware::updateSampleRate(int32_t propId, int32_t areaId,
                                                           float sampleRate) {
    ::grpc::ClientContext context;
    proto::UpdateSampleRateRequest request;
    proto::VehicleHalCallStatus protoStatus;
    request.set_prop(propId);
    request.set_area_id(areaId);
    request.set_sample_rate(sampleRate);
    auto grpc_status = mGrpcStub->UpdateSampleRate(&context, request, &protoStatus);
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__ << ": GRPC UpdateSampleRate Failed: " << grpc_status.error_message();
        return aidlvhal::StatusCode::INTERNAL_ERROR;
    }
    return static_cast<aidlvhal::StatusCode>(protoStatus.status_code());
}

bool GRPCVehicleHardware::waitForConnected(std::chrono::milliseconds waitTime) {
    return mGrpcChannel->WaitForConnected(gpr_time_add(
            gpr_now(GPR_CLOCK_MONOTONIC), gpr_time_from_millis(waitTime.count(), GPR_TIMESPAN)));
}

void GRPCVehicleHardware::ValuePollingLoop() {
    while (!mShuttingDownFlag.load()) {
        ::grpc::ClientContext context;

        bool rpc_stopped{false};
        std::thread shuttingdown_watcher([this, &rpc_stopped, &context]() {
            std::unique_lock<std::mutex> lck(mShutdownMutex);
            mShutdownCV.wait(lck, [this, &rpc_stopped]() {
                return rpc_stopped || mShuttingDownFlag.load();
            });
            context.TryCancel();
        });

        auto value_stream =
                mGrpcStub->StartPropertyValuesStream(&context, ::google::protobuf::Empty());
        LOG(INFO) << __func__ << ": GRPC Value Streaming Started";
        proto::VehiclePropValues protoValues;
        while (!mShuttingDownFlag.load() && value_stream->Read(&protoValues)) {
            std::vector<aidlvhal::VehiclePropValue> values;
            for (const auto protoValue : protoValues.values()) {
                values.push_back(aidlvhal::VehiclePropValue());
                proto_msg_converter::protoToAidl(protoValue, &values.back());
            }
            std::shared_lock lck(mCallbackMutex);
            if (mOnPropChange) {
                (*mOnPropChange)(values);
            }
        }

        {
            std::lock_guard lck(mShutdownMutex);
            rpc_stopped = true;
        }
        mShutdownCV.notify_all();
        shuttingdown_watcher.join();

        auto grpc_status = value_stream->Finish();
        // never reach here until connection lost
        LOG(ERROR) << __func__ << ": GRPC Value Streaming Failed: " << grpc_status.error_message();

        // try to reconnect
    }
}

}  // namespace android::hardware::automotive::vehicle::virtualization
