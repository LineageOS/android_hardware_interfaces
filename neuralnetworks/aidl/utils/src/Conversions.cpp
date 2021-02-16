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

#include "Conversions.h"

#include <aidl/android/hardware/common/NativeHandle.h>
#include <android-base/logging.h>
#include <android/hardware_buffer.h>
#include <cutils/native_handle.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <vndk/hardware_buffer.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

#define VERIFY_NON_NEGATIVE(value) \
    while (UNLIKELY(value < 0)) return NN_ERROR()

namespace {

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

constexpr auto kVersion = android::nn::Version::ANDROID_S;

}  // namespace

namespace android::nn {
namespace {

using ::aidl::android::hardware::common::NativeHandle;

constexpr auto validOperandType(nn::OperandType operandType) {
    switch (operandType) {
        case nn::OperandType::FLOAT32:
        case nn::OperandType::INT32:
        case nn::OperandType::UINT32:
        case nn::OperandType::TENSOR_FLOAT32:
        case nn::OperandType::TENSOR_INT32:
        case nn::OperandType::TENSOR_QUANT8_ASYMM:
        case nn::OperandType::BOOL:
        case nn::OperandType::TENSOR_QUANT16_SYMM:
        case nn::OperandType::TENSOR_FLOAT16:
        case nn::OperandType::TENSOR_BOOL8:
        case nn::OperandType::FLOAT16:
        case nn::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case nn::OperandType::TENSOR_QUANT16_ASYMM:
        case nn::OperandType::TENSOR_QUANT8_SYMM:
        case nn::OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
        case nn::OperandType::SUBGRAPH:
            return true;
        case nn::OperandType::OEM:
        case nn::OperandType::TENSOR_OEM_BYTE:
            return false;
    }
    return nn::isExtension(operandType);
}

template <typename Input>
using UnvalidatedConvertOutput =
        std::decay_t<decltype(unvalidatedConvert(std::declval<Input>()).value())>;

template <typename Type>
GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> unvalidatedConvertVec(
        const std::vector<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(nn::unvalidatedConvert(argument)));
    }
    return canonical;
}

template <typename Type>
GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> unvalidatedConvert(
        const std::vector<Type>& arguments) {
    return unvalidatedConvertVec(arguments);
}

template <typename Type>
GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& halObject) {
    auto canonical = NN_TRY(nn::unvalidatedConvert(halObject));
    const auto maybeVersion = validate(canonical);
    if (!maybeVersion.has_value()) {
        return error() << maybeVersion.error();
    }
    const auto version = maybeVersion.value();
    if (version > kVersion) {
        return NN_ERROR() << "Insufficient version: " << version << " vs required " << kVersion;
    }
    return canonical;
}

template <typename Type>
GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> validatedConvert(
        const std::vector<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(validatedConvert(argument)));
    }
    return canonical;
}

GeneralResult<Handle> unvalidatedConvertHelper(const NativeHandle& aidlNativeHandle) {
    std::vector<base::unique_fd> fds;
    fds.reserve(aidlNativeHandle.fds.size());
    for (const auto& fd : aidlNativeHandle.fds) {
        const int dupFd = dup(fd.get());
        if (dupFd == -1) {
            // TODO(b/120417090): is ANEURALNETWORKS_UNEXPECTED_NULL the correct error to return
            // here?
            return NN_ERROR() << "Failed to dup the fd";
        }
        fds.emplace_back(dupFd);
    }

    return Handle{.fds = std::move(fds), .ints = aidlNativeHandle.ints};
}

struct NativeHandleDeleter {
    void operator()(native_handle_t* handle) const {
        if (handle) {
            native_handle_close(handle);
            native_handle_delete(handle);
        }
    }
};

using UniqueNativeHandle = std::unique_ptr<native_handle_t, NativeHandleDeleter>;

