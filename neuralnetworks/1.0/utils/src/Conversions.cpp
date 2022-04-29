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

#include "Conversions.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/CommonUtils.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "Utils.h"

#ifdef __ANDROID__
#include <android/hardware_buffer.h>
#include <vndk/hardware_buffer.h>
#endif  // __ANDROID__

namespace {

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

}  // namespace

namespace android::nn {
namespace {

using hardware::hidl_handle;
using hardware::hidl_memory;
using hardware::hidl_vec;

template <typename Input>
using UnvalidatedConvertOutput =
        std::decay_t<decltype(unvalidatedConvert(std::declval<Input>()).value())>;

template <typename Type>
GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> unvalidatedConvert(
        const hidl_vec<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(nn::unvalidatedConvert(argument)));
    }
    return canonical;
}

template <typename Type>
GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& halObject) {
    auto canonical = NN_TRY(nn::unvalidatedConvert(halObject));
    NN_TRY(hal::V1_0::utils::compliantVersion(canonical));
    return canonical;
}

nn::GeneralResult<nn::Memory::Unknown::Handle> unknownHandleFromNativeHandle(
        const native_handle_t* handle) {
    if (handle == nullptr) {
        return NN_ERROR() << "unknownHandleFromNativeHandle failed because handle is nullptr";
    }

    std::vector<base::unique_fd> fds =
            NN_TRY(nn::dupFds(handle->data + 0, handle->data + handle->numFds));

    std::vector<int> ints(handle->data + handle->numFds,
                          handle->data + handle->numFds + handle->numInts);

    return nn::Memory::Unknown::Handle{.fds = std::move(fds), .ints = std::move(ints)};
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
        auto fd = NN_TRY(nn::dupFd(memory.handle()->data[0]));
        auto handle = nn::Memory::Ashmem{
                .fd = std::move(fd),
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
        auto handle = NN_TRY(unknownHandleFromNativeHandle(memory.handle()));
        auto unknown = nn::Memory::Unknown{
                .handle = std::move(handle),
                .size = static_cast<size_t>(memory.size()),
                .name = memory.name(),
        };
        return std::make_shared<const nn::Memory>(nn::Memory{.handle = std::move(unknown)});
    }

#ifdef __ANDROID__
    constexpr auto roundUpToMultiple = [](uint32_t value, uint32_t multiple) -> uint32_t {
        return (value + multiple - 1) / multiple * multiple;
    };

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
#else   // __ANDROID__
    LOG(FATAL) << "nn::GeneralResult<nn::SharedMemory> createSharedMemoryFromHidlMemory(const "
                  "hidl_memory& memory): Not Available on Host Build";
    return (NN_ERROR() << "createSharedMemoryFromHidlMemory failed")
            .
            operator nn::GeneralResult<nn::SharedMemory>();
#endif  // __ANDROID__
}

}  // anonymous namespace

