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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_DEVICE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_DEVICE_H

#include "nnapi/hal/Adapter.h"

#include <android/hardware/neuralnetworks/1.0/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IDevice.h>
#include <nnapi/Types.h>
#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::adapter {

using CacheToken = hidl_array<uint8_t, nn::kByteSizeOfCacheToken>;

// Class that adapts nn::IDevice to V1_3::IDevice.
class Device final : public V1_3::IDevice {
  public:
    Device(nn::SharedDevice device, Executor executor);

    Return<void> getCapabilities(getCapabilities_cb cb) override;
    Return<void> getCapabilities_1_1(getCapabilities_1_1_cb cb) override;
    Return<void> getCapabilities_1_2(getCapabilities_1_2_cb cb) override;
    Return<void> getCapabilities_1_3(getCapabilities_1_3_cb cb) override;
    Return<void> getVersionString(getVersionString_cb cb) override;
    Return<void> getType(getType_cb cb) override;
    Return<void> getSupportedExtensions(getSupportedExtensions_cb) override;
    Return<void> getSupportedOperations(const V1_0::Model& model,
                                        getSupportedOperations_cb cb) override;
    Return<void> getSupportedOperations_1_1(const V1_1::Model& model,
                                            getSupportedOperations_1_1_cb cb) override;
    Return<void> getSupportedOperations_1_2(const V1_2::Model& model,
                                            getSupportedOperations_1_2_cb cb) override;
    Return<void> getSupportedOperations_1_3(const V1_3::Model& model,
                                            getSupportedOperations_1_3_cb cb) override;
    Return<void> getNumberOfCacheFilesNeeded(getNumberOfCacheFilesNeeded_cb cb) override;
    Return<V1_0::ErrorStatus> prepareModel(
            const V1_0::Model& model, const sp<V1_0::IPreparedModelCallback>& callback) override;
    Return<V1_0::ErrorStatus> prepareModel_1_1(
            const V1_1::Model& model, V1_1::ExecutionPreference preference,
            const sp<V1_0::IPreparedModelCallback>& callback) override;
    Return<V1_0::ErrorStatus> prepareModel_1_2(
            const V1_2::Model& model, V1_1::ExecutionPreference preference,
            const hidl_vec<hidl_handle>& modelCache, const hidl_vec<hidl_handle>& dataCache,
            const CacheToken& token, const sp<V1_2::IPreparedModelCallback>& callback) override;
    Return<V1_3::ErrorStatus> prepareModel_1_3(
            const V1_3::Model& model, V1_1::ExecutionPreference preference, V1_3::Priority priority,
            const V1_3::OptionalTimePoint& deadline, const hidl_vec<hidl_handle>& modelCache,
            const hidl_vec<hidl_handle>& dataCache, const CacheToken& token,
            const sp<V1_3::IPreparedModelCallback>& callback) override;
    Return<V1_0::ErrorStatus> prepareModelFromCache(
            const hidl_vec<hidl_handle>& modelCache, const hidl_vec<hidl_handle>& dataCache,
            const CacheToken& token, const sp<V1_2::IPreparedModelCallback>& callback) override;
    Return<V1_3::ErrorStatus> prepareModelFromCache_1_3(
            const V1_3::OptionalTimePoint& deadline, const hidl_vec<hidl_handle>& modelCache,
            const hidl_vec<hidl_handle>& dataCache, const CacheToken& token,
            const sp<V1_3::IPreparedModelCallback>& callback) override;
    Return<V1_0::DeviceStatus> getStatus() override;
    Return<void> allocate(const V1_3::BufferDesc& desc,
                          const hidl_vec<sp<V1_3::IPreparedModel>>& preparedModels,
                          const hidl_vec<V1_3::BufferRole>& inputRoles,
                          const hidl_vec<V1_3::BufferRole>& outputRoles, allocate_cb cb) override;

  private:
    const nn::SharedDevice kDevice;
    const Executor kExecutor;
};

}  // namespace android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_DEVICE_H