static nn::GeneralResult<UniqueNativeHandle> nativeHandleFromAidlHandle(
        const NativeHandle& handle) {
    std::vector<base::unique_fd> fds;
    fds.reserve(handle.fds.size());
    for (const auto& fd : handle.fds) {
        const int dupFd = dup(fd.get());
        if (dupFd == -1) {
            return NN_ERROR() << "Failed to dup the fd";
        }
        fds.emplace_back(dupFd);
    }

    constexpr size_t kIntMax = std::numeric_limits<int>::max();
    CHECK_LE(handle.fds.size(), kIntMax);
    CHECK_LE(handle.ints.size(), kIntMax);
    native_handle_t* nativeHandle = native_handle_create(static_cast<int>(handle.fds.size()),
                                                         static_cast<int>(handle.ints.size()));
    if (nativeHandle == nullptr) {
        return NN_ERROR() << "Failed to create native_handle";
    }
    for (size_t i = 0; i < fds.size(); ++i) {
        nativeHandle->data[i] = fds[i].release();
    }
    std::copy(handle.ints.begin(), handle.ints.end(), &nativeHandle->data[nativeHandle->numFds]);

    return UniqueNativeHandle(nativeHandle);
}

}  // anonymous namespace

GeneralResult<OperandType> unvalidatedConvert(const aidl_hal::OperandType& operandType) {
    VERIFY_NON_NEGATIVE(underlyingType(operandType)) << "Negative operand types are not allowed.";
    return static_cast<OperandType>(operandType);
}

GeneralResult<OperationType> unvalidatedConvert(const aidl_hal::OperationType& operationType) {
    VERIFY_NON_NEGATIVE(underlyingType(operationType))
            << "Negative operation types are not allowed.";
    return static_cast<OperationType>(operationType);
}

GeneralResult<DeviceType> unvalidatedConvert(const aidl_hal::DeviceType& deviceType) {
    return static_cast<DeviceType>(deviceType);
}

GeneralResult<Priority> unvalidatedConvert(const aidl_hal::Priority& priority) {
    return static_cast<Priority>(priority);
}

GeneralResult<Capabilities> unvalidatedConvert(const aidl_hal::Capabilities& capabilities) {
    const bool validOperandTypes = std::all_of(
            capabilities.operandPerformance.begin(), capabilities.operandPerformance.end(),
            [](const aidl_hal::OperandPerformance& operandPerformance) {
                const auto maybeType = unvalidatedConvert(operandPerformance.type);
                return !maybeType.has_value() ? false : validOperandType(maybeType.value());
            });
    if (!validOperandTypes) {
        return NN_ERROR() << "Invalid OperandType when unvalidatedConverting OperandPerformance in "
                             "Capabilities";
    }

    auto operandPerformance = NN_TRY(unvalidatedConvert(capabilities.operandPerformance));
    auto table = NN_TRY(hal::utils::makeGeneralFailure(
            Capabilities::OperandPerformanceTable::create(std::move(operandPerformance)),
            nn::ErrorStatus::GENERAL_FAILURE));

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = NN_TRY(
                    unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceScalar)),
            .relaxedFloat32toFloat16PerformanceTensor = NN_TRY(
                    unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceTensor)),
            .operandPerformance = std::move(table),
            .ifPerformance = NN_TRY(unvalidatedConvert(capabilities.ifPerformance)),
            .whilePerformance = NN_TRY(unvalidatedConvert(capabilities.whilePerformance)),
    };
}

GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const aidl_hal::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(unvalidatedConvert(operandPerformance.type)),
            .info = NN_TRY(unvalidatedConvert(operandPerformance.info)),
    };
}

GeneralResult<Capabilities::PerformanceInfo> unvalidatedConvert(
        const aidl_hal::PerformanceInfo& performanceInfo) {
    return Capabilities::PerformanceInfo{
            .execTime = performanceInfo.execTime,
            .powerUsage = performanceInfo.powerUsage,
    };
}

