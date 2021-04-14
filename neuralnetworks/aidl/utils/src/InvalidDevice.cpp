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

#define LOG_TAG "InvalidDevice"

#include "InvalidDevice.h"

#include <aidl/android/hardware/neuralnetworks/BnBuffer.h>
#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <aidl/android/hardware/neuralnetworks/BnPreparedModel.h>
#include <android/binder_auto_utils.h>

#include "Conversions.h"
#include "Utils.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace aidl::android::hardware::neuralnetworks {
namespace {

ndk::ScopedAStatus toAStatus(ErrorStatus errorStatus, const std::string& errorMessage) {
    if (errorStatus == ErrorStatus::NONE) {
        return ndk::ScopedAStatus::ok();
    }
    return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
            static_cast<int32_t>(errorStatus), errorMessage.c_str());
}

}  // namespace

std::shared_ptr<InvalidDevice> InvalidDevice::create() {
    constexpr auto perf = PerformanceInfo{
            .execTime = std::numeric_limits<float>::max(),
            .powerUsage = std::numeric_limits<float>::max(),
    };
    auto capabilities = Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = perf,
            .relaxedFloat32toFloat16PerformanceTensor = perf,
            .operandPerformance = {},
            .ifPerformance = perf,
            .whilePerformance = perf,
    };
    constexpr auto numberOfCacheFiles = NumberOfCacheFiles{
            .numModelCache = 0,
            .numDataCache = 0,
    };
    std::vector<Extension> extensions{};
    constexpr auto deviceType = DeviceType::OTHER;
    std::string versionString = "invalid";

    return ndk::SharedRefBase::make<InvalidDevice>(std::move(capabilities), numberOfCacheFiles,
                                                   std::move(extensions), deviceType,
                                                   std::move(versionString));
}

InvalidDevice::InvalidDevice(Capabilities capabilities,
                             const NumberOfCacheFiles& numberOfCacheFiles,
                             std::vector<Extension> extensions, DeviceType deviceType,
                             std::string versionString)
    : kCapabilities(std::move(capabilities)),
      kNumberOfCacheFiles(numberOfCacheFiles),
      kExtensions(std::move(extensions)),
      kDeviceType(deviceType),
      kVersionString(std::move(versionString)) {}

ndk::ScopedAStatus InvalidDevice::allocate(
        const BufferDesc& /*desc*/, const std::vector<IPreparedModelParcel>& /*preparedModels*/,
        const std::vector<BufferRole>& /*inputRoles*/,
        const std::vector<BufferRole>& /*outputRoles*/, DeviceBuffer* /*deviceBuffer*/) {
    return toAStatus(ErrorStatus::GENERAL_FAILURE, "InvalidDevice");
}

ndk::ScopedAStatus InvalidDevice::getCapabilities(Capabilities* capabilities) {
    *capabilities = kCapabilities;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus InvalidDevice::getNumberOfCacheFilesNeeded(
        NumberOfCacheFiles* numberOfCacheFiles) {
    *numberOfCacheFiles = kNumberOfCacheFiles;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus InvalidDevice::getSupportedExtensions(std::vector<Extension>* extensions) {
    *extensions = kExtensions;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus InvalidDevice::getSupportedOperations(const Model& model,
                                                         std::vector<bool>* supportedOperations) {
    if (const auto result = utils::validate(model); !result.ok()) {
        return toAStatus(ErrorStatus::INVALID_ARGUMENT, result.error());
    }
    *supportedOperations = std::vector<bool>(model.main.operations.size(), false);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus InvalidDevice::getType(DeviceType* deviceType) {
    *deviceType = kDeviceType;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus InvalidDevice::getVersionString(std::string* versionString) {
    *versionString = kVersionString;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus InvalidDevice::prepareModel(
        const Model& model, ExecutionPreference preference, Priority priority, int64_t deadline,
        const std::vector<ndk::ScopedFileDescriptor>& modelCache,
        const std::vector<ndk::ScopedFileDescriptor>& dataCache, const std::vector<uint8_t>& token,
        const std::shared_ptr<IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                         "invalid callback passed to InvalidDevice::prepareModel");
    }
    if (const auto result = utils::validate(model); !result.ok()) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT, result.error());
    }
    if (const auto result = utils::validate(preference); !result.ok()) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT, result.error());
    }
    if (const auto result = utils::validate(priority); !result.ok()) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT, result.error());
    }
    if (deadline < -1) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                         "Invalid deadline " + std::to_string(deadline));
    }
    if (modelCache.size() != static_cast<size_t>(kNumberOfCacheFiles.numModelCache)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                         "Invalid modelCache, size = " + std::to_string(modelCache.size()));
    }
    if (dataCache.size() != static_cast<size_t>(kNumberOfCacheFiles.numDataCache)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(ErrorStatus::INVALID_ARGUMENT,
                         "Invalid modelCache, size = " + std::to_string(dataCache.size()));
    }
    if (token.size() != IDevice::BYTE_SIZE_OF_CACHE_TOKEN) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return toAStatus(
                ErrorStatus::INVALID_ARGUMENT,
                "Invalid cache token, size = " + std::to_string(IDevice::BYTE_SIZE_OF_CACHE_TOKEN));
    }
    callback->notify(ErrorStatus::GENERAL_FAILURE, nullptr);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus InvalidDevice::prepareModelFromCache(
        int64_t /*deadline*/, const std::vector<ndk::ScopedFileDescriptor>& /*modelCache*/,
        const std::vector<ndk::ScopedFileDescriptor>& /*dataCache*/,
        const std::vector<uint8_t>& /*token*/,
        const std::shared_ptr<IPreparedModelCallback>& callback) {
    callback->notify(ErrorStatus::GENERAL_FAILURE, nullptr);
    return toAStatus(ErrorStatus::GENERAL_FAILURE, "InvalidDevice");
}

}  // namespace aidl::android::hardware::neuralnetworks
