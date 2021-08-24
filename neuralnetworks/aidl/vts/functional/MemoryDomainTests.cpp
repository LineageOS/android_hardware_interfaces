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

#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <android/binder_status.h>
#include <gtest/gtest.h>

#include <LegacyUtils.h>
#include <TestHarness.h>
#include <Utils.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/hal/aidl/Conversions.h>
#include <nnapi/hal/aidl/Utils.h>

#include "AidlHalInterfaces.h"
#include "Callbacks.h"
#include "GeneratedTestHarness.h"
#include "MemoryUtils.h"
#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

namespace aidl::android::hardware::neuralnetworks::vts::functional {

using namespace test_helper;
using implementation::PreparedModelCallback;

namespace {

// An AIDL driver is likely to support at least one of the following operand types.
const std::vector<TestOperandType> kTestOperandTypeChoicesVector = {
        TestOperandType::TENSOR_FLOAT32,
        TestOperandType::TENSOR_FLOAT16,
        TestOperandType::TENSOR_QUANT8_ASYMM,
        TestOperandType::TENSOR_QUANT8_ASYMM_SIGNED,
};
const auto kTestOperandTypeChoices = testing::ValuesIn(kTestOperandTypeChoicesVector);
// TODO(b/179270601): restore kNamedDeviceChoices

bool isInChoices(TestOperandType type) {
    return std::count(kTestOperandTypeChoicesVector.begin(), kTestOperandTypeChoicesVector.end(),
                      type) > 0;
}

bool isFloat(TestOperandType type) {
    CHECK(isInChoices(type));
    return type == TestOperandType::TENSOR_FLOAT32 || type == TestOperandType::TENSOR_FLOAT16;
}

// Create placeholder buffers for model constants as well as inputs and outputs.
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
                        static_cast<nn::OperandType>(operand.type), operand.dimensions);
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

// A placeholder invalid IPreparedModel class for MemoryDomainAllocateTest.InvalidPreparedModel
class InvalidPreparedModel : public BnPreparedModel {
  public:
    ndk::ScopedAStatus executeSynchronously(const Request&, bool, int64_t, int64_t,
                                            ExecutionResult*) override {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(ErrorStatus::GENERAL_FAILURE));
    }
    ndk::ScopedAStatus executeFenced(const Request&, const std::vector<ndk::ScopedFileDescriptor>&,
                                     bool, int64_t, int64_t, int64_t,
                                     FencedExecutionResult*) override {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(ErrorStatus::GENERAL_FAILURE));
    }
    ndk::ScopedAStatus configureExecutionBurst(std::shared_ptr<IBurst>*) override {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(ErrorStatus::GENERAL_FAILURE));
    }
};

template <typename... Args>
std::vector<RequestMemoryPool> createRequestMemoryPools(const Args&... pools) {
    std::vector<RequestMemoryPool> memoryPools;
    memoryPools.reserve(sizeof...(Args));
    // This fold operator calls push_back on each of the function arguments.
    (memoryPools.push_back(utils::clone(pools).value()), ...);
    return memoryPools;
};

}  // namespace

class MemoryDomainTestBase : public testing::Test {
  protected:
    MemoryDomainTestBase(std::shared_ptr<IDevice> device, TestOperandType type)
        : kDevice(std::move(device)),
          kTestOperandType(type),
          kTestOperand(kTestOperandMap.at(type)),
          kTestOperandDataSize(nn::nonExtensionOperandSizeOfData(static_cast<nn::OperandType>(type),
                                                                 kTestOperand.dimensions)) {}

    void SetUp() override {
        testing::Test::SetUp();
        ASSERT_NE(kDevice, nullptr);
        const bool deviceIsResponsive =
                ndk::ScopedAStatus::fromStatus(AIBinder_ping(kDevice->asBinder().get())).isOk();
        ASSERT_TRUE(deviceIsResponsive);
    }

    std::shared_ptr<IPreparedModel> createConvPreparedModel(const TestOperand& testOperand,
                                                            uint32_t numOperations = 1) {
        const TestModel testModel = createConvModel(testOperand, numOperations);
        const Model model = createModel(testModel);
        std::shared_ptr<IPreparedModel> preparedModel;
        createPreparedModel(kDevice, model, &preparedModel, /*reportSkipping=*/false);
        return preparedModel;
    }