GeneralResult<DataLocation> unvalidatedConvert(const aidl_hal::DataLocation& location) {
    VERIFY_NON_NEGATIVE(location.poolIndex) << "DataLocation: pool index must not be negative";
    VERIFY_NON_NEGATIVE(location.offset) << "DataLocation: offset must not be negative";
    VERIFY_NON_NEGATIVE(location.length) << "DataLocation: length must not be negative";
    VERIFY_NON_NEGATIVE(location.padding) << "DataLocation: padding must not be negative";
    if (location.offset > std::numeric_limits<uint32_t>::max()) {
        return NN_ERROR() << "DataLocation: offset must be <= std::numeric_limits<uint32_t>::max()";
    }
    if (location.length > std::numeric_limits<uint32_t>::max()) {
        return NN_ERROR() << "DataLocation: length must be <= std::numeric_limits<uint32_t>::max()";
    }
    if (location.padding > std::numeric_limits<uint32_t>::max()) {
        return NN_ERROR()
               << "DataLocation: padding must be <= std::numeric_limits<uint32_t>::max()";
    }
    return DataLocation{
            .poolIndex = static_cast<uint32_t>(location.poolIndex),
            .offset = static_cast<uint32_t>(location.offset),
            .length = static_cast<uint32_t>(location.length),
            .padding = static_cast<uint32_t>(location.padding),
    };
}

GeneralResult<Operation> unvalidatedConvert(const aidl_hal::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = NN_TRY(toUnsigned(operation.inputs)),
            .outputs = NN_TRY(toUnsigned(operation.outputs)),
    };
}

GeneralResult<Operand::LifeTime> unvalidatedConvert(
        const aidl_hal::OperandLifeTime& operandLifeTime) {
    return static_cast<Operand::LifeTime>(operandLifeTime);
}

GeneralResult<Operand> unvalidatedConvert(const aidl_hal::Operand& operand) {
    return Operand{
            .type = NN_TRY(unvalidatedConvert(operand.type)),
            .dimensions = NN_TRY(toUnsigned(operand.dimensions)),
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(unvalidatedConvert(operand.lifetime)),
            .location = NN_TRY(unvalidatedConvert(operand.location)),
            .extraParams = NN_TRY(unvalidatedConvert(operand.extraParams)),
    };
}

GeneralResult<Operand::ExtraParams> unvalidatedConvert(
        const std::optional<aidl_hal::OperandExtraParams>& optionalExtraParams) {
    if (!optionalExtraParams.has_value()) {
        return Operand::NoParams{};
    }
    const auto& extraParams = optionalExtraParams.value();
    using Tag = aidl_hal::OperandExtraParams::Tag;
    switch (extraParams.getTag()) {
        case Tag::channelQuant:
            return unvalidatedConvert(extraParams.get<Tag::channelQuant>());
        case Tag::extension:
            return extraParams.get<Tag::extension>();
    }
    return NN_ERROR() << "Unrecognized Operand::ExtraParams tag: "
                      << underlyingType(extraParams.getTag());
}

GeneralResult<Operand::SymmPerChannelQuantParams> unvalidatedConvert(
        const aidl_hal::SymmPerChannelQuantParams& symmPerChannelQuantParams) {
    VERIFY_NON_NEGATIVE(symmPerChannelQuantParams.channelDim)
            << "Per-channel quantization channel dimension must not be negative.";
    return Operand::SymmPerChannelQuantParams{
            .scales = symmPerChannelQuantParams.scales,
            .channelDim = static_cast<uint32_t>(symmPerChannelQuantParams.channelDim),
    };
}

