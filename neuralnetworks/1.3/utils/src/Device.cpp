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

#include "Buffer.h"
#include "Callbacks.h"
#include "Conversions.h"
#include "PreparedModel.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.1/Conversions.h>
#include <nnapi/hal/1.2/Conversions.h>
#include <nnapi/hal/1.2/Device.h>
#include <nnapi/hal/1.2/Utils.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

nn::GeneralResult<hidl_vec<sp<IPreparedModel>>> convert(
        const std::vector<nn::SharedPreparedModel>& preparedModels) {
    hidl_vec<sp<IPreparedModel>> hidlPreparedModels(preparedModels.size());
    for (size_t i = 0; i < preparedModels.size(); ++i) {
        std::any underlyingResource = preparedModels[i]->getUnderlyingResource();
        if (const auto* hidlPreparedModel =
                    std::any_cast<sp<IPreparedModel>>(&underlyingResource)) {
            hidlPreparedModels[i] = *hidlPreparedModel;
        } else {
            return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
                   << "Unable to convert from nn::IPreparedModel to V1_3::IPreparedModel";
        }
    }
    return hidlPreparedModels;
}

nn::GeneralResult<nn::Capabilities> capabilitiesCallback(ErrorStatus status,
                                                         const Capabilities& capabilities) {
    HANDLE_HAL_STATUS(status) << "getting capabilities failed with " << toString(status);
    return nn::convert(capabilities);
}

nn::GeneralResult<nn::Capabilities> getCapabilitiesFrom(V1_3::IDevice* device) {
    CHECK(device != nullptr);

    auto cb = hal::utils::CallbackValue(capabilitiesCallback);

    const auto ret = device->getCapabilities_1_3(cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return cb.take();
}

nn::GeneralResult<nn::SharedBuffer> allocationCallback(ErrorStatus status,
                                                       const sp<IBuffer>& buffer, uint32_t token) {
    HANDLE_HAL_STATUS(status) << "IDevice::allocate failed with " << toString(status);
    return Buffer::create(buffer, static_cast<nn::Request::MemoryDomainToken>(token));
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const Device>> Device::create(std::string name,
                                                                sp<V1_3::IDevice> device) {
    if (name.empty()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_3::utils::Device::create must have non-empty name";
    }
    if (device == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_3::utils::Device::create must have non-null device";
    }

    auto versionString = NN_TRY(V1_2::utils::getVersionStringFrom(device.get()));
    const auto deviceType = NN_TRY(V1_2::utils::getDeviceTypeFrom(device.get()));
    auto extensions = NN_TRY(V1_2::utils::getSupportedExtensionsFrom(device.get()));
    auto capabilities = NN_TRY(getCapabilitiesFrom(device.get()));
    const auto numberOfCacheFilesNeeded =
            NN_TRY(V1_2::utils::getNumberOfCacheFilesNeededFrom(device.get()));

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(device));
    return std::make_shared<const Device>(
            PrivateConstructorTag{}, std::move(name), std::move(versionString), deviceType,
            std::move(extensions), std::move(capabilities), numberOfCacheFilesNeeded,
            std::move(device), std::move(deathHandler));
}

Device::Device(PrivateConstructorTag /*tag*/, std::string name, std::string versionString,
               nn::DeviceType deviceType, std::vector<nn::Extension> extensions,
               nn::Capabilities capabilities,
               std::pair<uint32_t, uint32_t> numberOfCacheFilesNeeded, sp<V1_3::IDevice> device,
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
    return nn::Version::ANDROID_R;
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

    auto cb = hal::utils::CallbackValue(supportedOperationsCallback);

    const auto ret = kDevice->getSupportedOperations_1_3(hidlModel, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return cb.take();
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModel(
        const nn::Model& model, nn::ExecutionPreference preference, nn::Priority priority,
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    // Ensure that model is ready for IPC.
    std::optional<nn::Model> maybeModelInShared;
    const nn::Model& modelInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&model, &maybeModelInShared));

    const auto hidlModel = NN_TRY(convert(modelInShared));
    const auto hidlPreference = NN_TRY(convert(preference));
    const auto hidlPriority = NN_TRY(convert(priority));
    const auto hidlDeadline = NN_TRY(convert(deadline));
    const auto hidlModelCache = NN_TRY(convert(modelCache));
    const auto hidlDataCache = NN_TRY(convert(dataCache));
    const auto hidlToken = V1_2::utils::CacheToken{token};

    const auto cb = sp<PreparedModelCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret =
            kDevice->prepareModel_1_3(hidlModel, hidlPreference, hidlPriority, hidlDeadline,
                                      hidlModelCache, hidlDataCache, hidlToken, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    HANDLE_HAL_STATUS(status) << "model preparation failed with " << toString(status);

    return cb->get();
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModelFromCache(
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    const auto hidlDeadline = NN_TRY(convert(deadline));
    const auto hidlModelCache = NN_TRY(convert(modelCache));
    const auto hidlDataCache = NN_TRY(convert(dataCache));
    const auto hidlToken = V1_2::utils::CacheToken{token};

    const auto cb = sp<PreparedModelCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kDevice->prepareModelFromCache_1_3(hidlDeadline, hidlModelCache, hidlDataCache,
                                                        hidlToken, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    HANDLE_HAL_STATUS(status) << "model preparation from cache failed with " << toString(status);

    return cb->get();
}

nn::GeneralResult<nn::SharedBuffer> Device::allocate(
        const nn::BufferDesc& desc, const std::vector<nn::SharedPreparedModel>& preparedModels,
        const std::vector<nn::BufferRole>& inputRoles,
        const std::vector<nn::BufferRole>& outputRoles) const {
    const auto hidlDesc = NN_TRY(convert(desc));
    const auto hidlPreparedModels = NN_TRY(convert(preparedModels));
    const auto hidlInputRoles = NN_TRY(convert(inputRoles));
    const auto hidlOutputRoles = NN_TRY(convert(outputRoles));

    auto cb = hal::utils::CallbackValue(allocationCallback);

    const auto ret =
            kDevice->allocate(hidlDesc, hidlPreparedModels, hidlInputRoles, hidlOutputRoles, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return cb.take();
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
