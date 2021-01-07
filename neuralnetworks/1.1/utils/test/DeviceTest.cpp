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

#include "MockDevice.h"
#include "MockPreparedModel.h"

#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IDevice.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.1/Device.h>

#include <functional>
#include <memory>
#include <string>

namespace android::hardware::neuralnetworks::V1_1::utils {
namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;

const nn::Model kSimpleModel = {
        .main = {.operands = {{.type = nn::OperandType::TENSOR_FLOAT32,
                               .dimensions = {1},
                               .lifetime = nn::Operand::LifeTime::SUBGRAPH_INPUT},
                              {.type = nn::OperandType::TENSOR_FLOAT32,
                               .dimensions = {1},
                               .lifetime = nn::Operand::LifeTime::SUBGRAPH_OUTPUT}},
                 .operations = {{.type = nn::OperationType::RELU, .inputs = {0}, .outputs = {1}}},
                 .inputIndexes = {0},
                 .outputIndexes = {1}}};

const std::string kName = "Google-MockV1";
const std::string kInvalidName = "";
const sp<V1_1::IDevice> kInvalidDevice;
constexpr V1_0::PerformanceInfo kNoPerformanceInfo = {
        .execTime = std::numeric_limits<float>::max(),
        .powerUsage = std::numeric_limits<float>::max()};

template <typename... Args>
auto makeCallbackReturn(Args&&... args) {
    return [argPack = std::make_tuple(std::forward<Args>(args)...)](const auto& cb) {
        std::apply(cb, argPack);
        return Void();
    };
}

sp<MockDevice> createMockDevice() {
    const auto mockDevice = MockDevice::create();

    // Setup default actions for each relevant call.
    const auto getCapabilities_ret =
            makeCallbackReturn(V1_0::ErrorStatus::NONE,
                               V1_1::Capabilities{
                                       .float32Performance = kNoPerformanceInfo,
                                       .quantized8Performance = kNoPerformanceInfo,
                                       .relaxedFloat32toFloat16Performance = kNoPerformanceInfo,
                               });

    // Setup default actions for each relevant call.
    ON_CALL(*mockDevice, getCapabilities_1_1(_)).WillByDefault(Invoke(getCapabilities_ret));

    // Ensure that older calls are not used.
    EXPECT_CALL(*mockDevice, getCapabilities(_)).Times(0);
    EXPECT_CALL(*mockDevice, getSupportedOperations(_, _)).Times(0);
    EXPECT_CALL(*mockDevice, prepareModel(_, _)).Times(0);

    // These EXPECT_CALL(...).Times(testing::AnyNumber()) calls are to suppress warnings on the
    // uninteresting methods calls.
    EXPECT_CALL(*mockDevice, getCapabilities_1_1(_)).Times(testing::AnyNumber());

    return mockDevice;
}

auto makePreparedModelReturn(V1_0::ErrorStatus launchStatus, V1_0::ErrorStatus returnStatus,
                             const sp<V1_0::utils::MockPreparedModel>& preparedModel) {
    return [launchStatus, returnStatus, preparedModel](const V1_1::Model& /*model*/,
                                                       V1_1::ExecutionPreference /*preference*/,
                                                       const sp<V1_0::IPreparedModelCallback>& cb)
                   -> hardware::Return<V1_0::ErrorStatus> {
        cb->notify(returnStatus, preparedModel).isOk();
        return launchStatus;
    };
}

std::function<hardware::Status()> makeTransportFailure(status_t status) {
    return [status] { return hardware::Status::fromStatusT(status); };
}

const auto makeGeneralTransportFailure = makeTransportFailure(NO_MEMORY);
const auto makeDeadObjectFailure = makeTransportFailure(DEAD_OBJECT);

}  // namespace

TEST(DeviceTest, invalidName) {
    // run test
    const auto device = MockDevice::create();
    const auto result = Device::create(kInvalidName, device);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::INVALID_ARGUMENT);
}

TEST(DeviceTest, invalidDevice) {
    // run test
    const auto result = Device::create(kName, kInvalidDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::INVALID_ARGUMENT);
}

