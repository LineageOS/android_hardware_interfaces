/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android-base/logging.h>
#include <gtest/gtest.h>

#include "1.3/Callbacks.h"
#include "1.3/Utils.h"
#include "GeneratedTestHarness.h"
#include "MemoryUtils.h"
#include "TestHarness.h"
#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

namespace android::hardware::neuralnetworks::V1_3::vts::functional {

using namespace test_helper;
using implementation::ExecutionCallback;
using implementation::PreparedModelCallback;
using V1_0::RequestArgument;
using V1_1::ExecutionPreference;
using V1_2::Constant;
using V1_2::MeasureTiming;
using V1_2::OutputShape;
using V1_2::Timing;

namespace {

const auto kNamedDeviceChoices = testing::ValuesIn(getNamedDevices());

// A 1.3 driver is likely to support at least one of the following operand types.
const std::vector<TestOperandType> kTestOperandTypeChoicesVector = {
        TestOperandType::TENSOR_FLOAT32,
        TestOperandType::TENSOR_FLOAT16,
        TestOperandType::TENSOR_QUANT8_ASYMM,
        TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
};
const auto kTestOperandTypeChoices = testing::ValuesIn(kTestOperandTypeChoicesVector);

bool isInChoices(TestOperandType type) {
    return std::count(kTestOperandTypeChoicesVector.begin(), kTestOperandTypeChoicesVector.end(),
                      type) > 0;
}

bool isFloat(TestOperandType type) {
    CHECK(isInChoices(type));
    return type == TestOperandType::TENSOR_FLOAT32 || type == TestOperandType::TENSOR_FLOAT16;
}

// Create dummy buffers for model constants as well as inputs and outputs.
// We only care about the size here because we will not check accuracy in validation tests.
void createDummyData(TestModel* testModel) {
    for (auto& operand : testModel->main.operands) {
        if (operand.data != nullptr) continue;
        switch (operand.lifetime) {
            case TestOperandLifeTime::SUBGRAPH_INPUT:
            case TestOperandLifeTime::SUBGRAPH_OUTPUT:
            case TestOperandLifeTime::CONSTANT_COPY:
            case TestOperandLifeTime::CONSTANT_REFERENCE: {
                const uint32_t size = nn::nonExtensionOperandSizeOfData(
                        static_cast<OperandType>(operand.type), operand.dimensions);
                operand.data = TestBuffer(size);
            } break;
            default:
                break;
        }
    }
}

TestOperand createInt32Scalar(int32_t value) {
    return {
            .type = TestOperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = TestOperandLifeTime::CONSTANT_COPY,
            .data = TestBuffer::createFromVector<int32_t>({value}),
    };
}

// Construct a test model with multiple CONV_2D operations with the given operand as inputs.
// The dimensions of the filters are chosen to ensure outputs has the same dimensions as inputs.
// We choose CONV_2D operation because it is commonly supported by most drivers.
TestModel createConvModel(const TestOperand& operand, uint32_t numOperations) {
    CHECK(isInChoices(operand.type));

    TestOperand weight = {.type = operand.type,
                          .dimensions = {operand.dimensions[3], 3, 3, operand.dimensions[3]},
                          .numberOfConsumers = 1,
                          .scale = isFloat(operand.type) ? 0.0f : 1.0f,
                          .zeroPoint = 0,
                          .lifetime = TestOperandLifeTime::CONSTANT_COPY};

    TestOperand bias = {
            .type = isFloat(operand.type) ? operand.type : TestOperandType::TENSOR_INT32,
            .dimensions = {operand.dimensions[3]},
            .numberOfConsumers = 1,
            .scale = operand.scale * weight.scale,
            .zeroPoint = 0,
            .lifetime = TestOperandLifeTime::CONSTANT_COPY};

    TestOperand output = operand;
    output.numberOfConsumers = 0;
    output.lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT;

    const std::vector<TestOperand> operands = {
            operand,
            std::move(weight),
            std::move(bias),
            createInt32Scalar(1),  // same padding
            createInt32Scalar(1),  // width stride
            createInt32Scalar(1),  // height stride
            createInt32Scalar(0),  // activation = NONE
            std::move(output),
    };

    TestModel model;
    for (uint32_t i = 0; i < numOperations; i++) {
        model.main.operands.insert(model.main.operands.end(), operands.begin(), operands.end());
        const uint32_t inputIndex = operands.size() * i;
        const uint32_t outputIndex = inputIndex + operands.size() - 1;
        std::vector<uint32_t> inputs(operands.size() - 1);
        std::iota(inputs.begin(), inputs.end(), inputIndex);
        model.main.operations.push_back({.type = TestOperationType::CONV_2D,
                                         .inputs = std::move(inputs),
                                         .outputs = {outputIndex}});
        model.main.inputIndexes.push_back(inputIndex);
        model.main.outputIndexes.push_back(outputIndex);
    }
    createDummyData(&model);
    return model;
}

// Construct a test model with a single ADD operation with the given operand as input0 and input1.
// This is to cover additional cases that the CONV_2D model does not support, e.g. arbitrary input
// operand rank, scalar input operand. We choose ADD operation because it is commonly supported by
// most drivers.
TestModel createSingleAddModel(const TestOperand& operand) {
    CHECK(isInChoices(operand.type));

    TestOperand act = {
            .type = TestOperandType::INT32,
            .dimensions = {},
            .numberOfConsumers = 1,
            .scale = 0.0f,
            .zeroPoint = 0,
            .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
    };

    TestOperand output = operand;
    output.numberOfConsumers = 0;
    output.lifetime = TestOperandLifeTime::SUBGRAPH_OUTPUT;

    TestModel model = {
            .main =
                    {
                            .operands =
                                    {
                                            operand,
                                            operand,
                                            std::move(act),
                                            output,
                                    },
                            .operations = {{.type = TestOperationType::ADD,
                                            .inputs = {0, 1, 2},
                                            .outputs = {3}}},
                            .inputIndexes = {0, 1, 2},
                            .outputIndexes = {3},
                    },
    };
    createDummyData(&model);
    return model;
}

// A dummy invalid IPreparedModel class for MemoryDomainAllocateTest.InvalidPreparedModel
class InvalidPreparedModel : public IPreparedModel {
  public:
    Return<V1_0::ErrorStatus> execute(const V1_0::Request&,
                                      const sp<V1_0::IExecutionCallback>&) override {
        return V1_0::ErrorStatus::GENERAL_FAILURE;
    }
    Return<V1_0::ErrorStatus> execute_1_2(const V1_0::Request&, V1_2::MeasureTiming,
                                          const sp<V1_2::IExecutionCallback>&) override {
        return V1_0::ErrorStatus::GENERAL_FAILURE;
    }
    Return<V1_3::ErrorStatus> execute_1_3(const V1_3::Request&, V1_2::MeasureTiming,
                                          const V1_3::OptionalTimePoint&,
                                          const V1_3::OptionalTimeoutDuration&,
                                          const sp<V1_3::IExecutionCallback>&) override {
        return V1_3::ErrorStatus::GENERAL_FAILURE;
    }
    Return<void> executeSynchronously(const V1_0::Request&, V1_2::MeasureTiming,
                                      executeSynchronously_cb) override {
        return Void();
    }
    Return<void> executeSynchronously_1_3(const V1_3::Request&, V1_2::MeasureTiming,
                                          const V1_3::OptionalTimePoint&,
                                          const V1_3::OptionalTimeoutDuration&,
                                          executeSynchronously_1_3_cb) override {
        return Void();
    }
    Return<void> configureExecutionBurst(const sp<V1_2::IBurstCallback>&,
                                         const MQDescriptorSync<V1_2::FmqRequestDatum>&,
                                         const MQDescriptorSync<V1_2::FmqResultDatum>&,
                                         configureExecutionBurst_cb) override {
        return Void();
    }
    Return<void> executeFenced(const V1_3::Request&, const hidl_vec<hidl_handle>&,
                               V1_2::MeasureTiming, const V1_3::OptionalTimePoint&,
                               const V1_3::OptionalTimeoutDuration&,
                               const V1_3::OptionalTimeoutDuration&, executeFenced_cb) override {
        return Void();
    }
};

}  // namespace

class MemoryDomainTestBase : public testing::Test {
  protected:
    MemoryDomainTestBase(sp<IDevice> device, TestOperandType type)
        : kDevice(std::move(device)),
          kTestOperandType(type),
          kTestOperand(kTestOperandMap.at(type)),
          kTestOperandDataSize(nn::nonExtensionOperandSizeOfData(static_cast<OperandType>(type),
                                                                 kTestOperand.dimensions)) {}