GeneralResult<Model> unvalidatedConvert(const aidl_hal::Model& model) {
    return Model{
            .main = NN_TRY(unvalidatedConvert(model.main)),
            .referenced = NN_TRY(unvalidatedConvert(model.referenced)),
            .operandValues = NN_TRY(unvalidatedConvert(model.operandValues)),
            .pools = NN_TRY(unvalidatedConvert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(unvalidatedConvert(model.extensionNameToPrefix)),
    };
}

GeneralResult<Model::Subgraph> unvalidatedConvert(const aidl_hal::Subgraph& subgraph) {
    return Model::Subgraph{
            .operands = NN_TRY(unvalidatedConvert(subgraph.operands)),
            .operations = NN_TRY(unvalidatedConvert(subgraph.operations)),
            .inputIndexes = NN_TRY(toUnsigned(subgraph.inputIndexes)),
            .outputIndexes = NN_TRY(toUnsigned(subgraph.outputIndexes)),
    };
}

GeneralResult<Model::ExtensionNameAndPrefix> unvalidatedConvert(
        const aidl_hal::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return Model::ExtensionNameAndPrefix{
            .name = extensionNameAndPrefix.name,
            .prefix = extensionNameAndPrefix.prefix,
    };
}

GeneralResult<Extension> unvalidatedConvert(const aidl_hal::Extension& extension) {
    return Extension{
            .name = extension.name,
            .operandTypes = NN_TRY(unvalidatedConvert(extension.operandTypes)),
    };
}

GeneralResult<Extension::OperandTypeInformation> unvalidatedConvert(
        const aidl_hal::ExtensionOperandTypeInformation& operandTypeInformation) {
    VERIFY_NON_NEGATIVE(operandTypeInformation.byteSize)
            << "Extension operand type byte size must not be negative";
    return Extension::OperandTypeInformation{
            .type = operandTypeInformation.type,
            .isTensor = operandTypeInformation.isTensor,
            .byteSize = static_cast<uint32_t>(operandTypeInformation.byteSize),
    };
}

GeneralResult<OutputShape> unvalidatedConvert(const aidl_hal::OutputShape& outputShape) {
    return OutputShape{
            .dimensions = NN_TRY(toUnsigned(outputShape.dimensions)),
            .isSufficient = outputShape.isSufficient,
    };
}

GeneralResult<MeasureTiming> unvalidatedConvert(bool measureTiming) {
    return measureTiming ? MeasureTiming::YES : MeasureTiming::NO;
}

static uint32_t roundUpToMultiple(uint32_t value, uint32_t multiple) {
    return (value + multiple - 1) / multiple * multiple;
}

GeneralResult<SharedMemory> unvalidatedConvert(const aidl_hal::Memory& memory) {
    VERIFY_NON_NEGATIVE(memory.size) << "Memory size must not be negative";
    if (memory.size > std::numeric_limits<uint32_t>::max()) {
        return NN_ERROR() << "Memory: size must be <= std::numeric_limits<size_t>::max()";
    }

    if (memory.name != "hardware_buffer_blob") {
        return std::make_shared<const Memory>(Memory{
                .handle = NN_TRY(unvalidatedConvertHelper(memory.handle)),
                .size = static_cast<uint32_t>(memory.size),
                .name = memory.name,
        });
    }

    const auto size = static_cast<uint32_t>(memory.size);
    const auto format = AHARDWAREBUFFER_FORMAT_BLOB;
    const auto usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    const uint32_t width = size;
    const uint32_t height = 1;  // height is always 1 for BLOB mode AHardwareBuffer.
    const uint32_t layers = 1;  // layers is always 1 for BLOB mode AHardwareBuffer.

    const UniqueNativeHandle handle = NN_TRY(nativeHandleFromAidlHandle(memory.handle));
    const native_handle_t* nativeHandle = handle.get();

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
        status = AHardwareBuffer_createFromHandle(&desc, nativeHandle,
                                                  AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE,
                                                  &hardwareBuffer);
        if (status == NO_ERROR) {
            break;
        }
    }
    if (status != NO_ERROR) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
               << "Can't create AHardwareBuffer from handle. Error: " << status;
    }

    return std::make_shared<const Memory>(Memory{
            .handle = HardwareBufferHandle(hardwareBuffer, /*takeOwnership=*/true),
            .size = static_cast<uint32_t>(memory.size),
            .name = memory.name,
    });
}

GeneralResult<Model::OperandValues> unvalidatedConvert(const std::vector<uint8_t>& operandValues) {
    return Model::OperandValues(operandValues.data(), operandValues.size());
}