    std::shared_ptr<IPreparedModel> createAddPreparedModel(const TestOperand& testOperand) {
        const TestModel testModel = createSingleAddModel(testOperand);
        const Model model = createModel(testModel);
        std::shared_ptr<IPreparedModel> preparedModel;
        createPreparedModel(kDevice, model, &preparedModel, /*reportSkipping=*/false);
        return preparedModel;
    }

    static const std::map<TestOperandType, TestOperand> kTestOperandMap;

    const std::shared_ptr<IDevice> kDevice;
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
        std::vector<int32_t> dimensions;
        std::vector<std::shared_ptr<IPreparedModel>> preparedModels;
        std::vector<BufferRole> inputRoles;
        std::vector<BufferRole> outputRoles;
    };

    // Validation test for IDevice::allocate. The driver is expected to fail with INVALID_ARGUMENT,
    // or GENERAL_FAILURE if memory domain is not supported.
    void validateAllocate(AllocateTestArgs args) {
        std::vector<IPreparedModelParcel> preparedModelParcels;
        preparedModelParcels.reserve(args.preparedModels.size());
        for (const auto& model : args.preparedModels) {
            preparedModelParcels.push_back({.preparedModel = model});
        }
        DeviceBuffer buffer;
        const auto ret =
                kDevice->allocate({.dimensions = std::move(args.dimensions)}, preparedModelParcels,
                                  args.inputRoles, args.outputRoles, &buffer);

        ASSERT_EQ(ret.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_TRUE(static_cast<ErrorStatus>(ret.getServiceSpecificError()) ==
                            ErrorStatus::INVALID_ARGUMENT ||
                    static_cast<ErrorStatus>(ret.getServiceSpecificError()) ==
                            ErrorStatus::GENERAL_FAILURE);
    }

    void testConflictOperands(const std::shared_ptr<IPreparedModel>& model1,
                              const std::shared_ptr<IPreparedModel>& model2) {
        validateAllocate({
                .preparedModels = {model1, model2},
                .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f},
                               {.modelIndex = 1, .ioIndex = 0, .probability = 1.0f}},
        });
        validateAllocate({
                .preparedModels = {model1, model2},
                .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
                .outputRoles = {{.modelIndex = 1, .ioIndex = 0, .probability = 1.0f}},
        });
        validateAllocate({
                .preparedModels = {model1, model2},
                .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f},
                                {.modelIndex = 1, .ioIndex = 0, .probability = 1.0f}},
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
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });

    // Test with nullptr prepared model as output role.
    validateAllocate({
            .preparedModels = {nullptr},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, InvalidPreparedModel) {
    std::shared_ptr<InvalidPreparedModel> invalidPreparedModel =
            ndk::SharedRefBase::make<InvalidPreparedModel>();

    // Test with invalid prepared model as input role.
    validateAllocate({
            .preparedModels = {invalidPreparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });

    // Test with invalid prepared model as output role.
    validateAllocate({
            .preparedModels = {invalidPreparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, InvalidModelIndex) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    // This should fail, because the model index is out of bound.
    validateAllocate({
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 1, .ioIndex = 0, .probability = 1.0f}},
    });

    // This should fail, because the model index is out of bound.
    validateAllocate({
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 1, .ioIndex = 0, .probability = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, InvalidIOIndex) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    // This should fail, because the model only has one input.
    validateAllocate({
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 1, .probability = 1.0f}},
    });

    // This should fail, because the model only has one output.
    validateAllocate({
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 1, .probability = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, InvalidProbability) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    for (float invalidFreq : {10.0f, 0.0f, -0.5f}) {
        // Test with invalid probability for input roles.
        validateAllocate({
                .preparedModels = {preparedModel},
                .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = invalidFreq}},
        });
        // Test with invalid probability for output roles.
        validateAllocate({
                .preparedModels = {preparedModel},
                .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = invalidFreq}},
        });
    }
}

TEST_P(MemoryDomainAllocateTest, SameRoleSpecifiedTwice) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    // Same role with same model index.
    validateAllocate({
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f},
                           {.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });
    validateAllocate({
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f},
                            {.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });

    // Different model indexes, but logically referring to the same role.
    validateAllocate({
            .preparedModels = {preparedModel, preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f},
                           {.modelIndex = 1, .ioIndex = 0, .probability = 1.0f}},
    });
    validateAllocate({
            .preparedModels = {preparedModel, preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f},
                            {.modelIndex = 1, .ioIndex = 0, .probability = 1.0f}},
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

    auto badDimensions = utils::toSigned(kTestOperand.dimensions).value();
    badDimensions.pop_back();

    validateAllocate({
            .dimensions = badDimensions,
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });
    validateAllocate({
            .dimensions = badDimensions,
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });
}

