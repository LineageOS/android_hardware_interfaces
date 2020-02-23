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
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/1.2/Conversions.h>
#include <nnapi/hal/CommonUtils.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

namespace {

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

}  // namespace

namespace android::nn {
namespace {

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
        case nn::OperandType::OEM:
        case nn::OperandType::TENSOR_OEM_BYTE:
            return true;
    }
    return nn::isExtension(operandType);
}

using hardware::hidl_vec;

template <typename Input>
using ConvertOutput = std::decay_t<decltype(convert(std::declval<Input>()).value())>;

template <typename Type>
Result<std::vector<ConvertOutput<Type>>> convertVec(const hidl_vec<Type>& arguments) {
    std::vector<ConvertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(nn::convert(argument)));
    }
    return canonical;
}

template <typename Type>
Result<std::vector<ConvertOutput<Type>>> convert(const hidl_vec<Type>& arguments) {
    return convertVec(arguments);
}

}  // anonymous namespace

Result<OperandType> convert(const hal::V1_3::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

Result<OperationType> convert(const hal::V1_3::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

Result<Priority> convert(const hal::V1_3::Priority& priority) {
    return static_cast<Priority>(priority);
}

Result<Capabilities> convert(const hal::V1_3::Capabilities& capabilities) {
    const bool validOperandTypes = std::all_of(
            capabilities.operandPerformance.begin(), capabilities.operandPerformance.end(),
            [](const hal::V1_3::Capabilities::OperandPerformance& operandPerformance) {
                const auto maybeType = convert(operandPerformance.type);
                return !maybeType.has_value() ? false : validOperandType(maybeType.value());
            });
    if (!validOperandTypes) {
        return NN_ERROR()
               << "Invalid OperandType when converting OperandPerformance in Capabilities";
    }

    auto operandPerformance = NN_TRY(convert(capabilities.operandPerformance));
    auto table =
            NN_TRY(Capabilities::OperandPerformanceTable::create(std::move(operandPerformance)));

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar =
                    NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceScalar)),
            .relaxedFloat32toFloat16PerformanceTensor =
                    NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceTensor)),
            .operandPerformance = std::move(table),
            .ifPerformance = NN_TRY(convert(capabilities.ifPerformance)),
            .whilePerformance = NN_TRY(convert(capabilities.whilePerformance)),
    };
}

Result<Capabilities::OperandPerformance> convert(
        const hal::V1_3::Capabilities::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(convert(operandPerformance.type)),
            .info = NN_TRY(convert(operandPerformance.info)),
    };
}

