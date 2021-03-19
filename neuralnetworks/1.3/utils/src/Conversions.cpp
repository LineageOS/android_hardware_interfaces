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
#include <nnapi/Validation.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/1.2/Conversions.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

#include "Utils.h"

namespace {

std::chrono::nanoseconds makeNanosFromUint64(uint64_t nanoseconds) {
    constexpr auto kMaxCount = std::chrono::nanoseconds::max().count();
    using CommonType = std::common_type_t<std::chrono::nanoseconds::rep, uint64_t>;
    const auto count = std::min<CommonType>(kMaxCount, nanoseconds);
    return std::chrono::nanoseconds{static_cast<std::chrono::nanoseconds::rep>(count)};
}

uint64_t makeUint64FromNanos(std::chrono::nanoseconds nanoseconds) {
    if (nanoseconds < std::chrono::nanoseconds::zero()) {
        return 0;
    }
    constexpr auto kMaxCount = std::numeric_limits<uint64_t>::max();
    using CommonType = std::common_type_t<std::chrono::nanoseconds::rep, uint64_t>;
    const auto count = std::min<CommonType>(kMaxCount, nanoseconds.count());
    return static_cast<uint64_t>(count);
}

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

}  // namespace

namespace android::nn {
namespace {

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
    NN_TRY(hal::V1_3::utils::compliantVersion(canonical));
    return canonical;
}

template <typename Type>
GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> validatedConvert(
        const hidl_vec<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(validatedConvert(argument)));
    }
    return canonical;
}

}  // anonymous namespace

