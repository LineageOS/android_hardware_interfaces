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

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.1/Conversions.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace android::hardware::neuralnetworks::V1_2::utils {
namespace {

nn::GeneralResult<nn::Capabilities> initCapabilities(V1_2::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<nn::Capabilities> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                                 << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status, const Capabilities& capabilities) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "getCapabilities_1_2 failed with " << toString(status);
        } else {
            result = nn::convert(capabilities);
        }
    };

    const auto ret = device->getCapabilities_1_2(cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return result;
}

}  // namespace

nn::GeneralResult<std::string> initVersionString(V1_2::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<std::string> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                            << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status, const hidl_string& versionString) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "getVersionString failed with " << toString(status);
        } else {
            result = versionString;
        }
    };

    const auto ret = device->getVersionString(cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return result;
}

nn::GeneralResult<nn::DeviceType> initDeviceType(V1_2::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<nn::DeviceType> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                               << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status, DeviceType deviceType) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "getDeviceType failed with " << toString(status);
        } else {
            result = nn::convert(deviceType);
        }
    };

    const auto ret = device->getType(cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return result;
}

nn::GeneralResult<std::vector<nn::Extension>> initExtensions(V1_2::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<std::vector<nn::Extension>> result =
            NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status, const hidl_vec<Extension>& extensions) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "getExtensions failed with " << toString(status);
        } else {
            result = nn::convert(extensions);
        }
    };

    const auto ret = device->getSupportedExtensions(cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return result;
}

nn::GeneralResult<std::pair<uint32_t, uint32_t>> initNumberOfCacheFilesNeeded(
        V1_2::IDevice* device) {
    CHECK(device != nullptr);

    nn::GeneralResult<std::pair<uint32_t, uint32_t>> result =
            NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
    const auto cb = [&result](V1_0::ErrorStatus status, uint32_t numModelCache,
                              uint32_t numDataCache) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical)
                     << "getNumberOfCacheFilesNeeded failed with " << toString(status);
        } else {
            result = std::make_pair(numModelCache, numDataCache);
        }
    };

    const auto ret = device->getNumberOfCacheFilesNeeded(cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return result;
}

nn::GeneralResult<std::shared_ptr<const Device>> Device::create(std::string name,
                                                                sp<V1_2::IDevice> device) {
    if (name.empty()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_2::utils::Device::create must have non-empty name";
    }
    if (device == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_2::utils::Device::create must have non-null device";
    }

    auto versionString = NN_TRY(initVersionString(device.get()));
    const auto deviceType = NN_TRY(initDeviceType(device.get()));
    auto extensions = NN_TRY(initExtensions(device.get()));
    auto capabilities = NN_TRY(initCapabilities(device.get()));
    const auto numberOfCacheFilesNeeded = NN_TRY(initNumberOfCacheFilesNeeded(device.get()));

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(device));
    return std::make_shared<const Device>(
            PrivateConstructorTag{}, std::move(name), std::move(versionString), deviceType,
            std::move(extensions), std::move(capabilities), numberOfCacheFilesNeeded,
            std::move(device), std::move(deathHandler));
}

Device::Device(PrivateConstructorTag /*tag*/, std::string name, std::string versionString,
               nn::DeviceType deviceType, std::vector<nn::Extension> extensions,
               nn::Capabilities capabilities,
               std::pair<uint32_t, uint32_t> numberOfCacheFilesNeeded, sp<V1_2::IDevice> device,
               hal::utils::DeathHandler deathHandler)
    : kName(std::move(name)),
      kVersionString(std::move(versionString)),
      kDeviceType(deviceType),
      kExtensions(std::move(extensions)),
      kCapabilities(std::move(capabilities)),
      kNumberOfCacheFilesNeeded(numberOfCacheFilesNeeded),
      kDevice(std::move(device)),
      kDeathHandler(std::move(deathHandler)) {}

const std::string& Device::getName() const {
    return kName;
}

const std::string& Device::getVersionString() const {
    return kVersionString;
}

nn::Version Device::getFeatureLevel() const {
    return nn::Version::ANDROID_Q;
}

nn::DeviceType Device::getType() const {
    return kDeviceType;
}

const std::vector<nn::Extension>& Device::getSupportedExtensions() const {
    return kExtensions;
}

const nn::Capabilities& Device::getCapabilities() const {
    return kCapabilities;
}

std::pair<uint32_t, uint32_t> Device::getNumberOfCacheFilesNeeded() const {
    return kNumberOfCacheFilesNeeded;
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

    nn::GeneralResult<std::vector<bool>> result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                                  << "uninitialized";
    auto cb = [&result, &model](V1_0::ErrorStatus status,
                                const hidl_vec<bool>& supportedOperations) {
        if (status != V1_0::ErrorStatus::NONE) {
            const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical)
                     << "getSupportedOperations_1_2 failed with " << toString(status);
        } else if (supportedOperations.size() != model.main.operations.size()) {
            result = NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                     << "getSupportedOperations_1_2 returned vector of size "
                     << supportedOperations.size() << " but expected "
                     << model.main.operations.size();
        } else {
            result = supportedOperations;
        }
    };

    const auto ret = kDevice->getSupportedOperations_1_2(hidlModel, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return result;
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModel(
        const nn::Model& model, nn::ExecutionPreference preference, nn::Priority /*priority*/,
        nn::OptionalTimePoint /*deadline*/, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    // Ensure that model is ready for IPC.
    std::optional<nn::Model> maybeModelInShared;
    const nn::Model& modelInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&model, &maybeModelInShared));

    const auto hidlModel = NN_TRY(convert(modelInShared));
    const auto hidlPreference = NN_TRY(V1_1::utils::convert(preference));
    const auto hidlModelCache = NN_TRY(convert(modelCache));
    const auto hidlDataCache = NN_TRY(convert(dataCache));
    const auto hidlToken = token;

    const auto cb = sp<PreparedModelCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kDevice->prepareModel_1_2(hidlModel, hidlPreference, hidlModelCache,
                                               hidlDataCache, hidlToken, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    if (status != V1_0::ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        return NN_ERROR(canonical) << "prepareModel_1_2 failed with " << toString(status);
    }

    return cb->get();
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModelFromCache(
        nn::OptionalTimePoint /*deadline*/, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    const auto hidlModelCache = NN_TRY(convert(modelCache));
    const auto hidlDataCache = NN_TRY(convert(dataCache));
    const auto hidlToken = token;

    const auto cb = sp<PreparedModelCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kDevice->prepareModelFromCache(hidlModelCache, hidlDataCache, hidlToken, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    if (status != V1_0::ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        return NN_ERROR(canonical) << "prepareModelFromCache failed with " << toString(status);
    }

    return cb->get();
}

nn::GeneralResult<nn::SharedBuffer> Device::allocate(
        const nn::BufferDesc& /*desc*/,
        const std::vector<nn::SharedPreparedModel>& /*preparedModels*/,
        const std::vector<nn::BufferRole>& /*inputRoles*/,
        const std::vector<nn::BufferRole>& /*outputRoles*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IDevice::allocate not supported on 1.2 HAL service";
}

}  // namespace android::hardware::neuralnetworks::V1_2::utils
