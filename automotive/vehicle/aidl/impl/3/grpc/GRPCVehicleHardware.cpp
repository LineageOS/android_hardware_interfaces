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
#include <utils/SystemClock.h>

#include <cstdlib>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace android::hardware::automotive::vehicle::virtualization {

namespace {

constexpr size_t MAX_RETRY_COUNT = 5;

std::shared_ptr<::grpc::ChannelCredentials> getChannelCredentials() {
    return ::grpc::InsecureChannelCredentials();
}

}  // namespace

GRPCVehicleHardware::GRPCVehicleHardware(std::string service_addr)
    : mServiceAddr(std::move(service_addr)),
      mGrpcChannel(::grpc::CreateChannel(mServiceAddr, getChannelCredentials())),
      mGrpcStub(proto::VehicleServer::NewStub(mGrpcChannel)),
      mValuePollingThread([this] { ValuePollingLoop(); }) {}

// Only used for unit testing.
GRPCVehicleHardware::GRPCVehicleHardware(std::unique_ptr<proto::VehicleServer::StubInterface> stub,
                                         bool startValuePollingLoop)
    : mServiceAddr(""), mGrpcChannel(nullptr), mGrpcStub(std::move(stub)) {
    if (startValuePollingLoop) {
        mValuePollingThread = std::thread([this] { ValuePollingLoop(); });
    }
}