GeneralResult<OperandType> unvalidatedConvert(const hal::V1_3::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

GeneralResult<OperationType> unvalidatedConvert(const hal::V1_3::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

GeneralResult<Priority> unvalidatedConvert(const hal::V1_3::Priority& priority) {
    return static_cast<Priority>(priority);
}

GeneralResult<Capabilities> unvalidatedConvert(const hal::V1_3::Capabilities& capabilities) {
    const bool validOperandTypes = std::all_of(
            capabilities.operandPerformance.begin(), capabilities.operandPerformance.end(),
            [](const hal::V1_3::Capabilities::OperandPerformance& operandPerformance) {
                return validatedConvert(operandPerformance.type).has_value();
            });
    if (!validOperandTypes) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
               << "Invalid OperandType when unvalidatedConverting OperandPerformance in "
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
        const hal::V1_3::Capabilities::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(unvalidatedConvert(operandPerformance.type)),
            .info = NN_TRY(unvalidatedConvert(operandPerformance.info)),
    };
}

GeneralResult<Operation> unvalidatedConvert(const hal::V1_3::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

GeneralResult<Operand::LifeTime> unvalidatedConvert(
        const hal::V1_3::OperandLifeTime& operandLifeTime) {
    return static_cast<Operand::LifeTime>(operandLifeTime);
}

GeneralResult<Operand> unvalidatedConvert(const hal::V1_3::Operand& operand) {
    return Operand{
            .type = NN_TRY(unvalidatedConvert(operand.type)),
            .dimensions = operand.dimensions,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(unvalidatedConvert(operand.lifetime)),
            .location = NN_TRY(unvalidatedConvert(operand.location)),
            .extraParams = NN_TRY(unvalidatedConvert(operand.extraParams)),
    };
}

GeneralResult<Model> unvalidatedConvert(const hal::V1_3::Model& model) {
    return Model{
            .main = NN_TRY(unvalidatedConvert(model.main)),
            .referenced = NN_TRY(unvalidatedConvert(model.referenced)),
            .operandValues = NN_TRY(unvalidatedConvert(model.operandValues)),
            .pools = NN_TRY(unvalidatedConvert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(unvalidatedConvert(model.extensionNameToPrefix)),
    };
}

GeneralResult<Model::Subgraph> unvalidatedConvert(const hal::V1_3::Subgraph& subgraph) {
    auto operations = NN_TRY(unvalidatedConvert(subgraph.operations));

    // Verify number of consumers.
    const auto numberOfConsumers =
            NN_TRY(hal::utils::countNumberOfConsumers(subgraph.operands.size(), operations));
    CHECK(subgraph.operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < subgraph.operands.size(); ++i) {
        if (subgraph.operands[i].numberOfConsumers != numberOfConsumers[i]) {
            return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                   << "Invalid numberOfConsumers for operand " << i << ", expected "
                   << numberOfConsumers[i] << " but found "
                   << subgraph.operands[i].numberOfConsumers;
        }
    }

    return Model::Subgraph{
            .operands = NN_TRY(unvalidatedConvert(subgraph.operands)),
            .operations = std::move(operations),
            .inputIndexes = subgraph.inputIndexes,
            .outputIndexes = subgraph.outputIndexes,
    };
}

GeneralResult<BufferDesc> unvalidatedConvert(const hal::V1_3::BufferDesc& bufferDesc) {
    return BufferDesc{.dimensions = bufferDesc.dimensions};
}

GeneralResult<BufferRole> unvalidatedConvert(const hal::V1_3::BufferRole& bufferRole) {
    return BufferRole{
            .modelIndex = bufferRole.modelIndex,
            .ioIndex = bufferRole.ioIndex,
            .probability = bufferRole.frequency,
    };
}

GeneralResult<Request> unvalidatedConvert(const hal::V1_3::Request& request) {
    return Request{
            .inputs = NN_TRY(unvalidatedConvert(request.inputs)),
            .outputs = NN_TRY(unvalidatedConvert(request.outputs)),
            .pools = NN_TRY(unvalidatedConvert(request.pools)),
    };
}

GeneralResult<Request::MemoryPool> unvalidatedConvert(
        const hal::V1_3::Request::MemoryPool& memoryPool) {
    using Discriminator = hal::V1_3::Request::MemoryPool::hidl_discriminator;
    switch (memoryPool.getDiscriminator()) {
        case Discriminator::hidlMemory:
            return hal::utils::createSharedMemoryFromHidlMemory(memoryPool.hidlMemory());
        case Discriminator::token:
            return static_cast<Request::MemoryDomainToken>(memoryPool.token());
    }
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "Invalid Request::MemoryPool discriminator "
           << underlyingType(memoryPool.getDiscriminator());
}

GeneralResult<OptionalTimePoint> unvalidatedConvert(
        const hal::V1_3::OptionalTimePoint& optionalTimePoint) {
    using Discriminator = hal::V1_3::OptionalTimePoint::hidl_discriminator;
    switch (optionalTimePoint.getDiscriminator()) {
        case Discriminator::none:
            return {};
        case Discriminator::nanosecondsSinceEpoch: {
            const auto currentSteadyTime = std::chrono::steady_clock::now();
            const auto currentBootTime = Clock::now();

            const auto timeSinceEpoch =
                    makeNanosFromUint64(optionalTimePoint.nanosecondsSinceEpoch());
            const auto steadyTimePoint = std::chrono::steady_clock::time_point{timeSinceEpoch};

            // Both steadyTimePoint and currentSteadyTime are guaranteed to be non-negative, so this
            // subtraction will never overflow or underflow.
            const auto timeRemaining = steadyTimePoint - currentSteadyTime;

            // currentBootTime is guaranteed to be non-negative, so this code only protects against
            // an overflow.
            nn::TimePoint bootTimePoint;
            constexpr auto kZeroNano = std::chrono::nanoseconds::zero();
            constexpr auto kMaxTime = nn::TimePoint::max();
            if (timeRemaining > kZeroNano && currentBootTime > kMaxTime - timeRemaining) {
                bootTimePoint = kMaxTime;
            } else {
                bootTimePoint = currentBootTime + timeRemaining;
            }

            constexpr auto kZeroTime = nn::TimePoint{};
            return std::max(bootTimePoint, kZeroTime);
        }
    }
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "Invalid OptionalTimePoint discriminator "
           << underlyingType(optionalTimePoint.getDiscriminator());
}

GeneralResult<OptionalDuration> unvalidatedConvert(
        const hal::V1_3::OptionalTimeoutDuration& optionalTimeoutDuration) {
    using Discriminator = hal::V1_3::OptionalTimeoutDuration::hidl_discriminator;
    switch (optionalTimeoutDuration.getDiscriminator()) {
        case Discriminator::none:
            return {};
        case Discriminator::nanoseconds:
            return Duration(optionalTimeoutDuration.nanoseconds());
    }
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "Invalid OptionalTimeoutDuration discriminator "
           << underlyingType(optionalTimeoutDuration.getDiscriminator());
}

GeneralResult<ErrorStatus> unvalidatedConvert(const hal::V1_3::ErrorStatus& status) {
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
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "Invalid ErrorStatus " << underlyingType(status);
}

GeneralResult<Priority> convert(const hal::V1_3::Priority& priority) {
    return validatedConvert(priority);
}

GeneralResult<Capabilities> convert(const hal::V1_3::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

GeneralResult<Model> convert(const hal::V1_3::Model& model) {
    return validatedConvert(model);
}

GeneralResult<BufferDesc> convert(const hal::V1_3::BufferDesc& bufferDesc) {
    return validatedConvert(bufferDesc);
}

GeneralResult<Request> convert(const hal::V1_3::Request& request) {
    return validatedConvert(request);
}

GeneralResult<OptionalTimePoint> convert(const hal::V1_3::OptionalTimePoint& optionalTimePoint) {
    return validatedConvert(optionalTimePoint);
}

GeneralResult<OptionalDuration> convert(
        const hal::V1_3::OptionalTimeoutDuration& optionalTimeoutDuration) {
    return validatedConvert(optionalTimeoutDuration);
}

GeneralResult<ErrorStatus> convert(const hal::V1_3::ErrorStatus& errorStatus) {
    return validatedConvert(errorStatus);
}

GeneralResult<SharedHandle> convert(const hardware::hidl_handle& handle) {
    return validatedConvert(handle);
}

GeneralResult<std::vector<BufferRole>> convert(
        const hardware::hidl_vec<hal::V1_3::BufferRole>& bufferRoles) {
    return validatedConvert(bufferRoles);
}

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

using utils::unvalidatedConvert;

nn::GeneralResult<V1_0::PerformanceInfo> unvalidatedConvert(
        const nn::Capabilities::PerformanceInfo& performanceInfo) {
    return V1_0::utils::unvalidatedConvert(performanceInfo);
}

nn::GeneralResult<V1_0::DataLocation> unvalidatedConvert(const nn::DataLocation& dataLocation) {
    return V1_0::utils::unvalidatedConvert(dataLocation);
}

nn::GeneralResult<hidl_vec<uint8_t>> unvalidatedConvert(
        const nn::Model::OperandValues& operandValues) {
    return V1_0::utils::unvalidatedConvert(operandValues);
}

nn::GeneralResult<hidl_handle> unvalidatedConvert(const nn::SharedHandle& handle) {
    return V1_2::utils::unvalidatedConvert(handle);
}

nn::GeneralResult<hidl_memory> unvalidatedConvert(const nn::SharedMemory& memory) {
    return V1_0::utils::unvalidatedConvert(memory);
}

nn::GeneralResult<V1_0::RequestArgument> unvalidatedConvert(const nn::Request::Argument& argument) {
    return V1_0::utils::unvalidatedConvert(argument);
}

nn::GeneralResult<V1_2::Operand::ExtraParams> unvalidatedConvert(
        const nn::Operand::ExtraParams& extraParams) {
    return V1_2::utils::unvalidatedConvert(extraParams);
}

nn::GeneralResult<V1_2::Model::ExtensionNameAndPrefix> unvalidatedConvert(
        const nn::Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return V1_2::utils::unvalidatedConvert(extensionNameAndPrefix);
}

template <typename Input>
using UnvalidatedConvertOutput =
        std::decay_t<decltype(unvalidatedConvert(std::declval<Input>()).value())>;

template <typename Type>
nn::GeneralResult<hidl_vec<UnvalidatedConvertOutput<Type>>> unvalidatedConvert(
        const std::vector<Type>& arguments) {
    hidl_vec<UnvalidatedConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(unvalidatedConvert(arguments[i]));
    }
    return halObject;
}

nn::GeneralResult<Request::MemoryPool> makeMemoryPool(const nn::SharedMemory& memory) {
    Request::MemoryPool ret;
    ret.hidlMemory(NN_TRY(unvalidatedConvert(memory)));
    return ret;
}

nn::GeneralResult<Request::MemoryPool> makeMemoryPool(const nn::Request::MemoryDomainToken& token) {
    Request::MemoryPool ret;
    ret.token(underlyingType(token));
    return ret;
}

nn::GeneralResult<Request::MemoryPool> makeMemoryPool(const nn::SharedBuffer& /*buffer*/) {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "Unable to make memory pool from IBuffer";
}

using utils::unvalidatedConvert;

template <typename Type>
nn::GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& canonical) {
    NN_TRY(compliantVersion(canonical));
    return unvalidatedConvert(canonical);
}

template <typename Type>
nn::GeneralResult<hidl_vec<UnvalidatedConvertOutput<Type>>> validatedConvert(
        const std::vector<Type>& arguments) {
    hidl_vec<UnvalidatedConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(validatedConvert(arguments[i]));
    }
    return halObject;
}

}  // anonymous namespace

