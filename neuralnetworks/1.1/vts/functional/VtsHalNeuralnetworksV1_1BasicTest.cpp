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

#include "VtsHalNeuralnetworksV1_1.h"

#include "Callbacks.h"
#include "Models.h"
#include "TestHarness.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

using ::android::hardware::neuralnetworks::V1_0::IPreparedModel;
using ::android::hardware::neuralnetworks::V1_0::DeviceStatus;
using ::android::hardware::neuralnetworks::V1_0::ErrorStatus;
using ::android::hardware::neuralnetworks::V1_0::FusedActivationFunc;
using ::android::hardware::neuralnetworks::V1_0::Operand;
using ::android::hardware::neuralnetworks::V1_0::OperandLifeTime;
using ::android::hardware::neuralnetworks::V1_0::OperandType;
using ::android::hardware::neuralnetworks::V1_0::Request;
using ::android::hardware::neuralnetworks::V1_1::Capabilities;
using ::android::hardware::neuralnetworks::V1_1::IDevice;
using ::android::hardware::neuralnetworks::V1_1::Model;
using ::android::hardware::neuralnetworks::V1_1::Operation;
using ::android::hardware::neuralnetworks::V1_1::OperationType;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::sp;

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_1 {
namespace vts {
namespace functional {
using ::android::hardware::neuralnetworks::V1_0::implementation::ExecutionCallback;
using ::android::hardware::neuralnetworks::V1_0::implementation::PreparedModelCallback;

static void doPrepareModelShortcut(const sp<IDevice>& device, sp<IPreparedModel>* preparedModel) {
    ASSERT_NE(nullptr, preparedModel);
    Model model = createValidTestModel_1_1();

    // see if service can handle model
    bool fullySupportsModel = false;
    Return<void> supportedOpsLaunchStatus = device->getSupportedOperations_1_1(
        model, [&fullySupportsModel](ErrorStatus status, const hidl_vec<bool>& supported) {
            ASSERT_EQ(ErrorStatus::NONE, status);
            ASSERT_NE(0ul, supported.size());
            fullySupportsModel =
                std::all_of(supported.begin(), supported.end(), [](bool valid) { return valid; });
        });
    ASSERT_TRUE(supportedOpsLaunchStatus.isOk());

    // launch prepare model
    sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    ASSERT_NE(nullptr, preparedModelCallback.get());
    Return<ErrorStatus> prepareLaunchStatus =
        device->prepareModel_1_1(model, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    ASSERT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(prepareLaunchStatus));

    // retrieve prepared model
    preparedModelCallback->wait();
    ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    *preparedModel = preparedModelCallback->getPreparedModel();

    // The getSupportedOperations call returns a list of operations that are
    // guaranteed not to fail if prepareModel is called, and
    // 'fullySupportsModel' is true i.f.f. the entire model is guaranteed.
    // If a driver has any doubt that it can prepare an operation, it must
    // return false. So here, if a driver isn't sure if it can support an
    // operation, but reports that it successfully prepared the model, the test
    // can continue.
    if (!fullySupportsModel && prepareReturnStatus != ErrorStatus::NONE) {
        ASSERT_EQ(nullptr, preparedModel->get());
        LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                     "prepare model that it does not support.";
        std::cout << "[          ]   Early termination of test because vendor service cannot "
                     "prepare model that it does not support."
                  << std::endl;
        return;
    }
    ASSERT_EQ(ErrorStatus::NONE, prepareReturnStatus);
    ASSERT_NE(nullptr, preparedModel->get());
}

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
        device->getCapabilities_1_1([](ErrorStatus status, const Capabilities& capabilities) {
            EXPECT_EQ(ErrorStatus::NONE, status);
            EXPECT_LT(0.0f, capabilities.float32Performance.execTime);
            EXPECT_LT(0.0f, capabilities.float32Performance.powerUsage);
            EXPECT_LT(0.0f, capabilities.quantized8Performance.execTime);
            EXPECT_LT(0.0f, capabilities.quantized8Performance.powerUsage);
            EXPECT_LT(0.0f, capabilities.relaxedFloat32toFloat16Performance.execTime);
            EXPECT_LT(0.0f, capabilities.relaxedFloat32toFloat16Performance.powerUsage);
        });
    EXPECT_TRUE(ret.isOk());
}

// supported operations positive test
TEST_F(NeuralnetworksHidlTest, SupportedOperationsPositiveTest) {
    Model model = createValidTestModel_1_1();
    Return<void> ret = device->getSupportedOperations_1_1(
        model, [&](ErrorStatus status, const hidl_vec<bool>& supported) {
            EXPECT_EQ(ErrorStatus::NONE, status);
            EXPECT_EQ(model.operations.size(), supported.size());
        });
    EXPECT_TRUE(ret.isOk());
}

// supported operations negative test 1
TEST_F(NeuralnetworksHidlTest, SupportedOperationsNegativeTest1) {
    Model model = createInvalidTestModel1_1_1();
    Return<void> ret = device->getSupportedOperations_1_1(
        model, [&](ErrorStatus status, const hidl_vec<bool>& supported) {
            EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
            (void)supported;
        });
    EXPECT_TRUE(ret.isOk());
}

// supported operations negative test 2
TEST_F(NeuralnetworksHidlTest, SupportedOperationsNegativeTest2) {
    Model model = createInvalidTestModel2_1_1();
    Return<void> ret = device->getSupportedOperations_1_1(
        model, [&](ErrorStatus status, const hidl_vec<bool>& supported) {
            EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
            (void)supported;
        });
    EXPECT_TRUE(ret.isOk());
}

// prepare simple model positive test
TEST_F(NeuralnetworksHidlTest, SimplePrepareModelPositiveTest) {
    sp<IPreparedModel> preparedModel;
    doPrepareModelShortcut(device, &preparedModel);
}

// prepare simple model negative test 1
TEST_F(NeuralnetworksHidlTest, SimplePrepareModelNegativeTest1) {
    Model model = createInvalidTestModel1_1_1();
    sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    ASSERT_NE(nullptr, preparedModelCallback.get());
    Return<ErrorStatus> prepareLaunchStatus =
        device->prepareModel_1_1(model, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(prepareLaunchStatus));

    preparedModelCallback->wait();
    ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, prepareReturnStatus);
    sp<IPreparedModel> preparedModel = preparedModelCallback->getPreparedModel();
    EXPECT_EQ(nullptr, preparedModel.get());
}

// prepare simple model negative test 2
TEST_F(NeuralnetworksHidlTest, SimplePrepareModelNegativeTest2) {
    Model model = createInvalidTestModel2_1_1();
    sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    ASSERT_NE(nullptr, preparedModelCallback.get());
    Return<ErrorStatus> prepareLaunchStatus =
        device->prepareModel_1_1(model, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(prepareLaunchStatus));

    preparedModelCallback->wait();
    ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, prepareReturnStatus);
    sp<IPreparedModel> preparedModel = preparedModelCallback->getPreparedModel();
    EXPECT_EQ(nullptr, preparedModel.get());
}

