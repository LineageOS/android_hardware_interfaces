/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "Models.h"
#include "Utils.h"

#include <android-base/logging.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <vector>

using ::android::sp;

namespace android {
namespace hardware {
namespace neuralnetworks {

// create a valid model
V1_1::Model createValidTestModel_1_1() {
    const std::vector<float> operand2Data = {5.0f, 6.0f, 7.0f, 8.0f};
    const uint32_t size = operand2Data.size() * sizeof(float);

    const uint32_t operand1 = 0;
    const uint32_t operand2 = 1;
    const uint32_t operand3 = 2;
    const uint32_t operand4 = 3;

    const std::vector<Operand> operands = {
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {1, 2, 2, 1},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::MODEL_INPUT,
            .location = {.poolIndex = 0, .offset = 0, .length = 0},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {1, 2, 2, 1},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = 0, .length = size},
        },
        {
            .type = OperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::CONSTANT_COPY,
            .location = {.poolIndex = 0, .offset = size, .length = sizeof(int32_t)},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {1, 2, 2, 1},
            .numberOfConsumers = 0,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = OperandLifeTime::MODEL_OUTPUT,
            .location = {.poolIndex = 0, .offset = 0, .length = 0},
        },
    };

    const std::vector<Operation> operations = {{
        .type = OperationType::ADD, .inputs = {operand1, operand2, operand3}, .outputs = {operand4},
    }};

    const std::vector<uint32_t> inputIndexes = {operand1};
    const std::vector<uint32_t> outputIndexes = {operand4};
    std::vector<uint8_t> operandValues(
        reinterpret_cast<const uint8_t*>(operand2Data.data()),
        reinterpret_cast<const uint8_t*>(operand2Data.data()) + size);
    int32_t activation[1] = {static_cast<int32_t>(FusedActivationFunc::NONE)};
    operandValues.insert(operandValues.end(), reinterpret_cast<const uint8_t*>(&activation[0]),
                         reinterpret_cast<const uint8_t*>(&activation[1]));

    const std::vector<hidl_memory> pools = {};

    return {
        .operands = operands,
        .operations = operations,
        .inputIndexes = inputIndexes,
        .outputIndexes = outputIndexes,
        .operandValues = operandValues,
        .pools = pools,
    };
}

// create first invalid model
V1_1::Model createInvalidTestModel1_1_1() {
    Model model = createValidTestModel_1_1();
    model.operations[0].type = static_cast<OperationType>(0xDEADBEEF); /* INVALID */
    return model;
}

// create second invalid model
V1_1::Model createInvalidTestModel2_1_1() {
    Model model = createValidTestModel_1_1();
    const uint32_t operand1 = 0;
    const uint32_t operand5 = 4;  // INVALID OPERAND
    model.inputIndexes = std::vector<uint32_t>({operand1, operand5 /* INVALID OPERAND */});
    return model;
}

V1_0::Model createValidTestModel_1_0() {
    V1_1::Model model = createValidTestModel_1_1();
    return nn::convertToV1_0(model);
}

V1_0::Model createInvalidTestModel1_1_0() {
    V1_1::Model model = createInvalidTestModel1_1_1();
    return nn::convertToV1_0(model);
}

V1_0::Model createInvalidTestModel2_1_0() {
    V1_1::Model model = createInvalidTestModel2_1_1();
    return nn::convertToV1_0(model);
}

// create a valid request
Request createValidTestRequest() {
    std::vector<float> inputData = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> outputData = {-1.0f, -1.0f, -1.0f, -1.0f};
    const uint32_t INPUT = 0;
    const uint32_t OUTPUT = 1;

    // prepare inputs
    uint32_t inputSize = static_cast<uint32_t>(inputData.size() * sizeof(float));
    uint32_t outputSize = static_cast<uint32_t>(outputData.size() * sizeof(float));
    std::vector<RequestArgument> inputs = {{
        .location = {.poolIndex = INPUT, .offset = 0, .length = inputSize}, .dimensions = {},
    }};
    std::vector<RequestArgument> outputs = {{
        .location = {.poolIndex = OUTPUT, .offset = 0, .length = outputSize}, .dimensions = {},
    }};
    std::vector<hidl_memory> pools = {nn::allocateSharedMemory(inputSize),
                                      nn::allocateSharedMemory(outputSize)};
    if (pools[INPUT].size() == 0 || pools[OUTPUT].size() == 0) {
        return {};
    }

    // load data
    sp<IMemory> inputMemory = mapMemory(pools[INPUT]);
    sp<IMemory> outputMemory = mapMemory(pools[OUTPUT]);
    if (inputMemory.get() == nullptr || outputMemory.get() == nullptr) {
        return {};
    }
    float* inputPtr = reinterpret_cast<float*>(static_cast<void*>(inputMemory->getPointer()));
    float* outputPtr = reinterpret_cast<float*>(static_cast<void*>(outputMemory->getPointer()));
    if (inputPtr == nullptr || outputPtr == nullptr) {
        return {};
    }
    inputMemory->update();
    outputMemory->update();
    std::copy(inputData.begin(), inputData.end(), inputPtr);
    std::copy(outputData.begin(), outputData.end(), outputPtr);
    inputMemory->commit();
    outputMemory->commit();

    return {.inputs = inputs, .outputs = outputs, .pools = pools};
}

// create first invalid request
Request createInvalidTestRequest1() {
    Request request = createValidTestRequest();
    const uint32_t INVALID = 2;
    std::vector<float> inputData = {1.0f, 2.0f, 3.0f, 4.0f};
    uint32_t inputSize = static_cast<uint32_t>(inputData.size() * sizeof(float));
    request.inputs[0].location = {
        .poolIndex = INVALID /* INVALID */, .offset = 0, .length = inputSize};
    return request;
}

// create second invalid request
Request createInvalidTestRequest2() {
    Request request = createValidTestRequest();
    request.inputs[0].dimensions = std::vector<uint32_t>({1, 2, 3, 4, 5, 6, 7, 8} /* INVALID */);
    return request;
}

}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
