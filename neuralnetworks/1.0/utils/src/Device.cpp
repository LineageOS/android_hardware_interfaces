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

#include "Callbacks.h"
#include "Conversions.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace android::hardware::neuralnetworks::V1_0::utils {
namespace {

nn::GeneralResult<nn::Capabilities> initCapabilities(V1_0::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<nn::Capabilities> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                                 << "uninitialized";
    const auto cb = [&result](ErrorStatus status, const Capabilities& capabilities) {
        if (status != ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "getCapabilities failed with " << toString(status);
        } else {
            result = validatedConvertToCanonical(capabilities);
        }
    };

    const auto ret = device->getCapabilities(cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const Device>> Device::create(std::string name,
                                                                sp<V1_0::IDevice> device) {
    if (name.empty()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_0::utils::Device::create must have non-empty name";
    }
    if (device == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_0::utils::Device::create must have non-null device";
    }

    auto capabilities = NN_TRY(initCapabilities(device.get()));

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(device));
    return std::make_shared<const Device>(PrivateConstructorTag{}, std::move(name),
                                          std::move(capabilities), std::move(device),
                                          std::move(deathHandler));
}

Device::Device(PrivateConstructorTag /*tag*/, std::string name, nn::Capabilities capabilities,
               sp<V1_0::IDevice> device, hal::utils::DeathHandler deathHandler)
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
    return nn::Version::ANDROID_OC_MR1;
}

nn::DeviceType Device::getType() const {
    return nn::DeviceType::OTHER;
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
    return hal::utils::handleTransportError(ret);
}

nn::GeneralResult<std::vector<bool>> Device::getSupportedOperations(const nn::Model& model) const {
    // Ensure that model is ready for IPC.
    std::optional<nn::Model> maybeModelInShared;
    const nn::Model& modelInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&model, &maybeModelInShared));

    const auto hidlModel = NN_TRY(convert(modelInShared));

    nn::GeneralResult<std::vector<bool>> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                                  << "uninitialized";
    auto cb = [&result, &model](ErrorStatus status, const hidl_vec<bool>& supportedOperations) {
        if (status != ErrorStatus::NONE) {
            const auto canonical =
                    validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical)
                     << "getSupportedOperations failed with " << toString(status);
        } else if (supportedOperations.size() != model.main.operations.size()) {
            result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                     << "getSupportedOperations returned vector of size "
                     << supportedOperations.size() << " but expected "
                     << model.main.operations.size();
        } else {
            result = supportedOperations;
        }
    };

    const auto ret = kDevice->getSupportedOperations(hidlModel, cb);
    NN_TRY(hal::utils::handleTransportError(ret));

    return result;
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModel(
        const nn::Model& model, nn::ExecutionPreference /*preference*/, nn::Priority /*priority*/,
        nn::OptionalTimePoint /*deadline*/, const std::vector<nn::NativeHandle>& /*modelCache*/,
        const std::vector<nn::NativeHandle>& /*dataCache*/, const nn::CacheToken& /*token*/) const {
    // Ensure that model is ready for IPC.
    std::optional<nn::Model> maybeModelInShared;
    const nn::Model& modelInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&model, &maybeModelInShared));

    const auto hidlModel = NN_TRY(convert(modelInShared));

    const auto cb = sp<PreparedModelCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kDevice->prepareModel(hidlModel, cb);
    const auto status = NN_TRY(hal::utils::handleTransportError(ret));
    if (status != ErrorStatus::NONE) {
        const auto canonical =
                validatedConvertToCanonical(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        return NN_ERROR(canonical) << "prepareModel failed with " << toString(status);
    }

    return cb->get();
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModelFromCache(
        nn::OptionalTimePoint /*deadline*/, const std::vector<nn::NativeHandle>& /*modelCache*/,
        const std::vector<nn::NativeHandle>& /*dataCache*/, const nn::CacheToken& /*token*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IDevice::prepareModelFromCache not supported on 1.0 HAL service";
}

nn::GeneralResult<nn::SharedBuffer> Device::allocate(
        const nn::BufferDesc& /*desc*/,
        const std::vector<nn::SharedPreparedModel>& /*preparedModels*/,
        const std::vector<nn::BufferRole>& /*inputRoles*/,
        const std::vector<nn::BufferRole>& /*outputRoles*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IDevice::allocate not supported on 1.0 HAL service";
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
