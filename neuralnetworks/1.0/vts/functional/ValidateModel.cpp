/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "1.0/Callbacks.h"
#include "GeneratedTestHarness.h"
#include "VtsHalNeuralnetworks.h"

namespace android::hardware::neuralnetworks::V1_0::vts::functional {

using implementation::PreparedModelCallback;

using PrepareModelMutation = std::function<void(Model*)>;

///////////////////////// UTILITY FUNCTIONS /////////////////////////

static void validateGetSupportedOperations(const sp<IDevice>& device, const std::string& message,
                                           const Model& model) {
    SCOPED_TRACE(message + " [getSupportedOperations]");

    Return<void> ret =
            device->getSupportedOperations(model, [&](ErrorStatus status, const hidl_vec<bool>&) {
                EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
            });
    EXPECT_TRUE(ret.isOk());
}

static void validatePrepareModel(const sp<IDevice>& device, const std::string& message,
                                 const Model& model) {
    SCOPED_TRACE(message + " [prepareModel]");

    sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    Return<ErrorStatus> prepareLaunchStatus = device->prepareModel(model, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(prepareLaunchStatus));

    preparedModelCallback->wait();
    ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, prepareReturnStatus);
    sp<IPreparedModel> preparedModel = preparedModelCallback->getPreparedModel();
    ASSERT_EQ(nullptr, preparedModel.get());
}

// Primary validation function. This function will take a valid model, apply a
// mutation to invalidate the model, then pass these to supportedOperations and
// prepareModel.
static void validate(const sp<IDevice>& device, const std::string& message,
                     const Model& originalModel, const PrepareModelMutation& mutate) {
    Model model = originalModel;
    mutate(&model);

    validateGetSupportedOperations(device, message, model);
    validatePrepareModel(device, message, model);
}

// Delete element from hidl_vec. hidl_vec doesn't support a "remove" operation,
// so this is efficiently accomplished by moving the element to the end and
// resizing the hidl_vec to one less.
template <typename Type>
static void hidl_vec_removeAt(hidl_vec<Type>* vec, uint32_t index) {
    if (vec) {
        std::rotate(vec->begin() + index, vec->begin() + index + 1, vec->end());
        vec->resize(vec->size() - 1);
    }
}

template <typename Type>
static uint32_t hidl_vec_push_back(hidl_vec<Type>* vec, const Type& value) {
    // assume vec is valid
    const uint32_t index = vec->size();
    vec->resize(index + 1);
    (*vec)[index] = value;
    return index;
}

static uint32_t addOperand(Model* model) {
    return hidl_vec_push_back(&model->operands,
                              {
                                      .type = OperandType::INT32,
                                      .dimensions = {},
                                      .numberOfConsumers = 0,
                                      .scale = 0.0f,
                                      .zeroPoint = 0,
                                      .lifetime = OperandLifeTime::MODEL_INPUT,
                                      .location = {.poolIndex = 0, .offset = 0, .length = 0},
                              });
}

static uint32_t addOperand(Model* model, OperandLifeTime lifetime) {
    uint32_t index = addOperand(model);
    model->operands[index].numberOfConsumers = 1;
    model->operands[index].lifetime = lifetime;
    return index;
}

///////////////////////// VALIDATE MODEL OPERAND TYPE /////////////////////////

static const int32_t invalidOperandTypes[] = {
        static_cast<int32_t>(OperandType::FLOAT32) - 1,              // lower bound fundamental
        static_cast<int32_t>(OperandType::TENSOR_QUANT8_ASYMM) + 1,  // upper bound fundamental
        static_cast<int32_t>(OperandType::OEM) - 1,                  // lower bound OEM
        static_cast<int32_t>(OperandType::TENSOR_OEM_BYTE) + 1,      // upper bound OEM
};

static void mutateOperandTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        for (int32_t invalidOperandType : invalidOperandTypes) {
            const std::string message = "mutateOperandTypeTest: operand " +
                                        std::to_string(operand) + " set to value " +
                                        std::to_string(invalidOperandType);
            validate(device, message, model, [operand, invalidOperandType](Model* model) {
                model->operands[operand].type = static_cast<OperandType>(invalidOperandType);
            });
        }
    }
}