GeneralResult<BufferDesc> unvalidatedConvert(const aidl_hal::BufferDesc& bufferDesc) {
    return BufferDesc{.dimensions = NN_TRY(toUnsigned(bufferDesc.dimensions))};
}

GeneralResult<BufferRole> unvalidatedConvert(const aidl_hal::BufferRole& bufferRole) {
    VERIFY_NON_NEGATIVE(bufferRole.modelIndex) << "BufferRole: modelIndex must not be negative";
    VERIFY_NON_NEGATIVE(bufferRole.ioIndex) << "BufferRole: ioIndex must not be negative";
    return BufferRole{
            .modelIndex = static_cast<uint32_t>(bufferRole.modelIndex),
            .ioIndex = static_cast<uint32_t>(bufferRole.ioIndex),
            .frequency = bufferRole.frequency,
    };
}

GeneralResult<Request> unvalidatedConvert(const aidl_hal::Request& request) {
    return Request{
            .inputs = NN_TRY(unvalidatedConvert(request.inputs)),
            .outputs = NN_TRY(unvalidatedConvert(request.outputs)),
            .pools = NN_TRY(unvalidatedConvert(request.pools)),
    };
}

GeneralResult<Request::Argument> unvalidatedConvert(const aidl_hal::RequestArgument& argument) {
    const auto lifetime = argument.hasNoValue ? Request::Argument::LifeTime::NO_VALUE
                                              : Request::Argument::LifeTime::POOL;
    return Request::Argument{
            .lifetime = lifetime,
            .location = NN_TRY(unvalidatedConvert(argument.location)),
            .dimensions = NN_TRY(toUnsigned(argument.dimensions)),
    };
}

GeneralResult<Request::MemoryPool> unvalidatedConvert(
        const aidl_hal::RequestMemoryPool& memoryPool) {
    using Tag = aidl_hal::RequestMemoryPool::Tag;
    switch (memoryPool.getTag()) {
        case Tag::pool:
            return unvalidatedConvert(memoryPool.get<Tag::pool>());
        case Tag::token: {
            const auto token = memoryPool.get<Tag::token>();
            VERIFY_NON_NEGATIVE(token) << "Memory pool token must not be negative";
            return static_cast<Request::MemoryDomainToken>(token);
        }
    }
    return NN_ERROR() << "Invalid Request::MemoryPool tag " << underlyingType(memoryPool.getTag());
}

GeneralResult<ErrorStatus> unvalidatedConvert(const aidl_hal::ErrorStatus& status) {
    switch (status) {
        case aidl_hal::ErrorStatus::NONE:
        case aidl_hal::ErrorStatus::DEVICE_UNAVAILABLE:
        case aidl_hal::ErrorStatus::GENERAL_FAILURE:
        case aidl_hal::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE:
        case aidl_hal::ErrorStatus::INVALID_ARGUMENT:
        case aidl_hal::ErrorStatus::MISSED_DEADLINE_TRANSIENT:
        case aidl_hal::ErrorStatus::MISSED_DEADLINE_PERSISTENT:
        case aidl_hal::ErrorStatus::RESOURCE_EXHAUSTED_TRANSIENT:
        case aidl_hal::ErrorStatus::RESOURCE_EXHAUSTED_PERSISTENT:
            return static_cast<ErrorStatus>(status);
    }
    return NN_ERROR() << "Invalid ErrorStatus " << underlyingType(status);
}

GeneralResult<ExecutionPreference> unvalidatedConvert(
        const aidl_hal::ExecutionPreference& executionPreference) {
    return static_cast<ExecutionPreference>(executionPreference);
}

GeneralResult<SharedHandle> unvalidatedConvert(const NativeHandle& aidlNativeHandle) {
    return std::make_shared<const Handle>(NN_TRY(unvalidatedConvertHelper(aidlNativeHandle)));
}