TEST_P(MemoryDomainAllocateTest, ConflictDimensionsBetweenRoleAndDesc) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    auto badDimensions = utils::toSigned(kTestOperand.dimensions).value();
    badDimensions[0] = 4;

    validateAllocate({
            .dimensions = badDimensions,
            .preparedModels = {preparedModel},
            .inputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
    });
    validateAllocate({
            .dimensions = badDimensions,
            .preparedModels = {preparedModel},
            .outputRoles = {{.modelIndex = 0, .ioIndex = 0, .probability = 1.0f}},
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
            .inputRoles = {{.modelIndex = 0, .ioIndex = 2, .probability = 1.0f}},
    });
}

std::string printMemoryDomainAllocateTest(
        const testing::TestParamInfo<MemoryDomainAllocateTestParam>& info) {
    const auto& [namedDevice, operandType] = info.param;
    const std::string type = toString(static_cast<OperandType>(operandType));
    return gtestCompliantName(getName(namedDevice) + "_" + type);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MemoryDomainAllocateTest);
INSTANTIATE_TEST_SUITE_P(TestMemoryDomain, MemoryDomainAllocateTest,
                         testing::Combine(testing::ValuesIn(getNamedDevices()),
                                          kTestOperandTypeChoices),
                         printMemoryDomainAllocateTest);

class MemoryDomainCopyTestBase : public MemoryDomainTestBase {
  protected:
    MemoryDomainCopyTestBase(std::shared_ptr<IDevice> device, TestOperandType type)
        : MemoryDomainTestBase(std::move(device), type) {}

    // Allocates device memory for roles of a single prepared model.
    // Returns {IBuffer, token} if success; returns {nullptr, 0} if not supported.
    DeviceBuffer allocateBuffer(const std::shared_ptr<IPreparedModel>& preparedModel,
                                const std::vector<int32_t>& inputIndexes,
                                const std::vector<int32_t>& outputIndexes,
                                const std::vector<int32_t>& dimensions) {
        if (preparedModel == nullptr) {
            return {.buffer = nullptr, .token = 0};
        }

        std::vector<BufferRole> inputRoles(inputIndexes.size()), outputRoles(outputIndexes.size());
        auto trans = [](int32_t ind) -> BufferRole {
            return {.modelIndex = 0, .ioIndex = ind, .probability = 1.0f};
        };
        std::transform(inputIndexes.begin(), inputIndexes.end(), inputRoles.begin(), trans);
        std::transform(outputIndexes.begin(), outputIndexes.end(), outputRoles.begin(), trans);

        IPreparedModelParcel parcel;
        parcel.preparedModel = preparedModel;

        DeviceBuffer buffer;

        const auto ret = kDevice->allocate({.dimensions = dimensions}, {parcel}, inputRoles,
                                           outputRoles, &buffer);

        if (!ret.isOk()) {
            EXPECT_EQ(ret.getExceptionCode(), EX_SERVICE_SPECIFIC);
            EXPECT_EQ(static_cast<ErrorStatus>(ret.getServiceSpecificError()),
                      ErrorStatus::GENERAL_FAILURE);
            return DeviceBuffer{
                    .buffer = nullptr,
                    .token = 0,
            };
        }

        EXPECT_NE(buffer.buffer, nullptr);
        EXPECT_GT(buffer.token, 0);

        return buffer;
    }

    DeviceBuffer allocateBuffer(const std::shared_ptr<IPreparedModel>& preparedModel,
                                const std::vector<int32_t>& inputIndexes,
                                const std::vector<int32_t>& outputIndexes) {
        return allocateBuffer(preparedModel, inputIndexes, outputIndexes, {});
    }

    size_t getSize(const Memory& memory) {
        switch (memory.getTag()) {
            case Memory::Tag::ashmem:
                return memory.get<Memory::Tag::ashmem>().size;
            case Memory::Tag::mappableFile:
                return memory.get<Memory::Tag::mappableFile>().length;
            case Memory::Tag::hardwareBuffer: {
                const auto& hardwareBuffer = memory.get<Memory::Tag::hardwareBuffer>();
                const bool isBlob =
                        hardwareBuffer.description.format == graphics::common::PixelFormat::BLOB;
                return isBlob ? hardwareBuffer.description.width : 0;
            }
        }
        return 0;
    }