TEST(DeviceTest, getCapabilitiesError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto ret =
            makeCallbackReturn(V1_0::ErrorStatus::GENERAL_FAILURE,
                               V1_1::Capabilities{
                                       .float32Performance = kNoPerformanceInfo,
                                       .quantized8Performance = kNoPerformanceInfo,
                                       .relaxedFloat32toFloat16Performance = kNoPerformanceInfo,
                               });
    EXPECT_CALL(*mockDevice, getCapabilities_1_1(_)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getCapabilitiesTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getCapabilities_1_1(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getCapabilitiesDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getCapabilities_1_1(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, linkToDeathError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto ret = []() -> Return<bool> { return false; };
    EXPECT_CALL(*mockDevice, linkToDeathRet()).Times(1).WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, linkToDeathTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, linkToDeathRet())
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, linkToDeathDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, linkToDeathRet())
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, getName) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();

    // run test
    const auto& name = device->getName();

    // verify result
    EXPECT_EQ(name, kName);
}

TEST(DeviceTest, getFeatureLevel) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();

    // run test
    const auto featureLevel = device->getFeatureLevel();

    // verify result
    EXPECT_EQ(featureLevel, nn::Version::ANDROID_P);
}

TEST(DeviceTest, getCachedData) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto result = Device::create(kName, mockDevice);
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    const auto& device = result.value();

    // run test and verify results
    EXPECT_EQ(device->getVersionString(), device->getVersionString());
    EXPECT_EQ(device->getType(), device->getType());
    EXPECT_EQ(device->getSupportedExtensions(), device->getSupportedExtensions());
    EXPECT_EQ(device->getNumberOfCacheFilesNeeded(), device->getNumberOfCacheFilesNeeded());
    EXPECT_EQ(device->getCapabilities(), device->getCapabilities());
}

TEST(DeviceTest, wait) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto ret = []() -> Return<void> { return {}; };
    EXPECT_CALL(*mockDevice, ping()).Times(1).WillOnce(InvokeWithoutArgs(ret));
    const auto device = Device::create(kName, mockDevice).value();

    // run test
    const auto result = device->wait();

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(DeviceTest, waitTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, ping())
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));
    const auto device = Device::create(kName, mockDevice).value();

    // run test
    const auto result = device->wait();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, waitDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, ping()).Times(1).WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));
    const auto device = Device::create(kName, mockDevice).value();

    // run test
    const auto result = device->wait();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, getSupportedOperations) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    const auto ret = [](const auto& model, const auto& cb) {
        cb(V1_0::ErrorStatus::NONE, std::vector<bool>(model.operations.size(), true));
        return hardware::Void();
    };
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_1(_, _)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = device->getSupportedOperations(kSimpleModel);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    const auto& supportedOperations = result.value();
    EXPECT_EQ(supportedOperations.size(), kSimpleModel.main.operations.size());
    EXPECT_THAT(supportedOperations, Each(testing::IsTrue()));
}

TEST(DeviceTest, getSupportedOperationsError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    const auto ret = [](const auto& /*model*/, const auto& cb) {
        cb(V1_0::ErrorStatus::GENERAL_FAILURE, {});
        return hardware::Void();
    };
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_1(_, _)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = device->getSupportedOperations(kSimpleModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getSupportedOperationsTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_1(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->getSupportedOperations(kSimpleModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getSupportedOperationsDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_1(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->getSupportedOperations(kSimpleModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, prepareModel) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    const auto mockPreparedModel = V1_0::utils::MockPreparedModel::create();
    EXPECT_CALL(*mockDevice, prepareModel_1_1(_, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(V1_0::ErrorStatus::NONE,
                                                     V1_0::ErrorStatus::NONE, mockPreparedModel)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST(DeviceTest, prepareModelLaunchError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModel_1_1(_, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(V1_0::ErrorStatus::GENERAL_FAILURE,
                                                     V1_0::ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, prepareModelReturnError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModel_1_1(_, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(V1_0::ErrorStatus::NONE,
                                                     V1_0::ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, prepareModelNullptrError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModel_1_1(_, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(V1_0::ErrorStatus::NONE,
                                                     V1_0::ErrorStatus::NONE, nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, prepareModelTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModel_1_1(_, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, prepareModelDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModel_1_1(_, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, prepareModelAsyncCrash) {
    // setup test
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    const auto ret = [&mockDevice]() -> hardware::Return<V1_0::ErrorStatus> {
        mockDevice->simulateCrash();
        return V1_0::ErrorStatus::NONE;
    };
    EXPECT_CALL(*mockDevice, prepareModel_1_1(_, _, _)).Times(1).WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, prepareModelFromCacheNotSupported) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, allocateNotSupported) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

}  // namespace android::hardware::neuralnetworks::V1_1::utils
