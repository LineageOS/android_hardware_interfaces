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

#include "VtsHalNeuralnetworks.h"

#include "Callbacks.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {

using V1_0::IPreparedModel;
using V1_0::OperandLifeTime;
using V1_1::ExecutionPreference;

namespace vts {
namespace functional {

using ::android::hardware::neuralnetworks::V1_0::implementation::ExecutionCallback;
using ::android::hardware::neuralnetworks::V1_0::implementation::PreparedModelCallback;

///////////////////////// UTILITY FUNCTIONS /////////////////////////

static void validateGetSupportedOperations(const sp<IDevice>& device, const std::string& message,
                                           const Model& model) {
    SCOPED_TRACE(message + " [getSupportedOperations_1_2]");

    Return<void> ret =
        device->getSupportedOperations_1_2(model, [&](ErrorStatus status, const hidl_vec<bool>&) {
            EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
        });
    EXPECT_TRUE(ret.isOk());
}

static void validatePrepareModel(const sp<IDevice>& device, const std::string& message,
                                 const Model& model, ExecutionPreference preference) {
    SCOPED_TRACE(message + " [prepareModel_1_2]");

    sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    ASSERT_NE(nullptr, preparedModelCallback.get());
    Return<ErrorStatus> prepareLaunchStatus =
        device->prepareModel_1_2(model, preference, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(prepareLaunchStatus));

    preparedModelCallback->wait();
    ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, prepareReturnStatus);
    sp<IPreparedModel> preparedModel = preparedModelCallback->getPreparedModel();
    ASSERT_EQ(nullptr, preparedModel.get());
}

static bool validExecutionPreference(ExecutionPreference preference) {
    return preference == ExecutionPreference::LOW_POWER ||
           preference == ExecutionPreference::FAST_SINGLE_ANSWER ||
           preference == ExecutionPreference::SUSTAINED_SPEED;
}

// Primary validation function. This function will take a valid model, apply a
// mutation to it to invalidate the model, then pass it to interface calls that
// use the model. Note that the model here is passed by value, and any mutation
// to the model does not leave this function.
static void validate(const sp<IDevice>& device, const std::string& message, Model model,
                     const std::function<void(Model*)>& mutation,
                     ExecutionPreference preference = ExecutionPreference::FAST_SINGLE_ANSWER) {
    mutation(&model);
    if (validExecutionPreference(preference)) {
        validateGetSupportedOperations(device, message, model);
    }
    validatePrepareModel(device, message, model, preference);
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

static const uint32_t invalidOperandTypes[] = {
    static_cast<uint32_t>(OperandTypeRange::OPERAND_FUNDAMENTAL_MIN) - 1,
    static_cast<uint32_t>(OperandTypeRange::OPERAND_FUNDAMENTAL_MAX) + 1,
    static_cast<uint32_t>(OperandTypeRange::OPERAND_OEM_MIN) - 1,
    static_cast<uint32_t>(OperandTypeRange::OPERAND_OEM_MAX) + 1,
};

static void mutateOperandTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        for (uint32_t invalidOperandType : invalidOperandTypes) {
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
        case OperandType::BOOL:
            return 1;
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::TENSOR_QUANT16_SYMM:
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
        case OperandType::BOOL:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_FLOAT32:
            return 1.0f;
        case OperandType::TENSOR_INT32:
            return -1.0f;
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::TENSOR_QUANT16_SYMM:
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
        case OperandType::BOOL:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
            return {1};
        case OperandType::TENSOR_QUANT8_ASYMM:
            return {-1, 256};
        case OperandType::TENSOR_QUANT16_SYMM:
            return {-32769, -1, 1, 32768};
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
        case OperandType::BOOL:
            newOperand.dimensions = hidl_vec<uint32_t>();
            newOperand.scale = 0.0f;
            newOperand.zeroPoint = 0;
            break;
        case OperandType::TENSOR_FLOAT16:
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
        case OperandType::TENSOR_QUANT16_SYMM:
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

static bool mutateOperationOperandTypeSkip(size_t operand, OperandType type, const Model& model) {
    // Do not test OEM types
    if (type == model.operands[operand].type || type == OperandType::OEM ||
        type == OperandType::TENSOR_OEM_BYTE) {
        return true;
    }
    for (const Operation& operation : model.operations) {
        // Skip mutateOperationOperandTypeTest for the following operations.
        // - LSH_PROJECTION's second argument is allowed to have any type.
        // - ARGMIN and ARGMAX's first argument can be any of TENSOR_(FLOAT32|INT32|QUANT8_ASYMM).
        // - CAST's argument can be any of TENSOR_(FLOAT32|INT32|QUANT8_ASYMM).
        switch (operation.type) {
            case OperationType::LSH_PROJECTION: {
                if (operand == operation.inputs[1]) {
                    return true;
                }
            } break;
            case OperationType::CAST:
            case OperationType::ARGMAX:
            case OperationType::ARGMIN: {
                if (type == OperandType::TENSOR_FLOAT32 || type == OperandType::TENSOR_INT32 ||
                    type == OperandType::TENSOR_QUANT8_ASYMM) {
                    return true;
                }
            } break;
            default:
                break;
        }
    }
    return false;
}

static void mutateOperationOperandTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        for (OperandType invalidOperandType : hidl_enum_range<OperandType>{}) {
            if (mutateOperationOperandTypeSkip(operand, invalidOperandType, model)) {
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

static const uint32_t invalidOperationTypes[] = {
    static_cast<uint32_t>(OperationTypeRange::OPERATION_FUNDAMENTAL_MIN) - 1,
    static_cast<uint32_t>(OperationTypeRange::OPERATION_FUNDAMENTAL_MAX) + 1,
    static_cast<uint32_t>(OperationTypeRange::OPERATION_OEM_MIN) - 1,
    static_cast<uint32_t>(OperationTypeRange::OPERATION_OEM_MAX) + 1,
};

static void mutateOperationTypeTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (uint32_t invalidOperationType : invalidOperationTypes) {
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

static bool removeOperandSkip(size_t operand, const Model& model) {
    for (const Operation& operation : model.operations) {
        // Skip removeOperandTest for the following operations.
        // - SPLIT's outputs are not checked during prepareModel.
        if (operation.type == OperationType::SPLIT) {
            for (const size_t outOprand : operation.outputs) {
                if (operand == outOprand) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void removeOperandTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operand = 0; operand < model.operands.size(); ++operand) {
        if (removeOperandSkip(operand, model)) {
            continue;
        }
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

static bool removeOperationInputSkip(const Operation& op, size_t input) {
    // Skip removeOperationInputTest for the following operations.
    // - CONCATENATION has at least 2 inputs, with the last element being INT32.
    // - CONV_2D, DEPTHWISE_CONV_2D, MAX_POOL_2D, AVERAGE_POOL_2D, L2_POOL_2D, RESIZE_BILINEAR,
    //   SPACE_TO_DEPTH, SPACE_TO_DEPTH, SPACE_TO_BATCH_ND, BATCH_TO_SPACE_ND can have an optional
    //   layout parameter.
    // - L2_NORMALIZATION, LOCAL_RESPONSE_NORMALIZATION, SOFTMAX can have an optional axis
    //   parameter.
    switch (op.type) {
        case OperationType::CONCATENATION: {
            if (op.inputs.size() > 2 && input != op.inputs.size() - 1) {
                return true;
            }
        } break;
        case OperationType::DEPTHWISE_CONV_2D: {
            if ((op.inputs.size() == 12 && input == 11) || (op.inputs.size() == 9 && input == 8)) {
                return true;
            }
        } break;
        case OperationType::CONV_2D:
        case OperationType::AVERAGE_POOL_2D:
        case OperationType::MAX_POOL_2D:
        case OperationType::L2_POOL_2D: {
            if ((op.inputs.size() == 11 && input == 10) || (op.inputs.size() == 8 && input == 7)) {
                return true;
            }
        } break;
        case OperationType::RESIZE_BILINEAR: {
            if (op.inputs.size() == 4 && input == 3) {
                return true;
            }
        } break;
        case OperationType::SPACE_TO_DEPTH:
        case OperationType::DEPTH_TO_SPACE:
        case OperationType::BATCH_TO_SPACE_ND: {
            if (op.inputs.size() == 3 && input == 2) {
                return true;
            }
        } break;
        case OperationType::SPACE_TO_BATCH_ND: {
            if (op.inputs.size() == 4 && input == 3) {
                return true;
            }
        } break;
        case OperationType::L2_NORMALIZATION: {
            if (op.inputs.size() == 2 && input == 1) {
                return true;
            }
        } break;
        case OperationType::LOCAL_RESPONSE_NORMALIZATION: {
            if (op.inputs.size() == 6 && input == 5) {
                return true;
            }
        } break;
        case OperationType::SOFTMAX: {
            if (op.inputs.size() == 3 && input == 2) {
                return true;
            }
        } break;
        default:
            break;
    }
    return false;
}

static void removeOperationInputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        for (size_t input = 0; input < model.operations[operation].inputs.size(); ++input) {
            const Operation& op = model.operations[operation];
            if (removeOperationInputSkip(op, input)) {
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

static bool addOperationInputSkip(const Operation& op) {
    // Skip addOperationInputTest for the following operations.
    // - L2_NORMALIZATION, LOCAL_RESPONSE_NORMALIZATION, SOFTMAX can have an optional INT32 axis
    //   parameter.
    if ((op.type == OperationType::L2_NORMALIZATION && op.inputs.size() == 1) ||
        (op.type == OperationType::LOCAL_RESPONSE_NORMALIZATION && op.inputs.size() == 5) ||
        (op.type == OperationType::SOFTMAX && op.inputs.size() == 2)) {
        return true;
    }
    return false;
}

static void addOperationInputTest(const sp<IDevice>& device, const Model& model) {
    for (size_t operation = 0; operation < model.operations.size(); ++operation) {
        if (addOperationInputSkip(model.operations[operation])) {
            continue;
        }
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

///////////////////////// VALIDATE EXECUTION PREFERENCE /////////////////////////

static const int32_t invalidExecutionPreferences[] = {
    static_cast<int32_t>(ExecutionPreference::LOW_POWER) - 1,        // lower bound
    static_cast<int32_t>(ExecutionPreference::SUSTAINED_SPEED) + 1,  // upper bound
};

static void mutateExecutionPreferenceTest(const sp<IDevice>& device, const Model& model) {
    for (int32_t preference : invalidExecutionPreferences) {
        const std::string message =
            "mutateExecutionPreferenceTest: preference " + std::to_string(preference);
        validate(device, message, model, [](Model*) {},
                 static_cast<ExecutionPreference>(preference));
    }
}

////////////////////////// ENTRY POINT //////////////////////////////

void ValidationTest::validateModel(const Model& model) {
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
    mutateExecutionPreferenceTest(device, model);
}

}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