// execute simple graph positive test
TEST_F(NeuralnetworksHidlTest, SimpleExecuteGraphPositiveTest) {
    std::vector<float> outputData = {-1.0f, -1.0f, -1.0f, -1.0f};
    std::vector<float> expectedData = {6.0f, 8.0f, 10.0f, 12.0f};
    const uint32_t OUTPUT = 1;

    sp<IPreparedModel> preparedModel;
    ASSERT_NO_FATAL_FAILURE(doPrepareModelShortcut(device, &preparedModel));
    if (preparedModel == nullptr) {
        return;
    }
    Request request = createValidTestRequest();

    auto postWork = [&] {
        sp<IMemory> outputMemory = mapMemory(request.pools[OUTPUT]);
        if (outputMemory == nullptr) {
            return false;
        }
        float* outputPtr = reinterpret_cast<float*>(static_cast<void*>(outputMemory->getPointer()));
        if (outputPtr == nullptr) {
            return false;
        }
        outputMemory->read();
        std::copy(outputPtr, outputPtr + outputData.size(), outputData.begin());
        outputMemory->commit();
        return true;
    };

    sp<ExecutionCallback> executionCallback = new ExecutionCallback();
    ASSERT_NE(nullptr, executionCallback.get());
    executionCallback->on_finish(postWork);
    Return<ErrorStatus> executeLaunchStatus = preparedModel->execute(request, executionCallback);
    ASSERT_TRUE(executeLaunchStatus.isOk());
    EXPECT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(executeLaunchStatus));

    executionCallback->wait();
    ErrorStatus executionReturnStatus = executionCallback->getStatus();
    EXPECT_EQ(ErrorStatus::NONE, executionReturnStatus);
    EXPECT_EQ(expectedData, outputData);
}

