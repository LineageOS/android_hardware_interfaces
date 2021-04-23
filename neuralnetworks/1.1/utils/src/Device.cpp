/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "Device.h"

#include "Conversions.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Callbacks.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_1::utils {
namespace {

nn::GeneralResult<nn::Capabilities> capabilitiesCallback(V1_0::ErrorStatus status,
                                                         const Capabilities& capabilities) {
    HANDLE_HAL_STATUS(status) << "getting capabilities failed with " << toString(status);
    return nn::convert(capabilities);
}

nn::GeneralResult<nn::Capabilities> getCapabilitiesFrom(V1_1::IDevice* device) {
    CHECK(device != nullptr);

    auto cb = hal::utils::CallbackValue(capabilitiesCallback);

    const auto ret = device->getCapabilities_1_1(cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return cb.take();
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const Device>> Device::create(std::string name,
                                                                sp<V1_1::IDevice> device) {
    if (name.empty()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_1::utils::Device::create must have non-empty name";
    }
    if (device == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_1::utils::Device::create must have non-null device";
    }

    auto capabilities = NN_TRY(getCapabilitiesFrom(device.get()));

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(device));
    return std::make_shared<const Device>(PrivateConstructorTag{}, std::move(name),
                                          std::move(capabilities), std::move(device),
                                          std::move(deathHandler));
}

Device::Device(PrivateConstructorTag /*tag*/, std::string name, nn::Capabilities capabilities,
               sp<V1_1::IDevice> device, hal::utils::DeathHandler deathHandler)
    : kName(std::move(name)),
      kCapabilities(std::move(capabilities)),
      kDevice(std::move(device)),
      kDeathHandler(std::move(deathHandler)) {}

const std::string& Device::getName() const {
    return kName;
}

const std::string& Device::getVersionString() const {
    return kVersionString;
}

nn::Version Device::getFeatureLevel() const {
    return nn::Version::ANDROID_P;
}

nn::DeviceType Device::getType() const {
    return nn::DeviceType::UNKNOWN;
}

const std::vector<nn::Extension>& Device::getSupportedExtensions() const {
    return kExtensions;
}

const nn::Capabilities& Device::getCapabilities() const {
    return kCapabilities;
}

std::pair<uint32_t, uint32_t> Device::getNumberOfCacheFilesNeeded() const {
    return std::make_pair(/*numModelCache=*/0, /*numDataCache=*/0);
}

nn::GeneralResult<void> Device::wait() const {
    const auto ret = kDevice->ping();
    HANDLE_TRANSPORT_FAILURE(ret);
    return {};
}

nn::GeneralResult<std::vector<bool>> Device::getSupportedOperations(const nn::Model& model) const {
    // Ensure that model is ready for IPC.
    std::optional<nn::Model> maybeModelInShared;
    const nn::Model& modelInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&model, &maybeModelInShared));

    const auto hidlModel = NN_TRY(convert(modelInShared));

    auto cb = hal::utils::CallbackValue(V1_0::utils::supportedOperationsCallback);

    const auto ret = kDevice->getSupportedOperations_1_1(hidlModel, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return cb.take();
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModel(
        const nn::Model& model, nn::ExecutionPreference preference, nn::Priority /*priority*/,
        nn::OptionalTimePoint /*deadline*/, const std::vector<nn::SharedHandle>& /*modelCache*/,
        const std::vector<nn::SharedHandle>& /*dataCache*/, const nn::CacheToken& /*token*/) const {
    // Ensure that model is ready for IPC.
    std::optional<nn::Model> maybeModelInShared;
    const nn::Model& modelInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&model, &maybeModelInShared));

    const auto hidlModel = NN_TRY(convert(modelInShared));
    const auto hidlPreference = NN_TRY(convert(preference));

    const auto cb = sp<V1_0::utils::PreparedModelCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kDevice->prepareModel_1_1(hidlModel, hidlPreference, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    HANDLE_HAL_STATUS(status) << "model preparation failed with " << toString(status);

    return cb->get();
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModelFromCache(
        nn::OptionalTimePoint /*deadline*/, const std::vector<nn::SharedHandle>& /*modelCache*/,
        const std::vector<nn::SharedHandle>& /*dataCache*/, const nn::CacheToken& /*token*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IDevice::prepareModelFromCache not supported on 1.1 HAL service";
}

nn::GeneralResult<nn::SharedBuffer> Device::allocate(
        const nn::BufferDesc& /*desc*/,
        const std::vector<nn::SharedPreparedModel>& /*preparedModels*/,
        const std::vector<nn::BufferRole>& /*inputRoles*/,
        const std::vector<nn::BufferRole>& /*outputRoles*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IDevice::allocate not supported on 1.1 HAL service";
}

}  // namespace android::hardware::neuralnetworks::V1_1::utils
