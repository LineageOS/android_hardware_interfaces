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
#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <string>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace vts {
namespace functional {

// A class for test environment setup
NeuralnetworksHidlEnvironment::NeuralnetworksHidlEnvironment() {}

NeuralnetworksHidlEnvironment* NeuralnetworksHidlEnvironment::getInstance() {
    // This has to return a "new" object because it is freed inside
    // ::testing::AddGlobalTestEnvironment when the gtest is being torn down
    static NeuralnetworksHidlEnvironment* instance = new NeuralnetworksHidlEnvironment();
    return instance;
}

void NeuralnetworksHidlEnvironment::registerTestServices() {
    registerTestService("android.hardware.neuralnetworks", "1.0", "IDevice");
}

// The main test class for NEURALNETWORK HIDL HAL.
void NeuralnetworksHidlTest::SetUp() {
    std::string instance =
        NeuralnetworksHidlEnvironment::getInstance()->getServiceName(IDevice::descriptor);
    LOG(INFO) << "running vts test with instance: " << instance;
    device = ::testing::VtsHalHidlTargetTestBase::getService<IDevice>(instance);
    ASSERT_NE(nullptr, device.get());
}

void NeuralnetworksHidlTest::TearDown() {}

// create device test
TEST_F(NeuralnetworksHidlTest, CreateDevice) {}

// status test
TEST_F(NeuralnetworksHidlTest, StatusTest) {
    DeviceStatus status = device->getStatus();
    EXPECT_EQ(DeviceStatus::AVAILABLE, status);
}

// initialization
TEST_F(NeuralnetworksHidlTest, InitializeTest) {
    Return<void> ret = device->initialize([](const Capabilities& capabilities) {
        EXPECT_NE(nullptr, capabilities.supportedOperationTuples.data());
        EXPECT_NE(0ull, capabilities.supportedOperationTuples.size());
        EXPECT_EQ(0u, static_cast<uint32_t>(capabilities.cachesCompilation) & ~0x1);
        EXPECT_LT(0.0f, capabilities.bootupTime);
        EXPECT_LT(0.0f, capabilities.float16Performance.execTime);
        EXPECT_LT(0.0f, capabilities.float16Performance.powerUsage);
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
            .location = {.poolIndex = static_cast<uint32_t>(LocationValues::LOCATION_AT_RUN_TIME),
                         .offset = 0,
                         .length = 0},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {1, 2, 2, 1},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .location = {.poolIndex = static_cast<uint32_t>(LocationValues::LOCATION_SAME_BLOCK),
                         .offset = 0,
                         .length = size},
        },
        {
            .type = OperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .location = {.poolIndex = static_cast<uint32_t>(LocationValues::LOCATION_SAME_BLOCK),
                         .offset = size,
                         .length = sizeof(int32_t)},
        },
        {
            .type = OperandType::TENSOR_FLOAT32,
            .dimensions = {1, 2, 2, 1},
            .numberOfConsumers = 0,
            .scale = 0.0f,
            .zeroPoint = 0,
            .location = {.poolIndex = static_cast<uint32_t>(LocationValues::LOCATION_AT_RUN_TIME),
                         .offset = 0,
                         .length = 0},
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
    int32_t activation[1] = {0};
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
}  // anonymous namespace

// supported subgraph test
TEST_F(NeuralnetworksHidlTest, SupportedSubgraphTest) {
    Model model = createTestModel();
    std::vector<bool> supported;
    Return<void> ret = device->getSupportedSubgraph(
        model, [&](const hidl_vec<bool>& hidl_supported) { supported = hidl_supported; });
    ASSERT_TRUE(ret.isOk());
    EXPECT_EQ(/*model.operations.size()*/ 0ull, supported.size());
}

// execute simple graph
TEST_F(NeuralnetworksHidlTest, SimpleExecuteGraphTest) {
    std::vector<float> inputData = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> outputData = {-1.0f, -1.0f, -1.0f, -1.0f};
    std::vector<float> expectedData = {6.0f, 8.0f, 10.0f, 12.0f};
    const uint32_t INPUT = 0;
    const uint32_t OUTPUT = 1;

    // prpeare request
    Model model = createTestModel();
    sp<IPreparedModel> preparedModel = device->prepareModel(model);
    ASSERT_NE(nullptr, preparedModel.get());

    // prepare inputs
    uint32_t inputSize = static_cast<uint32_t>(inputData.size() * sizeof(float));
    uint32_t outputSize = static_cast<uint32_t>(outputData.size() * sizeof(float));
    std::vector<InputOutputInfo> inputs = {{
        .location = {.poolIndex = INPUT, .offset = 0, .length = inputSize}, .dimensions = {},
    }};
    std::vector<InputOutputInfo> outputs = {{
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
    std::copy(inputData.begin(), inputData.end(), inputPtr);
    std::copy(outputData.begin(), outputData.end(), outputPtr);
    inputMemory->commit();
    outputMemory->commit();

    // execute request
    bool success = preparedModel->execute({.inputs = inputs, .outputs = outputs, .pools = pools});
    EXPECT_TRUE(success);

    // validate results { 1+5, 2+6, 3+7, 4+8 }
    outputMemory->update();
    std::copy(outputPtr, outputPtr + outputData.size(), outputData.begin());
    EXPECT_EQ(expectedData, outputData);
}

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