GeneralResult<ExecutionPreference> convert(
        const aidl_hal::ExecutionPreference& executionPreference) {
    return validatedConvert(executionPreference);
}

GeneralResult<SharedMemory> convert(const aidl_hal::Memory& operand) {
    return validatedConvert(operand);
}

GeneralResult<Model> convert(const aidl_hal::Model& model) {
    return validatedConvert(model);
}

GeneralResult<Operand> convert(const aidl_hal::Operand& operand) {
    return unvalidatedConvert(operand);
}

GeneralResult<OperandType> convert(const aidl_hal::OperandType& operandType) {
    return unvalidatedConvert(operandType);
}

GeneralResult<Priority> convert(const aidl_hal::Priority& priority) {
    return validatedConvert(priority);
}

GeneralResult<Request::MemoryPool> convert(const aidl_hal::RequestMemoryPool& memoryPool) {
    return unvalidatedConvert(memoryPool);
}

GeneralResult<Request> convert(const aidl_hal::Request& request) {
    return validatedConvert(request);
}

GeneralResult<std::vector<Operation>> convert(const std::vector<aidl_hal::Operation>& operations) {
    return unvalidatedConvert(operations);
}

GeneralResult<std::vector<SharedMemory>> convert(const std::vector<aidl_hal::Memory>& memories) {
    return validatedConvert(memories);
}

GeneralResult<std::vector<uint32_t>> toUnsigned(const std::vector<int32_t>& vec) {
    if (!std::all_of(vec.begin(), vec.end(), [](int32_t v) { return v >= 0; })) {
        return NN_ERROR() << "Negative value passed to conversion from signed to unsigned";
    }
    return std::vector<uint32_t>(vec.begin(), vec.end());
}

}  // namespace android::nn

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

template <typename Input>
using UnvalidatedConvertOutput =
        std::decay_t<decltype(unvalidatedConvert(std::declval<Input>()).value())>;

template <typename Type>
nn::GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> unvalidatedConvertVec(
        const std::vector<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(unvalidatedConvert(arguments[i]));
    }
    return halObject;
}

template <typename Type>
nn::GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& canonical) {
    const auto maybeVersion = nn::validate(canonical);
    if (!maybeVersion.has_value()) {
        return nn::error() << maybeVersion.error();
    }
    const auto version = maybeVersion.value();
    if (version > kVersion) {
        return NN_ERROR() << "Insufficient version: " << version << " vs required " << kVersion;
    }
    return utils::unvalidatedConvert(canonical);
}

template <typename Type>
nn::GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> validatedConvert(
        const std::vector<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(validatedConvert(arguments[i]));
    }
    return halObject;
}

nn::GeneralResult<common::NativeHandle> unvalidatedConvert(const nn::Handle& handle) {
    common::NativeHandle aidlNativeHandle;
    aidlNativeHandle.fds.reserve(handle.fds.size());
    for (const auto& fd : handle.fds) {
        const int dupFd = dup(fd.get());
        if (dupFd == -1) {
            // TODO(b/120417090): is ANEURALNETWORKS_UNEXPECTED_NULL the correct error to return
            // here?
            return NN_ERROR() << "Failed to dup the fd";
        }
        aidlNativeHandle.fds.emplace_back(dupFd);
    }
    aidlNativeHandle.ints = handle.ints;
    return aidlNativeHandle;
}

static nn::GeneralResult<common::NativeHandle> aidlHandleFromNativeHandle(
        const native_handle_t& handle) {
    common::NativeHandle aidlNativeHandle;

    aidlNativeHandle.fds.reserve(handle.numFds);
    for (int i = 0; i < handle.numFds; ++i) {
        const int dupFd = dup(handle.data[i]);
        if (dupFd == -1) {
            return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "Failed to dup the fd";
        }
        aidlNativeHandle.fds.emplace_back(dupFd);
    }

    aidlNativeHandle.ints = std::vector<int>(&handle.data[handle.numFds],
                                             &handle.data[handle.numFds + handle.numInts]);

    return aidlNativeHandle;
}

}  // namespace