GeneralResult<OperandType> unvalidatedConvert(const hal::V1_0::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

GeneralResult<OperationType> unvalidatedConvert(const hal::V1_0::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

GeneralResult<Operand::LifeTime> unvalidatedConvert(const hal::V1_0::OperandLifeTime& lifetime) {
    return static_cast<Operand::LifeTime>(lifetime);
}

GeneralResult<DeviceStatus> unvalidatedConvert(const hal::V1_0::DeviceStatus& deviceStatus) {
    return static_cast<DeviceStatus>(deviceStatus);
}

GeneralResult<Capabilities::PerformanceInfo> unvalidatedConvert(
        const hal::V1_0::PerformanceInfo& performanceInfo) {
    return Capabilities::PerformanceInfo{
            .execTime = performanceInfo.execTime,
            .powerUsage = performanceInfo.powerUsage,
    };
}

GeneralResult<Capabilities> unvalidatedConvert(const hal::V1_0::Capabilities& capabilities) {
    const auto quantized8Performance =
            NN_TRY(unvalidatedConvert(capabilities.quantized8Performance));
    const auto float32Performance = NN_TRY(unvalidatedConvert(capabilities.float32Performance));

    auto table = hal::utils::makeQuantized8PerformanceConsistentWithP(float32Performance,
                                                                      quantized8Performance);

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = float32Performance,
            .relaxedFloat32toFloat16PerformanceTensor = float32Performance,
            .operandPerformance = std::move(table),
    };
}

GeneralResult<DataLocation> unvalidatedConvert(const hal::V1_0::DataLocation& location) {
    return DataLocation{
            .poolIndex = location.poolIndex,
            .offset = location.offset,
            .length = location.length,
    };
}

GeneralResult<Operand> unvalidatedConvert(const hal::V1_0::Operand& operand) {
    const auto type = NN_TRY(unvalidatedConvert(operand.type));
    const auto lifetime = NN_TRY(unvalidatedConvert(operand.lifetime));
    const auto location = NN_TRY(unvalidatedConvert(operand.location));
    return Operand{
            .type = type,
            .dimensions = operand.dimensions,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = lifetime,
            .location = location,
    };
}

GeneralResult<Operation> unvalidatedConvert(const hal::V1_0::Operation& operation) {
    const auto type = NN_TRY(unvalidatedConvert(operation.type));
    return Operation{
            .type = type,
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

GeneralResult<Model::OperandValues> unvalidatedConvert(const hidl_vec<uint8_t>& operandValues) {
    return Model::OperandValues(operandValues.data(), operandValues.size());
}

GeneralResult<SharedHandle> unvalidatedConvert(const hidl_handle& handle) {
    if (handle.getNativeHandle() == nullptr) {
        return nullptr;
    }
    if (handle->numFds != 1 || handle->numInts != 0) {
        return NN_ERROR()
               << "unvalidatedConvert failed because handle does not only hold a single fd";
    }
    auto duplicatedFd = NN_TRY(nn::dupFd(handle->data[0]));
    return std::make_shared<const Handle>(std::move(duplicatedFd));
}

GeneralResult<SharedMemory> unvalidatedConvert(const hidl_memory& memory) {
    return createSharedMemoryFromHidlMemory(memory);
}

GeneralResult<Model> unvalidatedConvert(const hal::V1_0::Model& model) {
    auto operations = NN_TRY(unvalidatedConvert(model.operations));

    // Verify number of consumers.
    const auto numberOfConsumers =
            NN_TRY(countNumberOfConsumers(model.operands.size(), operations));
    CHECK(model.operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < model.operands.size(); ++i) {
        if (model.operands[i].numberOfConsumers != numberOfConsumers[i]) {
            return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
                   << "Invalid numberOfConsumers for operand " << i << ", expected "
                   << numberOfConsumers[i] << " but found " << model.operands[i].numberOfConsumers;
        }
    }

    auto operands = NN_TRY(unvalidatedConvert(model.operands));
    auto main = Model::Subgraph{
            .operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
    };

    auto operandValues = NN_TRY(unvalidatedConvert(model.operandValues));
    auto pools = NN_TRY(unvalidatedConvert(model.pools));
    return Model{
            .main = std::move(main),
            .operandValues = std::move(operandValues),
            .pools = std::move(pools),
    };
}

GeneralResult<Request::Argument> unvalidatedConvert(const hal::V1_0::RequestArgument& argument) {
    const auto lifetime = argument.hasNoValue ? Request::Argument::LifeTime::NO_VALUE
                                              : Request::Argument::LifeTime::POOL;
    const auto location = NN_TRY(unvalidatedConvert(argument.location));
    return Request::Argument{
            .lifetime = lifetime,
            .location = location,
            .dimensions = argument.dimensions,
    };
}

GeneralResult<Request> unvalidatedConvert(const hal::V1_0::Request& request) {
    auto memories = NN_TRY(unvalidatedConvert(request.pools));
    std::vector<Request::MemoryPool> pools;
    pools.reserve(memories.size());
    std::move(memories.begin(), memories.end(), std::back_inserter(pools));

    auto inputs = NN_TRY(unvalidatedConvert(request.inputs));
    auto outputs = NN_TRY(unvalidatedConvert(request.outputs));
    return Request{
            .inputs = std::move(inputs),
            .outputs = std::move(outputs),
            .pools = std::move(pools),
    };
}

GeneralResult<ErrorStatus> unvalidatedConvert(const hal::V1_0::ErrorStatus& status) {
    switch (status) {
        case hal::V1_0::ErrorStatus::NONE:
        case hal::V1_0::ErrorStatus::DEVICE_UNAVAILABLE:
        case hal::V1_0::ErrorStatus::GENERAL_FAILURE:
        case hal::V1_0::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE:
        case hal::V1_0::ErrorStatus::INVALID_ARGUMENT:
            return static_cast<ErrorStatus>(status);
    }
    return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
           << "Invalid ErrorStatus " << underlyingType(status);
}

GeneralResult<DeviceStatus> convert(const hal::V1_0::DeviceStatus& deviceStatus) {
    return validatedConvert(deviceStatus);
}

GeneralResult<Capabilities> convert(const hal::V1_0::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

GeneralResult<Model> convert(const hal::V1_0::Model& model) {
    return validatedConvert(model);
}

GeneralResult<Request> convert(const hal::V1_0::Request& request) {
    return validatedConvert(request);
}

GeneralResult<ErrorStatus> convert(const hal::V1_0::ErrorStatus& status) {
    return validatedConvert(status);
}

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_0::utils {
namespace {

template <typename Input>
using UnvalidatedConvertOutput =
        std::decay_t<decltype(unvalidatedConvert(std::declval<Input>()).value())>;

template <typename Type>
nn::GeneralResult<hidl_vec<UnvalidatedConvertOutput<Type>>> unvalidatedConvert(
        const std::vector<Type>& arguments) {
    hidl_vec<UnvalidatedConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(utils::unvalidatedConvert(arguments[i]));
    }
    return halObject;
}

template <typename Type>
nn::GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& canonical) {
    NN_TRY(compliantVersion(canonical));
    return utils::unvalidatedConvert(canonical);
}

nn::GeneralResult<hidl_handle> createNativeHandleFrom(std::vector<base::unique_fd> fds,
                                                      const std::vector<int32_t>& ints) {
    constexpr size_t kIntMax = std::numeric_limits<int>::max();
    CHECK_LE(fds.size(), kIntMax);
    CHECK_LE(ints.size(), kIntMax);
    native_handle_t* nativeHandle =
            native_handle_create(static_cast<int>(fds.size()), static_cast<int>(ints.size()));
    if (nativeHandle == nullptr) {
        return NN_ERROR() << "Failed to create native_handle";
    }

    for (size_t i = 0; i < fds.size(); ++i) {
        nativeHandle->data[i] = fds[i].release();
    }
    std::copy(ints.begin(), ints.end(), nativeHandle->data + nativeHandle->numFds);

    hidl_handle handle;
    handle.setTo(nativeHandle, /*shouldOwn=*/true);
    return handle;
}

nn::GeneralResult<hidl_handle> createNativeHandleFrom(base::unique_fd fd,
                                                      const std::vector<int32_t>& ints) {
    std::vector<base::unique_fd> fds;
    fds.push_back(std::move(fd));
    return createNativeHandleFrom(std::move(fds), ints);
}

nn::GeneralResult<hidl_handle> createNativeHandleFrom(const nn::Memory::Unknown::Handle& handle) {
    std::vector<base::unique_fd> fds = NN_TRY(nn::dupFds(handle.fds.begin(), handle.fds.end()));
    return createNativeHandleFrom(std::move(fds), handle.ints);
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
#ifdef __ANDROID__
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
#else   // __ANDROID__
    LOG(FATAL) << "nn::GeneralResult<hidl_memory> createHidlMemoryFrom(const "
                  "nn::Memory::HardwareBuffer& memory): Not Available on Host Build";
    (void)memory;
    return (NN_ERROR() << "createHidlMemoryFrom failed").operator nn::GeneralResult<hidl_memory>();
#endif  // __ANDROID__
}

nn::GeneralResult<hidl_memory> createHidlMemoryFrom(const nn::Memory::Unknown& memory) {
    return hidl_memory(memory.name, NN_TRY(createNativeHandleFrom(memory.handle)), memory.size);
}

}  // anonymous namespace

nn::GeneralResult<OperandType> unvalidatedConvert(const nn::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

nn::GeneralResult<OperandLifeTime> unvalidatedConvert(const nn::Operand::LifeTime& lifetime) {
    if (lifetime == nn::Operand::LifeTime::POINTER) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Model cannot be unvalidatedConverted because it contains pointer-based memory";
    }
    return static_cast<OperandLifeTime>(lifetime);
}

nn::GeneralResult<DeviceStatus> unvalidatedConvert(const nn::DeviceStatus& deviceStatus) {
    return static_cast<DeviceStatus>(deviceStatus);
}

nn::GeneralResult<PerformanceInfo> unvalidatedConvert(
        const nn::Capabilities::PerformanceInfo& performanceInfo) {
    return PerformanceInfo{
            .execTime = performanceInfo.execTime,
            .powerUsage = performanceInfo.powerUsage,
    };
}

nn::GeneralResult<Capabilities> unvalidatedConvert(const nn::Capabilities& capabilities) {
    const auto float32Performance = NN_TRY(unvalidatedConvert(
            capabilities.operandPerformance.lookup(nn::OperandType::TENSOR_FLOAT32)));
    const auto quantized8Performance = NN_TRY(unvalidatedConvert(
            capabilities.operandPerformance.lookup(nn::OperandType::TENSOR_QUANT8_ASYMM)));
    return Capabilities{
            .float32Performance = float32Performance,
            .quantized8Performance = quantized8Performance,
    };
}

nn::GeneralResult<DataLocation> unvalidatedConvert(const nn::DataLocation& location) {
    return DataLocation{
            .poolIndex = location.poolIndex,
            .offset = location.offset,
            .length = location.length,
    };
}

nn::GeneralResult<Operand> unvalidatedConvert(const nn::Operand& operand) {
    const auto type = NN_TRY(unvalidatedConvert(operand.type));
    const auto lifetime = NN_TRY(unvalidatedConvert(operand.lifetime));
    const auto location = NN_TRY(unvalidatedConvert(operand.location));
    return Operand{
            .type = type,
            .dimensions = operand.dimensions,
            .numberOfConsumers = 0,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = lifetime,
            .location = location,
    };
}

nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation) {
    const auto type = NN_TRY(unvalidatedConvert(operation.type));
    return Operation{
            .type = type,
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

nn::GeneralResult<hidl_vec<uint8_t>> unvalidatedConvert(
        const nn::Model::OperandValues& operandValues) {
    return hidl_vec<uint8_t>(operandValues.data(), operandValues.data() + operandValues.size());
}

nn::GeneralResult<hidl_handle> unvalidatedConvert(const nn::SharedHandle& handle) {
    if (handle == nullptr) {
        return {};
    }
    base::unique_fd fd = NN_TRY(nn::dupFd(handle->get()));
    return createNativeHandleFrom(std::move(fd), {});
}

nn::GeneralResult<hidl_memory> unvalidatedConvert(const nn::SharedMemory& memory) {
    if (memory == nullptr) {
        return NN_ERROR() << "Memory must be non-empty";
    }
    return std::visit([](const auto& x) { return createHidlMemoryFrom(x); }, memory->handle);
}

nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model) {
    if (!hal::utils::hasNoPointerData(model)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Mdoel cannot be unvalidatedConverted because it contains pointer-based memory";
    }

    auto operands = NN_TRY(unvalidatedConvert(model.main.operands));

    // Update number of consumers.
    const auto numberOfConsumers =
            NN_TRY(countNumberOfConsumers(operands.size(), model.main.operations));
    CHECK(operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < operands.size(); ++i) {
        operands[i].numberOfConsumers = numberOfConsumers[i];
    }

    auto operations = NN_TRY(unvalidatedConvert(model.main.operations));
    auto operandValues = NN_TRY(unvalidatedConvert(model.operandValues));
    auto pools = NN_TRY(unvalidatedConvert(model.pools));
    return Model{
            .operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = model.main.inputIndexes,
            .outputIndexes = model.main.outputIndexes,
            .operandValues = std::move(operandValues),
            .pools = std::move(pools),
    };
}

nn::GeneralResult<RequestArgument> unvalidatedConvert(
        const nn::Request::Argument& requestArgument) {
    if (requestArgument.lifetime == nn::Request::Argument::LifeTime::POINTER) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Request cannot be unvalidatedConverted because it contains pointer-based memory";
    }
    const bool hasNoValue = requestArgument.lifetime == nn::Request::Argument::LifeTime::NO_VALUE;
    const auto location = NN_TRY(unvalidatedConvert(requestArgument.location));
    return RequestArgument{
            .hasNoValue = hasNoValue,
            .location = location,
            .dimensions = requestArgument.dimensions,
    };
}

nn::GeneralResult<hidl_memory> unvalidatedConvert(const nn::Request::MemoryPool& memoryPool) {
    return unvalidatedConvert(std::get<nn::SharedMemory>(memoryPool));
}

nn::GeneralResult<Request> unvalidatedConvert(const nn::Request& request) {
    if (!hal::utils::hasNoPointerData(request)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Request cannot be unvalidatedConverted because it contains pointer-based memory";
    }

    auto inputs = NN_TRY(unvalidatedConvert(request.inputs));
    auto outputs = NN_TRY(unvalidatedConvert(request.outputs));
    auto pools = NN_TRY(unvalidatedConvert(request.pools));
    return Request{
            .inputs = std::move(inputs),
            .outputs = std::move(outputs),
            .pools = std::move(pools),
    };
}

nn::GeneralResult<ErrorStatus> unvalidatedConvert(const nn::ErrorStatus& status) {
    switch (status) {
        case nn::ErrorStatus::NONE:
        case nn::ErrorStatus::DEVICE_UNAVAILABLE:
        case nn::ErrorStatus::GENERAL_FAILURE:
        case nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE:
        case nn::ErrorStatus::INVALID_ARGUMENT:
            return static_cast<ErrorStatus>(status);
        default:
            return ErrorStatus::GENERAL_FAILURE;
    }
}

nn::GeneralResult<DeviceStatus> convert(const nn::DeviceStatus& deviceStatus) {
    return validatedConvert(deviceStatus);
}

nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

nn::GeneralResult<Model> convert(const nn::Model& model) {
    return validatedConvert(model);
}

nn::GeneralResult<Request> convert(const nn::Request& request) {
    return validatedConvert(request);
}

nn::GeneralResult<ErrorStatus> convert(const nn::ErrorStatus& status) {
    return validatedConvert(status);
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
