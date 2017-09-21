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

#include "VtsHalNeuralnetworksV1_0TargetTest.h"
#include "Event.h"
#include "TestHarness.h"

#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace vts {
namespace functional {

using ::android::hardware::neuralnetworks::V1_0::implementation::Event;
using ::generated_tests::MixedTypedExampleType;
namespace generated_tests {
extern void Execute(const sp<IDevice>&, std::function<Model(void)>, std::function<bool(int)>,
                    const std::vector<MixedTypedExampleType>&);
}

// A class for test environment setup
NeuralnetworksHidlEnvironment::NeuralnetworksHidlEnvironment() {}

NeuralnetworksHidlEnvironment::~NeuralnetworksHidlEnvironment() {}

NeuralnetworksHidlEnvironment* NeuralnetworksHidlEnvironment::getInstance() {
    // This has to return a "new" object because it is freed inside
    // ::testing::AddGlobalTestEnvironment when the gtest is being torn down
    static NeuralnetworksHidlEnvironment* instance = new NeuralnetworksHidlEnvironment();
    return instance;
}

void NeuralnetworksHidlEnvironment::registerTestServices() {
    registerTestService<IDevice>();
}

// The main test class for NEURALNETWORK HIDL HAL.
NeuralnetworksHidlTest::~NeuralnetworksHidlTest() {}

void NeuralnetworksHidlTest::SetUp() {
    device = ::testing::VtsHalHidlTargetTestBase::getService<IDevice>(
        NeuralnetworksHidlEnvironment::getInstance());
    ASSERT_NE(nullptr, device.get());
}

void NeuralnetworksHidlTest::TearDown() {}

// create device test
TEST_F(NeuralnetworksHidlTest, CreateDevice) {}

// status test
TEST_F(NeuralnetworksHidlTest, StatusTest) {
    Return<DeviceStatus> status = device->getStatus();
    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(DeviceStatus::AVAILABLE, static_cast<DeviceStatus>(status));
}

// initialization
TEST_F(NeuralnetworksHidlTest, GetCapabilitiesTest) {
    Return<void> ret =
        device->getCapabilities([](ErrorStatus status, const Capabilities& capabilities) {
            EXPECT_EQ(ErrorStatus::NONE, status);
            EXPECT_NE(nullptr, capabilities.supportedOperationTuples.data());
            EXPECT_NE(0ull, capabilities.supportedOperationTuples.size());
            EXPECT_EQ(0u, static_cast<uint32_t>(capabilities.cachesCompilation) & ~0x1);
            EXPECT_LT(0.0f, capabilities.float32Performance.execTime);
            EXPECT_LT(0.0f, capabilities.float32Performance.powerUsage);
            EXPECT_LT(0.0f, capabilities.quantized8Performance.execTime);
            EXPECT_LT(0.0f, capabilities.quantized8Performance.powerUsage);
        });
    EXPECT_TRUE(ret.isOk());
}

namespace {
// create the model
Model createTestModel() {
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
        .opTuple = {OperationType::ADD, OperandType::TENSOR_FLOAT32},
        .inputs = {operand1, operand2, operand3},
        .outputs = {operand4},
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
}  // anonymous namespace

// allocator helper
hidl_memory allocateSharedMemory(int64_t size, const std::string& type = "ashmem") {
    hidl_memory memory;

    sp<IAllocator> allocator = IAllocator::getService(type);
    if (!allocator.get()) {
        return {};
    }

    Return<void> ret = allocator->allocate(size, [&](bool success, const hidl_memory& mem) {
        ASSERT_TRUE(success);
        memory = mem;
    });
    if (!ret.isOk()) {
        return {};
    }

    return memory;
}

// supported subgraph test
TEST_F(NeuralnetworksHidlTest, SupportedOperationsTest) {
    Model model = createTestModel();
    Return<void> ret = device->getSupportedOperations(
        model, [&](ErrorStatus status, const hidl_vec<bool>& supported) {
            EXPECT_EQ(ErrorStatus::NONE, status);
            EXPECT_EQ(model.operations.size(), supported.size());
        });
    EXPECT_TRUE(ret.isOk());
}

// execute simple graph
TEST_F(NeuralnetworksHidlTest, SimpleExecuteGraphTest) {
    std::vector<float> inputData = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> outputData = {-1.0f, -1.0f, -1.0f, -1.0f};
    std::vector<float> expectedData = {6.0f, 8.0f, 10.0f, 12.0f};
    const uint32_t INPUT = 0;
    const uint32_t OUTPUT = 1;

    // prepare request
    Model model = createTestModel();
    sp<IPreparedModel> preparedModel;
    sp<Event> preparationEvent = new Event();
    ASSERT_NE(nullptr, preparationEvent.get());
    Return<void> prepareRet = device->prepareModel(
        model, preparationEvent, [&](ErrorStatus status, const sp<IPreparedModel>& prepared) {
            EXPECT_EQ(ErrorStatus::NONE, status);
            preparedModel = prepared;
        });
    ASSERT_TRUE(prepareRet.isOk());
    ASSERT_NE(nullptr, preparedModel.get());
    Event::Status preparationStatus = preparationEvent->wait();
    EXPECT_EQ(Event::Status::SUCCESS, preparationStatus);

    // prepare inputs
    uint32_t inputSize = static_cast<uint32_t>(inputData.size() * sizeof(float));
    uint32_t outputSize = static_cast<uint32_t>(outputData.size() * sizeof(float));
    std::vector<RequestArgument> inputs = {{
        .location = {.poolIndex = INPUT, .offset = 0, .length = inputSize}, .dimensions = {},
    }};
    std::vector<RequestArgument> outputs = {{
        .location = {.poolIndex = OUTPUT, .offset = 0, .length = outputSize}, .dimensions = {},
    }};
    std::vector<hidl_memory> pools = {allocateSharedMemory(inputSize),
                                      allocateSharedMemory(outputSize)};
    ASSERT_NE(0ull, pools[INPUT].size());
    ASSERT_NE(0ull, pools[OUTPUT].size());

    // load data
    sp<IMemory> inputMemory = mapMemory(pools[INPUT]);
    sp<IMemory> outputMemory = mapMemory(pools[OUTPUT]);
    ASSERT_NE(nullptr, inputMemory.get());
    ASSERT_NE(nullptr, outputMemory.get());
    float* inputPtr = reinterpret_cast<float*>(static_cast<void*>(inputMemory->getPointer()));
    float* outputPtr = reinterpret_cast<float*>(static_cast<void*>(outputMemory->getPointer()));
    ASSERT_NE(nullptr, inputPtr);
    ASSERT_NE(nullptr, outputPtr);
    inputMemory->update();
    outputMemory->update();
    std::copy(inputData.begin(), inputData.end(), inputPtr);
    std::copy(outputData.begin(), outputData.end(), outputPtr);
    inputMemory->commit();
    outputMemory->commit();

    // execute request
    sp<Event> executionEvent = new Event();
    ASSERT_NE(nullptr, executionEvent.get());
    Return<ErrorStatus> executeStatus = preparedModel->execute(
        {.inputs = inputs, .outputs = outputs, .pools = pools}, executionEvent);
    ASSERT_TRUE(executeStatus.isOk());
    EXPECT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(executeStatus));
    Event::Status eventStatus = executionEvent->wait();
    EXPECT_EQ(Event::Status::SUCCESS, eventStatus);

    // validate results { 1+5, 2+6, 3+7, 4+8 }
    outputMemory->read();
    std::copy(outputPtr, outputPtr + outputData.size(), outputData.begin());
    outputMemory->commit();
    EXPECT_EQ(expectedData, outputData);
}

// Mixed-typed examples
typedef MixedTypedExampleType MixedTypedExample;

// in frameworks/ml/nn/runtime/tests/generated/
#include "all_generated_vts_tests.cpp"

// TODO: Add tests for execution failure, or wait_for/wait_until timeout.
//       Discussion:
//       https://googleplex-android-review.git.corp.google.com/#/c/platform/hardware/interfaces/+/2654636/5/neuralnetworks/1.0/vts/functional/VtsHalNeuralnetworksV1_0TargetTest.cpp@222

}  // namespace functional
}  // namespace vts
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

using android::hardware::neuralnetworks::V1_0::vts::functional::NeuralnetworksHidlEnvironment;

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(NeuralnetworksHidlEnvironment::getInstance());
    ::testing::InitGoogleTest(&argc, argv);
    NeuralnetworksHidlEnvironment::getInstance()->init(&argc, argv);

    int status = RUN_ALL_TESTS();
    return status;
}
