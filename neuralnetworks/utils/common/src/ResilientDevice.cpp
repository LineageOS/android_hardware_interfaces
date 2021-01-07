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

#include "ResilientDevice.h"

#include "InvalidBuffer.h"
#include "InvalidDevice.h"
#include "InvalidPreparedModel.h"
#include "ResilientBuffer.h"
#include "ResilientPreparedModel.h"

#include <android-base/logging.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace android::hardware::neuralnetworks::utils {
namespace {

template <typename FnType>
auto protect(const ResilientDevice& resilientDevice, const FnType& fn, bool blocking)
        -> decltype(fn(*resilientDevice.getDevice())) {
    auto device = resilientDevice.getDevice();
    auto result = fn(*device);

    // Immediately return if device is not dead.
    if (result.has_value() || result.error().code != nn::ErrorStatus::DEAD_OBJECT) {
        return result;
    }

    // Attempt recovery and return if it fails.
    auto maybeDevice = resilientDevice.recover(device.get(), blocking);
    if (!maybeDevice.has_value()) {
        const auto& [resultErrorMessage, resultErrorCode] = result.error();
        const auto& [recoveryErrorMessage, recoveryErrorCode] = maybeDevice.error();
        return nn::error(resultErrorCode)
               << resultErrorMessage << ", and failed to recover dead device with error "
               << recoveryErrorCode << ": " << recoveryErrorMessage;
    }
    device = std::move(maybeDevice).value();

    return fn(*device);
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const ResilientDevice>> ResilientDevice::create(
        Factory makeDevice) {
    if (makeDevice == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "utils::ResilientDevice::create must have non-empty makeDevice";
    }
    auto device = NN_TRY(makeDevice(/*blocking=*/true));
    CHECK(device != nullptr);

    auto name = device->getName();
    auto versionString = device->getVersionString();
    auto extensions = device->getSupportedExtensions();
    auto capabilities = device->getCapabilities();

    return std::make_shared<ResilientDevice>(PrivateConstructorTag{}, std::move(makeDevice),
                                             std::move(name), std::move(versionString),
                                             std::move(extensions), std::move(capabilities),
                                             std::move(device));
}

ResilientDevice::ResilientDevice(PrivateConstructorTag /*tag*/, Factory makeDevice,
                                 std::string name, std::string versionString,
                                 std::vector<nn::Extension> extensions,
                                 nn::Capabilities capabilities, nn::SharedDevice device)
    : kMakeDevice(std::move(makeDevice)),
      kName(std::move(name)),
      kVersionString(std::move(versionString)),
      kExtensions(std::move(extensions)),
      kCapabilities(std::move(capabilities)),
      mDevice(std::move(device)) {
    CHECK(kMakeDevice != nullptr);
    CHECK(mDevice != nullptr);
}

nn::SharedDevice ResilientDevice::getDevice() const {
    std::lock_guard guard(mMutex);
    return mDevice;
}

nn::GeneralResult<nn::SharedDevice> ResilientDevice::recover(const nn::IDevice* failingDevice,
                                                             bool blocking) const {
    std::lock_guard guard(mMutex);

    // Another caller updated the failing device.
    if (mDevice.get() != failingDevice) {
        return mDevice;
    }

    auto device = NN_TRY(kMakeDevice(blocking));

    // If recovered device has different metadata than what is cached (i.e., because it was
    // updated), mark the device as invalid and preserve the cached data.
    auto compare = [this, &device](auto fn) REQUIRES(mMutex) {
        return std::invoke(fn, mDevice) != std::invoke(fn, device);
    };
    if (compare(&IDevice::getName) || compare(&IDevice::getVersionString) ||
        compare(&IDevice::getFeatureLevel) || compare(&IDevice::getType) ||
        compare(&IDevice::getSupportedExtensions) || compare(&IDevice::getCapabilities)) {
        LOG(ERROR) << "Recovered device has different metadata than what is cached. Marking "
                      "IDevice object as invalid.";
        device = std::make_shared<const InvalidDevice>(
                kName, kVersionString, mDevice->getFeatureLevel(), mDevice->getType(), kExtensions,
                kCapabilities, mDevice->getNumberOfCacheFilesNeeded());
        mIsValid = false;
    }

    mDevice = std::move(device);
    return mDevice;
}

const std::string& ResilientDevice::getName() const {
    return kName;
}

const std::string& ResilientDevice::getVersionString() const {
    return kVersionString;
}

nn::Version ResilientDevice::getFeatureLevel() const {
    return getDevice()->getFeatureLevel();
}

nn::DeviceType ResilientDevice::getType() const {
    return getDevice()->getType();
}

const std::vector<nn::Extension>& ResilientDevice::getSupportedExtensions() const {
    return kExtensions;
}

const nn::Capabilities& ResilientDevice::getCapabilities() const {
    return kCapabilities;
}

std::pair<uint32_t, uint32_t> ResilientDevice::getNumberOfCacheFilesNeeded() const {
    return getDevice()->getNumberOfCacheFilesNeeded();
}

nn::GeneralResult<void> ResilientDevice::wait() const {
    const auto fn = [](const nn::IDevice& device) { return device.wait(); };
    return protect(*this, fn, /*blocking=*/true);
}

nn::GeneralResult<std::vector<bool>> ResilientDevice::getSupportedOperations(
        const nn::Model& model) const {
    const auto fn = [&model](const nn::IDevice& device) {
        return device.getSupportedOperations(model);
    };
    return protect(*this, fn, /*blocking=*/false);
}

nn::GeneralResult<nn::SharedPreparedModel> ResilientDevice::prepareModel(
        const nn::Model& model, nn::ExecutionPreference preference, nn::Priority priority,
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
#if 0
    auto self = shared_from_this();
    ResilientPreparedModel::Factory makePreparedModel = [device = std::move(self), model,
                                                         preference, priority, deadline, modelCache,
                                                         dataCache, token] {
        return device->prepareModelInternal(model, preference, priority, deadline, modelCache,
                                            dataCache, token);
    };
    return ResilientPreparedModel::create(std::move(makePreparedModel));
#else
    return prepareModelInternal(model, preference, priority, deadline, modelCache, dataCache,
                                token);
#endif
}

nn::GeneralResult<nn::SharedPreparedModel> ResilientDevice::prepareModelFromCache(
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
#if 0
    auto self = shared_from_this();
    ResilientPreparedModel::Factory makePreparedModel = [device = std::move(self), deadline,
                                                         modelCache, dataCache, token] {
        return device->prepareModelFromCacheInternal(deadline, modelCache, dataCache, token);
    };
    return ResilientPreparedModel::create(std::move(makePreparedModel));
#else
    return prepareModelFromCacheInternal(deadline, modelCache, dataCache, token);
#endif
}

nn::GeneralResult<nn::SharedBuffer> ResilientDevice::allocate(
        const nn::BufferDesc& desc, const std::vector<nn::SharedPreparedModel>& preparedModels,
        const std::vector<nn::BufferRole>& inputRoles,
        const std::vector<nn::BufferRole>& outputRoles) const {
#if 0
    auto self = shared_from_this();
    ResilientBuffer::Factory makeBuffer = [device = std::move(self), desc, preparedModels,
                                           inputRoles, outputRoles] {
        return device->allocateInternal(desc, preparedModels, inputRoles, outputRoles);
    };
    return ResilientBuffer::create(std::move(makeBuffer));
#else
    return allocateInternal(desc, preparedModels, inputRoles, outputRoles);
#endif
}

bool ResilientDevice::isValidInternal() const {
    std::lock_guard hold(mMutex);
    return mIsValid;
}

nn::GeneralResult<nn::SharedPreparedModel> ResilientDevice::prepareModelInternal(
        const nn::Model& model, nn::ExecutionPreference preference, nn::Priority priority,
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    if (!isValidInternal()) {
        return std::make_shared<const InvalidPreparedModel>();
    }
    const auto fn = [&model, preference, priority, &deadline, &modelCache, &dataCache,
                     &token](const nn::IDevice& device) {
        return device.prepareModel(model, preference, priority, deadline, modelCache, dataCache,
                                   token);
    };
    return protect(*this, fn, /*blocking=*/false);
}

nn::GeneralResult<nn::SharedPreparedModel> ResilientDevice::prepareModelFromCacheInternal(
        nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
        const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const {
    if (!isValidInternal()) {
        return std::make_shared<const InvalidPreparedModel>();
    }
    const auto fn = [&deadline, &modelCache, &dataCache, &token](const nn::IDevice& device) {
        return device.prepareModelFromCache(deadline, modelCache, dataCache, token);
    };
    return protect(*this, fn, /*blocking=*/false);
}

nn::GeneralResult<nn::SharedBuffer> ResilientDevice::allocateInternal(
        const nn::BufferDesc& desc, const std::vector<nn::SharedPreparedModel>& preparedModels,
        const std::vector<nn::BufferRole>& inputRoles,
        const std::vector<nn::BufferRole>& outputRoles) const {
    if (!isValidInternal()) {
        return std::make_shared<const InvalidBuffer>();
    }
    const auto fn = [&desc, &preparedModels, &inputRoles, &outputRoles](const nn::IDevice& device) {
        return device.allocate(desc, preparedModels, inputRoles, outputRoles);
    };
    return protect(*this, fn, /*blocking=*/false);
}

}  // namespace android::hardware::neuralnetworks::utils
