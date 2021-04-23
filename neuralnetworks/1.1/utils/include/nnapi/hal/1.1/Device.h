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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_DEVICE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_DEVICE_H

#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/ProtectCallback.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_1::utils {

// Class that adapts V1_1::IDevice to nn::IDevice.
class Device final : public nn::IDevice {
    struct PrivateConstructorTag {};

  public:
    static nn::GeneralResult<std::shared_ptr<const Device>> create(std::string name,
                                                                   sp<V1_1::IDevice> device);

    Device(PrivateConstructorTag tag, std::string name, nn::Capabilities capabilities,
           sp<V1_1::IDevice> device, hal::utils::DeathHandler deathHandler);

    const std::string& getName() const override;
    const std::string& getVersionString() const override;
    nn::Version getFeatureLevel() const override;
    nn::DeviceType getType() const override;
    const std::vector<nn::Extension>& getSupportedExtensions() const override;
    const nn::Capabilities& getCapabilities() const override;
    std::pair<uint32_t, uint32_t> getNumberOfCacheFilesNeeded() const override;

    nn::GeneralResult<void> wait() const override;

    nn::GeneralResult<std::vector<bool>> getSupportedOperations(
            const nn::Model& model) const override;

    nn::GeneralResult<nn::SharedPreparedModel> prepareModel(
            const nn::Model& model, nn::ExecutionPreference preference, nn::Priority priority,
            nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
            const std::vector<nn::SharedHandle>& dataCache,
            const nn::CacheToken& token) const override;

    nn::GeneralResult<nn::SharedPreparedModel> prepareModelFromCache(
            nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
            const std::vector<nn::SharedHandle>& dataCache,
            const nn::CacheToken& token) const override;

    nn::GeneralResult<nn::SharedBuffer> allocate(
            const nn::BufferDesc& desc, const std::vector<nn::SharedPreparedModel>& preparedModels,
            const std::vector<nn::BufferRole>& inputRoles,
            const std::vector<nn::BufferRole>& outputRoles) const override;

  private:
    const std::string kName;
    const std::string kVersionString = "UNKNOWN";
    const std::vector<nn::Extension> kExtensions;
    const nn::Capabilities kCapabilities;
    const sp<V1_1::IDevice> kDevice;
    const hal::utils::DeathHandler kDeathHandler;
};

}  // namespace android::hardware::neuralnetworks::V1_1::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_DEVICE_H
