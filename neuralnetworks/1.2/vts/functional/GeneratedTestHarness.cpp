/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModelCallback.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <gtest/gtest.h>
#include <hidlmemory/mapping.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

#include "1.0/Utils.h"
#include "1.2/Callbacks.h"
#include "ExecutionBurstController.h"
#include "MemoryUtils.h"
#include "TestHarness.h"
#include "VtsHalNeuralnetworks.h"

namespace android::hardware::neuralnetworks::V1_2::vts::functional {

using namespace test_helper;
using hidl::memory::V1_0::IMemory;
using implementation::ExecutionCallback;
using implementation::PreparedModelCallback;
using V1_0::DataLocation;
using V1_0::ErrorStatus;
using V1_0::OperandLifeTime;
using V1_0::Request;
using V1_1::ExecutionPreference;
using HidlToken = hidl_array<uint8_t, static_cast<uint32_t>(Constant::BYTE_SIZE_OF_CACHE_TOKEN)>;

namespace {

enum class Executor { ASYNC, SYNC, BURST };

enum class OutputType { FULLY_SPECIFIED, UNSPECIFIED, INSUFFICIENT };

struct TestConfig {
    Executor executor;
    MeasureTiming measureTiming;
    OutputType outputType;
};

}  // namespace

Model createModel(const TestModel& testModel) {
    // Model operands.
    CHECK_EQ(testModel.referenced.size(), 0u);  // Not supported in 1.1.
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

        Operand::ExtraParams extraParams;
        if (op.type == TestOperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL) {
            extraParams.channelQuant(SymmPerChannelQuantParams{
                    .scales = op.channelQuant.scales, .channelDim = op.channelQuant.channelDim});
        }

        operands[i] = {.type = static_cast<OperandType>(op.type),
                       .dimensions = op.dimensions,
                       .numberOfConsumers = op.numberOfConsumers,
                       .scale = op.scale,
                       .zeroPoint = op.zeroPoint,
                       .lifetime = static_cast<OperandLifeTime>(op.lifetime),
                       .location = loc,
                       .extraParams = std::move(extraParams)};
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
    hidl_vec<hidl_memory> pools = {};
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
            .pools = std::move(pools),
            .relaxComputationFloat32toFloat16 = testModel.isRelaxed};
}

static bool isOutputSizeGreaterThanOne(const TestModel& testModel, uint32_t index) {
    const auto byteSize = testModel.main.operands[testModel.main.outputIndexes[index]].data.size();
    return byteSize > 1u;
}

static void makeOutputInsufficientSize(uint32_t outputIndex, Request* request) {
    auto& length = request->outputs[outputIndex].location.length;
    ASSERT_GT(length, 1u);
    length -= 1u;
}

static void makeOutputDimensionsUnspecified(Model* model) {
    for (auto i : model->outputIndexes) {
        auto& dims = model->operands[i].dimensions;
        std::fill(dims.begin(), dims.end(), 0);
    }
}

static Return<ErrorStatus> ExecutePreparedModel(const sp<IPreparedModel>& preparedModel,
                                                const Request& request, MeasureTiming measure,
                                                sp<ExecutionCallback>& callback) {
    return preparedModel->execute_1_2(request, measure, callback);
}
static Return<ErrorStatus> ExecutePreparedModel(const sp<IPreparedModel>& preparedModel,
                                                const Request& request, MeasureTiming measure,
                                                hidl_vec<OutputShape>* outputShapes,
                                                Timing* timing) {
    ErrorStatus result;
    Return<void> ret = preparedModel->executeSynchronously(
            request, measure,
            [&result, outputShapes, timing](ErrorStatus error, const hidl_vec<OutputShape>& shapes,
                                            const Timing& time) {
                result = error;
                *outputShapes = shapes;
                *timing = time;
            });
    if (!ret.isOk()) {
        return ErrorStatus::GENERAL_FAILURE;
    }
    return result;
}
static std::shared_ptr<::android::nn::ExecutionBurstController> CreateBurst(
        const sp<IPreparedModel>& preparedModel) {
    return android::nn::ExecutionBurstController::create(preparedModel,
                                                         std::chrono::microseconds{0});
}

