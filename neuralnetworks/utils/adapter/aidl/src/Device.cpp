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

#include "Adapter.h"
#include "Buffer.h"
#include "PreparedModel.h"

#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <aidl/android/hardware/neuralnetworks/BufferDesc.h>
#include <aidl/android/hardware/neuralnetworks/BufferRole.h>
#include <aidl/android/hardware/neuralnetworks/DeviceBuffer.h>
#include <aidl/android/hardware/neuralnetworks/DeviceType.h>
#include <aidl/android/hardware/neuralnetworks/ErrorStatus.h>
#include <aidl/android/hardware/neuralnetworks/ExecutionPreference.h>
#include <aidl/android/hardware/neuralnetworks/Extension.h>
#include <aidl/android/hardware/neuralnetworks/IPreparedModelCallback.h>
#include <aidl/android/hardware/neuralnetworks/IPreparedModelParcel.h>
#include <aidl/android/hardware/neuralnetworks/Model.h>
#include <aidl/android/hardware/neuralnetworks/NumberOfCacheFiles.h>
#include <aidl/android/hardware/neuralnetworks/Priority.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <nnapi/IDevice.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/aidl/Conversions.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace aidl::android::hardware::neuralnetworks::adapter {
namespace {

template <typename Type>
auto convertInput(const Type& object) -> decltype(nn::convert(std::declval<Type>())) {
    auto result = nn::convert(object);
    if (!result.has_value()) {
        result.error().code = nn::ErrorStatus::INVALID_ARGUMENT;
    }
    return result;
}

nn::Duration makeDuration(int64_t durationNs) {
    return nn::Duration(std::chrono::nanoseconds(durationNs));
}

nn::GeneralResult<nn::OptionalTimePoint> makeOptionalTimePoint(int64_t durationNs) {
    if (durationNs < -1) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid time point " << durationNs;
    }
    return durationNs < 0 ? nn::OptionalTimePoint{} : nn::TimePoint(makeDuration(durationNs));
}

nn::GeneralResult<nn::CacheToken> convertCacheToken(const std::vector<uint8_t>& token) {
    nn::CacheToken nnToken;
    if (token.size() != nnToken.size()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid token";
    }
    std::copy(token.begin(), token.end(), nnToken.begin());
    return nnToken;
}

nn::GeneralResult<nn::SharedPreparedModel> downcast(const IPreparedModelParcel& preparedModel) {
    if (preparedModel.preparedModel == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "preparedModel is nullptr";
    }
    if (preparedModel.preparedModel->isRemote()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Cannot convert remote models";
    }

    // This static_cast is safe because adapter::PreparedModel is the only class that implements
    // the IPreparedModel interface in the adapter service code.
    const auto* casted = static_cast<const PreparedModel*>(preparedModel.preparedModel.get());
    return casted->getUnderlyingPreparedModel();
}

nn::GeneralResult<std::vector<nn::SharedPreparedModel>> downcastAll(
        const std::vector<IPreparedModelParcel>& preparedModels) {
    std::vector<nn::SharedPreparedModel> canonical;
    canonical.reserve(preparedModels.size());
    for (const auto& preparedModel : preparedModels) {
        canonical.push_back(NN_TRY(downcast(preparedModel)));
    }
    return canonical;
}

nn::GeneralResult<DeviceBuffer> allocate(const nn::IDevice& device, const BufferDesc& desc,
                                         const std::vector<IPreparedModelParcel>& preparedModels,
                                         const std::vector<BufferRole>& inputRoles,
                                         const std::vector<BufferRole>& outputRoles) {
    auto nnDesc = NN_TRY(convertInput(desc));
    auto nnPreparedModels = NN_TRY(downcastAll(preparedModels));
    auto nnInputRoles = NN_TRY(convertInput(inputRoles));
    auto nnOutputRoles = NN_TRY(convertInput(outputRoles));

    auto buffer = NN_TRY(device.allocate(nnDesc, nnPreparedModels, nnInputRoles, nnOutputRoles));
    CHECK(buffer != nullptr);

    const nn::Request::MemoryDomainToken token = buffer->getToken();
    auto aidlBuffer = ndk::SharedRefBase::make<Buffer>(std::move(buffer));
    return DeviceBuffer{.buffer = std::move(aidlBuffer), .token = static_cast<int32_t>(token)};
}