    void SetUp() override {
        testing::Test::SetUp();
        ASSERT_NE(kDevice, nullptr);
    }

    sp<IPreparedModel> createConvPreparedModel(const TestOperand& testOperand,
                                               uint32_t numOperations = 1) {
        const TestModel testModel = createConvModel(testOperand, numOperations);
        const Model model = createModel(testModel);
        sp<IPreparedModel> preparedModel;
        createPreparedModel(kDevice, model, &preparedModel, /*reportSkipping=*/false);
        return preparedModel;
    }

    sp<IPreparedModel> createAddPreparedModel(const TestOperand& testOperand) {
        const TestModel testModel = createSingleAddModel(testOperand);
        const Model model = createModel(testModel);
        sp<IPreparedModel> preparedModel;
        createPreparedModel(kDevice, model, &preparedModel, /*reportSkipping=*/false);
        return preparedModel;
    }

    static const std::map<TestOperandType, TestOperand> kTestOperandMap;

    const sp<IDevice> kDevice;
    const TestOperandType kTestOperandType;
    const TestOperand& kTestOperand;
    const uint32_t kTestOperandDataSize;
};

const std::map<TestOperandType, TestOperand> MemoryDomainTestBase::kTestOperandMap = {
        {TestOperandType::TENSOR_FLOAT32,
         {
                 .type = TestOperandType::TENSOR_FLOAT32,
                 .dimensions = {1, 32, 32, 8},
                 .numberOfConsumers = 1,
                 .scale = 0.0f,
                 .zeroPoint = 0,
                 .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
         }},
        {TestOperandType::TENSOR_FLOAT16,
         {
                 .type = TestOperandType::TENSOR_FLOAT16,
                 .dimensions = {1, 32, 32, 8},
                 .numberOfConsumers = 1,
                 .scale = 0.0f,
                 .zeroPoint = 0,
                 .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
         }},
        {TestOperandType::TENSOR_QUANT8_ASYMM,
         {
                 .type = TestOperandType::TENSOR_QUANT8_ASYMM,
                 .dimensions = {1, 32, 32, 8},
                 .numberOfConsumers = 1,
                 .scale = 0.5f,
                 .zeroPoint = 0,
                 .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
         }},
        {TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
         {
                 .type = TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
                 .dimensions = {1, 32, 32, 8},
                 .numberOfConsumers = 1,
                 .scale = 0.5f,
                 .zeroPoint = 0,
                 .lifetime = TestOperandLifeTime::SUBGRAPH_INPUT,
         }},
};

using MemoryDomainAllocateTestParam = std::tuple<NamedDevice, TestOperandType>;
class MemoryDomainAllocateTest : public MemoryDomainTestBase,
                                 public testing::WithParamInterface<MemoryDomainAllocateTestParam> {
  protected:
    MemoryDomainAllocateTest()
        : MemoryDomainTestBase(getData(std::get<NamedDevice>(GetParam())),
                               std::get<TestOperandType>(GetParam())) {}

    struct AllocateTestArgs {
        hidl_vec<uint32_t> dimensions;
        hidl_vec<sp<IPreparedModel>> preparedModels;
        hidl_vec<BufferRole> inputRoles;
        hidl_vec<BufferRole> outputRoles;
    };

    // Validation test for IDevice::allocate. The driver is expected to fail with INVALID_ARGUMENT,
    // or GENERAL_FAILURE if memory domain is not supported.
    void validateAllocate(AllocateTestArgs args) {
        const auto ret = kDevice->allocate(
                {.dimensions = std::move(args.dimensions)}, std::move(args.preparedModels),
                std::move(args.inputRoles), std::move(args.outputRoles),
                [](ErrorStatus status, const sp<IBuffer>& buffer, uint32_t token) {
                    EXPECT_TRUE(status == ErrorStatus::INVALID_ARGUMENT ||
                                status == ErrorStatus::GENERAL_FAILURE);
                    EXPECT_EQ(buffer, nullptr);
                    EXPECT_EQ(token, 0);
                });
        ASSERT_TRUE(ret.isOk());
    }

    void testConflictOperands(const sp<IPreparedModel>& model1, const sp<IPreparedModel>& model2) {
        validateAllocate({
                .preparedModels = {model1, model2},
                .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f},
                               {.modelIndex = 1, .ioIndex = 0, .frequency = 1.0f}},
        });
        validateAllocate({
                .preparedModels = {model1, model2},
                .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
                .outputRoles = {{.modelIndex = 1, .ioIndex = 0, .frequency = 1.0f}},
        });
        validateAllocate({
                .preparedModels = {model1, model2},
                .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f},
                                {.modelIndex = 1, .ioIndex = 0, .frequency = 1.0f}},
        });
    }
};