Result<Operation> convert(const hal::V1_3::Operation& operation) {
    return Operation{
            .type = NN_TRY(convert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

Result<Operand::LifeTime> convert(const hal::V1_3::OperandLifeTime& operandLifeTime) {
    return static_cast<Operand::LifeTime>(operandLifeTime);
}

Result<Operand> convert(const hal::V1_3::Operand& operand) {
    return Operand{
            .type = NN_TRY(convert(operand.type)),
            .dimensions = operand.dimensions,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(convert(operand.lifetime)),
            .location = NN_TRY(convert(operand.location)),
            .extraParams = NN_TRY(convert(operand.extraParams)),
    };
}

Result<Model> convert(const hal::V1_3::Model& model) {
    return Model{
            .main = NN_TRY(convert(model.main)),
            .referenced = NN_TRY(convert(model.referenced)),
            .operandValues = NN_TRY(convert(model.operandValues)),
            .pools = NN_TRY(convert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(convert(model.extensionNameToPrefix)),
    };
}

Result<Model::Subgraph> convert(const hal::V1_3::Subgraph& subgraph) {
    auto operations = NN_TRY(convert(subgraph.operations));

    // Verify number of consumers.
    const auto numberOfConsumers =
            hal::utils::countNumberOfConsumers(subgraph.operands.size(), operations);
    CHECK(subgraph.operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < subgraph.operands.size(); ++i) {
        if (subgraph.operands[i].numberOfConsumers != numberOfConsumers[i]) {
            return NN_ERROR() << "Invalid numberOfConsumers for operand " << i << ", expected "
                              << numberOfConsumers[i] << " but found "
                              << subgraph.operands[i].numberOfConsumers;
        }
    }

    return Model::Subgraph{
            .operands = NN_TRY(convert(subgraph.operands)),
            .operations = std::move(operations),
            .inputIndexes = subgraph.inputIndexes,
            .outputIndexes = subgraph.outputIndexes,
    };
}

Result<BufferDesc> convert(const hal::V1_3::BufferDesc& bufferDesc) {
    return BufferDesc{.dimensions = bufferDesc.dimensions};
}

Result<BufferRole> convert(const hal::V1_3::BufferRole& bufferRole) {
    return BufferRole{
            .modelIndex = bufferRole.modelIndex,
            .ioIndex = bufferRole.ioIndex,
            .frequency = bufferRole.frequency,
    };
}

Result<Request> convert(const hal::V1_3::Request& request) {
    return Request{
            .inputs = NN_TRY(convert(request.inputs)),
            .outputs = NN_TRY(convert(request.outputs)),
            .pools = NN_TRY(convert(request.pools)),
    };
}

Result<Request::MemoryPool> convert(const hal::V1_3::Request::MemoryPool& memoryPool) {
    using Discriminator = hal::V1_3::Request::MemoryPool::hidl_discriminator;
    switch (memoryPool.getDiscriminator()) {
        case Discriminator::hidlMemory:
            return createSharedMemoryFromHidlMemory(memoryPool.hidlMemory());
        case Discriminator::token:
            return static_cast<Request::MemoryDomainToken>(memoryPool.token());
    }
    return NN_ERROR() << "Invalid Request::MemoryPool discriminator "
                      << underlyingType(memoryPool.getDiscriminator());
}

Result<OptionalTimePoint> convert(const hal::V1_3::OptionalTimePoint& optionalTimePoint) {
    constexpr auto kTimePointMaxCount = TimePoint::max().time_since_epoch().count();
    const auto makeTimePoint = [](uint64_t count) -> Result<OptionalTimePoint> {
        if (count > kTimePointMaxCount) {
            return NN_ERROR()
                   << "Unable to convert OptionalTimePoint because the count exceeds the max";
        }
        const auto nanoseconds = std::chrono::nanoseconds{count};
        return TimePoint{nanoseconds};
    };

    using Discriminator = hal::V1_3::OptionalTimePoint::hidl_discriminator;
    switch (optionalTimePoint.getDiscriminator()) {
        case Discriminator::none:
            return std::nullopt;
        case Discriminator::nanosecondsSinceEpoch:
            return makeTimePoint(optionalTimePoint.nanosecondsSinceEpoch());
    }
    return NN_ERROR() << "Invalid OptionalTimePoint discriminator "
                      << underlyingType(optionalTimePoint.getDiscriminator());
}

Result<OptionalTimeoutDuration> convert(
        const hal::V1_3::OptionalTimeoutDuration& optionalTimeoutDuration) {
    constexpr auto kTimeoutDurationMaxCount = TimeoutDuration::max().count();
    const auto makeTimeoutDuration = [](uint64_t count) -> Result<OptionalTimeoutDuration> {
        if (count > kTimeoutDurationMaxCount) {
            return NN_ERROR()
                   << "Unable to convert OptionalTimeoutDuration because the count exceeds the max";
        }
        return TimeoutDuration{count};
    };

    using Discriminator = hal::V1_3::OptionalTimeoutDuration::hidl_discriminator;
    switch (optionalTimeoutDuration.getDiscriminator()) {
        case Discriminator::none:
            return std::nullopt;
        case Discriminator::nanoseconds:
            return makeTimeoutDuration(optionalTimeoutDuration.nanoseconds());
    }
    return NN_ERROR() << "Invalid OptionalTimeoutDuration discriminator "
                      << underlyingType(optionalTimeoutDuration.getDiscriminator());
}

Result<ErrorStatus> convert(const hal::V1_3::ErrorStatus& status) {
    switch (status) {
        case hal::V1_3::ErrorStatus::NONE:
        case hal::V1_3::ErrorStatus::DEVICE_UNAVAILABLE:
        case hal::V1_3::ErrorStatus::GENERAL_FAILURE:
        case hal::V1_3::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE:
        case hal::V1_3::ErrorStatus::INVALID_ARGUMENT:
        case hal::V1_3::ErrorStatus::MISSED_DEADLINE_TRANSIENT:
        case hal::V1_3::ErrorStatus::MISSED_DEADLINE_PERSISTENT:
        case hal::V1_3::ErrorStatus::RESOURCE_EXHAUSTED_TRANSIENT:
        case hal::V1_3::ErrorStatus::RESOURCE_EXHAUSTED_PERSISTENT:
            return static_cast<ErrorStatus>(status);
    }
    return NN_ERROR() << "Invalid ErrorStatus " << underlyingType(status);
}

Result<std::vector<BufferRole>> convert(
        const hardware::hidl_vec<hal::V1_3::BufferRole>& bufferRoles) {
    return convertVec(bufferRoles);
}

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

using utils::convert;

nn::Result<V1_0::PerformanceInfo> convert(
        const nn::Capabilities::PerformanceInfo& performanceInfo) {
    return V1_0::utils::convert(performanceInfo);
}

nn::Result<V1_0::DataLocation> convert(const nn::DataLocation& dataLocation) {
    return V1_0::utils::convert(dataLocation);
}

nn::Result<hidl_vec<uint8_t>> convert(const nn::Model::OperandValues& operandValues) {
    return V1_0::utils::convert(operandValues);
}

nn::Result<hidl_memory> convert(const nn::Memory& memory) {
    return V1_0::utils::convert(memory);
}

nn::Result<V1_0::RequestArgument> convert(const nn::Request::Argument& argument) {
    return V1_0::utils::convert(argument);
}

nn::Result<V1_2::Operand::ExtraParams> convert(const nn::Operand::ExtraParams& extraParams) {
    return V1_2::utils::convert(extraParams);
}

nn::Result<V1_2::Model::ExtensionNameAndPrefix> convert(
        const nn::Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return V1_2::utils::convert(extensionNameAndPrefix);
}

template <typename Input>
using ConvertOutput = std::decay_t<decltype(convert(std::declval<Input>()).value())>;

template <typename Type>
nn::Result<hidl_vec<ConvertOutput<Type>>> convertVec(const std::vector<Type>& arguments) {
    hidl_vec<ConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(convert(arguments[i]));
    }
    return halObject;
}

template <typename Type>
nn::Result<hidl_vec<ConvertOutput<Type>>> convert(const std::vector<Type>& arguments) {
    return convertVec(arguments);
}

nn::Result<Request::MemoryPool> makeMemoryPool(const nn::Memory& memory) {
    Request::MemoryPool ret;
    ret.hidlMemory(NN_TRY(convert(memory)));
    return ret;
}

nn::Result<Request::MemoryPool> makeMemoryPool(const nn::Request::MemoryDomainToken& token) {
    Request::MemoryPool ret;
    ret.token(underlyingType(token));
    return ret;
}

nn::Result<Request::MemoryPool> makeMemoryPool(
        const std::shared_ptr<const nn::IBuffer>& /*buffer*/) {
    return NN_ERROR() << "Unable to make memory pool from IBuffer";
}

}  // anonymous namespace

nn::Result<OperandType> convert(const nn::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

nn::Result<OperationType> convert(const nn::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

nn::Result<Priority> convert(const nn::Priority& priority) {
    return static_cast<Priority>(priority);
}

nn::Result<Capabilities> convert(const nn::Capabilities& capabilities) {
    std::vector<nn::Capabilities::OperandPerformance> operandPerformance;
    operandPerformance.reserve(capabilities.operandPerformance.asVector().size());
    std::copy_if(capabilities.operandPerformance.asVector().begin(),
                 capabilities.operandPerformance.asVector().end(),
                 std::back_inserter(operandPerformance),
                 [](const nn::Capabilities::OperandPerformance& operandPerformance) {
                     return nn::validOperandType(operandPerformance.type);
                 });

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar =
                    NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceScalar)),
            .relaxedFloat32toFloat16PerformanceTensor =
                    NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceTensor)),
            .operandPerformance = NN_TRY(convert(operandPerformance)),
            .ifPerformance = NN_TRY(convert(capabilities.ifPerformance)),
            .whilePerformance = NN_TRY(convert(capabilities.whilePerformance)),
    };
}

