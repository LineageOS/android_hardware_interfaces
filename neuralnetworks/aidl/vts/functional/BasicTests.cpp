/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_aidl_hal_test"

#include <aidl/android/hardware/neuralnetworks/Capabilities.h>
#include <aidl/android/hardware/neuralnetworks/IDevice.h>
#include <aidl/android/hardware/neuralnetworks/Operand.h>
#include <aidl/android/hardware/neuralnetworks/OperandType.h>
#include <aidl/android/hardware/neuralnetworks/Priority.h>
#include <android/binder_interface_utils.h>

#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

namespace aidl::android::hardware::neuralnetworks::vts::functional {

using implementation::PreparedModelCallback;

// create device test
TEST_P(NeuralNetworksAidlTest, CreateDevice) {}

// initialization
TEST_P(NeuralNetworksAidlTest, GetCapabilitiesTest) {
    Capabilities capabilities;
    const auto retStatus = kDevice->getCapabilities(&capabilities);
    ASSERT_TRUE(retStatus.isOk());

    auto isPositive = [](const PerformanceInfo& perf) {
        return perf.execTime > 0.0f && perf.powerUsage > 0.0f;
    };

    EXPECT_TRUE(isPositive(capabilities.relaxedFloat32toFloat16PerformanceScalar));
    EXPECT_TRUE(isPositive(capabilities.relaxedFloat32toFloat16PerformanceTensor));
    const auto& opPerf = capabilities.operandPerformance;
    EXPECT_TRUE(
            std::all_of(opPerf.begin(), opPerf.end(),
                        [isPositive](const OperandPerformance& a) { return isPositive(a.info); }));
    EXPECT_TRUE(std::is_sorted(opPerf.begin(), opPerf.end(),
                               [](const OperandPerformance& a, const OperandPerformance& b) {
                                   return a.type < b.type;
                               }));
    EXPECT_TRUE(std::all_of(opPerf.begin(), opPerf.end(), [](const OperandPerformance& a) {
        return a.type != OperandType::SUBGRAPH;
    }));
    EXPECT_TRUE(isPositive(capabilities.ifPerformance));
    EXPECT_TRUE(isPositive(capabilities.whilePerformance));
}

// detect cycle
TEST_P(NeuralNetworksAidlTest, CycleTest) {
    // opnd0 = TENSOR_FLOAT32            // model input
    // opnd1 = TENSOR_FLOAT32            // model input
    // opnd2 = INT32                     // model input
    // opnd3 = ADD(opnd0, opnd4, opnd2)
    // opnd4 = ADD(opnd1, opnd3, opnd2)
    // opnd5 = ADD(opnd4, opnd0, opnd2)  // model output
    //
    //            +-----+
    //            |     |
    //            v     |
    // 3 = ADD(0, 4, 2) |
    // |                |
    // +----------+     |
    //            |     |
    //            v     |
    // 4 = ADD(1, 3, 2) |
    // |                |
    // +----------------+
    // |
    // |
    // +-------+
    //         |
    //         v
    // 5 = ADD(4, 0, 2)

    const std::vector<Operand> operands = {
            {
                    // operands[0]
                    .type = OperandType::TENSOR_FLOAT32,
                    .dimensions = {1},
                    .scale = 0.0f,
                    .zeroPoint = 0,
                    .lifetime = OperandLifeTime::SUBGRAPH_INPUT,
                    .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
            {
                    // operands[1]
                    .type = OperandType::TENSOR_FLOAT32,
                    .dimensions = {1},
                    .scale = 0.0f,
                    .zeroPoint = 0,
                    .lifetime = OperandLifeTime::SUBGRAPH_INPUT,
                    .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
            {
                    // operands[2]
                    .type = OperandType::INT32,
                    .dimensions = {},
                    .scale = 0.0f,
                    .zeroPoint = 0,
                    .lifetime = OperandLifeTime::SUBGRAPH_INPUT,
                    .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
            {
                    // operands[3]
                    .type = OperandType::TENSOR_FLOAT32,
                    .dimensions = {1},
                    .scale = 0.0f,
                    .zeroPoint = 0,
                    .lifetime = OperandLifeTime::TEMPORARY_VARIABLE,
                    .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
            {
                    // operands[4]
                    .type = OperandType::TENSOR_FLOAT32,
                    .dimensions = {1},
                    .scale = 0.0f,
                    .zeroPoint = 0,
                    .lifetime = OperandLifeTime::TEMPORARY_VARIABLE,
                    .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
            {
                    // operands[5]
                    .type = OperandType::TENSOR_FLOAT32,
                    .dimensions = {1},
                    .scale = 0.0f,
                    .zeroPoint = 0,
                    .lifetime = OperandLifeTime::SUBGRAPH_OUTPUT,
                    .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
    };

    const std::vector<Operation> operations = {
            {.type = OperationType::ADD, .inputs = {0, 4, 2}, .outputs = {3}},
            {.type = OperationType::ADD, .inputs = {1, 3, 2}, .outputs = {4}},
            {.type = OperationType::ADD, .inputs = {4, 0, 2}, .outputs = {5}},
    };

    Subgraph subgraph = {
            .operands = operands,
            .operations = operations,
            .inputIndexes = {0, 1, 2},
            .outputIndexes = {5},
    };
    const Model model = {
            .main = std::move(subgraph),
            .referenced = {},
            .operandValues = {},
            .pools = {},
    };

    // ensure that getSupportedOperations() checks model validity
    std::vector<bool> supportedOps;
    const auto supportedOpsStatus = kDevice->getSupportedOperations(model, &supportedOps);
    ASSERT_FALSE(supportedOpsStatus.isOk());
    ASSERT_EQ(supportedOpsStatus.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorStatus>(supportedOpsStatus.getServiceSpecificError()),
              ErrorStatus::INVALID_ARGUMENT);

    // ensure that prepareModel() checks model validity
    auto preparedModelCallback = ndk::SharedRefBase::make<PreparedModelCallback>();
    auto prepareLaunchStatus =
            kDevice->prepareModel(model, ExecutionPreference::FAST_SINGLE_ANSWER, kDefaultPriority,
                                  kNoDeadline, {}, {}, kEmptyCacheToken, preparedModelCallback);
    //     Note that preparation can fail for reasons other than an
    //     invalid model (invalid model should result in
    //     INVALID_ARGUMENT) -- for example, perhaps not all
    //     operations are supported, or perhaps the device hit some
    //     kind of capacity limit.
    ASSERT_FALSE(prepareLaunchStatus.isOk());
    EXPECT_EQ(prepareLaunchStatus.getExceptionCode(), EX_SERVICE_SPECIFIC);
    EXPECT_NE(static_cast<ErrorStatus>(prepareLaunchStatus.getServiceSpecificError()),
              ErrorStatus::NONE);

    EXPECT_NE(preparedModelCallback->getStatus(), ErrorStatus::NONE);
    EXPECT_EQ(preparedModelCallback->getPreparedModel(), nullptr);
}

}  // namespace aidl::android::hardware::neuralnetworks::vts::functional