TEST_P(MemoryDomainAllocateTest, EmptyRole) {
    // Test with empty prepared models and roles.
    validateAllocate({});

    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    // Test again with non-empty prepared models but empty roles.
    validateAllocate({
            .preparedModels = {preparedModel},
    });
}

TEST_P(MemoryDomainAllocateTest, NullptrPreparedModel) {
    // Test with nullptr prepared model as input role.
    validateAllocate({
            .preparedModels = {nullptr},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });

    // Test with nullptr prepared model as output role.
    validateAllocate({
            .preparedModels = {nullptr},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, InvalidPreparedModel) {
    sp<InvalidPreparedModel> invalidPreparedModel = new InvalidPreparedModel();

    // Test with invalid prepared model as input role.
    validateAllocate({
            .preparedModels = {invalidPreparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });

    // Test with invalid prepared model as output role.
    validateAllocate({
            .preparedModels = {invalidPreparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, InvalidModelIndex) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    // This should fail, because the model index is out of bound.
    validateAllocate({
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 1, .ioIndex = 0, .frequency = 1.0f}},
    });

    // This should fail, because the model index is out of bound.
    validateAllocate({
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 1, .ioIndex = 0, .frequency = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, InvalidIOIndex) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    // This should fail, because the model only has one input.
    validateAllocate({
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 1, .frequency = 1.0f}},
    });

    // This should fail, because the model only has one output.
    validateAllocate({
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 1, .frequency = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, InvalidFrequency) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    for (float invalidFreq : {10.0f, 0.0f, -0.5f}) {
        // Test with invalid frequency for input roles.
        validateAllocate({
                .preparedModels = {preparedModel},
                .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = invalidFreq}},
        });
        // Test with invalid frequency for output roles.
        validateAllocate({
                .preparedModels = {preparedModel},
                .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = invalidFreq}},
        });
    }
}