nn::GeneralResult<std::vector<bool>> getSupportedOperations(const nn::IDevice& device,
                                                            const Model& model) {
    const auto nnModel = NN_TRY(convertInput(model));
    return device.getSupportedOperations(nnModel);
}

using PrepareModelResult = nn::GeneralResult<nn::SharedPreparedModel>;

std::shared_ptr<PreparedModel> adaptPreparedModel(nn::SharedPreparedModel preparedModel) {
    if (preparedModel == nullptr) {
        return nullptr;
    }
    return ndk::SharedRefBase::make<PreparedModel>(std::move(preparedModel));
}

void notify(IPreparedModelCallback* callback, ErrorStatus status,
            const std::shared_ptr<IPreparedModel>& preparedModel) {
    if (callback != nullptr) {
        const auto ret = callback->notify(status, preparedModel);
        if (!ret.isOk()) {
            LOG(ERROR) << "IPreparedModelCallback::notify failed with " << ret.getDescription();
        }
    }
}

void notify(IPreparedModelCallback* callback, PrepareModelResult result) {
    if (!result.has_value()) {
        const auto& [message, status] = result.error();
        LOG(ERROR) << message;
        const auto aidlCode = utils::convert(status).value_or(ErrorStatus::GENERAL_FAILURE);
        notify(callback, aidlCode, nullptr);
    } else {
        auto preparedModel = std::move(result).value();
        auto aidlPreparedModel = adaptPreparedModel(std::move(preparedModel));
        notify(callback, ErrorStatus::NONE, std::move(aidlPreparedModel));
    }
}

nn::GeneralResult<void> prepareModel(
        const nn::SharedDevice& device, const Executor& executor, const Model& model,
        ExecutionPreference preference, Priority priority, int64_t deadlineNs,
        const std::vector<ndk::ScopedFileDescriptor>& modelCache,
        const std::vector<ndk::ScopedFileDescriptor>& dataCache, const std::vector<uint8_t>& token,
        const std::vector<TokenValuePair>& hints,
        const std::vector<ExtensionNameAndPrefix>& extensionNameToPrefix,
        const std::shared_ptr<IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    auto nnModel = NN_TRY(convertInput(model));
    const auto nnPreference = NN_TRY(convertInput(preference));
    const auto nnPriority = NN_TRY(convertInput(priority));
    const auto nnDeadline = NN_TRY(makeOptionalTimePoint(deadlineNs));
    auto nnModelCache = NN_TRY(convertInput(modelCache));
    auto nnDataCache = NN_TRY(convertInput(dataCache));
    const auto nnToken = NN_TRY(convertCacheToken(token));
    auto nnHints = NN_TRY(convertInput(hints));
    auto nnExtensionNameToPrefix = NN_TRY(convertInput(extensionNameToPrefix));

    Task task = [device, nnModel = std::move(nnModel), nnPreference, nnPriority, nnDeadline,
                 nnModelCache = std::move(nnModelCache), nnDataCache = std::move(nnDataCache),
                 nnToken, nnHints = std::move(nnHints),
                 nnExtensionNameToPrefix = std::move(nnExtensionNameToPrefix), callback] {
        auto result =
                device->prepareModel(nnModel, nnPreference, nnPriority, nnDeadline, nnModelCache,
                                     nnDataCache, nnToken, nnHints, nnExtensionNameToPrefix);
        notify(callback.get(), std::move(result));
    };
    executor(std::move(task), nnDeadline);

    return {};
}

