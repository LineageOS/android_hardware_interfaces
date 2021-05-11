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

#include "CommonUtils.h"

#include "HandleError.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android/hardware_buffer.h>
#include <hidl/HidlSupport.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <vndk/hardware_buffer.h>

#include <algorithm>
#include <any>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace android::hardware::neuralnetworks::utils {
namespace {

bool hasNoPointerData(const nn::Operand& operand);
bool hasNoPointerData(const nn::Model::Subgraph& subgraph);
bool hasNoPointerData(const nn::Request::Argument& argument);

template <typename Type>
bool hasNoPointerData(const std::vector<Type>& objects) {
    return std::all_of(objects.begin(), objects.end(),
                       [](const auto& object) { return hasNoPointerData(object); });
}

bool hasNoPointerData(const nn::DataLocation& location) {
    return std::visit([](auto ptr) { return ptr == nullptr; }, location.pointer);
}

bool hasNoPointerData(const nn::Operand& operand) {
    return hasNoPointerData(operand.location);
}

bool hasNoPointerData(const nn::Model::Subgraph& subgraph) {
    return hasNoPointerData(subgraph.operands);
}

bool hasNoPointerData(const nn::Request::Argument& argument) {
    return hasNoPointerData(argument.location);
}

void copyPointersToSharedMemory(nn::Operand* operand, nn::ConstantMemoryBuilder* memoryBuilder) {
    CHECK(operand != nullptr);
    CHECK(memoryBuilder != nullptr);

    if (operand->lifetime != nn::Operand::LifeTime::POINTER) {
        return;
    }

    const void* data = std::visit([](auto ptr) { return static_cast<const void*>(ptr); },
                                  operand->location.pointer);
    CHECK(data != nullptr);
    operand->lifetime = nn::Operand::LifeTime::CONSTANT_REFERENCE;
    operand->location = memoryBuilder->append(data, operand->location.length);
}

void copyPointersToSharedMemory(nn::Model::Subgraph* subgraph,
                                nn::ConstantMemoryBuilder* memoryBuilder) {
    CHECK(subgraph != nullptr);
    std::for_each(subgraph->operands.begin(), subgraph->operands.end(),
                  [memoryBuilder](auto& operand) {
                      copyPointersToSharedMemory(&operand, memoryBuilder);
                  });
}

nn::GeneralResult<hidl_handle> createNativeHandleFrom(base::unique_fd fd,
                                                      const std::vector<int32_t>& ints) {
    constexpr size_t kIntMax = std::numeric_limits<int>::max();
    CHECK_LE(ints.size(), kIntMax);
    native_handle_t* nativeHandle = native_handle_create(1, static_cast<int>(ints.size()));
    if (nativeHandle == nullptr) {
        return NN_ERROR() << "Failed to create native_handle";
    }

    nativeHandle->data[0] = fd.release();
    std::copy(ints.begin(), ints.end(), nativeHandle->data + 1);

    hidl_handle handle;
    handle.setTo(nativeHandle, /*shouldOwn=*/true);
    return handle;
}

nn::GeneralResult<hidl_memory> createHidlMemoryFrom(const nn::Memory::Ashmem& memory) {
    auto fd = NN_TRY(nn::dupFd(memory.fd));
    auto handle = NN_TRY(createNativeHandleFrom(std::move(fd), {}));
    return hidl_memory("ashmem", std::move(handle), memory.size);
}

nn::GeneralResult<hidl_memory> createHidlMemoryFrom(const nn::Memory::Fd& memory) {
    auto fd = NN_TRY(nn::dupFd(memory.fd));

    const auto [lowOffsetBits, highOffsetBits] = nn::getIntsFromOffset(memory.offset);
    const std::vector<int> ints = {memory.prot, lowOffsetBits, highOffsetBits};

    auto handle = NN_TRY(createNativeHandleFrom(std::move(fd), ints));
    return hidl_memory("mmap_fd", std::move(handle), memory.size);
}

nn::GeneralResult<hidl_memory> createHidlMemoryFrom(const nn::Memory::HardwareBuffer& memory) {
    const auto* ahwb = memory.handle.get();
    AHardwareBuffer_Desc bufferDesc;
    AHardwareBuffer_describe(ahwb, &bufferDesc);

    const bool isBlob = bufferDesc.format == AHARDWAREBUFFER_FORMAT_BLOB;
    const size_t size = isBlob ? bufferDesc.width : 0;
    const char* const name = isBlob ? "hardware_buffer_blob" : "hardware_buffer";

    const native_handle_t* nativeHandle = AHardwareBuffer_getNativeHandle(ahwb);
    const hidl_handle hidlHandle(nativeHandle);
    hidl_handle copiedHandle(hidlHandle);

    return hidl_memory(name, std::move(copiedHandle), size);
}

nn::GeneralResult<hidl_memory> createHidlMemoryFrom(const nn::Memory::Unknown& memory) {
    return hidl_memory(memory.name, NN_TRY(hidlHandleFromSharedHandle(memory.handle)), memory.size);
}

}  // anonymous namespace

