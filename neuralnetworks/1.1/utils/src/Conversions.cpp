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
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/CommonUtils.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

namespace android::nn {
namespace {

using hardware::hidl_vec;

template <typename Input>
using convertOutput = std::decay_t<decltype(convert(std::declval<Input>()).value())>;

template <typename Type>
Result<std::vector<convertOutput<Type>>> convert(const hidl_vec<Type>& arguments) {
    std::vector<convertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(nn::convert(argument)));
    }
    return canonical;
}

}  // anonymous namespace

Result<OperationType> convert(const hal::V1_1::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

Result<Capabilities> convert(const hal::V1_1::Capabilities& capabilities) {
    const auto quantized8Performance = NN_TRY(convert(capabilities.quantized8Performance));
    const auto float32Performance = NN_TRY(convert(capabilities.float32Performance));
    const auto relaxedFloat32toFloat16Performance =
            NN_TRY(convert(capabilities.relaxedFloat32toFloat16Performance));

    auto table = hal::utils::makeQuantized8PerformanceConsistentWithP(float32Performance,
                                                                      quantized8Performance);

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = relaxedFloat32toFloat16Performance,
            .relaxedFloat32toFloat16PerformanceTensor = relaxedFloat32toFloat16Performance,
            .operandPerformance = std::move(table),
    };
}

Result<Operation> convert(const hal::V1_1::Operation& operation) {
    return Operation{
            .type = NN_TRY(convert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

Result<Model> convert(const hal::V1_1::Model& model) {
    auto operations = NN_TRY(convert(model.operations));

    // Verify number of consumers.
    const auto numberOfConsumers =
            hal::utils::countNumberOfConsumers(model.operands.size(), operations);
    CHECK(model.operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < model.operands.size(); ++i) {
        if (model.operands[i].numberOfConsumers != numberOfConsumers[i]) {
            return NN_ERROR() << "Invalid numberOfConsumers for operand " << i << ", expected "
                              << numberOfConsumers[i] << " but found "
                              << model.operands[i].numberOfConsumers;
        }
    }

    auto main = Model::Subgraph{
            .operands = NN_TRY(convert(model.operands)),
            .operations = std::move(operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
    };

    return Model{
            .main = std::move(main),
            .operandValues = NN_TRY(convert(model.operandValues)),
            .pools = NN_TRY(convert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
    };
}

Result<ExecutionPreference> convert(const hal::V1_1::ExecutionPreference& executionPreference) {
    return static_cast<ExecutionPreference>(executionPreference);
}

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_1::utils {
namespace {

using utils::convert;

nn::Result<V1_0::PerformanceInfo> convert(
        const nn::Capabilities::PerformanceInfo& performanceInfo) {
    return V1_0::utils::convert(performanceInfo);
}

nn::Result<V1_0::Operand> convert(const nn::Operand& operand) {
    return V1_0::utils::convert(operand);
}

nn::Result<hidl_vec<uint8_t>> convert(const nn::Model::OperandValues& operandValues) {
    return V1_0::utils::convert(operandValues);
}

nn::Result<hidl_memory> convert(const nn::Memory& memory) {
    return V1_0::utils::convert(memory);
}

template <typename Input>
using convertOutput = std::decay_t<decltype(convert(std::declval<Input>()).value())>;

template <typename Type>
nn::Result<hidl_vec<convertOutput<Type>>> convert(const std::vector<Type>& arguments) {
    hidl_vec<convertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(convert(arguments[i]));
    }
    return halObject;
}

}  // anonymous namespace

nn::Result<OperationType> convert(const nn::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

nn::Result<Capabilities> convert(const nn::Capabilities& capabilities) {
    return Capabilities{
            .float32Performance = NN_TRY(convert(
                    capabilities.operandPerformance.lookup(nn::OperandType::TENSOR_FLOAT32))),
            .quantized8Performance = NN_TRY(convert(
                    capabilities.operandPerformance.lookup(nn::OperandType::TENSOR_QUANT8_ASYMM))),
            .relaxedFloat32toFloat16Performance =
                    NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceTensor)),
    };
}

nn::Result<Operation> convert(const nn::Operation& operation) {
    return Operation{
            .type = NN_TRY(convert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

nn::Result<Model> convert(const nn::Model& model) {
    if (!hal::utils::hasNoPointerData(model)) {
        return NN_ERROR() << "Mdoel cannot be converted because it contains pointer-based memory";
    }

    auto operands = NN_TRY(convert(model.main.operands));

    // Update number of consumers.
    const auto numberOfConsumers =
            hal::utils::countNumberOfConsumers(operands.size(), model.main.operations);
    CHECK(operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < operands.size(); ++i) {
        operands[i].numberOfConsumers = numberOfConsumers[i];
    }

    return Model{
            .operands = std::move(operands),
            .operations = NN_TRY(convert(model.main.operations)),
            .inputIndexes = model.main.inputIndexes,
            .outputIndexes = model.main.outputIndexes,
            .operandValues = NN_TRY(convert(model.operandValues)),
            .pools = NN_TRY(convert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
    };
}

nn::Result<ExecutionPreference> convert(const nn::ExecutionPreference& executionPreference) {
    return static_cast<ExecutionPreference>(executionPreference);
}

}  // namespace android::hardware::neuralnetworks::V1_1::utils