    Memory allocateSharedMemory(uint32_t size) {
        const auto sharedMemory = nn::createSharedMemory(size).value();
        auto memory = utils::convert(sharedMemory).value();
        EXPECT_EQ(getSize(memory), size);
        return memory;
    }

    void testCopyFrom(const std::shared_ptr<IBuffer>& buffer, const Memory& memory,
                      const std::vector<int32_t>& dimensions, ErrorStatus expectedStatus) {
        const auto ret = buffer->copyFrom(memory, dimensions);
        if (expectedStatus == ErrorStatus::NONE) {
            ASSERT_TRUE(ret.isOk());
        } else {
            ASSERT_EQ(ret.getExceptionCode(), EX_SERVICE_SPECIFIC);
            ASSERT_EQ(expectedStatus, static_cast<ErrorStatus>(ret.getServiceSpecificError()));
        }
    }

    void testCopyTo(const std::shared_ptr<IBuffer>& buffer, const Memory& memory,
                    ErrorStatus expectedStatus) {
        const auto ret = buffer->copyTo(memory);
        if (expectedStatus == ErrorStatus::NONE) {
            ASSERT_TRUE(ret.isOk());
        } else {
            ASSERT_EQ(ret.getExceptionCode(), EX_SERVICE_SPECIFIC);
            ASSERT_EQ(expectedStatus, static_cast<ErrorStatus>(ret.getServiceSpecificError()));
        }
    }

    void initializeDeviceMemory(const std::shared_ptr<IBuffer>& buffer) {
        Memory memory = allocateSharedMemory(kTestOperandDataSize);
        ASSERT_EQ(getSize(memory), kTestOperandDataSize);
        testCopyFrom(buffer, memory, utils::toSigned(kTestOperand.dimensions).value(),
                     ErrorStatus::NONE);
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
    Memory badMemory1 = allocateSharedMemory(badMemorySize1);
    Memory badMemory2 = allocateSharedMemory(badMemorySize2);
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
    Memory badMemory1 = allocateSharedMemory(badMemorySize1);
    Memory badMemory2 = allocateSharedMemory(badMemorySize2);
    Memory goodMemory = allocateSharedMemory(kTestOperandDataSize);

    const auto goodDimensions = utils::toSigned(kTestOperand.dimensions).value();
    auto badDimensions = goodDimensions;
    badDimensions[0] = 2;

    testCopyFrom(buffer, badMemory1, goodDimensions, ErrorStatus::INVALID_ARGUMENT);
    testCopyFrom(buffer, badMemory2, goodDimensions, ErrorStatus::INVALID_ARGUMENT);
    testCopyFrom(buffer, goodMemory, goodDimensions, ErrorStatus::NONE);
    testCopyFrom(buffer, goodMemory, badDimensions, ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainCopyTest, CopyFrom_InvalidDimensions) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    Memory memory = allocateSharedMemory(kTestOperandDataSize);

    const auto goodDimensions = utils::toSigned(kTestOperand.dimensions).value();
    std::vector<int32_t> badDimensions = goodDimensions;
    badDimensions.pop_back();
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    badDimensions = goodDimensions;
    badDimensions[0] = 2;
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    badDimensions = goodDimensions;
    badDimensions[0] = 0;
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    testCopyFrom(buffer, memory, {}, ErrorStatus::NONE);
    testCopyFrom(buffer, memory, goodDimensions, ErrorStatus::NONE);
}

TEST_P(MemoryDomainCopyTest, CopyFrom_InvalidDimensions_DynamicShape) {
    TestOperand testOperand = kTestOperand;
    testOperand.dimensions[0] = 0;
    auto preparedModel = createConvPreparedModel(testOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    Memory memory = allocateSharedMemory(kTestOperandDataSize);

    const auto goodDimensions = utils::toSigned(kTestOperand.dimensions).value();
    std::vector<int32_t> badDimensions = goodDimensions;
    badDimensions.pop_back();
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    badDimensions = goodDimensions;
    badDimensions[0] = 2;
    badDimensions[3] = 4;
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    badDimensions = goodDimensions;
    badDimensions[0] = 1;
    badDimensions[3] = 0;
    testCopyFrom(buffer, memory, badDimensions, ErrorStatus::INVALID_ARGUMENT);

    testCopyFrom(buffer, memory, {}, ErrorStatus::INVALID_ARGUMENT);
    testCopyFrom(buffer, memory, goodDimensions, ErrorStatus::NONE);
}

TEST_P(MemoryDomainCopyTest, CopyTo_UninitializedMemory) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    Memory memory = allocateSharedMemory(kTestOperandDataSize);
    testCopyTo(buffer, memory, ErrorStatus::GENERAL_FAILURE);
}

TEST_P(MemoryDomainCopyTest, CopyTo_InvalidMemorySize) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    uint32_t badMemorySize1 = kTestOperandDataSize / 2, badMemorySize2 = kTestOperandDataSize * 2;
    Memory badMemory1 = allocateSharedMemory(badMemorySize1);
    Memory badMemory2 = allocateSharedMemory(badMemorySize2);
    Memory goodMemory = allocateSharedMemory(kTestOperandDataSize);

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
    Memory badMemory1 = allocateSharedMemory(badMemorySize1);
    Memory badMemory2 = allocateSharedMemory(badMemorySize2);
    Memory goodMemory = allocateSharedMemory(kTestOperandDataSize);

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

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MemoryDomainCopyTest);
INSTANTIATE_TEST_SUITE_P(TestMemoryDomain, MemoryDomainCopyTest,
                         testing::Combine(testing::ValuesIn(getNamedDevices()),
                                          kTestOperandTypeChoices),
                         printMemoryDomainCopyTest);

