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

#include "GeneratedTestHarness.h"

#include "1.0/Callbacks.h"
#include "1.0/Utils.h"
#include "MemoryUtils.h"
#include "TestHarness.h"
#include "VtsHalNeuralnetworks.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include <gtest/gtest.h>
#include <iostream>

namespace android::hardware::neuralnetworks::V1_0::vts::functional {

using namespace test_helper;
using hidl::memory::V1_0::IMemory;
using implementation::ExecutionCallback;
using implementation::PreparedModelCallback;

Model createModel(const TestModel& testModel) {
    // Model operands.
    CHECK_EQ(testModel.referenced.size(), 0u);  // Not supported in 1.0.
    hidl_vec<Operand> operands(testModel.main.operands.size());
    size_t constCopySize = 0, constRefSize = 0;
    for (uint32_t i = 0; i < testModel.main.operands.size(); i++) {
        const auto& op = testModel.main.operands[i];

        DataLocation loc = {};
        if (op.lifetime == TestOperandLifeTime::CONSTANT_COPY) {
            loc = {.poolIndex = 0,
                   .offset = static_cast<uint32_t>(constCopySize),
                   .length = static_cast<uint32_t>(op.data.size())};
            constCopySize += op.data.alignedSize();
        } else if (op.lifetime == TestOperandLifeTime::CONSTANT_REFERENCE) {
            loc = {.poolIndex = 0,
                   .offset = static_cast<uint32_t>(constRefSize),
                   .length = static_cast<uint32_t>(op.data.size())};
            constRefSize += op.data.alignedSize();
        }

        operands[i] = {.type = static_cast<OperandType>(op.type),
                       .dimensions = op.dimensions,
                       .numberOfConsumers = op.numberOfConsumers,
                       .scale = op.scale,
                       .zeroPoint = op.zeroPoint,
                       .lifetime = static_cast<OperandLifeTime>(op.lifetime),
                       .location = loc};
    }

    // Model operations.
    hidl_vec<Operation> operations(testModel.main.operations.size());
    std::transform(testModel.main.operations.begin(), testModel.main.operations.end(),
                   operations.begin(), [](const TestOperation& op) -> Operation {
                       return {.type = static_cast<OperationType>(op.type),
                               .inputs = op.inputs,
                               .outputs = op.outputs};
                   });

    // Constant copies.
    hidl_vec<uint8_t> operandValues(constCopySize);
    for (uint32_t i = 0; i < testModel.main.operands.size(); i++) {
        const auto& op = testModel.main.operands[i];
        if (op.lifetime == TestOperandLifeTime::CONSTANT_COPY) {
            const uint8_t* begin = op.data.get<uint8_t>();
            const uint8_t* end = begin + op.data.size();
            std::copy(begin, end, operandValues.data() + operands[i].location.offset);
        }
    }

    // Shared memory.
    hidl_vec<hidl_memory> pools;
    if (constRefSize > 0) {
        hidl_vec_push_back(&pools, nn::allocateSharedMemory(constRefSize));
        CHECK_NE(pools[0].size(), 0u);

        // load data
        sp<IMemory> mappedMemory = mapMemory(pools[0]);
        CHECK(mappedMemory.get() != nullptr);
        uint8_t* mappedPtr =
                reinterpret_cast<uint8_t*>(static_cast<void*>(mappedMemory->getPointer()));
        CHECK(mappedPtr != nullptr);

        for (uint32_t i = 0; i < testModel.main.operands.size(); i++) {
            const auto& op = testModel.main.operands[i];
            if (op.lifetime == TestOperandLifeTime::CONSTANT_REFERENCE) {
                const uint8_t* begin = op.data.get<uint8_t>();
                const uint8_t* end = begin + op.data.size();
                std::copy(begin, end, mappedPtr + operands[i].location.offset);
            }
        }
    }

    return {.operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = testModel.main.inputIndexes,
            .outputIndexes = testModel.main.outputIndexes,
            .operandValues = std::move(operandValues),
            .pools = std::move(pools)};
}

// Top level driver for models and examples generated by test_generator.py
// Test driver for those generated from ml/nn/runtime/test/spec
void Execute(const sp<IDevice>& device, const TestModel& testModel) {
    const Model model = createModel(testModel);

    ExecutionContext context;
    const Request request = context.createRequest(testModel);

    // Create IPreparedModel.
    sp<IPreparedModel> preparedModel;
    createPreparedModel(device, model, &preparedModel);
    if (preparedModel == nullptr) return;

    // Launch execution.
    sp<ExecutionCallback> executionCallback = new ExecutionCallback();
    Return<ErrorStatus> executionLaunchStatus = preparedModel->execute(request, executionCallback);
    ASSERT_TRUE(executionLaunchStatus.isOk());
    EXPECT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(executionLaunchStatus));

    // Retrieve execution status.
    executionCallback->wait();
    ASSERT_EQ(ErrorStatus::NONE, executionCallback->getStatus());

    // Retrieve execution results.
    const std::vector<TestBuffer> outputs = context.getOutputBuffers(request);

    // We want "close-enough" results.
    checkResults(testModel, outputs);
}

void GeneratedTestBase::SetUp() {
    testing::TestWithParam<GeneratedTestParam>::SetUp();
    ASSERT_NE(kDevice, nullptr);
}

std::vector<NamedModel> getNamedModels(const FilterFn& filter) {
    return TestModelManager::get().getTestModels(filter);
}

std::vector<NamedModel> getNamedModels(const FilterNameFn& filter) {
    return TestModelManager::get().getTestModels(filter);
}

std::string printGeneratedTest(const testing::TestParamInfo<GeneratedTestParam>& info) {
    const auto& [namedDevice, namedModel] = info.param;
    return gtestCompliantName(getName(namedDevice) + "_" + getName(namedModel));
}

// Tag for the generated tests
class GeneratedTest : public GeneratedTestBase {};

TEST_P(GeneratedTest, Test) {
    Execute(kDevice, kTestModel);
}

INSTANTIATE_GENERATED_TEST(GeneratedTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

}  // namespace android::hardware::neuralnetworks::V1_0::vts::functional
