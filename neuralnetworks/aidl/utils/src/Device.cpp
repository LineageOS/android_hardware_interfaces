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

#include "Device.h"

#include "Buffer.h"
#include "Callbacks.h"
#include "Conversions.h"
#include "PreparedModel.h"
#include "ProtectCallback.h"
#include "Utils.h"

#include <aidl/android/hardware/neuralnetworks/IDevice.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::utils {

namespace {

nn::GeneralResult<std::vector<std::shared_ptr<IPreparedModel>>> convert(
        const std::vector<nn::SharedPreparedModel>& preparedModels) {
    std::vector<std::shared_ptr<IPreparedModel>> aidlPreparedModels(preparedModels.size());
    for (size_t i = 0; i < preparedModels.size(); ++i) {
        std::any underlyingResource = preparedModels[i]->getUnderlyingResource();
        if (const auto* aidlPreparedModel =
                    std::any_cast<std::shared_ptr<aidl_hal::IPreparedModel>>(&underlyingResource)) {
            aidlPreparedModels[i] = *aidlPreparedModel;
        } else {
            return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
                   << "Unable to convert from nn::IPreparedModel to aidl_hal::IPreparedModel";
        }
    }
    return aidlPreparedModels;
}

nn::GeneralResult<nn::Capabilities> getCapabilitiesFrom(IDevice* device) {
    CHECK(device != nullptr);
    Capabilities capabilities;
    const auto ret = device->getCapabilities(&capabilities);
    HANDLE_ASTATUS(ret) << "getCapabilities failed";
    return nn::convert(capabilities);
}

nn::GeneralResult<std::string> getVersionStringFrom(aidl_hal::IDevice* device) {
    CHECK(device != nullptr);
    std::string version;
    const auto ret = device->getVersionString(&version);
    HANDLE_ASTATUS(ret) << "getVersionString failed";
    return version;
}

nn::GeneralResult<nn::DeviceType> getDeviceTypeFrom(aidl_hal::IDevice* device) {
    CHECK(device != nullptr);
    DeviceType deviceType;
    const auto ret = device->getType(&deviceType);
    HANDLE_ASTATUS(ret) << "getDeviceType failed";
    return nn::convert(deviceType);
}

nn::GeneralResult<std::vector<nn::Extension>> getSupportedExtensionsFrom(
        aidl_hal::IDevice* device) {
    CHECK(device != nullptr);
    std::vector<Extension> supportedExtensions;
    const auto ret = device->getSupportedExtensions(&supportedExtensions);
    HANDLE_ASTATUS(ret) << "getExtensions failed";
    return nn::convert(supportedExtensions);
}

nn::GeneralResult<std::pair<uint32_t, uint32_t>> getNumberOfCacheFilesNeededFrom(
        aidl_hal::IDevice* device) {
    CHECK(device != nullptr);
    NumberOfCacheFiles numberOfCacheFiles;
    const auto ret = device->getNumberOfCacheFilesNeeded(&numberOfCacheFiles);
    HANDLE_ASTATUS(ret) << "getNumberOfCacheFilesNeeded failed";

    if (numberOfCacheFiles.numDataCache < 0 || numberOfCacheFiles.numModelCache < 0) {
        return NN_ERROR() << "Driver reported negative numer of cache files needed";
    }
    if (static_cast<uint32_t>(numberOfCacheFiles.numModelCache) > nn::kMaxNumberOfCacheFiles) {
        return NN_ERROR() << "getNumberOfCacheFilesNeeded returned numModelCache files greater "
                             "than allowed max ("
                          << numberOfCacheFiles.numModelCache << " vs "
                          << nn::kMaxNumberOfCacheFiles << ")";
    }
    if (static_cast<uint32_t>(numberOfCacheFiles.numDataCache) > nn::kMaxNumberOfCacheFiles) {
        return NN_ERROR() << "getNumberOfCacheFilesNeeded returned numDataCache files greater "
                             "than allowed max ("
                          << numberOfCacheFiles.numDataCache << " vs " << nn::kMaxNumberOfCacheFiles
                          << ")";
    }
    return std::make_pair(numberOfCacheFiles.numModelCache, numberOfCacheFiles.numDataCache);
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const Device>> Device::create(
        std::string name, std::shared_ptr<aidl_hal::IDevice> device) {
    if (name.empty()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "aidl_hal::utils::Device::create must have non-empty name";
    }
    if (device == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "aidl_hal::utils::Device::create must have non-null device";
    }

    auto versionString = NN_TRY(getVersionStringFrom(device.get()));
    const auto deviceType = NN_TRY(getDeviceTypeFrom(device.get()));
    auto extensions = NN_TRY(getSupportedExtensionsFrom(device.get()));
    auto capabilities = NN_TRY(getCapabilitiesFrom(device.get()));
    const auto numberOfCacheFilesNeeded = NN_TRY(getNumberOfCacheFilesNeededFrom(device.get()));

    auto deathHandler = NN_TRY(DeathHandler::create(device));
    return std::make_shared<const Device>(
            PrivateConstructorTag{}, std::move(name), std::move(versionString), deviceType,
            std::move(extensions), std::move(capabilities), numberOfCacheFilesNeeded,
            std::move(device), std::move(deathHandler));
}

Device::Device(PrivateConstructorTag /*tag*/, std::string name, std::string versionString,
               nn::DeviceType deviceType, std::vector<nn::Extension> extensions,
               nn::Capabilities capabilities,
               std::pair<uint32_t, uint32_t> numberOfCacheFilesNeeded,
               std::shared_ptr<aidl_hal::IDevice> device, DeathHandler deathHandler)
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
    return nn::Version::ANDROID_S;
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
    const auto ret = ndk::ScopedAStatus::fromStatus(AIBinder_ping(kDevice->asBinder().get()));
    HANDLE_ASTATUS(ret) << "ping failed";
    return {};
}

nn::GeneralResult<std::vector<bool>> Device::getSupportedOperations(const nn::Model& model) const {
    // Ensure that model is ready for IPC.
    std::optional<nn::Model> maybeModelInShared;
    const nn::Model& modelInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&model, &maybeModelInShared));

    const auto aidlModel = NN_TRY(convert(modelInShared));

    std::vector<bool> supportedOperations;
    const auto ret = kDevice->getSupportedOperations(aidlModel, &supportedOperations);
    HANDLE_ASTATUS(ret) << "getSupportedOperations failed";

    return supportedOperations;
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModel(
        const nn::Model& model, nn::ExecutionPreference preference, nn::Priority priority,
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    // Ensure that model is ready for IPC.
    std::optional<nn::Model> maybeModelInShared;
    const nn::Model& modelInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&model, &maybeModelInShared));

    const auto aidlModel = NN_TRY(convert(modelInShared));
    const auto aidlPreference = NN_TRY(convert(preference));
    const auto aidlPriority = NN_TRY(convert(priority));
    const auto aidlDeadline = NN_TRY(convert(deadline));
    const auto aidlModelCache = NN_TRY(convert(modelCache));
    const auto aidlDataCache = NN_TRY(convert(dataCache));
    const auto aidlToken = NN_TRY(convert(token));

    const auto cb = ndk::SharedRefBase::make<PreparedModelCallback>();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kDevice->prepareModel(aidlModel, aidlPreference, aidlPriority, aidlDeadline,
                                           aidlModelCache, aidlDataCache, aidlToken, cb);
    HANDLE_ASTATUS(ret) << "prepareModel failed";

    return cb->get();
}

