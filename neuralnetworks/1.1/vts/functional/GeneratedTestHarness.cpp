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

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include <gtest/gtest.h>
#include <iostream>

#include "1.0/Callbacks.h"
#include "1.0/Utils.h"
#include "MemoryUtils.h"
#include "TestHarness.h"
#include "VtsHalNeuralnetworks.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_1 {
namespace vts {
namespace functional {

using namespace test_helper;
using ::android::hardware::neuralnetworks::V1_0::DataLocation;
using ::android::hardware::neuralnetworks::V1_0::ErrorStatus;
using ::android::hardware::neuralnetworks::V1_0::IPreparedModel;
using ::android::hardware::neuralnetworks::V1_0::Operand;
using ::android::hardware::neuralnetworks::V1_0::OperandLifeTime;
using ::android::hardware::neuralnetworks::V1_0::OperandType;
using ::android::hardware::neuralnetworks::V1_0::Request;
using ::android::hardware::neuralnetworks::V1_0::RequestArgument;
using ::android::hardware::neuralnetworks::V1_0::implementation::ExecutionCallback;
using ::android::hardware::neuralnetworks::V1_0::implementation::PreparedModelCallback;
using ::android::hardware::neuralnetworks::V1_1::ExecutionPreference;
using ::android::hardware::neuralnetworks::V1_1::IDevice;
using ::android::hardware::neuralnetworks::V1_1::Model;
using ::android::hidl::memory::V1_0::IMemory;

Model createModel(const TestModel& testModel) {
    // Model operands.
    hidl_vec<Operand> operands(testModel.operands.size());
    size_t constCopySize = 0, constRefSize = 0;
    for (uint32_t i = 0; i < testModel.operands.size(); i++) {
        const auto& op = testModel.operands[i];

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
    hidl_vec<Operation> operations(testModel.operations.size());
    std::transform(testModel.operations.begin(), testModel.operations.end(), operations.begin(),
                   [](const TestOperation& op) -> Operation {
                       return {.type = static_cast<OperationType>(op.type),
                               .inputs = op.inputs,
                               .outputs = op.outputs};
                   });

    // Constant copies.
    hidl_vec<uint8_t> operandValues(constCopySize);
    for (uint32_t i = 0; i < testModel.operands.size(); i++) {
        const auto& op = testModel.operands[i];
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

        for (uint32_t i = 0; i < testModel.operands.size(); i++) {
            const auto& op = testModel.operands[i];
            if (op.lifetime == TestOperandLifeTime::CONSTANT_REFERENCE) {
                const uint8_t* begin = op.data.get<uint8_t>();
                const uint8_t* end = begin + op.data.size();
                std::copy(begin, end, mappedPtr + operands[i].location.offset);
            }
        }
    }

    return {.operands = std::move(operands),
            .operations = std::move(operations),
            .inputIndexes = testModel.inputIndexes,
            .outputIndexes = testModel.outputIndexes,
            .operandValues = std::move(operandValues),
            .pools = std::move(pools),
            .relaxComputationFloat32toFloat16 = testModel.isRelaxed};
}

// Top level driver for models and examples generated by test_generator.py
// Test driver for those generated from ml/nn/runtime/test/spec
void EvaluatePreparedModel(const sp<IPreparedModel>& preparedModel, const TestModel& testModel) {
    const Request request = createRequest(testModel);

    // Launch execution.
    sp<ExecutionCallback> executionCallback = new ExecutionCallback();
    Return<ErrorStatus> executionLaunchStatus = preparedModel->execute(request, executionCallback);
    ASSERT_TRUE(executionLaunchStatus.isOk());
    EXPECT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(executionLaunchStatus));

    // Retrieve execution status.
    executionCallback->wait();
    ASSERT_EQ(ErrorStatus::NONE, executionCallback->getStatus());

    // Retrieve execution results.
    const std::vector<TestBuffer> outputs = getOutputBuffers(request);

    // We want "close-enough" results.
    checkResults(testModel, outputs);
}

// Tag for the generated tests
class GeneratedTest : public GeneratedTestBase {
  protected:
    void Execute(const TestModel& testModel) {
        Model model = createModel(testModel);

        // see if service can handle model
        bool fullySupportsModel = false;
        Return<void> supportedCall = device->getSupportedOperations_1_1(
                model, [&fullySupportsModel](ErrorStatus status, const hidl_vec<bool>& supported) {
                    ASSERT_EQ(ErrorStatus::NONE, status);
                    ASSERT_NE(0ul, supported.size());
                    fullySupportsModel = std::all_of(supported.begin(), supported.end(),
                                                     [](bool valid) { return valid; });
                });
        ASSERT_TRUE(supportedCall.isOk());

        // launch prepare model
        sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
        Return<ErrorStatus> prepareLaunchStatus = device->prepareModel_1_1(
                model, ExecutionPreference::FAST_SINGLE_ANSWER, preparedModelCallback);
        ASSERT_TRUE(prepareLaunchStatus.isOk());
        ASSERT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(prepareLaunchStatus));

        // retrieve prepared model
        preparedModelCallback->wait();
        ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
        sp<IPreparedModel> preparedModel = preparedModelCallback->getPreparedModel();

        // early termination if vendor service cannot fully prepare model
        if (!fullySupportsModel && prepareReturnStatus != ErrorStatus::NONE) {
            ASSERT_EQ(nullptr, preparedModel.get());
            LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                         "prepare model that it does not support.";
            std::cout << "[          ]   Early termination of test because vendor service cannot "
                         "prepare model that it does not support."
                      << std::endl;
            GTEST_SKIP();
        }
        EXPECT_EQ(ErrorStatus::NONE, prepareReturnStatus);
        ASSERT_NE(nullptr, preparedModel.get());

        EvaluatePreparedModel(preparedModel, testModel);
    }
};

TEST_P(GeneratedTest, Test) {
    Execute(*mTestModel);
}

INSTANTIATE_GENERATED_TEST(GeneratedTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

}  // namespace functional
}  // namespace vts
}  // namespace V1_1
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