///////////////////////// VALIDATE OPERAND RANK /////////////////////////

static uint32_t getInvalidRank(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
            return 1;
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::TENSOR_QUANT8_ASYMM:
            return 0;
        default:
            return 0;
    }
}

static void mutateOperandRankTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const uint32_t invalidRank = getInvalidRank(model.operands[operand].type);
        const std::string message = "mutateOperandRankTest: operand " + std::to_string(operand) +
                                    " has rank of " + std::to_string(invalidRank);
        validate(device, message, model, [operand, invalidRank](Model* model) {
            model->operands[operand].dimensions = std::vector<uint32_t>(invalidRank, 0);
        });
    }
}

///////////////////////// VALIDATE OPERAND SCALE /////////////////////////

static float getInvalidScale(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
            return 1.0f;
        case OperandType::TENSOR_INT32:
            return -1.0f;
        case OperandType::TENSOR_QUANT8_ASYMM:
            return 0.0f;
        default:
            return 0.0f;
    }
}

static void mutateOperandScaleTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const float invalidScale = getInvalidScale(model.operands[operand].type);
        const std::string message = "mutateOperandScaleTest: operand " + std::to_string(operand) +
                                    " has scale of " + std::to_string(invalidScale);
        validate(device, message, model, [operand, invalidScale](Model* model) {
            model->operands[operand].scale = invalidScale;
        });
    }
}

///////////////////////// VALIDATE OPERAND ZERO POINT /////////////////////////

static std::vector<int32_t> getInvalidZeroPoints(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
            return {1};
        case OperandType::TENSOR_QUANT8_ASYMM:
            return {-1, 256};
        default:
            return {};
    }
}

static void mutateOperandZeroPointTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const std::vector<int32_t> invalidZeroPoints =
                getInvalidZeroPoints(model.operands[operand].type);
        for (int32_t invalidZeroPoint : invalidZeroPoints) {
            const std::string message = "mutateOperandZeroPointTest: operand " +
                                        std::to_string(operand) + " has zero point of " +
                                        std::to_string(invalidZeroPoint);
            validate(device, message, model, [operand, invalidZeroPoint](Model* model) {
                model->operands[operand].zeroPoint = invalidZeroPoint;
            });
        }
    }
}

///////////////////////// VALIDATE EXTRA ??? /////////////////////////

// TODO: Operand::lifetime
// TODO: Operand::location

///////////////////////// VALIDATE OPERATION OPERAND TYPE /////////////////////////

static void mutateOperand(Operand* operand, OperandType type) {
    Operand newOperand = *operand;
    newOperand.type = type;
    switch (type) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
            newOperand.dimensions = hidl_vec<uint32_t>();
            newOperand.scale = 0.0f;
            newOperand.zeroPoint = 0;
            break;
        case OperandType::TENSOR_FLOAT32:
            newOperand.dimensions =
                    operand->dimensions.size() > 0 ? operand->dimensions : hidl_vec<uint32_t>({1});
            newOperand.scale = 0.0f;
            newOperand.zeroPoint = 0;
            break;
        case OperandType::TENSOR_INT32:
            newOperand.dimensions =
                    operand->dimensions.size() > 0 ? operand->dimensions : hidl_vec<uint32_t>({1});
            newOperand.zeroPoint = 0;
            break;
        case OperandType::TENSOR_QUANT8_ASYMM:
            newOperand.dimensions =
                    operand->dimensions.size() > 0 ? operand->dimensions : hidl_vec<uint32_t>({1});
            newOperand.scale = operand->scale != 0.0f ? operand->scale : 1.0f;
            break;
        case OperandType::OEM:
        case OperandType::TENSOR_OEM_BYTE:
        default:
            break;
    }
    *operand = newOperand;
}

static bool mutateOperationOperandTypeSkip(size_t operand, const Model& model) {
    // LSH_PROJECTION's second argument is allowed to have any type. This is the
    // only operation that currently has a type that can be anything independent
    // from any other type. Changing the operand type to any other type will
    // result in a valid model for LSH_PROJECTION. If this is the case, skip the
    // test.
    for (const Operation& operation : model.operations) {
        if (operation.type == OperationType::LSH_PROJECTION && operand == operation.inputs[1]) {
            return true;
        }
    }
    return false;
}