nn::GeneralResult<common::NativeHandle> unvalidatedConvert(const nn::SharedHandle& sharedHandle) {
    CHECK(sharedHandle != nullptr);
    return unvalidatedConvert(*sharedHandle);
}

nn::GeneralResult<Memory> unvalidatedConvert(const nn::SharedMemory& memory) {
    CHECK(memory != nullptr);
    if (memory->size > std::numeric_limits<int64_t>::max()) {
        return NN_ERROR() << "Memory size doesn't fit into int64_t.";
    }
    if (const auto* handle = std::get_if<nn::Handle>(&memory->handle)) {
        return Memory{
                .handle = NN_TRY(unvalidatedConvert(*handle)),
                .size = static_cast<int64_t>(memory->size),
                .name = memory->name,
        };
    }

    const auto* ahwb = std::get<nn::HardwareBufferHandle>(memory->handle).get();
    AHardwareBuffer_Desc bufferDesc;
    AHardwareBuffer_describe(ahwb, &bufferDesc);

    if (bufferDesc.format == AHARDWAREBUFFER_FORMAT_BLOB) {
        CHECK_EQ(memory->size, bufferDesc.width);
        CHECK_EQ(memory->name, "hardware_buffer_blob");
    } else {
        CHECK_EQ(memory->size, 0u);
        CHECK_EQ(memory->name, "hardware_buffer");
    }

    const native_handle_t* nativeHandle = AHardwareBuffer_getNativeHandle(ahwb);
    if (nativeHandle == nullptr) {
        return NN_ERROR() << "unvalidatedConvert failed because AHardwareBuffer_getNativeHandle "
                             "returned nullptr";
    }

    return Memory{
            .handle = NN_TRY(aidlHandleFromNativeHandle(*nativeHandle)),
            .size = static_cast<int64_t>(memory->size),
            .name = memory->name,
    };
}

nn::GeneralResult<ErrorStatus> unvalidatedConvert(const nn::ErrorStatus& errorStatus) {
    switch (errorStatus) {
        case nn::ErrorStatus::NONE:
        case nn::ErrorStatus::DEVICE_UNAVAILABLE:
        case nn::ErrorStatus::GENERAL_FAILURE:
        case nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE:
        case nn::ErrorStatus::INVALID_ARGUMENT:
        case nn::ErrorStatus::MISSED_DEADLINE_TRANSIENT:
        case nn::ErrorStatus::MISSED_DEADLINE_PERSISTENT:
        case nn::ErrorStatus::RESOURCE_EXHAUSTED_TRANSIENT:
        case nn::ErrorStatus::RESOURCE_EXHAUSTED_PERSISTENT:
            return static_cast<ErrorStatus>(errorStatus);
        default:
            return ErrorStatus::GENERAL_FAILURE;
    }
}

nn::GeneralResult<OutputShape> unvalidatedConvert(const nn::OutputShape& outputShape) {
    return OutputShape{.dimensions = NN_TRY(toSigned(outputShape.dimensions)),
                       .isSufficient = outputShape.isSufficient};
}

nn::GeneralResult<Memory> convert(const nn::SharedMemory& memory) {
    return validatedConvert(memory);
}

nn::GeneralResult<ErrorStatus> convert(const nn::ErrorStatus& errorStatus) {
    return validatedConvert(errorStatus);
}

nn::GeneralResult<std::vector<OutputShape>> convert(
        const std::vector<nn::OutputShape>& outputShapes) {
    return validatedConvert(outputShapes);
}

nn::GeneralResult<std::vector<int32_t>> toSigned(const std::vector<uint32_t>& vec) {
    if (!std::all_of(vec.begin(), vec.end(),
                     [](uint32_t v) { return v <= std::numeric_limits<int32_t>::max(); })) {
        return NN_ERROR() << "Vector contains a value that doesn't fit into int32_t.";
    }
    return std::vector<int32_t>(vec.begin(), vec.end());
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