TEST_P(MemoryDomainAllocateTest, SameRoleSpecifiedTwice) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    // Same role with same model index.
    validateAllocate({
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f},
                           {.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });
    validateAllocate({
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f},
                            {.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });

    // Different model indexes, but logically referring to the same role.
    validateAllocate({
            .preparedModels = {preparedModel, preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f},
                           {.modelIndex = 1, .ioIndex = 0, .frequency = 1.0f}},
    });
    validateAllocate({
            .preparedModels = {preparedModel, preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f},
                            {.modelIndex = 1, .ioIndex = 0, .frequency = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, ConflictOperandType) {
    const std::map<TestOperandType, TestOperandType> conflictTypeMap = {
            {TestOperandType::TENSOR_FLOAT32, TestOperandType::TENSOR_FLOAT16},
            {TestOperandType::TENSOR_FLOAT16, TestOperandType::TENSOR_FLOAT32},
            {TestOperandType::TENSOR_QUANT8_ASYMM, TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED},
            {TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED, TestOperandType::TENSOR_QUANT8_ASYMM},
    };

    TestOperand conflictTestOperand = kTestOperand;
    const auto it = conflictTypeMap.find(kTestOperandType);
    ASSERT_FALSE(it == conflictTypeMap.end());
    conflictTestOperand.type = it->second;

    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto conflictPreparedModel = createConvPreparedModel(conflictTestOperand);
    if (preparedModel == nullptr || conflictPreparedModel == nullptr) return;
    testConflictOperands(preparedModel, conflictPreparedModel);
}

TEST_P(MemoryDomainAllocateTest, ConflictScale) {
    if (isFloat(kTestOperandType)) return;

    TestOperand conflictTestOperand = kTestOperand;
    ASSERT_NE(conflictTestOperand.scale, 1.0f);
    conflictTestOperand.scale = 1.0f;

    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto conflictPreparedModel = createConvPreparedModel(conflictTestOperand);
    if (preparedModel == nullptr || conflictPreparedModel == nullptr) return;
    testConflictOperands(preparedModel, conflictPreparedModel);
}

TEST_P(MemoryDomainAllocateTest, ConflictZeroPoint) {
    if (isFloat(kTestOperandType)) return;

    TestOperand conflictTestOperand = kTestOperand;
    ASSERT_NE(conflictTestOperand.zeroPoint, 10);
    conflictTestOperand.zeroPoint = 10;

    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto conflictPreparedModel = createConvPreparedModel(conflictTestOperand);
    if (preparedModel == nullptr || conflictPreparedModel == nullptr) return;
    testConflictOperands(preparedModel, conflictPreparedModel);
}

TEST_P(MemoryDomainAllocateTest, ConflictRankBetweenRoles) {
    TestOperand conflictTestOperand = kTestOperand;
    conflictTestOperand.dimensions.pop_back();

    auto preparedModel = createAddPreparedModel(kTestOperand);
    auto conflictPreparedModel = createAddPreparedModel(conflictTestOperand);
    if (preparedModel == nullptr || conflictPreparedModel == nullptr) return;
    testConflictOperands(preparedModel, conflictPreparedModel);
}

TEST_P(MemoryDomainAllocateTest, ConflictDimensionsBetweenRoles) {
    TestOperand conflictTestOperand = kTestOperand;
    conflictTestOperand.dimensions[0] = 4;

    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto conflictPreparedModel = createConvPreparedModel(conflictTestOperand);
    if (preparedModel == nullptr || conflictPreparedModel == nullptr) return;
    testConflictOperands(preparedModel, conflictPreparedModel);
}

TEST_P(MemoryDomainAllocateTest, ConflictRankBetweenRoleAndDesc) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    auto badDimensions = kTestOperand.dimensions;
    badDimensions.pop_back();

    validateAllocate({
            .dimensions = badDimensions,
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });
    validateAllocate({
            .dimensions = badDimensions,
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, ConflictDimensionsBetweenRoleAndDesc) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    auto badDimensions = kTestOperand.dimensions;
    badDimensions[0] = 4;

    validateAllocate({
            .dimensions = badDimensions,
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });
    validateAllocate({
            .dimensions = badDimensions,
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .frequency = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, ConflictRankWithScalarRole) {
    auto preparedModel = createAddPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    // This should fail, because the target operand is a scalar but a non-empty dimension is
    // specified.
    validateAllocate({
            .dimensions = {1},
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 2, .frequency = 1.0f}},
    });
}

std::string printMemoryDomainAllocateTest(
        const testing::TestParamInfo<MemoryDomainAllocateTestParam>& info) {
    const auto& [namedDevice, operandType] = info.param;
    const std::string type = toString(static_cast<OperandType>(operandType));
    return gtestCompliantName(getName(namedDevice) + "_" + type);
}

INSTANTIATE_TEST_CASE_P(TestMemoryDomain, MemoryDomainAllocateTest,
                        testing::Combine(kNamedDeviceChoices, kTestOperandTypeChoices),
                        printMemoryDomainAllocateTest);

class MemoryDomainCopyTestBase : public MemoryDomainTestBase {
  protected:
    MemoryDomainCopyTestBase(sp<IDevice> device, TestOperandType type)
        : MemoryDomainTestBase(std::move(device), type) {}

    // Allocates device memory for roles of a single prepared model.
    // Returns {IBuffer, token} if success; returns {nullptr, 0} if not supported.
    std::pair<sp<IBuffer>, uint32_t> allocateBuffer(const sp<IPreparedModel>& preparedModel,
                                                    const std::vector<uint32_t>& inputIndexes,
                                                    const std::vector<uint32_t>& outputIndexes,
                                                    const std::vector<uint32_t>& dimensions) {
        if (preparedModel == nullptr) {
            return {nullptr, 0};
        }

        hidl_vec<BufferRole> inputRoles(inputIndexes.size()), outputRoles(outputIndexes.size());
        auto trans = [](uint32_t ind) -> BufferRole {
            return {.modelIndex = 0, .ioIndex = ind, .frequency = 1.0f};
        };
        std::transform(inputIndexes.begin(), inputIndexes.end(), inputRoles.begin(), trans);
        std::transform(outputIndexes.begin(), outputIndexes.end(), outputRoles.begin(), trans);

        sp<IBuffer> buffer;
        uint32_t token = 0;
        const auto ret = kDevice->allocate(
                {.dimensions = dimensions}, {preparedModel}, std::move(inputRoles),
                std::move(outputRoles),
                [&buffer, &token](ErrorStatus err, const sp<IBuffer>& buf, uint32_t tok) {
                    if (err == ErrorStatus::NONE) {
                        EXPECT_NE(buf, nullptr);
                        EXPECT_GT(tok, 0);
                        buffer = buf;
                        token = tok;
                    } else {
                        EXPECT_EQ(err, ErrorStatus::GENERAL_FAILURE);
                        EXPECT_EQ(buf, nullptr);
                        EXPECT_EQ(tok, 0);
                    }
                });
        EXPECT_TRUE(ret.isOk());
        return {std::move(buffer), token};
    }

    std::pair<sp<IBuffer>, uint32_t> allocateBuffer(const sp<IPreparedModel>& preparedModel,
                                                    const std::vector<uint32_t>& inputIndexes,
                                                    const std::vector<uint32_t>& outputIndexes) {
        return allocateBuffer(preparedModel, inputIndexes, outputIndexes, {});
    }

    hidl_memory allocateSharedMemory(uint32_t size) {
        hidl_memory memory = nn::allocateSharedMemory(size);
        EXPECT_EQ(memory.size(), size);
        return memory;
    }

    void testCopyFrom(const sp<IBuffer>& buffer, const hidl_memory& memory,
                      const std::vector<uint32_t>& dimensions, ErrorStatus expectedStatus) {
        const auto ret = buffer->copyFrom(memory, dimensions);
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(static_cast<ErrorStatus>(ret), expectedStatus);
    }

    void testCopyTo(const sp<IBuffer>& buffer, const hidl_memory& memory,
                    ErrorStatus expectedStatus) {
        const auto ret = buffer->copyTo(memory);
        ASSERT_TRUE(ret.isOk());
        ASSERT_EQ(static_cast<ErrorStatus>(ret), expectedStatus);
    }

    void initializeDeviceMemory(const sp<IBuffer>& buffer) {
        hidl_memory memory = nn::allocateSharedMemory(kTestOperandDataSize);
        ASSERT_EQ(memory.size(), kTestOperandDataSize);
        testCopyFrom(buffer, memory, kTestOperand.dimensions, ErrorStatus::NONE);
    }
};

using MemoryDomainCopyTestParam = std::tuple<NamedDevice, TestOperandType>;
class MemoryDomainCopyTest : public MemoryDomainCopyTestBase,
                             public testing::WithParamInterface<MemoryDomainCopyTestParam> {
  protected:
    MemoryDomainCopyTest()
        : MemoryDomainCopyTestBase(getData(std::get<NamedDevice>(GetParam())),
                                   std::get<TestOperandType>(GetParam())) {}
};

TEST_P(MemoryDomainCopyTest, CopyFrom_InvalidMemorySize) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    uint32_t badMemorySize1 = kTestOperandDataSize / 2, badMemorySize2 = kTestOperandDataSize * 2;
    hidl_memory badMemory1 = allocateSharedMemory(badMemorySize1);
    hidl_memory badMemory2 = allocateSharedMemory(badMemorySize2);
    testCopyFrom(buffer, badMemory1, {}, ErrorStatus::INVALID_ARGUMENT);
    testCopyFrom(buffer, badMemory2, {}, ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainCopyTest, CopyFrom_InvalidMemorySize_DynamicShape) {
    TestOperand testOperand = kTestOperand;
    testOperand.dimensions[0] = 0;
    auto preparedModel = createConvPreparedModel(testOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    uint32_t badMemorySize1 = kTestOperandDataSize / 2, badMemorySize2 = kTestOperandDataSize * 2;
    hidl_memory badMemory1 = allocateSharedMemory(badMemorySize1);
    hidl_memory badMemory2 = allocateSharedMemory(badMemorySize2);
    hidl_memory goodMemory = allocateSharedMemory(kTestOperandDataSize);

    auto badDimensions = kTestOperand.dimensions;
    badDimensions[0] = 2;

    testCopyFrom(buffer, badMemory1, kTestOperand.dimensions, ErrorStatus::INVALID_ARGUMENT);
    testCopyFrom(buffer, badMemory2, kTestOperand.dimensions, ErrorStatus::INVALID_ARGUMENT);
    testCopyFrom(buffer, goodMemory, kTestOperand.dimensions, ErrorStatus::NONE);
    testCopyFrom(buffer, goodMemory, badDimensions, ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainCopyTest, CopyFrom_InvalidDimensions) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    hidl_memory memory = allocateSharedMemory(kTestOperandDataSize);

    std::vector<uint32_t> badDimensions;
    badDimensions = kTestOperand.dimensions;
    badDimensions.pop_back();
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    badDimensions = kTestOperand.dimensions;
    badDimensions[0] = 2;
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    badDimensions = kTestOperand.dimensions;
    badDimensions[0] = 0;
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    testCopyFrom(buffer, memory, {}, ErrorStatus::NONE);
    testCopyFrom(buffer, memory, kTestOperand.dimensions, ErrorStatus::NONE);
}

TEST_P(MemoryDomainCopyTest, CopyFrom_InvalidDimensions_DynamicShape) {
    TestOperand testOperand = kTestOperand;
    testOperand.dimensions[0] = 0;
    auto preparedModel = createConvPreparedModel(testOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    hidl_memory memory = allocateSharedMemory(kTestOperandDataSize);

    std::vector<uint32_t> badDimensions;
    badDimensions = kTestOperand.dimensions;
    badDimensions.pop_back();
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    badDimensions = kTestOperand.dimensions;
    badDimensions[0] = 2;
    badDimensions[3] = 4;
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    badDimensions = kTestOperand.dimensions;
    badDimensions[0] = 1;
    badDimensions[3] = 0;
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    testCopyFrom(buffer, memory, {}, ErrorStatus::INVALID_ARGUMENT);
    testCopyFrom(buffer, memory, kTestOperand.dimensions, ErrorStatus::NONE);
}

TEST_P(MemoryDomainCopyTest, CopyTo_UninitializedMemory) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    hidl_memory memory = allocateSharedMemory(kTestOperandDataSize);
    testCopyTo(buffer, memory, ErrorStatus::GENERAL_FAILURE);
}

TEST_P(MemoryDomainCopyTest, CopyTo_InvalidMemorySize) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    uint32_t badMemorySize1 = kTestOperandDataSize / 2, badMemorySize2 = kTestOperandDataSize * 2;
    hidl_memory badMemory1 = allocateSharedMemory(badMemorySize1);
    hidl_memory badMemory2 = allocateSharedMemory(badMemorySize2);
    hidl_memory goodMemory = allocateSharedMemory(kTestOperandDataSize);

    initializeDeviceMemory(buffer);
    testCopyTo(buffer, badMemory1, ErrorStatus::INVALID_ARGUMENT);
    testCopyTo(buffer, badMemory2, ErrorStatus::INVALID_ARGUMENT);
    testCopyTo(buffer, goodMemory, ErrorStatus::NONE);
}

TEST_P(MemoryDomainCopyTest, CopyTo_InvalidMemorySize_DynamicShape) {
    TestOperand testOperand = kTestOperand;
    testOperand.dimensions[0] = 0;
    auto preparedModel = createConvPreparedModel(testOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    uint32_t badMemorySize1 = kTestOperandDataSize / 2, badMemorySize2 = kTestOperandDataSize * 2;
    hidl_memory badMemory1 = allocateSharedMemory(badMemorySize1);
    hidl_memory badMemory2 = allocateSharedMemory(badMemorySize2);
    hidl_memory goodMemory = allocateSharedMemory(kTestOperandDataSize);

    initializeDeviceMemory(buffer);
    testCopyTo(buffer, badMemory1, ErrorStatus::INVALID_ARGUMENT);
    testCopyTo(buffer, badMemory2, ErrorStatus::INVALID_ARGUMENT);
    testCopyTo(buffer, goodMemory, ErrorStatus::NONE);
}

std::string printMemoryDomainCopyTest(
        const testing::TestParamInfo<MemoryDomainCopyTestParam>& info) {
    const auto& [namedDevice, operandType] = info.param;
    const std::string type = toString(static_cast<OperandType>(operandType));
    return gtestCompliantName(getName(namedDevice) + "_" + type);
}

INSTANTIATE_TEST_CASE_P(TestMemoryDomain, MemoryDomainCopyTest,
                        testing::Combine(kNamedDeviceChoices, kTestOperandTypeChoices),
                        printMemoryDomainCopyTest);

using MemoryDomainExecutionTestParam = std::tuple<NamedDevice, TestOperandType, Executor>;
class MemoryDomainExecutionTest
    : public MemoryDomainCopyTestBase,
      public testing::WithParamInterface<MemoryDomainExecutionTestParam> {
  protected:
    MemoryDomainExecutionTest()
        : MemoryDomainCopyTestBase(getData(std::get<NamedDevice>(GetParam())),
                                   std::get<TestOperandType>(GetParam())) {}

    Request::MemoryPool createSharedMemoryPool(uint32_t size) {
        hidl_memory memory = allocateSharedMemory(size);
        Request::MemoryPool pool;
        pool.hidlMemory(memory);
        return pool;
    }

    Request::MemoryPool createDeviceMemoryPool(uint32_t token) {
        Request::MemoryPool pool;
        pool.token(token);
        return pool;
    }

    void testExecution(const sp<IPreparedModel>& preparedModel, const Request& request,
                       ErrorStatus expectedStatus) {
        switch (kExecutor) {
            case Executor::ASYNC:
                EXPECT_EQ(executeAsync(preparedModel, request), expectedStatus);
                break;
            case Executor::SYNC:
                EXPECT_EQ(executeSync(preparedModel, request), expectedStatus);
                break;
            case Executor::FENCED:
                EXPECT_EQ(executeFenced(preparedModel, request), expectedStatus);
                break;
            default:
                ASSERT_TRUE(false);
        }
    }

    ErrorStatus executeAsync(const sp<IPreparedModel>& preparedModel, const Request& request) {
        ErrorStatus executionStatus;

        // launch execution
        sp<ExecutionCallback> executionCallback = new ExecutionCallback();
        const auto ret =
                preparedModel->execute_1_3(request, MeasureTiming::NO, {}, {}, executionCallback);
        EXPECT_TRUE(ret.isOk());
        executionStatus = static_cast<ErrorStatus>(ret);

        // retrieve execution status
        executionCallback->wait();
        if (executionStatus == ErrorStatus::NONE) {
            executionStatus = executionCallback->getStatus();
        } else {
            EXPECT_EQ(executionStatus, executionCallback->getStatus());
        }
        const auto timing = executionCallback->getTiming();
        EXPECT_EQ(UINT64_MAX, timing.timeOnDevice);
        EXPECT_EQ(UINT64_MAX, timing.timeInDriver);
        if (executionStatus != ErrorStatus::NONE) {
            EXPECT_EQ(executionCallback->getOutputShapes().size(), 0);
        }
        return executionStatus;
    }

    ErrorStatus executeSync(const sp<IPreparedModel>& preparedModel, const Request& request) {
        ErrorStatus executionStatus;
        const auto ret = preparedModel->executeSynchronously_1_3(
                request, MeasureTiming::NO, {}, {},
                [&executionStatus](ErrorStatus error, const hidl_vec<OutputShape>& shapes,
                                   const Timing& time) {
                    executionStatus = error;
                    EXPECT_EQ(UINT64_MAX, time.timeOnDevice);
                    EXPECT_EQ(UINT64_MAX, time.timeInDriver);
                    if (executionStatus != ErrorStatus::NONE) {
                        EXPECT_EQ(shapes.size(), 0);
                    }
                });
        EXPECT_TRUE(ret.isOk());
        return executionStatus;
    }

    ErrorStatus executeFenced(const sp<IPreparedModel>& preparedModel, const Request& request) {
        ErrorStatus executionStatus;
        hidl_handle syncFenceHandle;
        sp<IFencedExecutionCallback> fencedCallback;
        const auto callbackFunc = [&executionStatus, &syncFenceHandle, &fencedCallback](
                                          ErrorStatus error, const hidl_handle& handle,
                                          const sp<IFencedExecutionCallback>& callback) {
            executionStatus = error;
            syncFenceHandle = handle;
            fencedCallback = callback;
        };
        Return<void> ret = preparedModel->executeFenced(request, {}, MeasureTiming::NO, {}, {}, {},
                                                        callbackFunc);
        EXPECT_TRUE(ret.isOk());
        if (executionStatus != ErrorStatus::NONE) {
            EXPECT_EQ(syncFenceHandle.getNativeHandle(), nullptr);
            EXPECT_EQ(fencedCallback, nullptr);
            return executionStatus;
        }
        if (syncFenceHandle.getNativeHandle()) {
            waitForSyncFence(syncFenceHandle.getNativeHandle()->data[0]);
        }
        EXPECT_NE(fencedCallback, nullptr);
        ret = fencedCallback->getExecutionInfo(
                [&executionStatus](ErrorStatus error, Timing t, Timing) {
                    executionStatus = error;
                    EXPECT_EQ(UINT64_MAX, t.timeOnDevice);
                    EXPECT_EQ(UINT64_MAX, t.timeInDriver);
                });
        EXPECT_TRUE(ret.isOk());
        return executionStatus;
    }

    const Executor kExecutor = std::get<Executor>(GetParam());
};

TEST_P(MemoryDomainExecutionTest, InvalidToken) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    Request::MemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool badDeviceMemory1 = createDeviceMemoryPool(0);    // Invalid token.
    Request::MemoryPool badDeviceMemory2 = createDeviceMemoryPool(100);  // Unknown token.
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};

    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = {sharedMemory, badDeviceMemory1}},
                  ErrorStatus::INVALID_ARGUMENT);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = {sharedMemory, badDeviceMemory2}},
                  ErrorStatus::INVALID_ARGUMENT);
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = {sharedMemory, badDeviceMemory1}},
                  ErrorStatus::INVALID_ARGUMENT);
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = {sharedMemory, badDeviceMemory2}},
                  ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainExecutionTest, InvalidPreparedModel) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;
    auto badPreparedModel = createConvPreparedModel(kTestOperand);
    if (badPreparedModel == nullptr) return;

    Request::MemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool deviceMemory = createDeviceMemoryPool(token);
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};

    // This should fail, because the buffer is not allocated for badPreparedModel.
    initializeDeviceMemory(buffer);
    testExecution(badPreparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);
    testExecution(badPreparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainExecutionTest, InvalidIOIndex) {
    auto preparedModel = createConvPreparedModel(kTestOperand, 2);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {});
    if (buffer == nullptr) return;

    Request::MemoryPool sharedMemory1 = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool sharedMemory2 = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool sharedMemory3 = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool deviceMemory = createDeviceMemoryPool(token);
    RequestArgument sharedMemoryArg1 = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument sharedMemoryArg2 = {
            .location = {.poolIndex = 1, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument sharedMemoryArg3 = {
            .location = {.poolIndex = 2, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 3}};

    // This should fail, because the device memory is not allocated for input 1.
    initializeDeviceMemory(buffer);
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg1, deviceMemoryArg},
                   .outputs = {sharedMemoryArg2, sharedMemoryArg3},
                   .pools = {sharedMemory1, sharedMemory2, sharedMemory3, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);

    // This should fail, because the device memory is not allocated for output 1.
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg1, sharedMemoryArg2},
                   .outputs = {sharedMemoryArg3, deviceMemoryArg},
                   .pools = {sharedMemory1, sharedMemory2, sharedMemory3, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainExecutionTest, InvalidIOType) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [inputBuffer, inputToken] = allocateBuffer(preparedModel, {0}, {});
    auto [outputBuffer, outputToken] = allocateBuffer(preparedModel, {}, {0});
    if (inputBuffer == nullptr || outputBuffer == nullptr) return;

    Request::MemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool deviceMemory = createDeviceMemoryPool(inputToken);
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};

    // This should fail, because the device memory is allocated for input but used as output.
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);

    // This should fail, because the device memory is allocated for output but used as input.
    deviceMemory.token(outputToken);
    initializeDeviceMemory(outputBuffer);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainExecutionTest, UninitializedMemory) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    Request::MemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool deviceMemory = createDeviceMemoryPool(token);
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};

    // This should fail, because the device memory is not initialized.
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::GENERAL_FAILURE);

    // This should initialize the device memory.
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::NONE);

    // Test again with initialized device memory.
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::NONE);
}