static void mutateOperationOperandTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        if (mutateOperationOperandTypeSkip(operand, model)) {
            continue;
        }
        for (OperandType invalidOperandType : hidl_enum_range<OperandType>{}) {
            // Do not test OEM types
            if (invalidOperandType == model.operands[operand].type ||
                invalidOperandType == OperandType::OEM ||
                invalidOperandType == OperandType::TENSOR_OEM_BYTE) {
                continue;
            }
            const std::string message = "mutateOperationOperandTypeTest: operand " +
                                        std::to_string(operand) + " set to type " +
                                        toString(invalidOperandType);
            validate(device, message, model, [operand, invalidOperandType](Model* model) {
                mutateOperand(&model->operands[operand], invalidOperandType);
            });
        }
    }
}

///////////////////////// VALIDATE MODEL OPERATION TYPE /////////////////////////

static const int32_t invalidOperationTypes[] = {
        static_cast<int32_t>(OperationType::ADD) - 1,            // lower bound fundamental
        static_cast<int32_t>(OperationType::TANH) + 1,           // upper bound fundamental
        static_cast<int32_t>(OperationType::OEM_OPERATION) - 1,  // lower bound OEM
        static_cast<int32_t>(OperationType::OEM_OPERATION) + 1,  // upper bound OEM
};

static void mutateOperationTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (int32_t invalidOperationType : invalidOperationTypes) {
            const std::string message = "mutateOperationTypeTest: operation " +
                                        std::to_string(operation) + " set to value " +
                                        std::to_string(invalidOperationType);
            validate(device, message, model, [operation, invalidOperationType](Model* model) {
                model->operations[operation].type =
                        static_cast<OperationType>(invalidOperationType);
            });
        }
    }
}

///////////////////////// VALIDATE MODEL OPERATION INPUT OPERAND INDEX /////////////////////////

static void mutateOperationInputOperandIndexTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const uint32_t invalidOperand = model.operands.size();
        for (size_t input = 0; input < model.operations[operation].inputs.size(); ++input) {
            const std::string message = "mutateOperationInputOperandIndexTest: operation " +
                                        std::to_string(operation) + " input " +
                                        std::to_string(input);
            validate(device, message, model, [operation, input, invalidOperand](Model* model) {
                model->operations[operation].inputs[input] = invalidOperand;
            });
        }
    }
}

///////////////////////// VALIDATE MODEL OPERATION OUTPUT OPERAND INDEX /////////////////////////

static void mutateOperationOutputOperandIndexTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const uint32_t invalidOperand = model.operands.size();
        for (size_t output = 0; output < model.operations[operation].outputs.size(); ++output) {
            const std::string message = "mutateOperationOutputOperandIndexTest: operation " +
                                        std::to_string(operation) + " output " +
                                        std::to_string(output);
            validate(device, message, model, [operation, output, invalidOperand](Model* model) {
                model->operations[operation].outputs[output] = invalidOperand;
            });
        }
    }
}

///////////////////////// REMOVE OPERAND FROM EVERYTHING /////////////////////////

static void removeValueAndDecrementGreaterValues(hidl_vec<uint32_t>* vec, uint32_t value) {
    if (vec) {
        // remove elements matching "value"
        auto last = std::remove(vec->begin(), vec->end(), value);
        vec->resize(std::distance(vec->begin(), last));

        // decrement elements exceeding "value"
        std::transform(vec->begin(), vec->end(), vec->begin(),
                       [value](uint32_t v) { return v > value ? v-- : v; });
    }
}

static void removeOperand(Model* model, uint32_t index) {
    hidl_vec_removeAt(&model->operands, index);
    for (Operation& operation : model->operations) {
        removeValueAndDecrementGreaterValues(&operation.inputs, index);
        removeValueAndDecrementGreaterValues(&operation.outputs, index);
    }
    removeValueAndDecrementGreaterValues(&model->inputIndexes, index);
    removeValueAndDecrementGreaterValues(&model->outputIndexes, index);
}

