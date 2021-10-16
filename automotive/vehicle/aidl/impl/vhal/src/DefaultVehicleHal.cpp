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
using ::aidl::android::hardware::automotive::vehicle::SetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::automotive::car_binder_lib::LargeParcelableBase;
using ::android::base::expected;
using ::android::base::Result;
using ::ndk::ScopedAStatus;

DefaultVehicleHal::DefaultVehicleHal(std::unique_ptr<IVehicleHardware> hardware)
    : mVehicleHardware(std::move(hardware)) {
    auto configs = mVehicleHardware->getAllPropertyConfigs();
    for (auto& config : configs) {
        mConfigsByPropId[config.prop] = config;
    }
    auto result = LargeParcelableBase::parcelableVectorToStableLargeParcelable(configs);
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

ScopedAStatus DefaultVehicleHal::getValues(const CallbackType& callback,
                                           const GetValueRequests& requests) {
    // TODO(b/203713317): check for duplicate properties and duplicate request IDs.

    const std::vector<GetValueRequest>* getValueRequests;
    // Define deserializedResults here because we need it to have the same lifetime as
    // getValueRequests.
    expected<std::vector<GetValueRequest>, ScopedAStatus> deserializedResults;
    if (!requests.payloads.empty()) {
        getValueRequests = &requests.payloads;
    } else {
        deserializedResults = stableLargeParcelableToVector<GetValueRequest>(requests);
        if (!deserializedResults.ok()) {
            ALOGE("failed to parse getValues requests");
            return std::move(deserializedResults.error());
        }
        getValueRequests = &deserializedResults.value();
    }

    std::shared_ptr<GetValuesClient> client;
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        client = getOrCreateClient(&mGetValuesClients, callback);
    }

    if (StatusCode status =
                mVehicleHardware->getValues(client->getResultCallback(), *getValueRequests);
        status != StatusCode::OK) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                toInt(status), "failed to get value from VehicleHardware");
    }

    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::setValues(const CallbackType&, const SetValueRequests&) {
    // TODO(b/200737967): implement this.
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