using MemoryDomainExecutionTestParam = std::tuple<NamedDevice, TestOperandType, Executor>;
class MemoryDomainExecutionTest
    : public MemoryDomainCopyTestBase,
      public testing::WithParamInterface<MemoryDomainExecutionTestParam> {
  protected:
    MemoryDomainExecutionTest()
        : MemoryDomainCopyTestBase(getData(std::get<NamedDevice>(GetParam())),
                                   std::get<TestOperandType>(GetParam())) {}

    RequestMemoryPool createSharedMemoryPool(uint32_t size) {
        return RequestMemoryPool(allocateSharedMemory(size));
    }

    RequestMemoryPool createDeviceMemoryPool(uint32_t token) {
        return RequestMemoryPool(static_cast<int32_t>(token));
    }

    void testExecution(const std::shared_ptr<IPreparedModel>& preparedModel, const Request& request,
                       ErrorStatus expectedStatus) {
        switch (kExecutor) {
            case Executor::SYNC:
                EXPECT_EQ(executeSync(preparedModel, request), expectedStatus);
                break;
            case Executor::BURST:
                EXPECT_EQ(executeBurst(preparedModel, request), expectedStatus);
                break;
            case Executor::FENCED:
                EXPECT_EQ(executeFenced(preparedModel, request), expectedStatus);
                break;
            default:
                ASSERT_TRUE(false);
        }
    }

    ErrorStatus executeSync(const std::shared_ptr<IPreparedModel>& preparedModel,
                            const Request& request) {
        ExecutionResult executionResult;
        const auto ret = preparedModel->executeSynchronously(
                request, false, kNoDeadline, kOmittedTimeoutDuration, &executionResult);

        if (!ret.isOk()) {
            EXPECT_EQ(ret.getExceptionCode(), EX_SERVICE_SPECIFIC);
            return static_cast<ErrorStatus>(ret.getServiceSpecificError());
        }
        const ErrorStatus executionStatus = executionResult.outputSufficientSize
                                                    ? ErrorStatus::NONE
                                                    : ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;
        EXPECT_EQ(executionResult.timing, kNoTiming);
        return executionStatus;
    }

    ErrorStatus executeFenced(const std::shared_ptr<IPreparedModel>& preparedModel,
                              const Request& request) {
        FencedExecutionResult executionResult;
        const auto ret = preparedModel->executeFenced(request, {}, false, kNoDeadline,
                                                      kOmittedTimeoutDuration, kNoDuration,
                                                      &executionResult);
        if (!ret.isOk()) {
            EXPECT_EQ(ret.getExceptionCode(), EX_SERVICE_SPECIFIC);
            return static_cast<ErrorStatus>(ret.getServiceSpecificError());
        }
        if (executionResult.syncFence.get() != -1) {
            waitForSyncFence(executionResult.syncFence.get());
        }
        EXPECT_NE(executionResult.callback, nullptr);

        ErrorStatus executionStatus = ErrorStatus::GENERAL_FAILURE;
        Timing time = kNoTiming;
        Timing timeFenced = kNoTiming;
        const auto retExecutionInfo =
                executionResult.callback->getExecutionInfo(&time, &timeFenced, &executionStatus);
        EXPECT_TRUE(retExecutionInfo.isOk());
        EXPECT_EQ(time, kNoTiming);
        return executionStatus;
    }

    ErrorStatus executeBurst(const std::shared_ptr<IPreparedModel>& preparedModel,
                             const Request& request) {
        // create burst
        std::shared_ptr<IBurst> burst;
        auto ret = preparedModel->configureExecutionBurst(&burst);
        EXPECT_TRUE(ret.isOk()) << ret.getDescription();
        EXPECT_NE(nullptr, burst.get());
        if (!ret.isOk() || burst.get() == nullptr) {
            return ErrorStatus::GENERAL_FAILURE;
        }

        // use -1 for all memory identifier tokens
        const std::vector<int64_t> slots(request.pools.size(), -1);

        ExecutionResult executionResult;
        ret = burst->executeSynchronously(request, slots, false, kNoDeadline,
                                          kOmittedTimeoutDuration, &executionResult);

        if (!ret.isOk()) {
            EXPECT_EQ(ret.getExceptionCode(), EX_SERVICE_SPECIFIC);
            return static_cast<ErrorStatus>(ret.getServiceSpecificError());
        }
        const ErrorStatus executionStatus = executionResult.outputSufficientSize
                                                    ? ErrorStatus::NONE
                                                    : ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;
        EXPECT_EQ(executionResult.timing, kNoTiming);
        return executionStatus;
    }

    const Executor kExecutor = std::get<Executor>(GetParam());
};