nn::Result<Capabilities::OperandPerformance> convert(
        const nn::Capabilities::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(convert(operandPerformance.type)),
            .info = NN_TRY(convert(operandPerformance.info)),
    };
}

nn::Result<Operation> convert(const nn::Operation& operation) {
    return Operation{
            .type = NN_TRY(convert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

nn::Result<OperandLifeTime> convert(const nn::Operand::LifeTime& operandLifeTime) {
    if (operandLifeTime == nn::Operand::LifeTime::POINTER) {
        return NN_ERROR() << "Model cannot be converted because it contains pointer-based memory";
    }
    return static_cast<OperandLifeTime>(operandLifeTime);
}

nn::Result<Operand> convert(const nn::Operand& operand) {
    return Operand{
            .type = NN_TRY(convert(operand.type)),
            .dimensions = operand.dimensions,
            .numberOfConsumers = 0,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(convert(operand.lifetime)),
            .location = NN_TRY(convert(operand.location)),
            .extraParams = NN_TRY(convert(operand.extraParams)),
    };
}

nn::Result<Model> convert(const nn::Model& model) {
    if (!hal::utils::hasNoPointerData(model)) {
        return NN_ERROR() << "Model cannot be converted because it contains pointer-based memory";
    }

    return Model{
            .main = NN_TRY(convert(model.main)),
            .referenced = NN_TRY(convert(model.referenced)),
            .operandValues = NN_TRY(convert(model.operandValues)),
            .pools = NN_TRY(convert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(convert(model.extensionNameToPrefix)),
    };
}

nn::Result<Subgraph> convert(const nn::Model::Subgraph& subgraph) {
    auto operands = NN_TRY(convert(subgraph.operands));

    // Update number of consumers.
    const auto numberOfConsumers =
            hal::utils::countNumberOfConsumers(operands.size(), subgraph.operations);
    CHECK(operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < operands.size(); ++i) {
        operands[i].numberOfConsumers = numberOfConsumers[i];
    }

    return Subgraph{
            .operands = std::move(operands),
            .operations = NN_TRY(convert(subgraph.operations)),
            .inputIndexes = subgraph.inputIndexes,
            .outputIndexes = subgraph.outputIndexes,
    };
}

nn::Result<BufferDesc> convert(const nn::BufferDesc& bufferDesc) {
    return BufferDesc{.dimensions = bufferDesc.dimensions};
}

nn::Result<BufferRole> convert(const nn::BufferRole& bufferRole) {
    return BufferRole{
            .modelIndex = bufferRole.modelIndex,
            .ioIndex = bufferRole.ioIndex,
            .frequency = bufferRole.frequency,
    };
}

nn::Result<Request> convert(const nn::Request& request) {
    if (!hal::utils::hasNoPointerData(request)) {
        return NN_ERROR() << "Request cannot be converted because it contains pointer-based memory";
    }

    return Request{
            .inputs = NN_TRY(convert(request.inputs)),
            .outputs = NN_TRY(convert(request.outputs)),
            .pools = NN_TRY(convert(request.pools)),
    };
}

nn::Result<Request::MemoryPool> convert(const nn::Request::MemoryPool& memoryPool) {
    return std::visit([](const auto& o) { return makeMemoryPool(o); }, memoryPool);
}

nn::Result<OptionalTimePoint> convert(const nn::OptionalTimePoint& optionalTimePoint) {
    OptionalTimePoint ret;
    if (optionalTimePoint.has_value()) {
        const auto count = optionalTimePoint.value().time_since_epoch().count();
        if (count < 0) {
            return NN_ERROR() << "Unable to convert OptionalTimePoint because time since epoch "
                                 "count is negative";
        }
        ret.nanosecondsSinceEpoch(count);
    }
    return ret;
}

nn::Result<OptionalTimeoutDuration> convert(
        const nn::OptionalTimeoutDuration& optionalTimeoutDuration) {
    OptionalTimeoutDuration ret;
    if (optionalTimeoutDuration.has_value()) {
        const auto count = optionalTimeoutDuration.value().count();
        if (count < 0) {
            return NN_ERROR()
                   << "Unable to convert OptionalTimeoutDuration because count is negative";
        }
        ret.nanoseconds(count);
    }
    return ret;
}

nn::Result<ErrorStatus> convert(const nn::ErrorStatus& errorStatus) {
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

nn::Result<hidl_vec<BufferRole>> convert(const std::vector<nn::BufferRole>& bufferRoles) {
    return convertVec(bufferRoles);
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