void EvaluatePreparedModel(const sp<IPreparedModel>& preparedModel, const TestModel& testModel,
                           const TestConfig& testConfig) {
    // If output0 does not have size larger than one byte, we can not test with insufficient buffer.
    if (testConfig.outputType == OutputType::INSUFFICIENT &&
        !isOutputSizeGreaterThanOne(testModel, 0)) {
        return;
    }

    Request request = createRequest(testModel);
    if (testConfig.outputType == OutputType::INSUFFICIENT) {
        makeOutputInsufficientSize(/*outputIndex=*/0, &request);
    }

    ErrorStatus executionStatus;
    hidl_vec<OutputShape> outputShapes;
    Timing timing;
    switch (testConfig.executor) {
        case Executor::ASYNC: {
            SCOPED_TRACE("asynchronous");

            // launch execution
            sp<ExecutionCallback> executionCallback = new ExecutionCallback();
            Return<ErrorStatus> executionLaunchStatus = ExecutePreparedModel(
                    preparedModel, request, testConfig.measureTiming, executionCallback);
            ASSERT_TRUE(executionLaunchStatus.isOk());
            EXPECT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(executionLaunchStatus));

            // retrieve execution status
            executionCallback->wait();
            executionStatus = executionCallback->getStatus();
            outputShapes = executionCallback->getOutputShapes();
            timing = executionCallback->getTiming();

            break;
        }
        case Executor::SYNC: {
            SCOPED_TRACE("synchronous");

            // execute
            Return<ErrorStatus> executionReturnStatus = ExecutePreparedModel(
                    preparedModel, request, testConfig.measureTiming, &outputShapes, &timing);
            ASSERT_TRUE(executionReturnStatus.isOk());
            executionStatus = static_cast<ErrorStatus>(executionReturnStatus);

            break;
        }
        case Executor::BURST: {
            SCOPED_TRACE("burst");

            // create burst
            const std::shared_ptr<::android::nn::ExecutionBurstController> controller =
                    CreateBurst(preparedModel);
            ASSERT_NE(nullptr, controller.get());

            // create memory keys
            std::vector<intptr_t> keys(request.pools.size());
            for (size_t i = 0; i < keys.size(); ++i) {
                keys[i] = reinterpret_cast<intptr_t>(&request.pools[i]);
            }

            // execute burst
            int n;
            std::tie(n, outputShapes, timing, std::ignore) =
                    controller->compute(request, testConfig.measureTiming, keys);
            executionStatus = nn::legacyConvertResultCodeToErrorStatus(n);

            break;
        }
    }

    if (testConfig.outputType != OutputType::FULLY_SPECIFIED &&
        executionStatus == ErrorStatus::GENERAL_FAILURE) {
        LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                     "execute model that it does not support.";
        std::cout << "[          ]   Early termination of test because vendor service cannot "
                     "execute model that it does not support."
                  << std::endl;
        GTEST_SKIP();
    }
    if (testConfig.measureTiming == MeasureTiming::NO) {
        EXPECT_EQ(UINT64_MAX, timing.timeOnDevice);
        EXPECT_EQ(UINT64_MAX, timing.timeInDriver);
    } else {
        if (timing.timeOnDevice != UINT64_MAX && timing.timeInDriver != UINT64_MAX) {
            EXPECT_LE(timing.timeOnDevice, timing.timeInDriver);
        }
    }

    switch (testConfig.outputType) {
        case OutputType::FULLY_SPECIFIED:
            // If the model output operands are fully specified, outputShapes must be either
            // either empty, or have the same number of elements as the number of outputs.
            ASSERT_EQ(ErrorStatus::NONE, executionStatus);
            ASSERT_TRUE(outputShapes.size() == 0 ||
                        outputShapes.size() == testModel.main.outputIndexes.size());
            break;
        case OutputType::UNSPECIFIED:
            // If the model output operands are not fully specified, outputShapes must have
            // the same number of elements as the number of outputs.
            ASSERT_EQ(ErrorStatus::NONE, executionStatus);
            ASSERT_EQ(outputShapes.size(), testModel.main.outputIndexes.size());
            break;
        case OutputType::INSUFFICIENT:
            ASSERT_EQ(ErrorStatus::OUTPUT_INSUFFICIENT_SIZE, executionStatus);
            ASSERT_EQ(outputShapes.size(), testModel.main.outputIndexes.size());
            ASSERT_FALSE(outputShapes[0].isSufficient);
            return;
    }

    // Go through all outputs, check returned output shapes.
    for (uint32_t i = 0; i < outputShapes.size(); i++) {
        EXPECT_TRUE(outputShapes[i].isSufficient);
        const auto& expect = testModel.main.operands[testModel.main.outputIndexes[i]].dimensions;
        const std::vector<uint32_t> actual = outputShapes[i].dimensions;
        EXPECT_EQ(expect, actual);
    }

    // Retrieve execution results.
    const std::vector<TestBuffer> outputs = getOutputBuffers(request);

    // We want "close-enough" results.
    checkResults(testModel, outputs);
}

