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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_COMMON_UTILS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_COMMON_UTILS_H

#include <cutils/native_handle.h>
#include <hidl/HidlSupport.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <functional>
#include <vector>

// Shorthands
namespace android::hardware::neuralnetworks {
namespace hal = ::android::hardware::neuralnetworks;
}  // namespace android::hardware::neuralnetworks

// Shorthands
namespace aidl::android::hardware::neuralnetworks {
namespace aidl_hal = ::aidl::android::hardware::neuralnetworks;
namespace hal = ::android::hardware::neuralnetworks;
namespace nn = ::android::nn;
}  // namespace aidl::android::hardware::neuralnetworks

// Shorthands
namespace android::nn {
namespace hal = ::android::hardware::neuralnetworks;
namespace aidl_hal = ::aidl::android::hardware::neuralnetworks;
}  // namespace android::nn

namespace android::hardware::neuralnetworks::utils {

nn::Capabilities::OperandPerformanceTable makeQuantized8PerformanceConsistentWithP(
        const nn::Capabilities::PerformanceInfo& float32Performance,
        const nn::Capabilities::PerformanceInfo& quantized8Performance);

// Indicates if the object contains no pointer-based data that could be relocated to shared memory.
bool hasNoPointerData(const nn::Model& model);
bool hasNoPointerData(const nn::Request& request);

// Relocate pointer-based data to shared memory. If `model` has no Operand::LifeTime::POINTER data,
// the function returns with a reference to `model`. If `model` has Operand::LifeTime::POINTER data,
// the model is copied to `maybeModelInSharedOut` with the POINTER data relocated to a memory pool,
// and the function returns with a reference to `*maybeModelInSharedOut`.
nn::GeneralResult<std::reference_wrapper<const nn::Model>> flushDataFromPointerToShared(
        const nn::Model* model, std::optional<nn::Model>* maybeModelInSharedOut);

// Relocate pointer-based data to shared memory. If `request` has no
// Request::Argument::LifeTime::POINTER data, the function returns with a reference to `request`. If
// `request` has Request::Argument::LifeTime::POINTER data, the request is copied to
// `maybeRequestInSharedOut` with the POINTER data relocated to a memory pool, and the function
// returns with a reference to `*maybeRequestInSharedOut`.
nn::GeneralResult<std::reference_wrapper<const nn::Request>> flushDataFromPointerToShared(
        const nn::Request* request, std::optional<nn::Request>* maybeRequestInSharedOut);

// Undoes `flushDataFromPointerToShared` on a Request object. More specifically,
// `unflushDataFromSharedToPointer` copies the output shared memory data from the transformed
// Request object back to the output pointer-based memory in the original Request object.
nn::GeneralResult<void> unflushDataFromSharedToPointer(
        const nn::Request& request, const std::optional<nn::Request>& maybeRequestInShared);

nn::GeneralResult<std::vector<uint32_t>> countNumberOfConsumers(
        size_t numberOfOperands, const std::vector<nn::Operation>& operations);

nn::GeneralResult<hidl_memory> createHidlMemoryFromSharedMemory(const nn::SharedMemory& memory);
nn::GeneralResult<nn::SharedMemory> createSharedMemoryFromHidlMemory(const hidl_memory& memory);

nn::GeneralResult<hidl_handle> hidlHandleFromSharedHandle(const nn::Handle& handle);
nn::GeneralResult<nn::Handle> sharedHandleFromNativeHandle(const native_handle_t* handle);

nn::GeneralResult<hidl_vec<hidl_handle>> convertSyncFences(
        const std::vector<nn::SyncFence>& fences);

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_COMMON_UTILS_H