TEST_P(MemoryDomainExecutionTest, SameRequestMultipleRoles) {
    auto preparedModel = createConvPreparedModel(kTestOperand, 2);
    auto [buffer, token] = allocateBuffer(preparedModel, {0, 1}, {0, 1});
    if (buffer == nullptr) return;

    Request::MemoryPool sharedMemory1 = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool sharedMemory2 = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool deviceMemory = createDeviceMemoryPool(token);
    RequestArgument sharedMemoryArg1 = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument sharedMemoryArg2 = {
            .location = {.poolIndex = 1, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 2}};

    // This should fail, because the same device memory cannot be used for both input and output.
    initializeDeviceMemory(buffer);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg, sharedMemoryArg1},
                   .outputs = {deviceMemoryArg, sharedMemoryArg2},
                   .pools = {sharedMemory1, sharedMemory2, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);

    // This should fail, because the same device memory cannot be used for multiple outputs.
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg1, sharedMemoryArg2},
                   .outputs = {deviceMemoryArg, deviceMemoryArg},
                   .pools = {sharedMemory1, sharedMemory2, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);

    // The same device memory can be used for multiple inputs.
    initializeDeviceMemory(buffer);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg, deviceMemoryArg},
                   .outputs = {sharedMemoryArg1, sharedMemoryArg2},
                   .pools = {sharedMemory1, sharedMemory2, deviceMemory}},
                  ErrorStatus::NONE);
}