// execute simple graph negative test 1
TEST_F(NeuralnetworksHidlTest, SimpleExecuteGraphNegativeTest1) {
    sp<IPreparedModel> preparedModel;
    ASSERT_NO_FATAL_FAILURE(doPrepareModelShortcut(device, &preparedModel));
    if (preparedModel == nullptr) {
        return;
    }
    Request request = createInvalidTestRequest1();

    sp<ExecutionCallback> executionCallback = new ExecutionCallback();
    ASSERT_NE(nullptr, executionCallback.get());
    Return<ErrorStatus> executeLaunchStatus = preparedModel->execute(request, executionCallback);
    ASSERT_TRUE(executeLaunchStatus.isOk());
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(executeLaunchStatus));

    executionCallback->wait();
    ErrorStatus executionReturnStatus = executionCallback->getStatus();
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, executionReturnStatus);
}

// execute simple graph negative test 2
TEST_F(NeuralnetworksHidlTest, SimpleExecuteGraphNegativeTest2) {
    sp<IPreparedModel> preparedModel;
    ASSERT_NO_FATAL_FAILURE(doPrepareModelShortcut(device, &preparedModel));
    if (preparedModel == nullptr) {
        return;
    }
    Request request = createInvalidTestRequest2();

    sp<ExecutionCallback> executionCallback = new ExecutionCallback();
    ASSERT_NE(nullptr, executionCallback.get());
    Return<ErrorStatus> executeLaunchStatus = preparedModel->execute(request, executionCallback);
    ASSERT_TRUE(executeLaunchStatus.isOk());
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(executeLaunchStatus));

    executionCallback->wait();
    ErrorStatus executionReturnStatus = executionCallback->getStatus();
    EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, executionReturnStatus);
}