void EvaluatePreparedModel(const sp<IPreparedModel>& preparedModel, const TestModel& testModel,
                           bool testDynamicOutputShape) {
    std::vector<OutputType> outputTypesList;
    std::vector<MeasureTiming> measureTimingList;
    std::vector<Executor> executorList;

    if (testDynamicOutputShape) {
        outputTypesList = {OutputType::UNSPECIFIED, OutputType::INSUFFICIENT};
        measureTimingList = {MeasureTiming::NO, MeasureTiming::YES};
        executorList = {Executor::ASYNC, Executor::SYNC, Executor::BURST};
    } else {
        outputTypesList = {OutputType::FULLY_SPECIFIED};
        measureTimingList = {MeasureTiming::NO, MeasureTiming::YES};
        executorList = {Executor::ASYNC, Executor::SYNC, Executor::BURST};
    }

    for (const OutputType outputType : outputTypesList) {
        for (const MeasureTiming measureTiming : measureTimingList) {
            for (const Executor executor : executorList) {
                const TestConfig testConfig = {.executor = executor,
                                               .measureTiming = measureTiming,
                                               .outputType = outputType};
                EvaluatePreparedModel(preparedModel, testModel, testConfig);
            }
        }
    }
}

void Execute(const sp<IDevice>& device, const TestModel& testModel, bool testDynamicOutputShape) {
    Model model = createModel(testModel);
    if (testDynamicOutputShape) {
        makeOutputDimensionsUnspecified(&model);
    }

    sp<IPreparedModel> preparedModel;
    createPreparedModel(device, model, &preparedModel);
    if (preparedModel == nullptr) return;

    EvaluatePreparedModel(preparedModel, testModel, testDynamicOutputShape);
}

void GeneratedTestBase::SetUp() {
    testing::TestWithParam<GeneratedTestParam>::SetUp();
    ASSERT_NE(kDevice, nullptr);
}

std::vector<NamedModel> getNamedModels(const FilterFn& filter) {
    return TestModelManager::get().getTestModels(filter);
}

std::string printGeneratedTest(const testing::TestParamInfo<GeneratedTestParam>& info) {
    const auto& [namedDevice, namedModel] = info.param;
    return gtestCompliantName(getName(namedDevice) + "_" + getName(namedModel));
}

// Tag for the generated tests
class GeneratedTest : public GeneratedTestBase {};

// Tag for the dynamic output shape tests
class DynamicOutputShapeTest : public GeneratedTest {};

TEST_P(GeneratedTest, Test) {
    Execute(kDevice, kTestModel, /*testDynamicOutputShape=*/false);
}

TEST_P(DynamicOutputShapeTest, Test) {
    Execute(kDevice, kTestModel, /*testDynamicOutputShape=*/true);
}

INSTANTIATE_GENERATED_TEST(GeneratedTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

INSTANTIATE_GENERATED_TEST(DynamicOutputShapeTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

}  // namespace android::hardware::neuralnetworks::V1_2::vts::functional