nn::GeneralResult<OperandType> unvalidatedConvert(const nn::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

nn::GeneralResult<Priority> unvalidatedConvert(const nn::Priority& priority) {
    return static_cast<Priority>(priority);
}

nn::GeneralResult<Capabilities> unvalidatedConvert(const nn::Capabilities& capabilities) {
    std::vector<nn::Capabilities::OperandPerformance> operandPerformance;
    operandPerformance.reserve(capabilities.operandPerformance.asVector().size());
    std::copy_if(capabilities.operandPerformance.asVector().begin(),
                 capabilities.operandPerformance.asVector().end(),
                 std::back_inserter(operandPerformance),
                 [](const nn::Capabilities::OperandPerformance& operandPerformance) {
                     return compliantVersion(operandPerformance.type).has_value();
                 });

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = NN_TRY(
                    unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceScalar)),
            .relaxedFloat32toFloat16PerformanceTensor = NN_TRY(
                    unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceTensor)),
            .operandPerformance = NN_TRY(unvalidatedConvert(operandPerformance)),
            .ifPerformance = NN_TRY(unvalidatedConvert(capabilities.ifPerformance)),
            .whilePerformance = NN_TRY(unvalidatedConvert(capabilities.whilePerformance)),
    };
}

nn::GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const nn::Capabilities::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(unvalidatedConvert(operandPerformance.type)),
            .info = NN_TRY(unvalidatedConvert(operandPerformance.info)),
    };
}

nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

nn::GeneralResult<OperandLifeTime> unvalidatedConvert(
        const nn::Operand::LifeTime& operandLifeTime) {
    if (operandLifeTime == nn::Operand::LifeTime::POINTER) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Model cannot be unvalidatedConverted because it contains pointer-based memory";
    }
    return static_cast<OperandLifeTime>(operandLifeTime);
}

nn::GeneralResult<Operand> unvalidatedConvert(const nn::Operand& operand) {
    return Operand{
            .type = NN_TRY(unvalidatedConvert(operand.type)),
            .dimensions = operand.dimensions,
            .numberOfConsumers = 0,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(unvalidatedConvert(operand.lifetime)),
            .location = NN_TRY(unvalidatedConvert(operand.location)),
            .extraParams = NN_TRY(unvalidatedConvert(operand.extraParams)),
    };
}

nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model) {
    if (!hal::utils::hasNoPointerData(model)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Model cannot be unvalidatedConverted because it contains pointer-based memory";
    }

    return Model{
            .main = NN_TRY(unvalidatedConvert(model.main)),
            .referenced = NN_TRY(unvalidatedConvert(model.referenced)),
            .operandValues = NN_TRY(unvalidatedConvert(model.operandValues)),
            .pools = NN_TRY(unvalidatedConvert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(unvalidatedConvert(model.extensionNameToPrefix)),
    };
}

nn::GeneralResult<Subgraph> unvalidatedConvert(const nn::Model::Subgraph& subgraph) {
    auto operands = NN_TRY(unvalidatedConvert(subgraph.operands));

    // Update number of consumers.
    const auto numberOfConsumers =
            NN_TRY(hal::utils::countNumberOfConsumers(operands.size(), subgraph.operations));
    CHECK(operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < operands.size(); ++i) {
        operands[i].numberOfConsumers = numberOfConsumers[i];
    }

    return Subgraph{
            .operands = std::move(operands),
            .operations = NN_TRY(unvalidatedConvert(subgraph.operations)),
            .inputIndexes = subgraph.inputIndexes,
            .outputIndexes = subgraph.outputIndexes,
    };
}

nn::GeneralResult<BufferDesc> unvalidatedConvert(const nn::BufferDesc& bufferDesc) {
    return BufferDesc{.dimensions = bufferDesc.dimensions};
}

nn::GeneralResult<BufferRole> unvalidatedConvert(const nn::BufferRole& bufferRole) {
    return BufferRole{
            .modelIndex = bufferRole.modelIndex,
            .ioIndex = bufferRole.ioIndex,
            .frequency = bufferRole.probability,
    };
}

nn::GeneralResult<Request> unvalidatedConvert(const nn::Request& request) {
    if (!hal::utils::hasNoPointerData(request)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Request cannot be unvalidatedConverted because it contains pointer-based memory";
    }

    return Request{
            .inputs = NN_TRY(unvalidatedConvert(request.inputs)),
            .outputs = NN_TRY(unvalidatedConvert(request.outputs)),
            .pools = NN_TRY(unvalidatedConvert(request.pools)),
    };
}

nn::GeneralResult<Request::MemoryPool> unvalidatedConvert(
        const nn::Request::MemoryPool& memoryPool) {
    return std::visit([](const auto& o) { return makeMemoryPool(o); }, memoryPool);
}

nn::GeneralResult<OptionalTimePoint> unvalidatedConvert(
        const nn::OptionalTimePoint& optionalTimePoint) {
    const auto currentSteadyTime = std::chrono::steady_clock::now();
    const auto currentBootTime = nn::Clock::now();

    OptionalTimePoint ret;
    if (optionalTimePoint.has_value()) {
        const auto bootTimePoint = optionalTimePoint.value();

        if (bootTimePoint < nn::TimePoint{}) {
            return NN_ERROR() << "Trying to cast invalid time point";
        }

        // Both bootTimePoint and currentBootTime are guaranteed to be non-negative, so this
        // subtraction will never overflow or underflow.
        const auto timeRemaining = bootTimePoint - currentBootTime;

        // currentSteadyTime is guaranteed to be non-negative, so this code only protects against an
        // overflow.
        std::chrono::steady_clock::time_point steadyTimePoint;
        constexpr auto kZeroNano = std::chrono::nanoseconds::zero();
        constexpr auto kMaxTime = std::chrono::steady_clock::time_point::max();
        if (timeRemaining > kZeroNano && currentSteadyTime > kMaxTime - timeRemaining) {
            steadyTimePoint = kMaxTime;
        } else {
            steadyTimePoint = currentSteadyTime + timeRemaining;
        }

        const uint64_t count = makeUint64FromNanos(steadyTimePoint.time_since_epoch());
        ret.nanosecondsSinceEpoch(count);
    }
    return ret;
}

nn::GeneralResult<OptionalTimeoutDuration> unvalidatedConvert(
        const nn::OptionalDuration& optionalTimeoutDuration) {
    OptionalTimeoutDuration ret;
    if (optionalTimeoutDuration.has_value()) {
        const auto count = optionalTimeoutDuration.value().count();
        ret.nanoseconds(count);
    }
    return ret;
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

nn::GeneralResult<Priority> convert(const nn::Priority& priority) {
    return validatedConvert(priority);
}

nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

nn::GeneralResult<Model> convert(const nn::Model& model) {
    return validatedConvert(model);
}

nn::GeneralResult<BufferDesc> convert(const nn::BufferDesc& bufferDesc) {
    return validatedConvert(bufferDesc);
}

nn::GeneralResult<Request> convert(const nn::Request& request) {
    return validatedConvert(request);
}

nn::GeneralResult<OptionalTimePoint> convert(const nn::OptionalTimePoint& optionalTimePoint) {
    return validatedConvert(optionalTimePoint);
}

nn::GeneralResult<OptionalTimeoutDuration> convert(
        const nn::OptionalDuration& optionalTimeoutDuration) {
    return validatedConvert(optionalTimeoutDuration);
}

nn::GeneralResult<ErrorStatus> convert(const nn::ErrorStatus& errorStatus) {
    return validatedConvert(errorStatus);
}

nn::GeneralResult<hidl_handle> convert(const nn::SharedHandle& handle) {
    return validatedConvert(handle);
}

nn::GeneralResult<hidl_memory> convert(const nn::SharedMemory& memory) {
    return validatedConvert(memory);
}

nn::GeneralResult<hidl_vec<BufferRole>> convert(const std::vector<nn::BufferRole>& bufferRoles) {
    return validatedConvert(bufferRoles);
}

nn::GeneralResult<V1_0::DeviceStatus> convert(const nn::DeviceStatus& deviceStatus) {
    return V1_2::utils::convert(deviceStatus);
}

nn::GeneralResult<V1_1::ExecutionPreference> convert(
        const nn::ExecutionPreference& executionPreference) {
    return V1_2::utils::convert(executionPreference);
}

nn::GeneralResult<hidl_vec<V1_2::Extension>> convert(const std::vector<nn::Extension>& extensions) {
    return V1_2::utils::convert(extensions);
}

nn::GeneralResult<hidl_vec<hidl_handle>> convert(const std::vector<nn::SharedHandle>& handles) {
    return V1_2::utils::convert(handles);
}

nn::GeneralResult<hidl_vec<V1_2::OutputShape>> convert(
        const std::vector<nn::OutputShape>& outputShapes) {
    return V1_2::utils::convert(outputShapes);
}

nn::GeneralResult<V1_2::DeviceType> convert(const nn::DeviceType& deviceType) {
    return V1_2::utils::convert(deviceType);
}

nn::GeneralResult<V1_2::MeasureTiming> convert(const nn::MeasureTiming& measureTiming) {
    return V1_2::utils::convert(measureTiming);
}

nn::GeneralResult<V1_2::Timing> convert(const nn::Timing& timing) {
    return V1_2::utils::convert(timing);
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