class NeuralnetworksInputsOutputsTest
    : public NeuralnetworksHidlTest,
      public ::testing::WithParamInterface<std::tuple<bool, bool>> {
   protected:
    virtual void SetUp() { NeuralnetworksHidlTest::SetUp(); }
    virtual void TearDown() { NeuralnetworksHidlTest::TearDown(); }
    V1_1::Model createModel(const std::vector<uint32_t>& inputs,
                            const std::vector<uint32_t>& outputs) {
        // We set up the operands as floating-point with no designated
        // model inputs and outputs, and then patch type and lifetime
        // later on in this function.

        std::vector<Operand> operands = {
            {
                .type = OperandType::TENSOR_FLOAT32,
                .dimensions = {1},
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .zeroPoint = 0,
                .lifetime = OperandLifeTime::TEMPORARY_VARIABLE,
                .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
            {
                .type = OperandType::TENSOR_FLOAT32,
                .dimensions = {1},
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .zeroPoint = 0,
                .lifetime = OperandLifeTime::TEMPORARY_VARIABLE,
                .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
            {
                .type = OperandType::INT32,
                .dimensions = {},
                .numberOfConsumers = 1,
                .scale = 0.0f,
                .zeroPoint = 0,
                .lifetime = OperandLifeTime::CONSTANT_COPY,
                .location = {.poolIndex = 0, .offset = 0, .length = sizeof(int32_t)},
            },
            {
                .type = OperandType::TENSOR_FLOAT32,
                .dimensions = {1},
                .numberOfConsumers = 0,
                .scale = 0.0f,
                .zeroPoint = 0,
                .lifetime = OperandLifeTime::TEMPORARY_VARIABLE,
                .location = {.poolIndex = 0, .offset = 0, .length = 0},
            },
        };

        const std::vector<Operation> operations = {{
            .type = OperationType::ADD, .inputs = {0, 1, 2}, .outputs = {3},
        }};

        std::vector<uint8_t> operandValues;
        int32_t activation[1] = {static_cast<int32_t>(FusedActivationFunc::NONE)};
        operandValues.insert(operandValues.end(), reinterpret_cast<const uint8_t*>(&activation[0]),
                             reinterpret_cast<const uint8_t*>(&activation[1]));

        if (kQuantized) {
            for (auto& operand : operands) {
                if (operand.type == OperandType::TENSOR_FLOAT32) {
                    operand.type = OperandType::TENSOR_QUANT8_ASYMM;
                    operand.scale = 1.0f;
                    operand.zeroPoint = 0;
                }
            }
        }

        auto patchLifetime = [&operands](const std::vector<uint32_t>& operandIndexes,
                                         OperandLifeTime lifetime) {
            for (uint32_t index : operandIndexes) {
                operands[index].lifetime = lifetime;
            }
        };
        if (kInputHasPrecedence) {
            patchLifetime(outputs, OperandLifeTime::MODEL_OUTPUT);
            patchLifetime(inputs, OperandLifeTime::MODEL_INPUT);
        } else {
            patchLifetime(inputs, OperandLifeTime::MODEL_INPUT);
            patchLifetime(outputs, OperandLifeTime::MODEL_OUTPUT);
        }

        return {
            .operands = operands,
            .operations = operations,
            .inputIndexes = inputs,
            .outputIndexes = outputs,
            .operandValues = operandValues,
            .pools = {},
        };
    }
    void check(const std::string& name,
               bool expectation,  // true = success
               const std::vector<uint32_t>& inputs, const std::vector<uint32_t>& outputs) {
        SCOPED_TRACE(name + " (HAL calls should " + (expectation ? "succeed" : "fail") + ", " +
                     (kInputHasPrecedence ? "input" : "output") + " precedence, " +
                     (kQuantized ? "quantized" : "float"));

        V1_1::Model model = createModel(inputs, outputs);

        // ensure that getSupportedOperations_1_1() checks model validity
        ErrorStatus supportedOpsErrorStatus = ErrorStatus::GENERAL_FAILURE;
        Return<void> supportedOpsReturn = device->getSupportedOperations_1_1(
            model, [&model, &supportedOpsErrorStatus](ErrorStatus status,
                                                      const hidl_vec<bool>& supported) {
                supportedOpsErrorStatus = status;
                if (status == ErrorStatus::NONE) {
                    ASSERT_EQ(supported.size(), model.operations.size());
                }
            });
        ASSERT_TRUE(supportedOpsReturn.isOk());
        ASSERT_EQ(supportedOpsErrorStatus,
                  (expectation ? ErrorStatus::NONE : ErrorStatus::INVALID_ARGUMENT));

        // ensure that prepareModel_1_1() checks model validity
        sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback;
        ASSERT_NE(preparedModelCallback.get(), nullptr);
        Return<ErrorStatus> prepareLaunchReturn =
            device->prepareModel_1_1(model, preparedModelCallback);
        ASSERT_TRUE(prepareLaunchReturn.isOk());
        ASSERT_TRUE(prepareLaunchReturn == ErrorStatus::NONE ||
                    prepareLaunchReturn == ErrorStatus::INVALID_ARGUMENT);
        bool preparationOk = (prepareLaunchReturn == ErrorStatus::NONE);
        if (preparationOk) {
            preparedModelCallback->wait();
            preparationOk = (preparedModelCallback->getStatus() == ErrorStatus::NONE);
        }

        if (preparationOk) {
            ASSERT_TRUE(expectation);
        } else {
            // Preparation can fail for reasons other than an invalid model --
            // for example, perhaps not all operations are supported, or perhaps
            // the device hit some kind of capacity limit.
            bool invalid = prepareLaunchReturn == ErrorStatus::INVALID_ARGUMENT ||
                           preparedModelCallback->getStatus() == ErrorStatus::INVALID_ARGUMENT;
            ASSERT_NE(expectation, invalid);
        }
    }

    // Indicates whether an operand that appears in both the inputs
    // and outputs vector should have lifetime appropriate for input
    // rather than for output.
    const bool kInputHasPrecedence = std::get<0>(GetParam());

    // Indicates whether we should test TENSOR_QUANT8_ASYMM rather
    // than TENSOR_FLOAT32.
    const bool kQuantized = std::get<1>(GetParam());
};

TEST_P(NeuralnetworksInputsOutputsTest, Validate) {
    check("Ok", true, {0, 1}, {3});
    check("InputIsOutput", false, {0, 1}, {3, 0});
    check("OutputIsInput", false, {0, 1, 3}, {3});
    check("DuplicateInputs", false, {0, 1, 0}, {3});
    check("DuplicateOutputs", false, {0, 1}, {3, 3});
}

INSTANTIATE_TEST_CASE_P(Flavor, NeuralnetworksInputsOutputsTest,
                        ::testing::Combine(::testing::Bool(), ::testing::Bool()));

}  // namespace functional
}  // namespace vts
}  // namespace V1_1
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

using android::hardware::neuralnetworks::V1_1::vts::functional::NeuralnetworksHidlEnvironment;

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(NeuralnetworksHidlEnvironment::getInstance());
    ::testing::InitGoogleTest(&argc, argv);
    NeuralnetworksHidlEnvironment::getInstance()->init(&argc, argv);

    int status = RUN_ALL_TESTS();
    return status;
}
