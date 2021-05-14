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
#include <nnapi/SharedMemory.h>
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

// Record a relocation mapping between pointer-based data and shared memory.
// Only two specializations of this template may exist:
// - RelocationInfo<const void*> for request inputs
// - RelocationInfo<void*> for request outputs
template <typename PointerType>
struct RelocationInfo {
    PointerType data;
    size_t length;
    size_t offset;
};
using InputRelocationInfo = RelocationInfo<const void*>;
using OutputRelocationInfo = RelocationInfo<void*>;

// Keep track of the relocation mapping between pointer-based data and shared memory pool,
// and provide method to copy the data between pointers and the shared memory pool.
// Only two specializations of this template may exist:
// - RelocationTracker<InputRelocationInfo> for request inputs
// - RelocationTracker<OutputRelocationInfo> for request outputs
template <typename RelocationInfoType>
class RelocationTracker {
  public:
    static nn::GeneralResult<std::unique_ptr<RelocationTracker>> create(
            std::vector<RelocationInfoType> relocationInfos, nn::SharedMemory memory) {
        auto mapping = NN_TRY(map(memory));
        return std::make_unique<RelocationTracker<RelocationInfoType>>(
                std::move(relocationInfos), std::move(memory), std::move(mapping));
    }

    RelocationTracker(std::vector<RelocationInfoType> relocationInfos, nn::SharedMemory memory,
                      nn::Mapping mapping)
        : kRelocationInfos(std::move(relocationInfos)),
          kMemory(std::move(memory)),
          kMapping(std::move(mapping)) {}

    // Specializations defined in CommonUtils.cpp.
    // For InputRelocationTracker, this method will copy pointer data to the shared memory pool.
    // For OutputRelocationTracker, this method will copy shared memory data to the pointers.
    void flush() const;

  private:
    const std::vector<RelocationInfoType> kRelocationInfos;
    const nn::SharedMemory kMemory;
    const nn::Mapping kMapping;
};
using InputRelocationTracker = RelocationTracker<InputRelocationInfo>;
using OutputRelocationTracker = RelocationTracker<OutputRelocationInfo>;

struct RequestRelocation {
    std::unique_ptr<InputRelocationTracker> input;
    std::unique_ptr<OutputRelocationTracker> output;
};

// Relocate pointer-based data to shared memory. If `request` has no
// Request::Argument::LifeTime::POINTER data, the function returns with a reference to `request`. If
// `request` has Request::Argument::LifeTime::POINTER data, the request is copied to
// `maybeRequestInSharedOut` with the POINTER data relocated to a memory pool, and the function
// returns with a reference to `*maybeRequestInSharedOut`. The `relocationOut` will be set to track
// the input and output relocations.
//
// Unlike `flushDataFromPointerToShared`, this method will not copy the input pointer data to the
// shared memory pool. Use `relocationOut` to flush the input or output data after the call.
nn::GeneralResult<std::reference_wrapper<const nn::Request>> convertRequestFromPointerToShared(
        const nn::Request* request, uint32_t alignment, uint32_t padding,
        std::optional<nn::Request>* maybeRequestInSharedOut, RequestRelocation* relocationOut);

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