TEST_P(MemoryDomainExecutionTest, InvalidDimensions) {
    // FENCED execution does not support dynamic shape.
    if (kExecutor == Executor::FENCED) return;

    TestOperand testOperand = kTestOperand;
    testOperand.dimensions[0] = 0;
    auto preparedModel = createConvPreparedModel(testOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0}, kTestOperand.dimensions);
    if (buffer == nullptr) return;

    Request::MemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    Request::MemoryPool deviceMemory = createDeviceMemoryPool(token);
    auto badDimensions = kTestOperand.dimensions;
    badDimensions[0] = 2;
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize},
            .dimensions = badDimensions};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};
    RequestArgument deviceMemoryArgWithBadDimensions = {.location = {.poolIndex = 1},
                                                        .dimensions = badDimensions};

    initializeDeviceMemory(buffer);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArgWithBadDimensions},
                   .outputs = {sharedMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);

    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArgWithBadDimensions},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::INVALID_ARGUMENT);

    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = {sharedMemory, deviceMemory}},
                  ErrorStatus::GENERAL_FAILURE);
}

const auto kExecutorChoices = testing::Values(Executor::ASYNC, Executor::SYNC, Executor::FENCED);

std::string printMemoryDomainExecutionTest(
        const testing::TestParamInfo<MemoryDomainExecutionTestParam>& info) {
    const auto& [namedDevice, operandType, executor] = info.param;
    const std::string type = toString(static_cast<OperandType>(operandType));
    const std::string executorStr = toString(executor);
    return gtestCompliantName(getName(namedDevice) + "_" + type + "_" + executorStr);
}

INSTANTIATE_TEST_CASE_P(TestMemoryDomain, MemoryDomainExecutionTest,
                        testing::Combine(kNamedDeviceChoices, kTestOperandTypeChoices,
                                         kExecutorChoices),
                        printMemoryDomainExecutionTest);

}  // namespace android::hardware::neuralnetworks::V1_3::vts::functional
