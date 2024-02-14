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

#include <aidl/android/hardware/common/Ashmem.h>
#include <aidl/android/hardware/common/MappableFile.h>
#include <aidl/android/hardware/common/NativeHandle.h>
#include <aidl/android/hardware/graphics/common/HardwareBuffer.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <android-base/mapped_file.h>
#include <android-base/unique_fd.h>
#include <android/binder_auto_utils.h>
#include <cutils/native_handle.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/CommonUtils.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

#include "Utils.h"

#ifdef __ANDROID__
#include <android/hardware_buffer.h>
#include <vndk/hardware_buffer.h>
#endif  // __ANDROID__

#define VERIFY_NON_NEGATIVE(value) \
    while (UNLIKELY(value < 0)) return NN_ERROR()

#define VERIFY_LE_INT32_MAX(value) \
    while (UNLIKELY(value > std::numeric_limits<int32_t>::max())) return NN_ERROR()

namespace {

constexpr int64_t kNoTiming = -1;

}  // namespace

namespace android::nn {
namespace {

using ::aidl::android::hardware::common::NativeHandle;
using ::aidl::android::hardware::neuralnetworks::utils::underlyingType;

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
    NN_TRY(aidl_hal::utils::compliantVersion(canonical));
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

struct NativeHandleDeleter {
    void operator()(native_handle_t* handle) const {
        if (handle) {
            native_handle_close(handle);
            native_handle_delete(handle);
        }
    }
};

using UniqueNativeHandle = std::unique_ptr<native_handle_t, NativeHandleDeleter>;

#ifdef __ANDROID__
GeneralResult<UniqueNativeHandle> nativeHandleFromAidlHandle(const NativeHandle& handle) {
    auto nativeHandle = UniqueNativeHandle(dupFromAidl(handle));
    if (nativeHandle.get() == nullptr) {
        return NN_ERROR() << "android::dupFromAidl failed to convert the common::NativeHandle to a "
                             "native_handle_t";
    }
    if (!std::all_of(nativeHandle->data + 0, nativeHandle->data + nativeHandle->numFds,
                     [](int fd) { return fd >= 0; })) {
        return NN_ERROR() << "android::dupFromAidl returned an invalid native_handle_t";
    }
    return nativeHandle;
}
#endif  // __ANDROID__

}  // anonymous namespace

GeneralResult<OperandType> unvalidatedConvert(const aidl_hal::OperandType& operandType) {
    VERIFY_NON_NEGATIVE(underlyingType(operandType)) << "Negative operand types are not allowed.";
    const auto canonical = static_cast<OperandType>(operandType);
    if (canonical == OperandType::OEM || canonical == OperandType::TENSOR_OEM_BYTE) {
        return NN_ERROR() << "Unable to convert invalid OperandType " << canonical;
    }
    return canonical;
}

GeneralResult<OperationType> unvalidatedConvert(const aidl_hal::OperationType& operationType) {
    VERIFY_NON_NEGATIVE(underlyingType(operationType))
            << "Negative operation types are not allowed.";
    const auto canonical = static_cast<OperationType>(operationType);
    if (canonical == OperationType::OEM_OPERATION) {
        return NN_ERROR() << "Unable to convert invalid OperationType OEM_OPERATION";
    }
    return canonical;
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
                return validatedConvert(operandPerformance.type).has_value();
            });
    if (!validOperandTypes) {
        return NN_ERROR() << "Invalid OperandType when unvalidatedConverting OperandPerformance in "
                             "Capabilities";
    }

    auto operandPerformance = NN_TRY(unvalidatedConvert(capabilities.operandPerformance));
    auto table =
            NN_TRY(Capabilities::OperandPerformanceTable::create(std::move(operandPerformance)));

    const auto relaxedFloat32toFloat16PerformanceScalar =
            NN_TRY(unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceScalar));
    const auto relaxedFloat32toFloat16PerformanceTensor =
            NN_TRY(unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceTensor));
    const auto ifPerformance = NN_TRY(unvalidatedConvert(capabilities.ifPerformance));
    const auto whilePerformance = NN_TRY(unvalidatedConvert(capabilities.whilePerformance));
    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = relaxedFloat32toFloat16PerformanceScalar,
            .relaxedFloat32toFloat16PerformanceTensor = relaxedFloat32toFloat16PerformanceTensor,
            .operandPerformance = std::move(table),
            .ifPerformance = ifPerformance,
            .whilePerformance = whilePerformance,
    };
}

GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const aidl_hal::OperandPerformance& operandPerformance) {
    const auto type = NN_TRY(unvalidatedConvert(operandPerformance.type));
    const auto info = NN_TRY(unvalidatedConvert(operandPerformance.info));
    return Capabilities::OperandPerformance{
            .type = type,
            .info = info,
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
    const auto type = NN_TRY(unvalidatedConvert(operation.type));
    auto inputs = NN_TRY(toUnsigned(operation.inputs));
    auto outputs = NN_TRY(toUnsigned(operation.outputs));
    return Operation{
            .type = type,
            .inputs = std::move(inputs),
            .outputs = std::move(outputs),
    };
}

GeneralResult<Operand::LifeTime> unvalidatedConvert(
        const aidl_hal::OperandLifeTime& operandLifeTime) {
    return static_cast<Operand::LifeTime>(operandLifeTime);
}

GeneralResult<Operand> unvalidatedConvert(const aidl_hal::Operand& operand) {
    const auto type = NN_TRY(unvalidatedConvert(operand.type));
    auto dimensions = NN_TRY(toUnsigned(operand.dimensions));
    const auto lifetime = NN_TRY(unvalidatedConvert(operand.lifetime));
    const auto location = NN_TRY(unvalidatedConvert(operand.location));
    auto extraParams = NN_TRY(unvalidatedConvert(operand.extraParams));
    return Operand{
            .type = type,
            .dimensions = std::move(dimensions),
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = lifetime,
            .location = location,
            .extraParams = std::move(extraParams),
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
    auto main = NN_TRY(unvalidatedConvert(model.main));
    auto referenced = NN_TRY(unvalidatedConvert(model.referenced));
    auto operandValues = NN_TRY(unvalidatedConvert(model.operandValues));
    auto pools = NN_TRY(unvalidatedConvert(model.pools));
    auto extensionNameToPrefix = NN_TRY(unvalidatedConvert(model.extensionNameToPrefix));
    return Model{
            .main = std::move(main),
            .referenced = std::move(referenced),
            .operandValues = std::move(operandValues),
            .pools = std::move(pools),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = std::move(extensionNameToPrefix),
    };
}

GeneralResult<Model::Subgraph> unvalidatedConvert(const aidl_hal::Subgraph& subgraph) {
    auto operands = NN_TRY(unvalidatedConvert(subgraph.operands));
    auto operations = NN_TRY(unvalidatedConvert(subgraph.operations));
    auto inputIndexes = NN_TRY(toUnsigned(subgraph.inputIndexes));
    auto outputIndexes = NN_TRY(toUnsigned(subgraph.outputIndexes));
    return Model::Subgraph{
            .operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = std::move(inputIndexes),
            .outputIndexes = std::move(outputIndexes),
    };
}

GeneralResult<ExtensionNameAndPrefix> unvalidatedConvert(
        const aidl_hal::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return ExtensionNameAndPrefix{
            .name = extensionNameAndPrefix.name,
            .prefix = extensionNameAndPrefix.prefix,
    };
}

GeneralResult<Extension> unvalidatedConvert(const aidl_hal::Extension& extension) {
    auto operandTypes = NN_TRY(unvalidatedConvert(extension.operandTypes));
    return Extension{
            .name = extension.name,
            .operandTypes = std::move(operandTypes),
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
    auto dimensions = NN_TRY(toUnsigned(outputShape.dimensions));
    return OutputShape{
            .dimensions = std::move(dimensions),
            .isSufficient = outputShape.isSufficient,
    };
}

GeneralResult<MeasureTiming> unvalidatedConvert(bool measureTiming) {
    return measureTiming ? MeasureTiming::YES : MeasureTiming::NO;
}

GeneralResult<SharedMemory> unvalidatedConvert(const aidl_hal::Memory& memory) {
    using Tag = aidl_hal::Memory::Tag;
    switch (memory.getTag()) {
        case Tag::ashmem: {
            const auto& ashmem = memory.get<Tag::ashmem>();
            VERIFY_NON_NEGATIVE(ashmem.size) << "Memory size must not be negative";
            if (ashmem.size > std::numeric_limits<size_t>::max()) {
                return NN_ERROR() << "Memory: size must be <= std::numeric_limits<size_t>::max()";
            }

            auto fd = NN_TRY(dupFd(ashmem.fd.get()));
            auto handle = Memory::Ashmem{
                    .fd = std::move(fd),
                    .size = static_cast<size_t>(ashmem.size),
            };
            return std::make_shared<const Memory>(Memory{.handle = std::move(handle)});
        }
        case Tag::mappableFile: {
            const auto& mappableFile = memory.get<Tag::mappableFile>();
            VERIFY_NON_NEGATIVE(mappableFile.length) << "Memory size must not be negative";
            VERIFY_NON_NEGATIVE(mappableFile.offset) << "Memory offset must not be negative";
            if (mappableFile.length > std::numeric_limits<size_t>::max()) {
                return NN_ERROR() << "Memory: size must be <= std::numeric_limits<size_t>::max()";
            }
            if (mappableFile.offset > std::numeric_limits<size_t>::max()) {
                return NN_ERROR() << "Memory: offset must be <= std::numeric_limits<size_t>::max()";
            }

            const size_t size = static_cast<size_t>(mappableFile.length);
            const int prot = mappableFile.prot;
            const int fd = mappableFile.fd.get();
            const size_t offset = static_cast<size_t>(mappableFile.offset);

            return createSharedMemoryFromFd(size, prot, fd, offset);
        }
        case Tag::hardwareBuffer: {
#ifdef __ANDROID__
            const auto& hardwareBuffer = memory.get<Tag::hardwareBuffer>();

            const UniqueNativeHandle handle =
                    NN_TRY(nativeHandleFromAidlHandle(hardwareBuffer.handle));
            const native_handle_t* nativeHandle = handle.get();

            const AHardwareBuffer_Desc desc{
                    .width = static_cast<uint32_t>(hardwareBuffer.description.width),
                    .height = static_cast<uint32_t>(hardwareBuffer.description.height),
                    .layers = static_cast<uint32_t>(hardwareBuffer.description.layers),
                    .format = static_cast<uint32_t>(hardwareBuffer.description.format),
                    .usage = static_cast<uint64_t>(hardwareBuffer.description.usage),
                    .stride = static_cast<uint32_t>(hardwareBuffer.description.stride),
            };
            AHardwareBuffer* ahwb = nullptr;
            const status_t status = AHardwareBuffer_createFromHandle(
                    &desc, nativeHandle, AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE, &ahwb);
            if (status != NO_ERROR) {
                return NN_ERROR() << "createFromHandle failed";
            }

            return createSharedMemoryFromAHWB(ahwb, /*takeOwnership=*/true);
#else   // __ANDROID__
            LOG(ERROR) << "GeneralResult<SharedMemory> unvalidatedConvert(const aidl_hal::Memory& "
                          "memory): Not Available on Host Build";
            return NN_ERROR() << "createFromHandle failed";
#endif  // __ANDROID__
        }
    }
    return NN_ERROR() << "Unrecognized Memory::Tag: " << underlyingType(memory.getTag());
}

GeneralResult<Timing> unvalidatedConvert(const aidl_hal::Timing& timing) {
    if (timing.timeInDriverNs < -1) {
        return NN_ERROR() << "Timing: timeInDriverNs must not be less than -1";
    }
    if (timing.timeOnDeviceNs < -1) {
        return NN_ERROR() << "Timing: timeOnDeviceNs must not be less than -1";
    }
    constexpr auto convertTiming = [](int64_t halTiming) -> OptionalDuration {
        if (halTiming == kNoTiming) {
            return {};
        }
        return nn::Duration(static_cast<uint64_t>(halTiming));
    };
    return Timing{.timeOnDevice = convertTiming(timing.timeOnDeviceNs),
                  .timeInDriver = convertTiming(timing.timeInDriverNs)};
}

GeneralResult<Model::OperandValues> unvalidatedConvert(const std::vector<uint8_t>& operandValues) {
    return Model::OperandValues(operandValues.data(), operandValues.size());
}

GeneralResult<BufferDesc> unvalidatedConvert(const aidl_hal::BufferDesc& bufferDesc) {
    auto dimensions = NN_TRY(toUnsigned(bufferDesc.dimensions));
    return BufferDesc{.dimensions = std::move(dimensions)};
}

GeneralResult<BufferRole> unvalidatedConvert(const aidl_hal::BufferRole& bufferRole) {
    VERIFY_NON_NEGATIVE(bufferRole.modelIndex) << "BufferRole: modelIndex must not be negative";
    VERIFY_NON_NEGATIVE(bufferRole.ioIndex) << "BufferRole: ioIndex must not be negative";
    return BufferRole{
            .modelIndex = static_cast<uint32_t>(bufferRole.modelIndex),
            .ioIndex = static_cast<uint32_t>(bufferRole.ioIndex),
            .probability = bufferRole.probability,
    };
}

GeneralResult<Request> unvalidatedConvert(const aidl_hal::Request& request) {
    auto inputs = NN_TRY(unvalidatedConvert(request.inputs));
    auto outputs = NN_TRY(unvalidatedConvert(request.outputs));
    auto pools = NN_TRY(unvalidatedConvert(request.pools));
    return Request{
            .inputs = std::move(inputs),
            .outputs = std::move(outputs),
            .pools = std::move(pools),
    };
}

GeneralResult<Request::Argument> unvalidatedConvert(const aidl_hal::RequestArgument& argument) {
    const auto lifetime = argument.hasNoValue ? Request::Argument::LifeTime::NO_VALUE
                                              : Request::Argument::LifeTime::POOL;
    const auto location = NN_TRY(unvalidatedConvert(argument.location));
    auto dimensions = NN_TRY(toUnsigned(argument.dimensions));
    return Request::Argument{
            .lifetime = lifetime,
            .location = location,
            .dimensions = std::move(dimensions),
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

GeneralResult<std::vector<Operation>> unvalidatedConvert(
        const std::vector<aidl_hal::Operation>& operations) {
    return unvalidatedConvertVec(operations);
}

GeneralResult<SharedHandle> unvalidatedConvert(const ndk::ScopedFileDescriptor& handle) {
    auto duplicatedFd = NN_TRY(dupFd(handle.get()));
    return std::make_shared<const Handle>(std::move(duplicatedFd));
}

#ifdef NN_AIDL_V4_OR_ABOVE
GeneralResult<TokenValuePair> unvalidatedConvert(const aidl_hal::TokenValuePair& tokenValuePair) {
    return TokenValuePair{.token = tokenValuePair.token, .value = tokenValuePair.value};
}
#endif  // NN_AIDL_V4_OR_ABOVE

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

GeneralResult<OperandType> convert(const aidl_hal::OperandType& operandType) {
    return validatedConvert(operandType);
}

GeneralResult<Priority> convert(const aidl_hal::Priority& priority) {
    return validatedConvert(priority);
}

GeneralResult<Request> convert(const aidl_hal::Request& request) {
    return validatedConvert(request);
}

GeneralResult<Timing> convert(const aidl_hal::Timing& timing) {
    return validatedConvert(timing);
}

GeneralResult<SharedHandle> convert(const ndk::ScopedFileDescriptor& handle) {
    return validatedConvert(handle);
}

GeneralResult<BufferDesc> convert(const aidl_hal::BufferDesc& bufferDesc) {
    return validatedConvert(bufferDesc);
}

GeneralResult<std::vector<Extension>> convert(const std::vector<aidl_hal::Extension>& extension) {
    return validatedConvert(extension);
}

GeneralResult<std::vector<SharedMemory>> convert(const std::vector<aidl_hal::Memory>& memories) {
    return validatedConvert(memories);
}
GeneralResult<std::vector<ExtensionNameAndPrefix>> convert(
        const std::vector<aidl_hal::ExtensionNameAndPrefix>& extensionNameAndPrefix) {
    return unvalidatedConvert(extensionNameAndPrefix);
}

#ifdef NN_AIDL_V4_OR_ABOVE
GeneralResult<std::vector<TokenValuePair>> convert(
        const std::vector<aidl_hal::TokenValuePair>& metaData) {
    return validatedConvert(metaData);
}
#endif  // NN_AIDL_V4_OR_ABOVE

GeneralResult<std::vector<OutputShape>> convert(
        const std::vector<aidl_hal::OutputShape>& outputShapes) {
    return validatedConvert(outputShapes);
}

GeneralResult<std::vector<SharedHandle>> convert(
        const std::vector<ndk::ScopedFileDescriptor>& handles) {
    return validatedConvert(handles);
}

GeneralResult<std::vector<BufferRole>> convert(const std::vector<aidl_hal::BufferRole>& roles) {
    return validatedConvert(roles);
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

using utils::unvalidatedConvert;

// Helper template for std::visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#ifdef __ANDROID__
nn::GeneralResult<common::NativeHandle> aidlHandleFromNativeHandle(
        const native_handle_t& nativeHandle) {
    auto handle = ::android::dupToAidl(&nativeHandle);
    if (!std::all_of(handle.fds.begin(), handle.fds.end(),
                     [](const ndk::ScopedFileDescriptor& fd) { return fd.get() >= 0; })) {
        return NN_ERROR() << "android::dupToAidl returned an invalid common::NativeHandle";
    }
    return handle;
}
#endif  // __ANDROID__

nn::GeneralResult<Memory> unvalidatedConvert(const nn::Memory::Ashmem& memory) {
    if constexpr (std::numeric_limits<size_t>::max() > std::numeric_limits<int64_t>::max()) {
        if (memory.size > std::numeric_limits<int64_t>::max()) {
            return (
                           NN_ERROR()
                           << "Memory::Ashmem: size must be <= std::numeric_limits<int64_t>::max()")
                    .
                    operator nn::GeneralResult<Memory>();
        }
    }

    auto fd = NN_TRY(nn::dupFd(memory.fd));
    auto handle = common::Ashmem{
            .fd = ndk::ScopedFileDescriptor(fd.release()),
            .size = static_cast<int64_t>(memory.size),
    };
    return Memory::make<Memory::Tag::ashmem>(std::move(handle));
}

nn::GeneralResult<Memory> unvalidatedConvert(const nn::Memory::Fd& memory) {
    if constexpr (std::numeric_limits<size_t>::max() > std::numeric_limits<int64_t>::max()) {
        if (memory.size > std::numeric_limits<int64_t>::max()) {
            return (NN_ERROR() << "Memory::Fd: size must be <= std::numeric_limits<int64_t>::max()")
                    .
                    operator nn::GeneralResult<Memory>();
        }
        if (memory.offset > std::numeric_limits<int64_t>::max()) {
            return (
                           NN_ERROR()
                           << "Memory::Fd: offset must be <= std::numeric_limits<int64_t>::max()")
                    .
                    operator nn::GeneralResult<Memory>();
        }
    }

    auto fd = NN_TRY(nn::dupFd(memory.fd));
    auto handle = common::MappableFile{
            .length = static_cast<int64_t>(memory.size),
            .prot = memory.prot,
            .fd = ndk::ScopedFileDescriptor(fd.release()),
            .offset = static_cast<int64_t>(memory.offset),
    };
    return Memory::make<Memory::Tag::mappableFile>(std::move(handle));
}

nn::GeneralResult<Memory> unvalidatedConvert(const nn::Memory::HardwareBuffer& memory) {
#ifdef __ANDROID__
    const native_handle_t* nativeHandle = AHardwareBuffer_getNativeHandle(memory.handle.get());
    if (nativeHandle == nullptr) {
        return (NN_ERROR() << "unvalidatedConvert failed because AHardwareBuffer_getNativeHandle "
                              "returned nullptr")
                .
                operator nn::GeneralResult<Memory>();
    }

    auto handle = NN_TRY(aidlHandleFromNativeHandle(*nativeHandle));

    AHardwareBuffer_Desc desc;
    AHardwareBuffer_describe(memory.handle.get(), &desc);

    const auto description = graphics::common::HardwareBufferDescription{
            .width = static_cast<int32_t>(desc.width),
            .height = static_cast<int32_t>(desc.height),
            .layers = static_cast<int32_t>(desc.layers),
            .format = static_cast<graphics::common::PixelFormat>(desc.format),
            .usage = static_cast<graphics::common::BufferUsage>(desc.usage),
            .stride = static_cast<int32_t>(desc.stride),
    };

    auto hardwareBuffer = graphics::common::HardwareBuffer{
            .description = std::move(description),
            .handle = std::move(handle),
    };
    return Memory::make<Memory::Tag::hardwareBuffer>(std::move(hardwareBuffer));
#else   // __ANDROID__
    LOG(ERROR) << "nn::GeneralResult<Memory> unvalidatedConvert(const nn::Memory::HardwareBuffer& "
                  "memory): Not Available on Host Build";
    (void)memory;
    return (NN_ERROR() << "unvalidatedConvert failed").operator nn::GeneralResult<Memory>();
#endif  // __ANDROID__
}

nn::GeneralResult<Memory> unvalidatedConvert(const nn::Memory::Unknown& /*memory*/) {
    return (NN_ERROR() << "Unable to convert Unknown memory type")
            .
            operator nn::GeneralResult<Memory>();
}

nn::GeneralResult<PerformanceInfo> unvalidatedConvert(
        const nn::Capabilities::PerformanceInfo& info) {
    return PerformanceInfo{.execTime = info.execTime, .powerUsage = info.powerUsage};
}

nn::GeneralResult<OperandPerformance> unvalidatedConvert(
        const nn::Capabilities::OperandPerformance& operandPerformance) {
    const auto type = NN_TRY(unvalidatedConvert(operandPerformance.type));
    const auto info = NN_TRY(unvalidatedConvert(operandPerformance.info));
    return OperandPerformance{.type = type, .info = info};
}

nn::GeneralResult<std::vector<OperandPerformance>> unvalidatedConvert(
        const nn::Capabilities::OperandPerformanceTable& table) {
    std::vector<OperandPerformance> operandPerformances;
    operandPerformances.reserve(table.asVector().size());
    for (const auto& operandPerformance : table.asVector()) {
        operandPerformances.push_back(NN_TRY(unvalidatedConvert(operandPerformance)));
    }
    return operandPerformances;
}

nn::GeneralResult<ExtensionOperandTypeInformation> unvalidatedConvert(
        const nn::Extension::OperandTypeInformation& info) {
    return ExtensionOperandTypeInformation{.type = info.type,
                                           .isTensor = info.isTensor,
                                           .byteSize = static_cast<int32_t>(info.byteSize)};
}

nn::GeneralResult<int64_t> unvalidatedConvert(const nn::Duration& duration) {
    if (duration < nn::Duration::zero()) {
        return NN_ERROR() << "Unable to convert invalid (negative) duration";
    }
    constexpr std::chrono::nanoseconds::rep kIntMax = std::numeric_limits<int64_t>::max();
    const auto count = duration.count();
    return static_cast<int64_t>(std::min(count, kIntMax));
}

template <typename Input>
using UnvalidatedConvertOutput =
        std::decay_t<decltype(unvalidatedConvert(std::declval<Input>()).value())>;

template <typename Type>
nn::GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> unvalidatedConvert(
        const std::vector<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> halObject;
    halObject.reserve(arguments.size());
    for (const auto& argument : arguments) {
        halObject.push_back(NN_TRY(unvalidatedConvert(argument)));
    }
    return halObject;
}

template <typename Type>
nn::GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& canonical) {
    NN_TRY(compliantVersion(canonical));
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

}  // namespace

nn::GeneralResult<std::vector<uint8_t>> unvalidatedConvert(const nn::CacheToken& cacheToken) {
    return std::vector<uint8_t>(cacheToken.begin(), cacheToken.end());
}

nn::GeneralResult<BufferDesc> unvalidatedConvert(const nn::BufferDesc& bufferDesc) {
    auto dimensions = NN_TRY(toSigned(bufferDesc.dimensions));
    return BufferDesc{.dimensions = std::move(dimensions)};
}

nn::GeneralResult<BufferRole> unvalidatedConvert(const nn::BufferRole& bufferRole) {
    VERIFY_LE_INT32_MAX(bufferRole.modelIndex)
            << "BufferRole: modelIndex must be <= std::numeric_limits<int32_t>::max()";
    VERIFY_LE_INT32_MAX(bufferRole.ioIndex)
            << "BufferRole: ioIndex must be <= std::numeric_limits<int32_t>::max()";
    return BufferRole{
            .modelIndex = static_cast<int32_t>(bufferRole.modelIndex),
            .ioIndex = static_cast<int32_t>(bufferRole.ioIndex),
            .probability = bufferRole.probability,
    };
}

nn::GeneralResult<DeviceType> unvalidatedConvert(const nn::DeviceType& deviceType) {
    switch (deviceType) {
        case nn::DeviceType::UNKNOWN:
            break;
        case nn::DeviceType::OTHER:
        case nn::DeviceType::CPU:
        case nn::DeviceType::GPU:
        case nn::DeviceType::ACCELERATOR:
            return static_cast<DeviceType>(deviceType);
    }
    return NN_ERROR() << "Invalid DeviceType " << deviceType;
}

nn::GeneralResult<bool> unvalidatedConvert(const nn::MeasureTiming& measureTiming) {
    return measureTiming == nn::MeasureTiming::YES;
}

nn::GeneralResult<Memory> unvalidatedConvert(const nn::SharedMemory& memory) {
    if (memory == nullptr) {
        return (NN_ERROR() << "Unable to convert nullptr memory")
                .
                operator nn::GeneralResult<Memory>();
    }
    return std::visit([](const auto& x) { return unvalidatedConvert(x); }, memory->handle);
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
    auto dimensions = NN_TRY(toSigned(outputShape.dimensions));
    return OutputShape{.dimensions = std::move(dimensions),
                       .isSufficient = outputShape.isSufficient};
}

nn::GeneralResult<ExecutionPreference> unvalidatedConvert(
        const nn::ExecutionPreference& executionPreference) {
    return static_cast<ExecutionPreference>(executionPreference);
}

nn::GeneralResult<OperandType> unvalidatedConvert(const nn::OperandType& operandType) {
    if (operandType == nn::OperandType::OEM || operandType == nn::OperandType::TENSOR_OEM_BYTE) {
        return NN_ERROR() << "Unable to convert invalid OperandType " << operandType;
    }
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
    const auto type = NN_TRY(unvalidatedConvert(operand.type));
    auto dimensions = NN_TRY(toSigned(operand.dimensions));
    const auto lifetime = NN_TRY(unvalidatedConvert(operand.lifetime));
    const auto location = NN_TRY(unvalidatedConvert(operand.location));
    auto extraParams = NN_TRY(unvalidatedConvert(operand.extraParams));
    return Operand{
            .type = type,
            .dimensions = std::move(dimensions),
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = lifetime,
            .location = location,
            .extraParams = std::move(extraParams),
    };
}

nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType) {
    if (operationType == nn::OperationType::OEM_OPERATION) {
        return NN_ERROR() << "Unable to convert invalid OperationType OEM_OPERATION";
    }
    return static_cast<OperationType>(operationType);
}

nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation) {
    const auto type = NN_TRY(unvalidatedConvert(operation.type));
    auto inputs = NN_TRY(toSigned(operation.inputs));
    auto outputs = NN_TRY(toSigned(operation.outputs));
    return Operation{
            .type = type,
            .inputs = std::move(inputs),
            .outputs = std::move(outputs),
    };
}

nn::GeneralResult<Subgraph> unvalidatedConvert(const nn::Model::Subgraph& subgraph) {
    auto operands = NN_TRY(unvalidatedConvert(subgraph.operands));
    auto operations = NN_TRY(unvalidatedConvert(subgraph.operations));
    auto inputIndexes = NN_TRY(toSigned(subgraph.inputIndexes));
    auto outputIndexes = NN_TRY(toSigned(subgraph.outputIndexes));
    return Subgraph{
            .operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = std::move(inputIndexes),
            .outputIndexes = std::move(outputIndexes),
    };
}

nn::GeneralResult<std::vector<uint8_t>> unvalidatedConvert(
        const nn::Model::OperandValues& operandValues) {
    return std::vector<uint8_t>(operandValues.data(), operandValues.data() + operandValues.size());
}

nn::GeneralResult<ExtensionNameAndPrefix> unvalidatedConvert(
        const nn::ExtensionNameAndPrefix& extensionNameToPrefix) {
    return ExtensionNameAndPrefix{
            .name = extensionNameToPrefix.name,
            .prefix = extensionNameToPrefix.prefix,
    };
}

nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model) {
    if (!hal::utils::hasNoPointerData(model)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Model cannot be unvalidatedConverted because it contains pointer-based memory";
    }

    auto main = NN_TRY(unvalidatedConvert(model.main));
    auto referenced = NN_TRY(unvalidatedConvert(model.referenced));
    auto operandValues = NN_TRY(unvalidatedConvert(model.operandValues));
    auto pools = NN_TRY(unvalidatedConvert(model.pools));
    auto extensionNameToPrefix = NN_TRY(unvalidatedConvert(model.extensionNameToPrefix));
    return Model{
            .main = std::move(main),
            .referenced = std::move(referenced),
            .operandValues = std::move(operandValues),
            .pools = std::move(pools),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = std::move(extensionNameToPrefix),
    };
}

nn::GeneralResult<Priority> unvalidatedConvert(const nn::Priority& priority) {
    return static_cast<Priority>(priority);
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

nn::GeneralResult<RequestArgument> unvalidatedConvert(
        const nn::Request::Argument& requestArgument) {
    if (requestArgument.lifetime == nn::Request::Argument::LifeTime::POINTER) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Request cannot be unvalidatedConverted because it contains pointer-based memory";
    }
    const bool hasNoValue = requestArgument.lifetime == nn::Request::Argument::LifeTime::NO_VALUE;
    const auto location = NN_TRY(unvalidatedConvert(requestArgument.location));
    auto dimensions = NN_TRY(toSigned(requestArgument.dimensions));
    return RequestArgument{
            .hasNoValue = hasNoValue,
            .location = location,
            .dimensions = std::move(dimensions),
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
    const auto timeOnDeviceNs = NN_TRY(unvalidatedConvert(timing.timeOnDevice));
    const auto timeInDriverNs = NN_TRY(unvalidatedConvert(timing.timeInDriver));
    return Timing{
            .timeOnDeviceNs = timeOnDeviceNs,
            .timeInDriverNs = timeInDriverNs,
    };
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

nn::GeneralResult<ndk::ScopedFileDescriptor> unvalidatedConvert(const nn::SharedHandle& handle) {
    auto duplicatedFd = NN_TRY(nn::dupFd(handle->get()));
    return ndk::ScopedFileDescriptor(duplicatedFd.release());
}

nn::GeneralResult<Capabilities> unvalidatedConvert(const nn::Capabilities& capabilities) {
    const auto relaxedFloat32toFloat16PerformanceTensor =
            NN_TRY(unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceTensor));
    const auto relaxedFloat32toFloat16PerformanceScalar =
            NN_TRY(unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceScalar));
    auto operandPerformance = NN_TRY(unvalidatedConvert(capabilities.operandPerformance));
    const auto ifPerformance = NN_TRY(unvalidatedConvert(capabilities.ifPerformance));
    const auto whilePerformance = NN_TRY(unvalidatedConvert(capabilities.whilePerformance));
    return Capabilities{
            .relaxedFloat32toFloat16PerformanceTensor = relaxedFloat32toFloat16PerformanceTensor,
            .relaxedFloat32toFloat16PerformanceScalar = relaxedFloat32toFloat16PerformanceScalar,
            .operandPerformance = std::move(operandPerformance),
            .ifPerformance = ifPerformance,
            .whilePerformance = whilePerformance,
    };
}

nn::GeneralResult<Extension> unvalidatedConvert(const nn::Extension& extension) {
    auto operandTypes = NN_TRY(unvalidatedConvert(extension.operandTypes));
    return Extension{.name = extension.name, .operandTypes = std::move(operandTypes)};
}
#ifdef NN_AIDL_V4_OR_ABOVE
nn::GeneralResult<TokenValuePair> unvalidatedConvert(const nn::TokenValuePair& tokenValuePair) {
    return TokenValuePair{.token = tokenValuePair.token, .value = tokenValuePair.value};
}
#endif  // NN_AIDL_V4_OR_ABOVE

nn::GeneralResult<std::vector<uint8_t>> convert(const nn::CacheToken& cacheToken) {
    return validatedConvert(cacheToken);
}

nn::GeneralResult<BufferDesc> convert(const nn::BufferDesc& bufferDesc) {
    return validatedConvert(bufferDesc);
}

nn::GeneralResult<DeviceType> convert(const nn::DeviceType& deviceType) {
    return validatedConvert(deviceType);
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

nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

nn::GeneralResult<Extension> convert(const nn::Extension& extension) {
    return validatedConvert(extension);
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
    return validatedConvert(cacheHandles);
}

nn::GeneralResult<std::vector<ndk::ScopedFileDescriptor>> convert(
        const std::vector<nn::SyncFence>& syncFences) {
    return validatedConvert(syncFences);
}
nn::GeneralResult<std::vector<ExtensionNameAndPrefix>> convert(
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) {
    return unvalidatedConvert(extensionNameToPrefix);
}

#ifdef NN_AIDL_V4_OR_ABOVE
nn::GeneralResult<std::vector<TokenValuePair>> convert(
        const std::vector<nn::TokenValuePair>& metaData) {
    return validatedConvert(metaData);
}
#endif  // NN_AIDL_V4_OR_ABOVE

nn::GeneralResult<std::vector<Extension>> convert(const std::vector<nn::Extension>& extensions) {
    return validatedConvert(extensions);
}

nn::GeneralResult<std::vector<int32_t>> toSigned(const std::vector<uint32_t>& vec) {
    if (!std::all_of(vec.begin(), vec.end(),
                     [](uint32_t v) { return v <= std::numeric_limits<int32_t>::max(); })) {
        return NN_ERROR() << "Vector contains a value that doesn't fit into int32_t.";
    }
    return std::vector<int32_t>(vec.begin(), vec.end());
}

std::vector<uint8_t> toVec(const std::array<uint8_t, IDevice::BYTE_SIZE_OF_CACHE_TOKEN>& token) {
    return std::vector<uint8_t>(token.begin(), token.end());
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