TEST_P(MemoryDomainExecutionTest, InvalidToken) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    if (preparedModel == nullptr) return;

    RequestMemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool badDeviceMemory1 = createDeviceMemoryPool(0);    // Invalid token.
    RequestMemoryPool badDeviceMemory2 = createDeviceMemoryPool(100);  // Unknown token.
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};

    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, badDeviceMemory1)},
                  ErrorStatus::INVALID_ARGUMENT);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, badDeviceMemory2)},
                  ErrorStatus::INVALID_ARGUMENT);
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, badDeviceMemory1)},
                  ErrorStatus::INVALID_ARGUMENT);
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, badDeviceMemory2)},
                  ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainExecutionTest, InvalidPreparedModel) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;
    auto badPreparedModel = createConvPreparedModel(kTestOperand);
    if (badPreparedModel == nullptr) return;

    RequestMemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool deviceMemory = createDeviceMemoryPool(token);
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};

    // This should fail, because the buffer is not allocated for badPreparedModel.
    initializeDeviceMemory(buffer);
    testExecution(badPreparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);
    testExecution(badPreparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainExecutionTest, InvalidIOIndex) {
    auto preparedModel = createConvPreparedModel(kTestOperand, 2);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {});
    if (buffer == nullptr) return;

    RequestMemoryPool sharedMemory1 = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool sharedMemory2 = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool sharedMemory3 = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool deviceMemory = createDeviceMemoryPool(token);
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
                   .pools = createRequestMemoryPools(sharedMemory1, sharedMemory2, sharedMemory3,
                                                     deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);

    // This should fail, because the device memory is not allocated for output 1.
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg1, sharedMemoryArg2},
                   .outputs = {sharedMemoryArg3, deviceMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory1, sharedMemory2, sharedMemory3,
                                                     deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainExecutionTest, InvalidIOType) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [inputBuffer, inputToken] = allocateBuffer(preparedModel, {0}, {});
    auto [outputBuffer, outputToken] = allocateBuffer(preparedModel, {}, {0});
    if (inputBuffer == nullptr || outputBuffer == nullptr) return;

    RequestMemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool deviceMemory = createDeviceMemoryPool(inputToken);
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};

    // This should fail, because the device memory is allocated for input but used as output.
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);

    // This should fail, because the device memory is allocated for output but used as input.
    deviceMemory.set<RequestMemoryPool::Tag::token>(outputToken);
    initializeDeviceMemory(outputBuffer);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(MemoryDomainExecutionTest, UninitializedMemory) {
    auto preparedModel = createConvPreparedModel(kTestOperand);
    auto [buffer, token] = allocateBuffer(preparedModel, {0}, {0});
    if (buffer == nullptr) return;

    RequestMemoryPool sharedMemory = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool deviceMemory = createDeviceMemoryPool(token);
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = kTestOperandDataSize}};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};

    // This should fail, because the device memory is not initialized.
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::GENERAL_FAILURE);

    // This should initialize the device memory.
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::NONE);

    // Test again with initialized device memory.
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg},
                   .outputs = {sharedMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::NONE);
}

