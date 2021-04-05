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

namespace {

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

}  // namespace

namespace android::nn {
namespace {

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
    return Operand{
            .type = NN_TRY(unvalidatedConvert(operand.type)),
            .dimensions = operand.dimensions,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(unvalidatedConvert(operand.lifetime)),
            .location = NN_TRY(unvalidatedConvert(operand.location)),
    };
}

GeneralResult<Operation> unvalidatedConvert(const hal::V1_0::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

GeneralResult<Model::OperandValues> unvalidatedConvert(const hidl_vec<uint8_t>& operandValues) {
    return Model::OperandValues(operandValues.data(), operandValues.size());
}

GeneralResult<SharedMemory> unvalidatedConvert(const hidl_memory& memory) {
    return hal::utils::createSharedMemoryFromHidlMemory(memory);
}

GeneralResult<Model> unvalidatedConvert(const hal::V1_0::Model& model) {
    auto operations = NN_TRY(unvalidatedConvert(model.operations));

    // Verify number of consumers.
    const auto numberOfConsumers =
            NN_TRY(hal::utils::countNumberOfConsumers(model.operands.size(), operations));
    CHECK(model.operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < model.operands.size(); ++i) {
        if (model.operands[i].numberOfConsumers != numberOfConsumers[i]) {
            return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
                   << "Invalid numberOfConsumers for operand " << i << ", expected "
                   << numberOfConsumers[i] << " but found " << model.operands[i].numberOfConsumers;
        }
    }

    auto main = Model::Subgraph{
            .operands = NN_TRY(unvalidatedConvert(model.operands)),
            .operations = std::move(operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
    };

    return Model{
            .main = std::move(main),
            .operandValues = NN_TRY(unvalidatedConvert(model.operandValues)),
            .pools = NN_TRY(unvalidatedConvert(model.pools)),
    };
}

GeneralResult<Request::Argument> unvalidatedConvert(const hal::V1_0::RequestArgument& argument) {
    const auto lifetime = argument.hasNoValue ? Request::Argument::LifeTime::NO_VALUE
                                              : Request::Argument::LifeTime::POOL;
    return Request::Argument{
            .lifetime = lifetime,
            .location = NN_TRY(unvalidatedConvert(argument.location)),
            .dimensions = argument.dimensions,
    };
}

GeneralResult<Request> unvalidatedConvert(const hal::V1_0::Request& request) {
    auto memories = NN_TRY(unvalidatedConvert(request.pools));
    std::vector<Request::MemoryPool> pools;
    pools.reserve(memories.size());
    std::move(memories.begin(), memories.end(), std::back_inserter(pools));

    return Request{
            .inputs = NN_TRY(unvalidatedConvert(request.inputs)),
            .outputs = NN_TRY(unvalidatedConvert(request.outputs)),
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
    return Capabilities{
            .float32Performance = NN_TRY(unvalidatedConvert(
                    capabilities.operandPerformance.lookup(nn::OperandType::TENSOR_FLOAT32))),
            .quantized8Performance = NN_TRY(unvalidatedConvert(
                    capabilities.operandPerformance.lookup(nn::OperandType::TENSOR_QUANT8_ASYMM))),
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
    return Operand{
            .type = NN_TRY(unvalidatedConvert(operand.type)),
            .dimensions = operand.dimensions,
            .numberOfConsumers = 0,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(unvalidatedConvert(operand.lifetime)),
            .location = NN_TRY(unvalidatedConvert(operand.location)),
    };
}

nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

nn::GeneralResult<hidl_vec<uint8_t>> unvalidatedConvert(
        const nn::Model::OperandValues& operandValues) {
    return hidl_vec<uint8_t>(operandValues.data(), operandValues.data() + operandValues.size());
}

nn::GeneralResult<hidl_memory> unvalidatedConvert(const nn::SharedMemory& memory) {
    return hal::utils::createHidlMemoryFromSharedMemory(memory);
}

nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model) {
    if (!hal::utils::hasNoPointerData(model)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Mdoel cannot be unvalidatedConverted because it contains pointer-based memory";
    }

    auto operands = NN_TRY(unvalidatedConvert(model.main.operands));

    // Update number of consumers.
    const auto numberOfConsumers =
            NN_TRY(hal::utils::countNumberOfConsumers(operands.size(), model.main.operations));
    CHECK(operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < operands.size(); ++i) {
        operands[i].numberOfConsumers = numberOfConsumers[i];
    }

    return Model{
            .operands = std::move(operands),
            .operations = NN_TRY(unvalidatedConvert(model.main.operations)),
            .inputIndexes = model.main.inputIndexes,
            .outputIndexes = model.main.outputIndexes,
            .operandValues = NN_TRY(unvalidatedConvert(model.operandValues)),
            .pools = NN_TRY(unvalidatedConvert(model.pools)),
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

    return Request{
            .inputs = NN_TRY(unvalidatedConvert(request.inputs)),
            .outputs = NN_TRY(unvalidatedConvert(request.outputs)),
            .pools = NN_TRY(unvalidatedConvert(request.pools)),
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