GRPCVehicleHardware::~GRPCVehicleHardware() {
    {
        std::lock_guard lck(mShutdownMutex);
        mShuttingDownFlag.store(true);
    }
    mShutdownCV.notify_all();
    if (mValuePollingThread.joinable()) {
        mValuePollingThread.join();
    }
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

std::optional<aidlvhal::VehiclePropConfig> GRPCVehicleHardware::getPropertyConfig(
        int32_t propId) const {
    // TODO(b/354055835): Use GRPC call to get one config instead of getting all the configs.
    for (const auto& config : getAllPropertyConfigs()) {
        if (config.prop == propId) {
            return config;
        }
    }
    return std::nullopt;
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
    std::vector<aidlvhal::GetValueResult> results;
    auto status = getValuesWithRetry(requests, &results, /*retryCount=*/0);
    if (status != aidlvhal::StatusCode::OK) {
        return status;
    }
    if (!results.empty()) {
        (*callback)(std::move(results));
    }
    return status;
}

aidlvhal::StatusCode GRPCVehicleHardware::getValuesWithRetry(
        const std::vector<aidlvhal::GetValueRequest>& requests,
        std::vector<aidlvhal::GetValueResult>* results, size_t retryCount) const {
    if (retryCount == MAX_RETRY_COUNT) {
        LOG(ERROR) << __func__ << ": GRPC GetValues Failed, failed to get the latest value after "
                   << retryCount << " retries";
        return aidlvhal::StatusCode::TRY_AGAIN;
    }

    proto::VehiclePropValueRequests protoRequests;
    std::unordered_map<int64_t, const aidlvhal::GetValueRequest*> requestById;
    for (const auto& request : requests) {
        auto& protoRequest = *protoRequests.add_requests();
        protoRequest.set_request_id(request.requestId);
        proto_msg_converter::aidlToProto(request.prop, protoRequest.mutable_value());
        requestById[request.requestId] = &request;
    }

    // TODO(chenhaosjtuacm): Make it Async.
    ::grpc::ClientContext context;
    proto::GetValueResults protoResults;
    auto grpc_status = mGrpcStub->GetValues(&context, protoRequests, &protoResults);
    if (!grpc_status.ok()) {
        LOG(ERROR) << __func__ << ": GRPC GetValues Failed: " << grpc_status.error_message();
        return aidlvhal::StatusCode::INTERNAL_ERROR;
    }

    std::vector<aidlvhal::GetValueRequest> retryRequests;
    for (const auto& protoResult : protoResults.results()) {
        int64_t requestId = protoResult.request_id();
        auto it = requestById.find(requestId);
        if (it == requestById.end()) {
            LOG(ERROR) << __func__
                       << "Invalid getValue request with unknown request ID: " << requestId
                       << ", ignore";
            continue;
        }

        if (!protoResult.has_value()) {
            auto& result = results->emplace_back();
            result.requestId = requestId;
            result.status = static_cast<aidlvhal::StatusCode>(protoResult.status());
            continue;
        }

        aidlvhal::VehiclePropValue value;
        proto_msg_converter::protoToAidl(protoResult.value(), &value);

        // VHAL proxy server uses a different timestamp then AAOS timestamp, so we have to reset
        // the timestamp.
        // TODO(b/350822044): Remove this once we use timestamp from proxy server.
        if (!setAndroidTimestamp(&value)) {
            // This is a rare case when we receive a property update event reflecting a new value
            // for the property before we receive the get value result. This means that the result
            // is already outdated, hence we should retry getting the latest value again.
            LOG(WARNING) << __func__ << "getValue result for propId: " << value.prop
                         << " areaId: " << value.areaId << " is oudated, retry";
            retryRequests.push_back(*(it->second));
            continue;
        }

        auto& result = results->emplace_back();
        result.requestId = requestId;
        result.status = static_cast<aidlvhal::StatusCode>(protoResult.status());
        result.prop = std::move(value);
    }

    if (retryRequests.size() != 0) {
        return getValuesWithRetry(retryRequests, results, retryCount++);
    }

    return aidlvhal::StatusCode::OK;
}

bool GRPCVehicleHardware::setAndroidTimestamp(aidlvhal::VehiclePropValue* propValue) const {
    PropIdAreaId propIdAreaId = {
            .propId = propValue->prop,
            .areaId = propValue->areaId,
    };
    int64_t now = elapsedRealtimeNano();
    int64_t externalTimestamp = propValue->timestamp;

    {
        std::lock_guard lck(mLatestUpdateTimestampsMutex);
        auto it = mLatestUpdateTimestamps.find(propIdAreaId);
        if (it == mLatestUpdateTimestamps.end() || externalTimestamp > (it->second).first) {
            mLatestUpdateTimestamps[propIdAreaId].first = externalTimestamp;
            mLatestUpdateTimestamps[propIdAreaId].second = now;
            propValue->timestamp = now;
            return true;
        }
        if (externalTimestamp == (it->second).first) {
            propValue->timestamp = (it->second).second;
            return true;
        }
    }
    // externalTimestamp < (it->second).first, the value is outdated.
    return false;
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
            .refreshPropertyConfigs = protoDumpResult.refresh_property_configs(),
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

aidlvhal::StatusCode GRPCVehicleHardware::subscribe(aidlvhal::SubscribeOptions options) {
    proto::SubscribeRequest request;
    ::grpc::ClientContext context;
    proto::VehicleHalCallStatus protoStatus;
    proto_msg_converter::aidlToProto(options, request.mutable_options());
    auto grpc_status = mGrpcStub->Subscribe(&context, request, &protoStatus);
    if (!grpc_status.ok()) {
        if (grpc_status.error_code() == ::grpc::StatusCode::UNIMPLEMENTED) {
            // This is a legacy sever. It should handle updateSampleRate.
            LOG(INFO) << __func__ << ": GRPC Subscribe is not supported by the server";
            return aidlvhal::StatusCode::OK;
        }
        LOG(ERROR) << __func__ << ": GRPC Subscribe Failed: " << grpc_status.error_message();
        return aidlvhal::StatusCode::INTERNAL_ERROR;
    }
    return static_cast<aidlvhal::StatusCode>(protoStatus.status_code());
}

aidlvhal::StatusCode GRPCVehicleHardware::unsubscribe(int32_t propId, int32_t areaId) {
    proto::UnsubscribeRequest request;
    ::grpc::ClientContext context;
    proto::VehicleHalCallStatus protoStatus;
    request.set_prop_id(propId);
    request.set_area_id(areaId);
    auto grpc_status = mGrpcStub->Unsubscribe(&context, request, &protoStatus);
    if (!grpc_status.ok()) {
        if (grpc_status.error_code() == ::grpc::StatusCode::UNIMPLEMENTED) {
            // This is a legacy sever. Ignore unsubscribe request.
            LOG(INFO) << __func__ << ": GRPC Unsubscribe is not supported by the server";
            return aidlvhal::StatusCode::OK;
        }
        LOG(ERROR) << __func__ << ": GRPC Unsubscribe Failed: " << grpc_status.error_message();
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
        pollValue();
        // try to reconnect
    }
}

void GRPCVehicleHardware::pollValue() {
    ::grpc::ClientContext context;

    bool rpc_stopped{false};
    std::thread shuttingdown_watcher([this, &rpc_stopped, &context]() {
        std::unique_lock<std::mutex> lck(mShutdownMutex);
        mShutdownCV.wait(
                lck, [this, &rpc_stopped]() { return rpc_stopped || mShuttingDownFlag.load(); });
        context.TryCancel();
    });

    auto value_stream = mGrpcStub->StartPropertyValuesStream(&context, ::google::protobuf::Empty());
    LOG(INFO) << __func__ << ": GRPC Value Streaming Started";
    proto::VehiclePropValues protoValues;
    while (!mShuttingDownFlag.load() && value_stream->Read(&protoValues)) {
        std::vector<aidlvhal::VehiclePropValue> values;
        for (const auto protoValue : protoValues.values()) {
            aidlvhal::VehiclePropValue aidlValue = {};
            proto_msg_converter::protoToAidl(protoValue, &aidlValue);

            // VHAL proxy server uses a different timestamp then AAOS timestamp, so we have to
            // reset the timestamp.
            // TODO(b/350822044): Remove this once we use timestamp from proxy server.
            if (!setAndroidTimestamp(&aidlValue)) {
                LOG(WARNING) << __func__ << ": property event for propId: " << aidlValue.prop
                             << " areaId: " << aidlValue.areaId << " is outdated, ignore";
                continue;
            }

            values.push_back(std::move(aidlValue));
        }
        if (values.empty()) {
            continue;
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
}

}  // namespace android::hardware::automotive::vehicle::virtualization