TEST_P(MemoryDomainExecutionTest, SameRequestMultipleRoles) {
    auto preparedModel = createConvPreparedModel(kTestOperand, 2);
    auto [buffer, token] = allocateBuffer(preparedModel, {0, 1}, {0, 1});
    if (buffer == nullptr) return;

    RequestMemoryPool sharedMemory1 = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool sharedMemory2 = createSharedMemoryPool(kTestOperandDataSize);
    RequestMemoryPool deviceMemory = createDeviceMemoryPool(token);
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
                   .pools = createRequestMemoryPools(sharedMemory1, sharedMemory2, deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);

    // This should fail, because the same device memory cannot be used for multiple outputs.
    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg1, sharedMemoryArg2},
                   .outputs = {deviceMemoryArg, deviceMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory1, sharedMemory2, deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);

    // The same device memory can be used for multiple inputs.
    initializeDeviceMemory(buffer);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArg, deviceMemoryArg},
                   .outputs = {sharedMemoryArg1, sharedMemoryArg2},
                   .pools = createRequestMemoryPools(sharedMemory1, sharedMemory2, deviceMemory)},
                  ErrorStatus::NONE);
}

TEST_P(MemoryDomainExecutionTest, InvalidDimensions) {
    // FENCED execution does not support dynamic shape.
    if (kExecutor == Executor::FENCED) return;

    TestOperand testOperand = kTestOperand;
    testOperand.dimensions[0] = 0;
    auto preparedModel = createConvPreparedModel(testOperand);
    auto deviceBuffer = allocateBuffer(preparedModel, {0}, {0},
                                       utils::toSigned(kTestOperand.dimensions).value());
    if (deviceBuffer.buffer == nullptr) return;

    // Use an incompatible dimension and make sure the length matches with the bad dimension.
    auto badDimensions = utils::toSigned(kTestOperand.dimensions).value();
    badDimensions[0] = 2;
    const uint32_t badTestOperandDataSize = kTestOperandDataSize * 2;

    RequestMemoryPool sharedMemory = createSharedMemoryPool(badTestOperandDataSize);
    RequestMemoryPool deviceMemory = createDeviceMemoryPool(deviceBuffer.token);
    RequestArgument sharedMemoryArg = {
            .location = {.poolIndex = 0, .offset = 0, .length = badTestOperandDataSize},
            .dimensions = badDimensions};
    RequestArgument deviceMemoryArg = {.location = {.poolIndex = 1}};
    RequestArgument deviceMemoryArgWithBadDimensions = {.location = {.poolIndex = 1},
                                                        .dimensions = badDimensions};

    initializeDeviceMemory(deviceBuffer.buffer);
    testExecution(preparedModel,
                  {.inputs = {deviceMemoryArgWithBadDimensions},
                   .outputs = {sharedMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);

    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArgWithBadDimensions},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::INVALID_ARGUMENT);

    testExecution(preparedModel,
                  {.inputs = {sharedMemoryArg},
                   .outputs = {deviceMemoryArg},
                   .pools = createRequestMemoryPools(sharedMemory, deviceMemory)},
                  ErrorStatus::GENERAL_FAILURE);
}

const auto kExecutorChoices = testing::Values(Executor::SYNC, Executor::BURST, Executor::FENCED);

std::string printMemoryDomainExecutionTest(
        const testing::TestParamInfo<MemoryDomainExecutionTestParam>& info) {
    const auto& [namedDevice, operandType, executor] = info.param;
    const std::string type = toString(static_cast<OperandType>(operandType));
    const std::string executorStr = toString(executor);
    return gtestCompliantName(getName(namedDevice) + "_" + type + "_" + executorStr);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MemoryDomainExecutionTest);
INSTANTIATE_TEST_SUITE_P(TestMemoryDomain, MemoryDomainExecutionTest,
                         testing::Combine(testing::ValuesIn(getNamedDevices()),
                                          kTestOperandTypeChoices, kExecutorChoices),
                         printMemoryDomainExecutionTest);

}  // namespace aidl::android::hardware::neuralnetworks::vts::functional