nn::Capabilities::OperandPerformanceTable makeQuantized8PerformanceConsistentWithP(
        const nn::Capabilities::PerformanceInfo& float32Performance,
        const nn::Capabilities::PerformanceInfo& quantized8Performance) {
    // In Android P, most data types are treated as having the same performance as
    // TENSOR_QUANT8_ASYMM. This collection must be in sorted order.
    std::vector<nn::Capabilities::OperandPerformance> operandPerformances = {
            {.type = nn::OperandType::FLOAT32, .info = float32Performance},
            {.type = nn::OperandType::INT32, .info = quantized8Performance},
            {.type = nn::OperandType::UINT32, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_FLOAT32, .info = float32Performance},
            {.type = nn::OperandType::TENSOR_INT32, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_QUANT8_ASYMM, .info = quantized8Performance},
            {.type = nn::OperandType::OEM, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_OEM_BYTE, .info = quantized8Performance},
    };
    return nn::Capabilities::OperandPerformanceTable::create(std::move(operandPerformances))
            .value();
}

bool hasNoPointerData(const nn::Model& model) {
    return hasNoPointerData(model.main) && hasNoPointerData(model.referenced);
}

bool hasNoPointerData(const nn::Request& request) {
    return hasNoPointerData(request.inputs) && hasNoPointerData(request.outputs);
}

nn::GeneralResult<std::reference_wrapper<const nn::Model>> flushDataFromPointerToShared(
        const nn::Model* model, std::optional<nn::Model>* maybeModelInSharedOut) {
    CHECK(model != nullptr);
    CHECK(maybeModelInSharedOut != nullptr);

    if (hasNoPointerData(*model)) {
        return *model;
    }

    // Make a copy of the model in order to make modifications. The modified model is returned to
    // the caller through `maybeModelInSharedOut` if the function succeeds.
    nn::Model modelInShared = *model;

    nn::ConstantMemoryBuilder memoryBuilder(modelInShared.pools.size());
    copyPointersToSharedMemory(&modelInShared.main, &memoryBuilder);
    std::for_each(modelInShared.referenced.begin(), modelInShared.referenced.end(),
                  [&memoryBuilder](auto& subgraph) {
                      copyPointersToSharedMemory(&subgraph, &memoryBuilder);
                  });

    if (!memoryBuilder.empty()) {
        auto memory = NN_TRY(memoryBuilder.finish());
        modelInShared.pools.push_back(std::move(memory));
    }

    *maybeModelInSharedOut = modelInShared;
    return **maybeModelInSharedOut;
}

template <>
void InputRelocationTracker::flush() const {
    // Copy from pointers to shared memory.
    uint8_t* memoryPtr = static_cast<uint8_t*>(std::get<void*>(kMapping.pointer));
    for (const auto& [data, length, offset] : kRelocationInfos) {
        std::memcpy(memoryPtr + offset, data, length);
    }
}

template <>
void OutputRelocationTracker::flush() const {
    // Copy from shared memory to pointers.
    const uint8_t* memoryPtr = static_cast<const uint8_t*>(
            std::visit([](auto ptr) { return static_cast<const void*>(ptr); }, kMapping.pointer));
    for (const auto& [data, length, offset] : kRelocationInfos) {
        std::memcpy(data, memoryPtr + offset, length);
    }
}

