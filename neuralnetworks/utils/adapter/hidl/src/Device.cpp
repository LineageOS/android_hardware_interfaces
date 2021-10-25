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
#include "PreparedModel.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <hwbinder/IPCThreadState.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/1.0/Utils.h>
#include <nnapi/hal/1.1/Conversions.h>
#include <nnapi/hal/1.1/Utils.h>
#include <nnapi/hal/1.2/Conversions.h>
#include <nnapi/hal/1.2/Utils.h>
#include <nnapi/hal/1.3/Conversions.h>
#include <nnapi/hal/1.3/Utils.h>
#include <sys/types.h>

#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::adapter {
namespace {

template <typename Type>
auto convertInput(const Type& object) -> decltype(nn::convert(std::declval<Type>())) {
    auto result = nn::convert(object);
    if (!result.has_value()) {
        result.error().code = nn::ErrorStatus::INVALID_ARGUMENT;
    }
    return result;
}

using PrepareModelResult = nn::GeneralResult<nn::SharedPreparedModel>;

sp<PreparedModel> adaptPreparedModel(nn::SharedPreparedModel preparedModel, Executor executor,
                                     uid_t userId) {
    if (preparedModel == nullptr) {
        return nullptr;
    }
    return sp<PreparedModel>::make(std::move(preparedModel), std::move(executor), userId);
}

void notify(V1_0::IPreparedModelCallback* callback, nn::ErrorStatus status,
            const sp<PreparedModel>& hidlPreparedModel) {
    if (callback != nullptr) {
        const auto hidlStatus = V1_0::utils::convert(status).value();
        const auto ret = callback->notify(hidlStatus, hidlPreparedModel);
        if (!ret.isOk()) {
            LOG(ERROR) << "V1_0::IPreparedModelCallback::notify failed with " << ret.description();
        }
    }
}

void notify(V1_2::IPreparedModelCallback* callback, nn::ErrorStatus status,
            const sp<PreparedModel>& hidlPreparedModel) {
    if (callback != nullptr) {
        const auto hidlStatus = V1_2::utils::convert(status).value();
        const auto ret = callback->notify_1_2(hidlStatus, hidlPreparedModel);
        if (!ret.isOk()) {
            LOG(ERROR) << "V1_2::IPreparedModelCallback::notify_1_2 failed with "
                       << ret.description();
        }
    }
}

void notify(V1_3::IPreparedModelCallback* callback, nn::ErrorStatus status,
            const sp<PreparedModel>& hidlPreparedModel) {
    if (callback != nullptr) {
        const auto hidlStatus = V1_3::utils::convert(status).value();
        const auto ret = callback->notify_1_3(hidlStatus, hidlPreparedModel);
        if (!ret.isOk()) {
            LOG(ERROR) << "V1_3::IPreparedModelCallback::notify_1_3 failed with "
                       << ret.description();
        }
    }
}

template <typename CallbackType>
void notify(CallbackType* callback, PrepareModelResult result, Executor executor, uid_t userId) {
    if (!result.has_value()) {
        const auto [message, status] = std::move(result).error();
        LOG(ERROR) << message;
        notify(callback, status, nullptr);
    } else {
        auto preparedModel = std::move(result).value();
        auto hidlPreparedModel =
                adaptPreparedModel(std::move(preparedModel), std::move(executor), userId);
        notify(callback, nn::ErrorStatus::NONE, std::move(hidlPreparedModel));
    }
}

template <typename ModelType>
nn::GeneralResult<hidl_vec<bool>> getSupportedOperations(const nn::SharedDevice& device,
                                                         const ModelType& model) {
    const auto nnModel = NN_TRY(convertInput(model));
    return NN_TRY(device->getSupportedOperations(nnModel));
}

nn::GeneralResult<void> prepareModel(const nn::SharedDevice& device, const Executor& executor,
                                     const V1_0::Model& model,
                                     const sp<V1_0::IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    auto nnModel = NN_TRY(convertInput(model));

    const uid_t userId = hardware::IPCThreadState::self()->getCallingUid();
    Task task = [device, nnModel = std::move(nnModel), userId, executor, callback] {
        auto result = device->prepareModel(nnModel, nn::ExecutionPreference::DEFAULT,
                                           nn::Priority::DEFAULT, {}, {}, {}, {});
        notify(callback.get(), std::move(result), executor, userId);
    };
    executor(std::move(task), userId, {});

    return {};
}

nn::GeneralResult<void> prepareModel_1_1(const nn::SharedDevice& device, const Executor& executor,
                                         const V1_1::Model& model,
                                         V1_1::ExecutionPreference preference,
                                         const sp<V1_0::IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    auto nnModel = NN_TRY(convertInput(model));
    const auto nnPreference = NN_TRY(convertInput(preference));

    const uid_t userId = hardware::IPCThreadState::self()->getCallingUid();
    Task task = [device, nnModel = std::move(nnModel), nnPreference, userId, executor, callback] {
        auto result =
                device->prepareModel(nnModel, nnPreference, nn::Priority::DEFAULT, {}, {}, {}, {});
        notify(callback.get(), std::move(result), executor, userId);
    };
    executor(std::move(task), userId, {});

    return {};
}

nn::GeneralResult<void> prepareModel_1_2(const nn::SharedDevice& device, const Executor& executor,
                                         const V1_2::Model& model,
                                         V1_1::ExecutionPreference preference,
                                         const hidl_vec<hidl_handle>& modelCache,
                                         const hidl_vec<hidl_handle>& dataCache,
                                         const CacheToken& token,
                                         const sp<V1_2::IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    auto nnModel = NN_TRY(convertInput(model));
    const auto nnPreference = NN_TRY(convertInput(preference));
    auto nnModelCache = NN_TRY(convertInput(modelCache));
    auto nnDataCache = NN_TRY(convertInput(dataCache));
    const auto nnToken = nn::CacheToken(token);

    const uid_t userId = hardware::IPCThreadState::self()->getCallingUid();
    Task task = [device, nnModel = std::move(nnModel), nnPreference,
                 nnModelCache = std::move(nnModelCache), nnDataCache = std::move(nnDataCache),
                 nnToken, userId, executor, callback] {
        auto result = device->prepareModel(nnModel, nnPreference, nn::Priority::DEFAULT, {},
                                           nnModelCache, nnDataCache, nnToken);
        notify(callback.get(), std::move(result), executor, userId);
    };
    executor(std::move(task), userId, {});

    return {};
}

nn::GeneralResult<void> prepareModel_1_3(
        const nn::SharedDevice& device, const Executor& executor, const V1_3::Model& model,
        V1_1::ExecutionPreference preference, V1_3::Priority priority,
        const V1_3::OptionalTimePoint& deadline, const hidl_vec<hidl_handle>& modelCache,
        const hidl_vec<hidl_handle>& dataCache, const CacheToken& token,
        const sp<V1_3::IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    auto nnModel = NN_TRY(convertInput(model));
    const auto nnPreference = NN_TRY(convertInput(preference));
    const auto nnPriority = NN_TRY(convertInput(priority));
    const auto nnDeadline = NN_TRY(convertInput(deadline));
    auto nnModelCache = NN_TRY(convertInput(modelCache));
    auto nnDataCache = NN_TRY(convertInput(dataCache));
    const auto nnToken = nn::CacheToken(token);

    const uid_t userId = hardware::IPCThreadState::self()->getCallingUid();
    Task task = [device, nnModel = std::move(nnModel), nnPreference, nnPriority, nnDeadline,
                 nnModelCache = std::move(nnModelCache), nnDataCache = std::move(nnDataCache),
                 nnToken, userId, executor, callback] {
        auto result = device->prepareModel(nnModel, nnPreference, nnPriority, nnDeadline,
                                           nnModelCache, nnDataCache, nnToken);
        notify(callback.get(), std::move(result), executor, userId);
    };
    executor(std::move(task), userId, nnDeadline);

    return {};
}

nn::GeneralResult<void> prepareModelFromCache(const nn::SharedDevice& device,
                                              const Executor& executor,
                                              const hidl_vec<hidl_handle>& modelCache,
                                              const hidl_vec<hidl_handle>& dataCache,
                                              const CacheToken& token,
                                              const sp<V1_2::IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    auto nnModelCache = NN_TRY(convertInput(modelCache));
    auto nnDataCache = NN_TRY(convertInput(dataCache));
    const auto nnToken = nn::CacheToken(token);

    const uid_t userId = hardware::IPCThreadState::self()->getCallingUid();
    Task task = [device, nnModelCache = std::move(nnModelCache),
                 nnDataCache = std::move(nnDataCache), nnToken, userId, executor, callback] {
        auto result = device->prepareModelFromCache({}, nnModelCache, nnDataCache, nnToken);
        notify(callback.get(), std::move(result), executor, userId);
    };
    executor(std::move(task), userId, {});

    return {};
}

nn::GeneralResult<void> prepareModelFromCache_1_3(
        const nn::SharedDevice& device, const Executor& executor,
        const V1_3::OptionalTimePoint& deadline, const hidl_vec<hidl_handle>& modelCache,
        const hidl_vec<hidl_handle>& dataCache, const CacheToken& token,
        const sp<V1_3::IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    const auto nnDeadline = NN_TRY(convertInput(deadline));
    auto nnModelCache = NN_TRY(convertInput(modelCache));
    auto nnDataCache = NN_TRY(convertInput(dataCache));
    const auto nnToken = nn::CacheToken(token);

    const uid_t userId = hardware::IPCThreadState::self()->getCallingUid();
    auto task = [device, nnDeadline, nnModelCache = std::move(nnModelCache),
                 nnDataCache = std::move(nnDataCache), nnToken, userId, executor, callback] {
        auto result = device->prepareModelFromCache(nnDeadline, nnModelCache, nnDataCache, nnToken);
        notify(callback.get(), std::move(result), executor, userId);
    };
    executor(std::move(task), userId, nnDeadline);

    return {};
}

nn::GeneralResult<nn::SharedPreparedModel> downcast(const sp<V1_3::IPreparedModel>& preparedModel) {
    if (preparedModel == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "preparedModel is nullptr";
    }
    if (preparedModel->isRemote()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Cannot convert remote models";
    }

    // This static_cast is safe because adapter::PreparedModel is the only class that implements
    // the IPreparedModel interface in the adapter service code.
    const auto* casted = static_cast<const PreparedModel*>(preparedModel.get());
    return casted->getUnderlyingPreparedModel();
}

nn::GeneralResult<std::vector<nn::SharedPreparedModel>> downcastAll(
        const hidl_vec<sp<V1_3::IPreparedModel>>& preparedModels) {
    std::vector<nn::SharedPreparedModel> canonical;
    canonical.reserve(preparedModels.size());
    for (const auto& preparedModel : preparedModels) {
        canonical.push_back(NN_TRY(downcast(preparedModel)));
    }
    return canonical;
}

nn::GeneralResult<std::pair<sp<V1_3::IBuffer>, uint32_t>> allocate(
        const nn::SharedDevice& device, const V1_3::BufferDesc& desc,
        const hidl_vec<sp<V1_3::IPreparedModel>>& preparedModels,
        const hidl_vec<V1_3::BufferRole>& inputRoles,
        const hidl_vec<V1_3::BufferRole>& outputRoles) {
    auto nnDesc = NN_TRY(convertInput(desc));
    auto nnPreparedModels = NN_TRY(downcastAll(preparedModels));
    auto nnInputRoles = NN_TRY(convertInput(inputRoles));
    auto nnOutputRoles = NN_TRY(convertInput(outputRoles));

    auto buffer = NN_TRY(device->allocate(nnDesc, nnPreparedModels, nnInputRoles, nnOutputRoles));

    const nn::Request::MemoryDomainToken token = buffer->getToken();
    auto hidlBuffer = sp<Buffer>::make(std::move(buffer));
    return std::make_pair(std::move(hidlBuffer), static_cast<uint32_t>(token));
}

}  // namespace

