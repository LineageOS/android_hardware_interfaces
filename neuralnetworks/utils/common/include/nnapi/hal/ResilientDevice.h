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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_DEVICE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_DEVICE_H

#include <android-base/thread_annotations.h>
#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

class ResilientDevice final : public nn::IDevice,
                              public std::enable_shared_from_this<ResilientDevice> {
    struct PrivateConstructorTag {};

  public:
    using Factory = std::function<nn::GeneralResult<nn::SharedDevice>(bool blocking)>;

    static nn::GeneralResult<std::shared_ptr<const ResilientDevice>> create(Factory makeDevice);

    explicit ResilientDevice(PrivateConstructorTag tag, Factory makeDevice, std::string name,
                             std::string versionString, std::vector<nn::Extension> extensions,
                             nn::Capabilities capabilities, nn::SharedDevice device);

    nn::SharedDevice getDevice() const EXCLUDES(mMutex);
    nn::GeneralResult<nn::SharedDevice> recover(const nn::IDevice* failingDevice,
                                                bool blocking) const EXCLUDES(mMutex);

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
    bool isValidInternal() const EXCLUDES(mMutex);
    nn::GeneralResult<nn::SharedPreparedModel> prepareModelInternal(
            const nn::Model& model, nn::ExecutionPreference preference, nn::Priority priority,
            nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
            const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const;
    nn::GeneralResult<nn::SharedPreparedModel> prepareModelFromCacheInternal(
            nn::OptionalTimePoint deadline, const std::vector<nn::SharedHandle>& modelCache,
            const std::vector<nn::SharedHandle>& dataCache, const nn::CacheToken& token) const;
    nn::GeneralResult<nn::SharedBuffer> allocateInternal(
            const nn::BufferDesc& desc, const std::vector<nn::SharedPreparedModel>& preparedModels,
            const std::vector<nn::BufferRole>& inputRoles,
            const std::vector<nn::BufferRole>& outputRoles) const;

    const Factory kMakeDevice;
    const std::string kName;
    const std::string kVersionString;
    const std::vector<nn::Extension> kExtensions;
    const nn::Capabilities kCapabilities;
    mutable std::mutex mMutex;
    mutable nn::SharedDevice mDevice GUARDED_BY(mMutex);
    mutable bool mIsValid GUARDED_BY(mMutex) = true;
};

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_DEVICE_H