nn::GeneralResult<void> prepareModelFromCache(
        const nn::SharedDevice& device, const Executor& executor, int64_t deadlineNs,
        const std::vector<ndk::ScopedFileDescriptor>& modelCache,
        const std::vector<ndk::ScopedFileDescriptor>& dataCache, const std::vector<uint8_t>& token,
        const std::shared_ptr<IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    const auto nnDeadline = NN_TRY(makeOptionalTimePoint(deadlineNs));
    auto nnModelCache = NN_TRY(convertInput(modelCache));
    auto nnDataCache = NN_TRY(convertInput(dataCache));
    const auto nnToken = NN_TRY(convertCacheToken(token));

    auto task = [device, nnDeadline, nnModelCache = std::move(nnModelCache),
                 nnDataCache = std::move(nnDataCache), nnToken, callback] {
        auto result = device->prepareModelFromCache(nnDeadline, nnModelCache, nnDataCache, nnToken);
        notify(callback.get(), std::move(result));
    };
    executor(std::move(task), nnDeadline);

    return {};
}

}  // namespace

Device::Device(::android::nn::SharedDevice device, Executor executor)
    : kDevice(std::move(device)), kExecutor(std::move(executor)) {
    CHECK(kDevice != nullptr);
    CHECK(kExecutor != nullptr);
}

ndk::ScopedAStatus Device::allocate(const BufferDesc& desc,
                                    const std::vector<IPreparedModelParcel>& preparedModels,
                                    const std::vector<BufferRole>& inputRoles,
                                    const std::vector<BufferRole>& outputRoles,
                                    DeviceBuffer* buffer) {
    auto result = adapter::allocate(*kDevice, desc, preparedModels, inputRoles, outputRoles);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *buffer = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::getCapabilities(Capabilities* capabilities) {
    *capabilities = utils::convert(kDevice->getCapabilities()).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::getNumberOfCacheFilesNeeded(NumberOfCacheFiles* numberOfCacheFiles) {
    const auto [numModelCache, numDataCache] = kDevice->getNumberOfCacheFilesNeeded();
    *numberOfCacheFiles = NumberOfCacheFiles{.numModelCache = static_cast<int32_t>(numModelCache),
                                             .numDataCache = static_cast<int32_t>(numDataCache)};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::getSupportedExtensions(std::vector<Extension>* extensions) {
    *extensions = utils::convert(kDevice->getSupportedExtensions()).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::getSupportedOperations(const Model& model,
                                                  std::vector<bool>* supported) {
    auto result = adapter::getSupportedOperations(*kDevice, model);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *supported = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::getType(DeviceType* deviceType) {
    *deviceType = utils::convert(kDevice->getType()).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::getVersionString(std::string* version) {
    *version = kDevice->getVersionString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::prepareModel(const Model& model, ExecutionPreference preference,
                                        Priority priority, int64_t deadlineNs,
                                        const std::vector<ndk::ScopedFileDescriptor>& modelCache,
                                        const std::vector<ndk::ScopedFileDescriptor>& dataCache,
                                        const std::vector<uint8_t>& token,
                                        const std::shared_ptr<IPreparedModelCallback>& callback) {
    const auto result =
            adapter::prepareModel(kDevice, kExecutor, model, preference, priority, deadlineNs,
                                  modelCache, dataCache, token, {}, {}, callback);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        notify(callback.get(), aidlCode, nullptr);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::prepareModelFromCache(
        int64_t deadlineNs, const std::vector<ndk::ScopedFileDescriptor>& modelCache,
        const std::vector<ndk::ScopedFileDescriptor>& dataCache, const std::vector<uint8_t>& token,
        const std::shared_ptr<IPreparedModelCallback>& callback) {
    const auto result = adapter::prepareModelFromCache(kDevice, kExecutor, deadlineNs, modelCache,
                                                       dataCache, token, callback);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        notify(callback.get(), aidlCode, nullptr);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Device::prepareModelWithConfig(
        const Model& model, const PrepareModelConfig& config,
        const std::shared_ptr<IPreparedModelCallback>& callback) {
    const auto result = adapter::prepareModel(
            kDevice, kExecutor, model, config.preference, config.priority, config.deadlineNs,
            config.modelCache, config.dataCache, utils::toVec(config.cacheToken),
            config.compilationHints, config.extensionNameToPrefix, callback);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        notify(callback.get(), aidlCode, nullptr);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::neuralnetworks::adapter