static void removeOperandTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        const std::string message = "removeOperandTest: operand " + std::to_string(operand);
        validate(device, message, model,
                 [operand](Model* model) { removeOperand(model, operand); });
    }
}

///////////////////////// REMOVE OPERATION /////////////////////////

static void removeOperation(Model* model, uint32_t index) {
    for (uint32_t operand : model->operations[index].inputs) {
        model->operands[operand].numberOfConsumers--;
    }
    hidl_vec_removeAt(&model->operations, index);
}

static void removeOperationTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const std::string message = "removeOperationTest: operation " + std::to_string(operation);
        validate(device, message, model,
                 [operation](Model* model) { removeOperation(model, operation); });
    }
}

///////////////////////// REMOVE OPERATION INPUT /////////////////////////

static void removeOperationInputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (size_t input = 0; input < model.operations[operation].inputs.size(); ++input) {
            const Operation& op = model.operations[operation];
            // CONCATENATION has at least 2 inputs, with the last element being
            // INT32. Skip this test if removing one of CONCATENATION's
            // inputs still produces a valid model.
            if (op.type == OperationType::CONCATENATION && op.inputs.size() > 2 &&
                input != op.inputs.size() - 1) {
                continue;
            }
            const std::string message = "removeOperationInputTest: operation " +
                                        std::to_string(operation) + ", input " +
                                        std::to_string(input);
            validate(device, message, model, [operation, input](Model* model) {
                uint32_t operand = model->operations[operation].inputs[input];
                model->operands[operand].numberOfConsumers--;
                hidl_vec_removeAt(&model->operations[operation].inputs, input);
            });
        }
    }
}

///////////////////////// REMOVE OPERATION OUTPUT /////////////////////////

static void removeOperationOutputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (size_t output = 0; output < model.operations[operation].outputs.size(); ++output) {
            const std::string message = "removeOperationOutputTest: operation " +
                                        std::to_string(operation) + ", output " +
                                        std::to_string(output);
            validate(device, message, model, [operation, output](Model* model) {
                hidl_vec_removeAt(&model->operations[operation].outputs, output);
            });
        }
    }
}

///////////////////////// MODEL VALIDATION /////////////////////////

// TODO: remove model input
// TODO: remove model output
// TODO: add unused operation

///////////////////////// ADD OPERATION INPUT /////////////////////////

static void addOperationInputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const std::string message = "addOperationInputTest: operation " + std::to_string(operation);
        validate(device, message, model, [operation](Model* model) {
            uint32_t index = addOperand(model, OperandLifeTime::MODEL_INPUT);
            hidl_vec_push_back(&model->operations[operation].inputs, index);
            hidl_vec_push_back(&model->inputIndexes, index);
        });
    }
}

///////////////////////// ADD OPERATION OUTPUT /////////////////////////

static void addOperationOutputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        const std::string message =
                "addOperationOutputTest: operation " + std::to_string(operation);
        validate(device, message, model, [operation](Model* model) {
            uint32_t index = addOperand(model, OperandLifeTime::MODEL_OUTPUT);
            hidl_vec_push_back(&model->operations[operation].outputs, index);
            hidl_vec_push_back(&model->outputIndexes, index);
        });
    }
}

////////////////////////// ENTRY POINT //////////////////////////////

void validateModel(const sp<IDevice>& device, const Model& model) {
    mutateOperandTypeTest(device, model);
    mutateOperandRankTest(device, model);
    mutateOperandScaleTest(device, model);
    mutateOperandZeroPointTest(device, model);
    mutateOperationOperandTypeTest(device, model);
    mutateOperationTypeTest(device, model);
    mutateOperationInputOperandIndexTest(device, model);
    mutateOperationOutputOperandIndexTest(device, model);
    removeOperandTest(device, model);
    removeOperationTest(device, model);
    removeOperationInputTest(device, model);
    removeOperationOutputTest(device, model);
    addOperationInputTest(device, model);
    addOperationOutputTest(device, model);
}

}  // namespace android::hardware::neuralnetworks::V1_0::vts::functional
