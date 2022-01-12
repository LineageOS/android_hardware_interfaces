/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "DefaultVehicleHal"

#include <DefaultVehicleHal.h>

#include <LargeParcelableBase.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <android-base/result.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::GetValueResults;
using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::SetValueResults;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::automotive::car_binder_lib::LargeParcelableBase;
using ::android::base::Error;
using ::android::base::expected;
using ::android::base::Result;
using ::ndk::ScopedAStatus;

DefaultVehicleHal::DefaultVehicleHal(std::unique_ptr<IVehicleHardware> hardware)
    : mVehicleHardware(std::move(hardware)) {
    auto configs = mVehicleHardware->getAllPropertyConfigs();
    for (auto& config : configs) {
        mConfigsByPropId[config.prop] = config;
    }
    VehiclePropConfigs vehiclePropConfigs;
    vehiclePropConfigs.payloads = std::move(configs);
    auto result = LargeParcelableBase::parcelableToStableLargeParcelable(vehiclePropConfigs);
    if (!result.ok()) {
        ALOGE("failed to convert configs to shared memory file, error: %s, code: %d",
              getErrorMsg(result).c_str(), getIntErrorCode(result));
        return;
    }

    if (result.value() != nullptr) {
        mConfigFile = std::move(result.value());
    }
}

ScopedAStatus DefaultVehicleHal::getAllPropConfigs(VehiclePropConfigs* output) {
    if (mConfigFile != nullptr) {
        output->payloads.clear();
        output->sharedMemoryFd.set(dup(mConfigFile->get()));
        return ScopedAStatus::ok();
    }
    output->payloads.reserve(mConfigsByPropId.size());
    for (const auto& [_, config] : mConfigsByPropId) {
        output->payloads.push_back(config);
    }
    return ScopedAStatus::ok();
}

template <class T>
std::shared_ptr<T> DefaultVehicleHal::getOrCreateClient(
        std::unordered_map<CallbackType, std::shared_ptr<T>>* clients,
        const CallbackType& callback) {
    if (clients->find(callback) == clients->end()) {
        // TODO(b/204943359): Remove client from clients when linkToDeath is implemented.
        (*clients)[callback] = std::make_shared<T>(callback);
    }
    return (*clients)[callback];
}

template std::shared_ptr<DefaultVehicleHal::GetValuesClient>
DefaultVehicleHal::getOrCreateClient<DefaultVehicleHal::GetValuesClient>(
        std::unordered_map<CallbackType, std::shared_ptr<GetValuesClient>>* clients,
        const CallbackType& callback);

template std::shared_ptr<DefaultVehicleHal::SetValuesClient>
DefaultVehicleHal::getOrCreateClient<DefaultVehicleHal::SetValuesClient>(
        std::unordered_map<CallbackType, std::shared_ptr<SetValuesClient>>* clients,
        const CallbackType& callback);

Result<void> DefaultVehicleHal::checkProperty(const VehiclePropValue& propValue) {
    int32_t propId = propValue.prop;
    auto it = mConfigsByPropId.find(propId);
    if (it == mConfigsByPropId.end()) {
        return Error() << "no config for property, ID: " << propId;
    }
    const VehiclePropConfig& config = it->second;
    const VehicleAreaConfig* areaConfig = getAreaConfig(propValue, config);
    if (!isGlobalProp(propId) && areaConfig == nullptr) {
        // Ignore areaId for global property. For non global property, check whether areaId is
        // allowed. areaId must appear in areaConfig.
        return Error() << "invalid area ID: " << propValue.areaId << " for prop ID: " << propId
                       << ", not listed in config";
    }
    if (auto result = checkPropValue(propValue, &config); !result.ok()) {
        return Error() << "invalid property value: " << propValue.toString()
                       << ", error: " << result.error().message();
    }
    if (auto result = checkValueRange(propValue, areaConfig); !result.ok()) {
        return Error() << "property value out of range: " << propValue.toString()
                       << ", error: " << result.error().message();
    }
    return {};
}

ScopedAStatus DefaultVehicleHal::getValues(const CallbackType& callback,
                                           const GetValueRequests& requests) {
    // TODO(b/203713317): check for duplicate properties and duplicate request IDs.

    expected<LargeParcelableBase::BorrowedOwnedObject<GetValueRequests>, ScopedAStatus>
            deserializedResults = fromStableLargeParcelable(requests);
    if (!deserializedResults.ok()) {
        ALOGE("getValues: failed to parse getValues requests");
        return std::move(deserializedResults.error());
    }
    const std::vector<GetValueRequest>& getValueRequests =
            deserializedResults.value().getObject()->payloads;

    std::shared_ptr<GetValuesClient> client;
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        client = getOrCreateClient(&mGetValuesClients, callback);
    }

    if (StatusCode status =
                mVehicleHardware->getValues(client->getResultCallback(), getValueRequests);
        status != StatusCode::OK) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                toInt(status), "failed to get value from VehicleHardware");
    }

    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::setValues(const CallbackType& callback,
                                           const SetValueRequests& requests) {
    // TODO(b/203713317): check for duplicate properties and duplicate request IDs.

    expected<LargeParcelableBase::BorrowedOwnedObject<SetValueRequests>, ScopedAStatus>
            deserializedResults = fromStableLargeParcelable(requests);
    if (!deserializedResults.ok()) {
        ALOGE("setValues: failed to parse setValues requests");
        return std::move(deserializedResults.error());
    }
    const std::vector<SetValueRequest>& setValueRequests =
            deserializedResults.value().getObject()->payloads;

    // A list of failed result we already know before sending to hardware.
    std::vector<SetValueResult> failedResults;
    // The list of requests that we would send to hardware.
    std::vector<SetValueRequest> hardwareRequests;

    for (auto& request : setValueRequests) {
        int64_t requestId = request.requestId;
        if (auto result = checkProperty(request.value); !result.ok()) {
            ALOGW("property not valid: %s", result.error().message().c_str());
            failedResults.push_back(SetValueResult{
                    .requestId = requestId,
                    .status = StatusCode::INVALID_ARG,
            });
            continue;
        }
        hardwareRequests.push_back(request);
    }

    std::shared_ptr<SetValuesClient> client;
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        client = getOrCreateClient(&mSetValuesClients, callback);
    }

    if (!failedResults.empty()) {
        client->sendResults(failedResults);
    }

    if (StatusCode status =
                mVehicleHardware->setValues(client->getResultCallback(), hardwareRequests);
        status != StatusCode::OK) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                toInt(status), "failed to set value to VehicleHardware");
    }

    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::getPropConfigs(const std::vector<int32_t>& props,
                                                VehiclePropConfigs* output) {
    std::vector<VehiclePropConfig> configs;
    for (int32_t prop : props) {
        if (mConfigsByPropId.find(prop) != mConfigsByPropId.end()) {
            configs.push_back(mConfigsByPropId[prop]);
        }
    }
    return vectorToStableLargeParcelable(std::move(configs), output);
}

ScopedAStatus DefaultVehicleHal::subscribe(const CallbackType&,
                                           const std::vector<SubscribeOptions>&, int32_t) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::unsubscribe(const CallbackType&, const std::vector<int32_t>&) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::returnSharedMemory(const CallbackType&, int64_t) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

IVehicleHardware* DefaultVehicleHal::getHardware() {
    return mVehicleHardware.get();
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
