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
#include <android-base/unique_fd.h>
#include <android/binder_auto_utils.h>
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

#define VERIFY_LE_INT32_MAX(value) \
    while (UNLIKELY(value > std::numeric_limits<int32_t>::max())) return NN_ERROR()

namespace {
template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

constexpr auto kVersion = android::nn::Version::ANDROID_S;
constexpr int64_t kNoTiming = -1;

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
        auto duplicatedFd = NN_TRY(dupFd(fd.get()));
        fds.emplace_back(duplicatedFd.release());
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

static GeneralResult<UniqueNativeHandle> nativeHandleFromAidlHandle(const NativeHandle& handle) {
    std::vector<base::unique_fd> fds;
    fds.reserve(handle.fds.size());
    for (const auto& fd : handle.fds) {
        auto duplicatedFd = NN_TRY(dupFd(fd.get()));
        fds.emplace_back(duplicatedFd.release());
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
    if (location.offset > std::numeric_limits<uint32_t>::max()) {
        return NN_ERROR() << "DataLocation: offset must be <= std::numeric_limits<uint32_t>::max()";
    }
    if (location.length > std::numeric_limits<uint32_t>::max()) {
        return NN_ERROR() << "DataLocation: length must be <= std::numeric_limits<uint32_t>::max()";
    }
    return DataLocation{
            .poolIndex = static_cast<uint32_t>(location.poolIndex),
            .offset = static_cast<uint32_t>(location.offset),
            .length = static_cast<uint32_t>(location.length),
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
    if (memory.size > std::numeric_limits<size_t>::max()) {
        return NN_ERROR() << "Memory: size must be <= std::numeric_limits<size_t>::max()";
    }

    if (memory.name != "hardware_buffer_blob") {
        return std::make_shared<const Memory>(Memory{
                .handle = NN_TRY(unvalidatedConvertHelper(memory.handle)),
                .size = static_cast<size_t>(memory.size),
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
            .size = static_cast<size_t>(memory.size),
            .name = memory.name,
    });
}

GeneralResult<Timing> unvalidatedConvert(const aidl_hal::Timing& timing) {
    if (timing.timeInDriver < -1) {
        return NN_ERROR() << "Timing: timeInDriver must not be less than -1";
    }
    if (timing.timeOnDevice < -1) {
        return NN_ERROR() << "Timing: timeOnDevice must not be less than -1";
    }
    constexpr auto convertTiming = [](int64_t halTiming) -> OptionalDuration {
        if (halTiming == kNoTiming) {
            return {};
        }
        return nn::Duration(static_cast<uint64_t>(halTiming));
    };
    return Timing{.timeOnDevice = convertTiming(timing.timeOnDevice),
                  .timeInDriver = convertTiming(timing.timeInDriver)};
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

GeneralResult<SyncFence> unvalidatedConvert(const ndk::ScopedFileDescriptor& syncFence) {
    auto duplicatedFd = NN_TRY(dupFd(syncFence.get()));
    return SyncFence::create(std::move(duplicatedFd));
}

GeneralResult<Capabilities> convert(const aidl_hal::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

GeneralResult<DeviceType> convert(const aidl_hal::DeviceType& deviceType) {
    return validatedConvert(deviceType);
}

GeneralResult<ErrorStatus> convert(const aidl_hal::ErrorStatus& errorStatus) {
    return validatedConvert(errorStatus);
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

GeneralResult<Timing> convert(const aidl_hal::Timing& timing) {
    return validatedConvert(timing);
}

GeneralResult<SyncFence> convert(const ndk::ScopedFileDescriptor& syncFence) {
    return unvalidatedConvert(syncFence);
}

GeneralResult<std::vector<Extension>> convert(const std::vector<aidl_hal::Extension>& extension) {
    return validatedConvert(extension);
}

GeneralResult<std::vector<Operation>> convert(const std::vector<aidl_hal::Operation>& operations) {
    return unvalidatedConvert(operations);
}

GeneralResult<std::vector<SharedMemory>> convert(const std::vector<aidl_hal::Memory>& memories) {
    return validatedConvert(memories);
}

GeneralResult<std::vector<OutputShape>> convert(
        const std::vector<aidl_hal::OutputShape>& outputShapes) {
    return validatedConvert(outputShapes);
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
    std::vector<UnvalidatedConvertOutput<Type>> halObject;
    halObject.reserve(arguments.size());
    for (const auto& argument : arguments) {
        halObject.push_back(NN_TRY(unvalidatedConvert(argument)));
    }
    return halObject;
}

template <typename Type>
nn::GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> unvalidatedConvert(
        const std::vector<Type>& arguments) {
    return unvalidatedConvertVec(arguments);
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
        auto duplicatedFd = NN_TRY(nn::dupFd(fd.get()));
        aidlNativeHandle.fds.emplace_back(duplicatedFd.release());
    }
    aidlNativeHandle.ints = handle.ints;
    return aidlNativeHandle;
}

// Helper template for std::visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

static nn::GeneralResult<common::NativeHandle> aidlHandleFromNativeHandle(
        const native_handle_t& handle) {
    common::NativeHandle aidlNativeHandle;

    aidlNativeHandle.fds.reserve(handle.numFds);
    for (int i = 0; i < handle.numFds; ++i) {
        auto duplicatedFd = NN_TRY(nn::dupFd(handle.data[i]));
        aidlNativeHandle.fds.emplace_back(duplicatedFd.release());
    }

    aidlNativeHandle.ints = std::vector<int>(&handle.data[handle.numFds],
                                             &handle.data[handle.numFds + handle.numInts]);

    return aidlNativeHandle;
}

}  // namespace

nn::GeneralResult<std::vector<uint8_t>> unvalidatedConvert(const nn::CacheToken& cacheToken) {
    return std::vector<uint8_t>(cacheToken.begin(), cacheToken.end());
}

nn::GeneralResult<BufferDesc> unvalidatedConvert(const nn::BufferDesc& bufferDesc) {
    return BufferDesc{.dimensions = NN_TRY(toSigned(bufferDesc.dimensions))};
}

nn::GeneralResult<BufferRole> unvalidatedConvert(const nn::BufferRole& bufferRole) {
    VERIFY_LE_INT32_MAX(bufferRole.modelIndex)
            << "BufferRole: modelIndex must be <= std::numeric_limits<int32_t>::max()";
    VERIFY_LE_INT32_MAX(bufferRole.ioIndex)
            << "BufferRole: ioIndex must be <= std::numeric_limits<int32_t>::max()";
    return BufferRole{
            .modelIndex = static_cast<int32_t>(bufferRole.modelIndex),
            .ioIndex = static_cast<int32_t>(bufferRole.ioIndex),
            .frequency = bufferRole.frequency,
    };
}

nn::GeneralResult<bool> unvalidatedConvert(const nn::MeasureTiming& measureTiming) {
    return measureTiming == nn::MeasureTiming::YES;
}

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

nn::GeneralResult<ExecutionPreference> unvalidatedConvert(
        const nn::ExecutionPreference& executionPreference) {
    return static_cast<ExecutionPreference>(executionPreference);
}

nn::GeneralResult<OperandType> unvalidatedConvert(const nn::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

nn::GeneralResult<OperandLifeTime> unvalidatedConvert(
        const nn::Operand::LifeTime& operandLifeTime) {
    return static_cast<OperandLifeTime>(operandLifeTime);
}

nn::GeneralResult<DataLocation> unvalidatedConvert(const nn::DataLocation& location) {
    VERIFY_LE_INT32_MAX(location.poolIndex)
            << "DataLocation: pool index must be <= std::numeric_limits<int32_t>::max()";
    return DataLocation{
            .poolIndex = static_cast<int32_t>(location.poolIndex),
            .offset = static_cast<int64_t>(location.offset),
            .length = static_cast<int64_t>(location.length),
    };
}

nn::GeneralResult<std::optional<OperandExtraParams>> unvalidatedConvert(
        const nn::Operand::ExtraParams& extraParams) {
    return std::visit(
            overloaded{
                    [](const nn::Operand::NoParams&)
                            -> nn::GeneralResult<std::optional<OperandExtraParams>> {
                        return std::nullopt;
                    },
                    [](const nn::Operand::SymmPerChannelQuantParams& symmPerChannelQuantParams)
                            -> nn::GeneralResult<std::optional<OperandExtraParams>> {
                        if (symmPerChannelQuantParams.channelDim >
                            std::numeric_limits<int32_t>::max()) {
                            // Using explicit type conversion because std::optional in successful
                            // result confuses the compiler.
                            return (NN_ERROR() << "symmPerChannelQuantParams.channelDim must be <= "
                                                  "std::numeric_limits<int32_t>::max(), received: "
                                               << symmPerChannelQuantParams.channelDim)
                                    .
                                    operator nn::GeneralResult<std::optional<OperandExtraParams>>();
                        }
                        return OperandExtraParams::make<OperandExtraParams::Tag::channelQuant>(
                                SymmPerChannelQuantParams{
                                        .scales = symmPerChannelQuantParams.scales,
                                        .channelDim = static_cast<int32_t>(
                                                symmPerChannelQuantParams.channelDim),
                                });
                    },
                    [](const nn::Operand::ExtensionParams& extensionParams)
                            -> nn::GeneralResult<std::optional<OperandExtraParams>> {
                        return OperandExtraParams::make<OperandExtraParams::Tag::extension>(
                                extensionParams);
                    },
            },
            extraParams);
}

nn::GeneralResult<Operand> unvalidatedConvert(const nn::Operand& operand) {
    return Operand{
            .type = NN_TRY(unvalidatedConvert(operand.type)),
            .dimensions = NN_TRY(toSigned(operand.dimensions)),
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(unvalidatedConvert(operand.lifetime)),
            .location = NN_TRY(unvalidatedConvert(operand.location)),
            .extraParams = NN_TRY(unvalidatedConvert(operand.extraParams)),
    };
}

nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = NN_TRY(toSigned(operation.inputs)),
            .outputs = NN_TRY(toSigned(operation.outputs)),
    };
}

nn::GeneralResult<Subgraph> unvalidatedConvert(const nn::Model::Subgraph& subgraph) {
    return Subgraph{
            .operands = NN_TRY(unvalidatedConvert(subgraph.operands)),
            .operations = NN_TRY(unvalidatedConvert(subgraph.operations)),
            .inputIndexes = NN_TRY(toSigned(subgraph.inputIndexes)),
            .outputIndexes = NN_TRY(toSigned(subgraph.outputIndexes)),
    };
}

nn::GeneralResult<std::vector<uint8_t>> unvalidatedConvert(
        const nn::Model::OperandValues& operandValues) {
    return std::vector<uint8_t>(operandValues.data(), operandValues.data() + operandValues.size());
}

nn::GeneralResult<ExtensionNameAndPrefix> unvalidatedConvert(
        const nn::Model::ExtensionNameAndPrefix& extensionNameToPrefix) {
    return ExtensionNameAndPrefix{
            .name = extensionNameToPrefix.name,
            .prefix = extensionNameToPrefix.prefix,
    };
}

nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model) {
    return Model{
            .main = NN_TRY(unvalidatedConvert(model.main)),
            .referenced = NN_TRY(unvalidatedConvert(model.referenced)),
            .operandValues = NN_TRY(unvalidatedConvert(model.operandValues)),
            .pools = NN_TRY(unvalidatedConvert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(unvalidatedConvert(model.extensionNameToPrefix)),
    };
}

nn::GeneralResult<Priority> unvalidatedConvert(const nn::Priority& priority) {
    return static_cast<Priority>(priority);
}

nn::GeneralResult<Request> unvalidatedConvert(const nn::Request& request) {
    return Request{
            .inputs = NN_TRY(unvalidatedConvert(request.inputs)),
            .outputs = NN_TRY(unvalidatedConvert(request.outputs)),
            .pools = NN_TRY(unvalidatedConvert(request.pools)),
    };
}

nn::GeneralResult<RequestArgument> unvalidatedConvert(
        const nn::Request::Argument& requestArgument) {
    if (requestArgument.lifetime == nn::Request::Argument::LifeTime::POINTER) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Request cannot be unvalidatedConverted because it contains pointer-based memory";
    }
    const bool hasNoValue = requestArgument.lifetime == nn::Request::Argument::LifeTime::NO_VALUE;
    return RequestArgument{
            .hasNoValue = hasNoValue,
            .location = NN_TRY(unvalidatedConvert(requestArgument.location)),
            .dimensions = NN_TRY(toSigned(requestArgument.dimensions)),
    };
}

nn::GeneralResult<RequestMemoryPool> unvalidatedConvert(const nn::Request::MemoryPool& memoryPool) {
    return std::visit(
            overloaded{
                    [](const nn::SharedMemory& memory) -> nn::GeneralResult<RequestMemoryPool> {
                        return RequestMemoryPool::make<RequestMemoryPool::Tag::pool>(
                                NN_TRY(unvalidatedConvert(memory)));
                    },
                    [](const nn::Request::MemoryDomainToken& token)
                            -> nn::GeneralResult<RequestMemoryPool> {
                        return RequestMemoryPool::make<RequestMemoryPool::Tag::token>(
                                underlyingType(token));
                    },
                    [](const nn::SharedBuffer& /*buffer*/) {
                        return (NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                                << "Unable to make memory pool from IBuffer")
                                .
                                operator nn::GeneralResult<RequestMemoryPool>();
                    },
            },
            memoryPool);
}

nn::GeneralResult<Timing> unvalidatedConvert(const nn::Timing& timing) {
    return Timing{
            .timeOnDevice = NN_TRY(unvalidatedConvert(timing.timeOnDevice)),
            .timeInDriver = NN_TRY(unvalidatedConvert(timing.timeInDriver)),
    };
}

nn::GeneralResult<int64_t> unvalidatedConvert(const nn::Duration& duration) {
    const uint64_t nanoseconds = duration.count();
    if (nanoseconds > std::numeric_limits<int64_t>::max()) {
        return std::numeric_limits<int64_t>::max();
    }
    return static_cast<int64_t>(nanoseconds);
}

nn::GeneralResult<int64_t> unvalidatedConvert(const nn::OptionalDuration& optionalDuration) {
    if (!optionalDuration.has_value()) {
        return kNoTiming;
    }
    return unvalidatedConvert(optionalDuration.value());
}

nn::GeneralResult<int64_t> unvalidatedConvert(const nn::OptionalTimePoint& optionalTimePoint) {
    if (!optionalTimePoint.has_value()) {
        return kNoTiming;
    }
    return unvalidatedConvert(optionalTimePoint->time_since_epoch());
}

nn::GeneralResult<ndk::ScopedFileDescriptor> unvalidatedConvert(const nn::SyncFence& syncFence) {
    auto duplicatedFd = NN_TRY(nn::dupFd(syncFence.getFd()));
    return ndk::ScopedFileDescriptor(duplicatedFd.release());
}

nn::GeneralResult<ndk::ScopedFileDescriptor> unvalidatedConvertCache(
        const nn::SharedHandle& handle) {
    if (handle->ints.size() != 0) {
        NN_ERROR() << "Cache handle must not contain ints";
    }
    if (handle->fds.size() != 1) {
        NN_ERROR() << "Cache handle must contain exactly one fd but contains "
                   << handle->fds.size();
    }
    auto duplicatedFd = NN_TRY(nn::dupFd(handle->fds.front().get()));
    return ndk::ScopedFileDescriptor(duplicatedFd.release());
}

nn::GeneralResult<std::vector<uint8_t>> convert(const nn::CacheToken& cacheToken) {
    return unvalidatedConvert(cacheToken);
}

nn::GeneralResult<BufferDesc> convert(const nn::BufferDesc& bufferDesc) {
    return validatedConvert(bufferDesc);
}

nn::GeneralResult<bool> convert(const nn::MeasureTiming& measureTiming) {
    return validatedConvert(measureTiming);
}

nn::GeneralResult<Memory> convert(const nn::SharedMemory& memory) {
    return validatedConvert(memory);
}

nn::GeneralResult<ErrorStatus> convert(const nn::ErrorStatus& errorStatus) {
    return validatedConvert(errorStatus);
}

nn::GeneralResult<ExecutionPreference> convert(const nn::ExecutionPreference& executionPreference) {
    return validatedConvert(executionPreference);
}

nn::GeneralResult<Model> convert(const nn::Model& model) {
    return validatedConvert(model);
}

nn::GeneralResult<Priority> convert(const nn::Priority& priority) {
    return validatedConvert(priority);
}

nn::GeneralResult<Request> convert(const nn::Request& request) {
    return validatedConvert(request);
}

nn::GeneralResult<Timing> convert(const nn::Timing& timing) {
    return validatedConvert(timing);
}

nn::GeneralResult<int64_t> convert(const nn::OptionalDuration& optionalDuration) {
    return validatedConvert(optionalDuration);
}

nn::GeneralResult<int64_t> convert(const nn::OptionalTimePoint& outputShapes) {
    return validatedConvert(outputShapes);
}

nn::GeneralResult<std::vector<BufferRole>> convert(const std::vector<nn::BufferRole>& bufferRoles) {
    return validatedConvert(bufferRoles);
}

nn::GeneralResult<std::vector<OutputShape>> convert(
        const std::vector<nn::OutputShape>& outputShapes) {
    return validatedConvert(outputShapes);
}

nn::GeneralResult<std::vector<ndk::ScopedFileDescriptor>> convert(
        const std::vector<nn::SharedHandle>& cacheHandles) {
    const auto version = NN_TRY(hal::utils::makeGeneralFailure(nn::validate(cacheHandles)));
    if (version > kVersion) {
        return NN_ERROR() << "Insufficient version: " << version << " vs required " << kVersion;
    }
    std::vector<ndk::ScopedFileDescriptor> cacheFds;
    cacheFds.reserve(cacheHandles.size());
    for (const auto& cacheHandle : cacheHandles) {
        cacheFds.push_back(NN_TRY(unvalidatedConvertCache(cacheHandle)));
    }
    return cacheFds;
}

nn::GeneralResult<std::vector<ndk::ScopedFileDescriptor>> convert(
        const std::vector<nn::SyncFence>& syncFences) {
    return unvalidatedConvert(syncFences);
}

nn::GeneralResult<std::vector<int32_t>> toSigned(const std::vector<uint32_t>& vec) {
    if (!std::all_of(vec.begin(), vec.end(),
                     [](uint32_t v) { return v <= std::numeric_limits<int32_t>::max(); })) {
        return NN_ERROR() << "Vector contains a value that doesn't fit into int32_t.";
    }
    return std::vector<int32_t>(vec.begin(), vec.end());
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