nn::GeneralResult<std::reference_wrapper<const nn::Request>> convertRequestFromPointerToShared(
        const nn::Request* request, uint32_t alignment, uint32_t padding,
        std::optional<nn::Request>* maybeRequestInSharedOut, RequestRelocation* relocationOut) {
    CHECK(request != nullptr);
    CHECK(maybeRequestInSharedOut != nullptr);
    CHECK(relocationOut != nullptr);

    if (hasNoPointerData(*request)) {
        return *request;
    }

    // Make a copy of the request in order to make modifications. The modified request is returned
    // to the caller through `maybeRequestInSharedOut` if the function succeeds.
    nn::Request requestInShared = *request;

    RequestRelocation relocation;

    // Change input pointers to shared memory.
    nn::MutableMemoryBuilder inputBuilder(requestInShared.pools.size());
    std::vector<InputRelocationInfo> inputRelocationInfos;
    for (auto& input : requestInShared.inputs) {
        const auto& location = input.location;
        if (input.lifetime != nn::Request::Argument::LifeTime::POINTER) {
            continue;
        }

        input.lifetime = nn::Request::Argument::LifeTime::POOL;
        const void* data = std::visit([](auto ptr) { return static_cast<const void*>(ptr); },
                                      location.pointer);
        CHECK(data != nullptr);
        input.location = inputBuilder.append(location.length, alignment, padding);
        inputRelocationInfos.push_back({data, input.location.length, input.location.offset});
    }

    // Allocate input memory.
    if (!inputBuilder.empty()) {
        auto memory = NN_TRY(inputBuilder.finish());
        requestInShared.pools.push_back(memory);
        relocation.input = NN_TRY(
                InputRelocationTracker::create(std::move(inputRelocationInfos), std::move(memory)));
    }

    // Change output pointers to shared memory.
    nn::MutableMemoryBuilder outputBuilder(requestInShared.pools.size());
    std::vector<OutputRelocationInfo> outputRelocationInfos;
    for (auto& output : requestInShared.outputs) {
        const auto& location = output.location;
        if (output.lifetime != nn::Request::Argument::LifeTime::POINTER) {
            continue;
        }

        output.lifetime = nn::Request::Argument::LifeTime::POOL;
        void* data = std::get<void*>(location.pointer);
        CHECK(data != nullptr);
        output.location = outputBuilder.append(location.length, alignment, padding);
        outputRelocationInfos.push_back({data, output.location.length, output.location.offset});
    }

    // Allocate output memory.
    if (!outputBuilder.empty()) {
        auto memory = NN_TRY(outputBuilder.finish());
        requestInShared.pools.push_back(memory);
        relocation.output = NN_TRY(OutputRelocationTracker::create(std::move(outputRelocationInfos),
                                                                   std::move(memory)));
    }

    *maybeRequestInSharedOut = requestInShared;
    *relocationOut = std::move(relocation);
    return **maybeRequestInSharedOut;
}

nn::GeneralResult<std::vector<uint32_t>> countNumberOfConsumers(
        size_t numberOfOperands, const std::vector<nn::Operation>& operations) {
    return makeGeneralFailure(nn::countNumberOfConsumers(numberOfOperands, operations));
}

nn::GeneralResult<hidl_memory> createHidlMemoryFromSharedMemory(const nn::SharedMemory& memory) {
    if (memory == nullptr) {
        return NN_ERROR() << "Memory must be non-empty";
    }
    return std::visit([](const auto& x) { return createHidlMemoryFrom(x); }, memory->handle);
}

static uint32_t roundUpToMultiple(uint32_t value, uint32_t multiple) {
    return (value + multiple - 1) / multiple * multiple;
}

