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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_DEVICE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_DEVICE_H

#include "nnapi/hal/aidl/Adapter.h"

#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <aidl/android/hardware/neuralnetworks/BufferDesc.h>
#include <aidl/android/hardware/neuralnetworks/BufferRole.h>
#include <aidl/android/hardware/neuralnetworks/Capabilities.h>
#include <aidl/android/hardware/neuralnetworks/DeviceBuffer.h>
#include <aidl/android/hardware/neuralnetworks/DeviceType.h>
#include <aidl/android/hardware/neuralnetworks/ExecutionPreference.h>
#include <aidl/android/hardware/neuralnetworks/Extension.h>
#include <aidl/android/hardware/neuralnetworks/IPreparedModelCallback.h>
#include <aidl/android/hardware/neuralnetworks/IPreparedModelParcel.h>
#include <aidl/android/hardware/neuralnetworks/Model.h>
#include <aidl/android/hardware/neuralnetworks/NumberOfCacheFiles.h>
#include <aidl/android/hardware/neuralnetworks/PrepareModelConfig.h>
#include <aidl/android/hardware/neuralnetworks/Priority.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IDevice.h>

#include <memory>
#include <string>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::adapter {

// Class that adapts nn::IDevice to BnDevice.
class Device : public BnDevice {
  public:
    Device(::android::nn::SharedDevice device, Executor executor);

    ndk::ScopedAStatus allocate(const BufferDesc& desc,
                                const std::vector<IPreparedModelParcel>& preparedModels,
                                const std::vector<BufferRole>& inputRoles,
                                const std::vector<BufferRole>& outputRoles,
                                DeviceBuffer* buffer) override;
    ndk::ScopedAStatus getCapabilities(Capabilities* capabilities) override;
    ndk::ScopedAStatus getNumberOfCacheFilesNeeded(NumberOfCacheFiles* numberOfCacheFiles) override;
    ndk::ScopedAStatus getSupportedExtensions(std::vector<Extension>* extensions) override;
    ndk::ScopedAStatus getSupportedOperations(const Model& model,
                                              std::vector<bool>* supported) override;
    ndk::ScopedAStatus getType(DeviceType* deviceType) override;
    ndk::ScopedAStatus getVersionString(std::string* version) override;
    ndk::ScopedAStatus prepareModel(
            const Model& model, ExecutionPreference preference, Priority priority,
            int64_t deadlineNs, const std::vector<ndk::ScopedFileDescriptor>& modelCache,
            const std::vector<ndk::ScopedFileDescriptor>& dataCache,
            const std::vector<uint8_t>& token,
            const std::shared_ptr<IPreparedModelCallback>& callback) override;
    ndk::ScopedAStatus prepareModelFromCache(
            int64_t deadlineNs, const std::vector<ndk::ScopedFileDescriptor>& modelCache,
            const std::vector<ndk::ScopedFileDescriptor>& dataCache,
            const std::vector<uint8_t>& token,
            const std::shared_ptr<IPreparedModelCallback>& callback) override;
    ndk::ScopedAStatus prepareModelWithConfig(
            const Model& model, const PrepareModelConfig& config,
            const std::shared_ptr<IPreparedModelCallback>& callback) override;

  protected:
    const ::android::nn::SharedDevice kDevice;
    const Executor kExecutor;
};

}  // namespace aidl::android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_DEVICE_H
