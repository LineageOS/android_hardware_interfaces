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
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/CommonUtils.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

#include "Utils.h"

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
    NN_TRY(hal::V1_1::utils::compliantVersion(canonical));
    return canonical;
}

}  // anonymous namespace

GeneralResult<OperationType> unvalidatedConvert(const hal::V1_1::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

GeneralResult<Capabilities> unvalidatedConvert(const hal::V1_1::Capabilities& capabilities) {
    const auto quantized8Performance =
            NN_TRY(unvalidatedConvert(capabilities.quantized8Performance));
    const auto float32Performance = NN_TRY(unvalidatedConvert(capabilities.float32Performance));
    const auto relaxedFloat32toFloat16Performance =
            NN_TRY(unvalidatedConvert(capabilities.relaxedFloat32toFloat16Performance));

    auto table = hal::utils::makeQuantized8PerformanceConsistentWithP(float32Performance,
                                                                      quantized8Performance);

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = relaxedFloat32toFloat16Performance,
            .relaxedFloat32toFloat16PerformanceTensor = relaxedFloat32toFloat16Performance,
            .operandPerformance = std::move(table),
    };
}

GeneralResult<Operation> unvalidatedConvert(const hal::V1_1::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

GeneralResult<Model> unvalidatedConvert(const hal::V1_1::Model& model) {
    auto operations = NN_TRY(unvalidatedConvert(model.operations));

    // Verify number of consumers.
    const auto numberOfConsumers =
            NN_TRY(hal::utils::countNumberOfConsumers(model.operands.size(), operations));
    CHECK(model.operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < model.operands.size(); ++i) {
        if (model.operands[i].numberOfConsumers != numberOfConsumers[i]) {
            return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
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
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
    };
}

GeneralResult<ExecutionPreference> unvalidatedConvert(
        const hal::V1_1::ExecutionPreference& executionPreference) {
    return static_cast<ExecutionPreference>(executionPreference);
}

GeneralResult<Capabilities> convert(const hal::V1_1::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

GeneralResult<Model> convert(const hal::V1_1::Model& model) {
    return validatedConvert(model);
}

GeneralResult<ExecutionPreference> convert(
        const hal::V1_1::ExecutionPreference& executionPreference) {
    return validatedConvert(executionPreference);
}

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_1::utils {
namespace {

using utils::unvalidatedConvert;

nn::GeneralResult<V1_0::PerformanceInfo> unvalidatedConvert(
        const nn::Capabilities::PerformanceInfo& performanceInfo) {
    return V1_0::utils::unvalidatedConvert(performanceInfo);
}

nn::GeneralResult<V1_0::Operand> unvalidatedConvert(const nn::Operand& operand) {
    return V1_0::utils::unvalidatedConvert(operand);
}

nn::GeneralResult<hidl_vec<uint8_t>> unvalidatedConvert(
        const nn::Model::OperandValues& operandValues) {
    return V1_0::utils::unvalidatedConvert(operandValues);
}

nn::GeneralResult<hidl_memory> unvalidatedConvert(const nn::SharedMemory& memory) {
    return V1_0::utils::unvalidatedConvert(memory);
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

template <typename Type>
nn::GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& canonical) {
    NN_TRY(compliantVersion(canonical));
    return unvalidatedConvert(canonical);
}

}  // anonymous namespace

nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

nn::GeneralResult<Capabilities> unvalidatedConvert(const nn::Capabilities& capabilities) {
    return Capabilities{
            .float32Performance = NN_TRY(unvalidatedConvert(
                    capabilities.operandPerformance.lookup(nn::OperandType::TENSOR_FLOAT32))),
            .quantized8Performance = NN_TRY(unvalidatedConvert(
                    capabilities.operandPerformance.lookup(nn::OperandType::TENSOR_QUANT8_ASYMM))),
            .relaxedFloat32toFloat16Performance = NN_TRY(
                    unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceTensor)),
    };
}

nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
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
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
    };
}

nn::GeneralResult<ExecutionPreference> unvalidatedConvert(
        const nn::ExecutionPreference& executionPreference) {
    return static_cast<ExecutionPreference>(executionPreference);
}

nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

nn::GeneralResult<Model> convert(const nn::Model& model) {
    return validatedConvert(model);
}

nn::GeneralResult<ExecutionPreference> convert(const nn::ExecutionPreference& executionPreference) {
    return validatedConvert(executionPreference);
}

nn::GeneralResult<V1_0::DeviceStatus> convert(const nn::DeviceStatus& deviceStatus) {
    return V1_0::utils::convert(deviceStatus);
}

nn::GeneralResult<V1_0::Request> convert(const nn::Request& request) {
    return V1_0::utils::convert(request);
}

nn::GeneralResult<V1_0::ErrorStatus> convert(const nn::ErrorStatus& status) {
    return V1_0::utils::convert(status);
}

}  // namespace android::hardware::neuralnetworks::V1_1::utils