nn::GeneralResult<nn::SharedPreparedModel> Device::prepareModelFromCache(
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    const auto aidlDeadline = NN_TRY(convert(deadline));
    const auto aidlModelCache = NN_TRY(convert(modelCache));
    const auto aidlDataCache = NN_TRY(convert(dataCache));
    const auto aidlToken = NN_TRY(convert(token));

    const auto cb = ndk::SharedRefBase::make<PreparedModelCallback>();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kDevice->prepareModelFromCache(aidlDeadline, aidlModelCache, aidlDataCache,
                                                    aidlToken, cb);
    HANDLE_ASTATUS(ret) << "prepareModelFromCache failed";

    return cb->get();
}

nn::GeneralResult<nn::SharedBuffer> Device::allocate(
        const nn::BufferDesc& desc, const std::vector<nn::SharedPreparedModel>& preparedModels,
        const std::vector<nn::BufferRole>& inputRoles,
        const std::vector<nn::BufferRole>& outputRoles) const {
    const auto aidlDesc = NN_TRY(convert(desc));
    const auto aidlPreparedModels = NN_TRY(convert(preparedModels));
    const auto aidlInputRoles = NN_TRY(convert(inputRoles));
    const auto aidlOutputRoles = NN_TRY(convert(outputRoles));

    std::vector<IPreparedModelParcel> aidlPreparedModelParcels;
    aidlPreparedModelParcels.reserve(aidlPreparedModels.size());
    for (const auto& preparedModel : aidlPreparedModels) {
        aidlPreparedModelParcels.push_back({.preparedModel = preparedModel});
    }

    DeviceBuffer buffer;
    const auto ret = kDevice->allocate(aidlDesc, aidlPreparedModelParcels, aidlInputRoles,
                                       aidlOutputRoles, &buffer);
    HANDLE_ASTATUS(ret) << "IDevice::allocate failed";

    if (buffer.token < 0) {
        return NN_ERROR() << "IDevice::allocate returned negative token";
    }

    return Buffer::create(buffer.buffer, static_cast<nn::Request::MemoryDomainToken>(buffer.token));
}

DeathMonitor* Device::getDeathMonitor() const {
    return kDeathHandler.getDeathMonitor().get();
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