Device::Device(nn::SharedDevice device, Executor executor)
    : kDevice(std::move(device)), kExecutor(std::move(executor)) {
    CHECK(kDevice != nullptr);
    CHECK(kExecutor != nullptr);
}

Return<void> Device::getCapabilities(getCapabilities_cb cb) {
    const auto capabilities = V1_0::utils::convert(kDevice->getCapabilities()).value();
    cb(V1_0::ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> Device::getCapabilities_1_1(getCapabilities_1_1_cb cb) {
    const auto capabilities = V1_1::utils::convert(kDevice->getCapabilities()).value();
    cb(V1_0::ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> Device::getCapabilities_1_2(getCapabilities_1_2_cb cb) {
    const auto capabilities = V1_2::utils::convert(kDevice->getCapabilities()).value();
    cb(V1_0::ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> Device::getCapabilities_1_3(getCapabilities_1_3_cb cb) {
    const auto capabilities = V1_3::utils::convert(kDevice->getCapabilities()).value();
    cb(V1_3::ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> Device::getVersionString(getVersionString_cb cb) {
    cb(V1_0::ErrorStatus::NONE, kDevice->getVersionString());
    return Void();
}

Return<void> Device::getType(getType_cb cb) {
    const auto maybeDeviceType = V1_2::utils::convert(kDevice->getType());
    if (!maybeDeviceType.has_value()) {
        const auto& [message, code] = maybeDeviceType.error();
        LOG(ERROR) << "adapter::Device::getType failed with " << code << ": " << message;
        cb(V1_2::utils::convert(code).value(), {});
    } else {
        cb(V1_0::ErrorStatus::NONE, maybeDeviceType.value());
    }
    return Void();
}

Return<void> Device::getSupportedExtensions(getSupportedExtensions_cb cb) {
    const auto maybeSupportedExtensions = V1_2::utils::convert(kDevice->getSupportedExtensions());
    if (!maybeSupportedExtensions.has_value()) {
        const auto& [message, code] = maybeSupportedExtensions.error();
        LOG(ERROR) << "adapter::Device::getSupportedExtensions failed with " << code << ": "
                   << message;
        cb(V1_2::utils::convert(code).value(), {});
    } else {
        cb(V1_0::ErrorStatus::NONE, maybeSupportedExtensions.value());
    }
    return Void();
}

Return<void> Device::getSupportedOperations(const V1_0::Model& model,
                                            getSupportedOperations_cb cb) {
    const auto result = adapter::getSupportedOperations(kDevice, model);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        LOG(ERROR) << "adapter::Device::getSupportedOperations_1_0 failed with " << code << ": "
                   << message;
        cb(V1_0::utils::convert(code).value(), {});
    } else {
        cb(V1_0::ErrorStatus::NONE, result.value());
    }
    return Void();
}

Return<void> Device::getSupportedOperations_1_1(const V1_1::Model& model,
                                                getSupportedOperations_1_1_cb cb) {
    const auto result = adapter::getSupportedOperations(kDevice, model);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        LOG(ERROR) << "adapter::Device::getSupportedOperations_1_1 failed with " << code << ": "
                   << message;
        cb(V1_1::utils::convert(code).value(), {});
    } else {
        cb(V1_0::ErrorStatus::NONE, result.value());
    }
    return Void();
}

Return<void> Device::getSupportedOperations_1_2(const V1_2::Model& model,
                                                getSupportedOperations_1_2_cb cb) {
    const auto result = adapter::getSupportedOperations(kDevice, model);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        LOG(ERROR) << "adapter::Device::getSupportedOperations_1_2 failed with " << code << ": "
                   << message;
        cb(V1_2::utils::convert(code).value(), {});
    } else {
        cb(V1_0::ErrorStatus::NONE, result.value());
    }
    return Void();
}

Return<void> Device::getSupportedOperations_1_3(const V1_3::Model& model,
                                                getSupportedOperations_1_3_cb cb) {
    const auto result = adapter::getSupportedOperations(kDevice, model);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        LOG(ERROR) << "adapter::Device::getSupportedOperations_1_3 failed with " << code << ": "
                   << message;
        cb(V1_3::utils::convert(code).value(), {});
    } else {
        cb(V1_3::ErrorStatus::NONE, result.value());
    }
    return Void();
}

Return<void> Device::getNumberOfCacheFilesNeeded(getNumberOfCacheFilesNeeded_cb cb) {
    const auto [numModelCache, numDataCache] = kDevice->getNumberOfCacheFilesNeeded();
    cb(V1_0::ErrorStatus::NONE, numModelCache, numDataCache);
    return Void();
}

Return<V1_0::ErrorStatus> Device::prepareModel(const V1_0::Model& model,
                                               const sp<V1_0::IPreparedModelCallback>& callback) {
    auto result = adapter::prepareModel(kDevice, kExecutor, model, callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Device::prepareModel failed with " << code << ": " << message;
        notify(callback.get(), code, nullptr);
        return V1_0::utils::convert(code).value();
    }
    return V1_0::ErrorStatus::NONE;
}

Return<V1_0::ErrorStatus> Device::prepareModel_1_1(
        const V1_1::Model& model, V1_1::ExecutionPreference preference,
        const sp<V1_0::IPreparedModelCallback>& callback) {
    auto result = adapter::prepareModel_1_1(kDevice, kExecutor, model, preference, callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Device::prepareModel_1_1 failed with " << code << ": " << message;
        notify(callback.get(), code, nullptr);
        return V1_1::utils::convert(code).value();
    }
    return V1_0::ErrorStatus::NONE;
}

Return<V1_0::ErrorStatus> Device::prepareModel_1_2(
        const V1_2::Model& model, V1_1::ExecutionPreference preference,
        const hidl_vec<hidl_handle>& modelCache, const hidl_vec<hidl_handle>& dataCache,
        const CacheToken& token, const sp<V1_2::IPreparedModelCallback>& callback) {
    auto result = adapter::prepareModel_1_2(kDevice, kExecutor, model, preference, modelCache,
                                            dataCache, token, callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Device::prepareModel_1_2 failed with " << code << ": " << message;
        notify(callback.get(), code, nullptr);
        return V1_2::utils::convert(code).value();
    }
    return V1_0::ErrorStatus::NONE;
}

Return<V1_3::ErrorStatus> Device::prepareModel_1_3(
        const V1_3::Model& model, V1_1::ExecutionPreference preference, V1_3::Priority priority,
        const V1_3::OptionalTimePoint& deadline, const hidl_vec<hidl_handle>& modelCache,
        const hidl_vec<hidl_handle>& dataCache, const CacheToken& token,
        const sp<V1_3::IPreparedModelCallback>& callback) {
    auto result = adapter::prepareModel_1_3(kDevice, kExecutor, model, preference, priority,
                                            deadline, modelCache, dataCache, token, callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Device::prepareModel_1_3 failed with " << code << ": " << message;
        notify(callback.get(), code, nullptr);
        return V1_3::utils::convert(code).value();
    }
    return V1_3::ErrorStatus::NONE;
}

Return<V1_0::ErrorStatus> Device::prepareModelFromCache(
        const hidl_vec<hidl_handle>& modelCache, const hidl_vec<hidl_handle>& dataCache,
        const CacheToken& token, const sp<V1_2::IPreparedModelCallback>& callback) {
    auto result = adapter::prepareModelFromCache(kDevice, kExecutor, modelCache, dataCache, token,
                                                 callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Device::prepareModelFromCache failed with " << code << ": "
                   << message;
        notify(callback.get(), code, nullptr);
        return V1_2::utils::convert(code).value();
    }
    return V1_0::ErrorStatus::NONE;
}

Return<V1_3::ErrorStatus> Device::prepareModelFromCache_1_3(
        const V1_3::OptionalTimePoint& deadline, const hidl_vec<hidl_handle>& modelCache,
        const hidl_vec<hidl_handle>& dataCache, const CacheToken& token,
        const sp<V1_3::IPreparedModelCallback>& callback) {
    auto result = adapter::prepareModelFromCache_1_3(kDevice, kExecutor, deadline, modelCache,
                                                     dataCache, token, callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Device::prepareModelFromCache_1_3 failed with " << code << ": "
                   << message;
        notify(callback.get(), code, nullptr);
        return V1_3::utils::convert(code).value();
    }
    return V1_3::ErrorStatus::NONE;
}

Return<V1_0::DeviceStatus> Device::getStatus() {
    return V1_0::DeviceStatus::AVAILABLE;
}

Return<void> Device::allocate(const V1_3::BufferDesc& desc,
                              const hidl_vec<sp<V1_3::IPreparedModel>>& preparedModels,
                              const hidl_vec<V1_3::BufferRole>& inputRoles,
                              const hidl_vec<V1_3::BufferRole>& outputRoles, allocate_cb cb) {
    auto result = adapter::allocate(kDevice, desc, preparedModels, inputRoles, outputRoles);
    if (!result.has_value()) {
        const auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Device::allocate failed with " << code << ": " << message;
        cb(V1_3::utils::convert(code).value(), nullptr, /*token=*/0);
        return Void();
    }
    auto [buffer, token] = std::move(result).value();
    cb(V1_3::ErrorStatus::NONE, buffer, token);
    return Void();
}

}  // namespace android::hardware::neuralnetworks::adapter