nn::GeneralResult<nn::SharedMemory> createSharedMemoryFromHidlMemory(const hidl_memory& memory) {
    CHECK_LE(memory.size(), std::numeric_limits<size_t>::max());
    if (!memory.valid()) {
        return NN_ERROR() << "Unable to convert invalid hidl_memory";
    }

    if (memory.name() == "ashmem") {
        if (memory.handle()->numFds != 1) {
            return NN_ERROR() << "Unable to convert invalid ashmem memory object with "
                              << memory.handle()->numFds << " numFds, but expected 1";
        }
        if (memory.handle()->numInts != 0) {
            return NN_ERROR() << "Unable to convert invalid ashmem memory object with "
                              << memory.handle()->numInts << " numInts, but expected 0";
        }
        auto handle = nn::Memory::Ashmem{
                .fd = NN_TRY(nn::dupFd(memory.handle()->data[0])),
                .size = static_cast<size_t>(memory.size()),
        };
        return std::make_shared<const nn::Memory>(nn::Memory{.handle = std::move(handle)});
    }

    if (memory.name() == "mmap_fd") {
        if (memory.handle()->numFds != 1) {
            return NN_ERROR() << "Unable to convert invalid mmap_fd memory object with "
                              << memory.handle()->numFds << " numFds, but expected 1";
        }
        if (memory.handle()->numInts != 3) {
            return NN_ERROR() << "Unable to convert invalid mmap_fd memory object with "
                              << memory.handle()->numInts << " numInts, but expected 3";
        }

        const int fd = memory.handle()->data[0];
        const int prot = memory.handle()->data[1];
        const int lower = memory.handle()->data[2];
        const int higher = memory.handle()->data[3];
        const size_t offset = nn::getOffsetFromInts(lower, higher);

        return nn::createSharedMemoryFromFd(static_cast<size_t>(memory.size()), prot, fd, offset);
    }

    if (memory.name() != "hardware_buffer_blob") {
        auto handle = nn::Memory::Unknown{
                .handle = NN_TRY(sharedHandleFromNativeHandle(memory.handle())),
                .size = static_cast<size_t>(memory.size()),
                .name = memory.name(),
        };
        return std::make_shared<const nn::Memory>(nn::Memory{.handle = std::move(handle)});
    }

    const auto size = memory.size();
    const auto format = AHARDWAREBUFFER_FORMAT_BLOB;
    const auto usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    const uint32_t width = size;
    const uint32_t height = 1;  // height is always 1 for BLOB mode AHardwareBuffer.
    const uint32_t layers = 1;  // layers is always 1 for BLOB mode AHardwareBuffer.

    // AHardwareBuffer_createFromHandle() might fail because an allocator
    // expects a specific stride value. In that case, we try to guess it by
    // aligning the width to small powers of 2.
    // TODO(b/174120849): Avoid stride assumptions.
    AHardwareBuffer* hardwareBuffer = nullptr;
    status_t status = UNKNOWN_ERROR;
    for (uint32_t alignment : {1, 4, 32, 64, 128, 2, 8, 16}) {
        const uint32_t stride = roundUpToMultiple(width, alignment);
        AHardwareBuffer_Desc desc{
                .width = width,
                .height = height,
                .layers = layers,
                .format = format,
                .usage = usage,
                .stride = stride,
        };
        status = AHardwareBuffer_createFromHandle(&desc, memory.handle(),
                                                  AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE,
                                                  &hardwareBuffer);
        if (status == NO_ERROR) {
            break;
        }
    }
    if (status != NO_ERROR) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
               << "Can't create AHardwareBuffer from handle. Error: " << status;
    }

    return nn::createSharedMemoryFromAHWB(hardwareBuffer, /*takeOwnership=*/true);
}

nn::GeneralResult<hidl_handle> hidlHandleFromSharedHandle(const nn::Handle& handle) {
    std::vector<base::unique_fd> fds;
    fds.reserve(handle.fds.size());
    for (const auto& fd : handle.fds) {
        const int dupFd = dup(fd);
        if (dupFd == -1) {
            return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "Failed to dup the fd";
        }
        fds.emplace_back(dupFd);
    }

    constexpr size_t kIntMax = std::numeric_limits<int>::max();
    CHECK_LE(handle.fds.size(), kIntMax);
    CHECK_LE(handle.ints.size(), kIntMax);
    native_handle_t* nativeHandle = native_handle_create(static_cast<int>(handle.fds.size()),
                                                         static_cast<int>(handle.ints.size()));
    if (nativeHandle == nullptr) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "Failed to create native_handle";
    }
    for (size_t i = 0; i < fds.size(); ++i) {
        nativeHandle->data[i] = fds[i].release();
    }
    std::copy(handle.ints.begin(), handle.ints.end(), &nativeHandle->data[nativeHandle->numFds]);

    hidl_handle hidlHandle;
    hidlHandle.setTo(nativeHandle, /*shouldOwn=*/true);
    return hidlHandle;
}

nn::GeneralResult<nn::Handle> sharedHandleFromNativeHandle(const native_handle_t* handle) {
    if (handle == nullptr) {
        return NN_ERROR() << "sharedHandleFromNativeHandle failed because handle is nullptr";
    }

    std::vector<base::unique_fd> fds;
    fds.reserve(handle->numFds);
    for (int i = 0; i < handle->numFds; ++i) {
        const int dupFd = dup(handle->data[i]);
        if (dupFd == -1) {
            return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "Failed to dup the fd";
        }
        fds.emplace_back(dupFd);
    }

    std::vector<int> ints(&handle->data[handle->numFds],
                          &handle->data[handle->numFds + handle->numInts]);

    return nn::Handle{.fds = std::move(fds), .ints = std::move(ints)};
}

nn::GeneralResult<hidl_vec<hidl_handle>> convertSyncFences(
        const std::vector<nn::SyncFence>& syncFences) {
    hidl_vec<hidl_handle> handles(syncFences.size());
    for (size_t i = 0; i < syncFences.size(); ++i) {
        const auto& handle = syncFences[i].getSharedHandle();
        if (handle == nullptr) {
            return NN_ERROR() << "convertSyncFences failed because sync fence is empty";
        }
        handles[i] = NN_TRY(hidlHandleFromSharedHandle(*handle));
    }
    return handles;
}

}  // namespace android::hardware::neuralnetworks::utils
