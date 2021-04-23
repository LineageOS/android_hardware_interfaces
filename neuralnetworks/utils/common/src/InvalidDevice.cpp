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

#include "InvalidDevice.h"

#include "InvalidBuffer.h"
#include "InvalidPreparedModel.h"

#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <string>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

InvalidDevice::InvalidDevice(std::string name, std::string versionString, nn::Version featureLevel,
                             nn::DeviceType type, std::vector<nn::Extension> extensions,
                             nn::Capabilities capabilities,
                             std::pair<uint32_t, uint32_t> numberOfCacheFilesNeeded)
    : kName(std::move(name)),
      kVersionString(std::move(versionString)),
      kFeatureLevel(featureLevel),
      kType(type),
      kExtensions(std::move(extensions)),
      kCapabilities(std::move(capabilities)),
      kNumberOfCacheFilesNeeded(numberOfCacheFilesNeeded) {}

const std::string& InvalidDevice::getName() const {
    return kName;
}

const std::string& InvalidDevice::getVersionString() const {
    return kVersionString;
}

nn::Version InvalidDevice::getFeatureLevel() const {
    return kFeatureLevel;
}

nn::DeviceType InvalidDevice::getType() const {
    return kType;
}

const std::vector<nn::Extension>& InvalidDevice::getSupportedExtensions() const {
    return kExtensions;
}

const nn::Capabilities& InvalidDevice::getCapabilities() const {
    return kCapabilities;
}

std::pair<uint32_t, uint32_t> InvalidDevice::getNumberOfCacheFilesNeeded() const {
    return kNumberOfCacheFilesNeeded;
}

nn::GeneralResult<void> InvalidDevice::wait() const {
    return NN_ERROR() << "InvalidDevice";
}

nn::GeneralResult<std::vector<bool>> InvalidDevice::getSupportedOperations(
        const nn::Model& /*model*/) const {
    return NN_ERROR() << "InvalidDevice";
}

nn::GeneralResult<nn::SharedPreparedModel> InvalidDevice::prepareModel(
        const nn::Model& /*model*/, nn::ExecutionPreference /*preference*/,
        nn::Priority /*priority*/, nn::OptionalTimePoint /*deadline*/,
        const std::vector<nn::SharedHandle>& /*modelCache*/,
        const std::vector<nn::SharedHandle>& /*dataCache*/, const nn::CacheToken& /*token*/) const {
    return NN_ERROR() << "InvalidDevice";
}

nn::GeneralResult<nn::SharedPreparedModel> InvalidDevice::prepareModelFromCache(
        nn::OptionalTimePoint /*deadline*/, const std::vector<nn::SharedHandle>& /*modelCache*/,
        const std::vector<nn::SharedHandle>& /*dataCache*/, const nn::CacheToken& /*token*/) const {
    return NN_ERROR() << "InvalidDevice";
}

nn::GeneralResult<nn::SharedBuffer> InvalidDevice::allocate(
        const nn::BufferDesc& /*desc*/,
        const std::vector<nn::SharedPreparedModel>& /*preparedModels*/,
        const std::vector<nn::BufferRole>& /*inputRoles*/,
        const std::vector<nn::BufferRole>& /*outputRoles*/) const {
    return NN_ERROR() << "InvalidDevice";
}

}  // namespace android::hardware::neuralnetworks::utils
