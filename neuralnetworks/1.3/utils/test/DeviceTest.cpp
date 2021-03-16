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

#include "MockBuffer.h"
#include "MockDevice.h"
#include "MockPreparedModel.h"

#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IDevice.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.3/Device.h>

#include <functional>
#include <memory>
#include <string>

namespace android::hardware::neuralnetworks::V1_3::utils {
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
const sp<V1_3::IDevice> kInvalidDevice;
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
    const auto getVersionString_ret = makeCallbackReturn(V1_0::ErrorStatus::NONE, kName);
    const auto getType_ret = makeCallbackReturn(V1_0::ErrorStatus::NONE, V1_2::DeviceType::OTHER);
    const auto getSupportedExtensions_ret =
            makeCallbackReturn(V1_0::ErrorStatus::NONE, hidl_vec<V1_2::Extension>{});
    const auto getNumberOfCacheFilesNeeded_ret = makeCallbackReturn(
            V1_0::ErrorStatus::NONE, nn::kMaxNumberOfCacheFiles, nn::kMaxNumberOfCacheFiles);
    const auto getCapabilities_ret = makeCallbackReturn(
            V1_3::ErrorStatus::NONE,
            V1_3::Capabilities{
                    .relaxedFloat32toFloat16PerformanceScalar = kNoPerformanceInfo,
                    .relaxedFloat32toFloat16PerformanceTensor = kNoPerformanceInfo,
                    .ifPerformance = kNoPerformanceInfo,
                    .whilePerformance = kNoPerformanceInfo,
            });

    // Setup default actions for each relevant call.
    ON_CALL(*mockDevice, getVersionString(_)).WillByDefault(Invoke(getVersionString_ret));
    ON_CALL(*mockDevice, getType(_)).WillByDefault(Invoke(getType_ret));
    ON_CALL(*mockDevice, getSupportedExtensions(_))
            .WillByDefault(Invoke(getSupportedExtensions_ret));
    ON_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .WillByDefault(Invoke(getNumberOfCacheFilesNeeded_ret));
    ON_CALL(*mockDevice, getCapabilities_1_3(_)).WillByDefault(Invoke(getCapabilities_ret));

    // Ensure that older calls are not used.
    EXPECT_CALL(*mockDevice, getCapabilities(_)).Times(0);
    EXPECT_CALL(*mockDevice, getCapabilities_1_1(_)).Times(0);
    EXPECT_CALL(*mockDevice, getCapabilities_1_2(_)).Times(0);
    EXPECT_CALL(*mockDevice, getSupportedOperations(_, _)).Times(0);
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_1(_, _)).Times(0);
    EXPECT_CALL(*mockDevice, prepareModel(_, _)).Times(0);
    EXPECT_CALL(*mockDevice, prepareModel_1_1(_, _, _)).Times(0);
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_2(_, _)).Times(0);
    EXPECT_CALL(*mockDevice, prepareModel_1_2(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _)).Times(0);

    // These EXPECT_CALL(...).Times(testing::AnyNumber()) calls are to suppress warnings on the
    // uninteresting methods calls.
    EXPECT_CALL(*mockDevice, getVersionString(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getType(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getCapabilities_1_3(_)).Times(testing::AnyNumber());

    return mockDevice;
}

auto makePreparedModelReturn(V1_3::ErrorStatus launchStatus, V1_3::ErrorStatus returnStatus,
                             const sp<MockPreparedModel>& preparedModel) {
    return [launchStatus, returnStatus, preparedModel](
                   const V1_3::Model& /*model*/, V1_1::ExecutionPreference /*preference*/,
                   V1_3::Priority /*priority*/, const V1_3::OptionalTimePoint& /*deadline*/,
                   const hardware::hidl_vec<hardware::hidl_handle>& /*modelCache*/,
                   const hardware::hidl_vec<hardware::hidl_handle>& /*dataCache*/,
                   const CacheToken& /*token*/, const sp<V1_3::IPreparedModelCallback>& cb)
                   -> hardware::Return<V1_3::ErrorStatus> {
        cb->notify_1_3(returnStatus, preparedModel).isOk();
        return launchStatus;
    };
}
auto makePreparedModelFromCacheReturn(V1_3::ErrorStatus launchStatus,
                                      V1_3::ErrorStatus returnStatus,
                                      const sp<MockPreparedModel>& preparedModel) {
    return [launchStatus, returnStatus, preparedModel](
                   const V1_3::OptionalTimePoint& /*deadline*/,
                   const hardware::hidl_vec<hardware::hidl_handle>& /*modelCache*/,
                   const hardware::hidl_vec<hardware::hidl_handle>& /*dataCache*/,
                   const CacheToken& /*token*/, const sp<V1_3::IPreparedModelCallback>& cb)
                   -> hardware::Return<V1_3::ErrorStatus> {
        cb->notify_1_3(returnStatus, preparedModel).isOk();
        return launchStatus;
    };
}
auto makeAllocateReturn(ErrorStatus status, const sp<MockBuffer>& buffer, uint32_t token) {
    return [status, buffer, token](
                   const V1_3::BufferDesc& /*desc*/,
                   const hardware::hidl_vec<sp<V1_3::IPreparedModel>>& /*preparedModels*/,
                   const hardware::hidl_vec<V1_3::BufferRole>& /*inputRoles*/,
                   const hardware::hidl_vec<V1_3::BufferRole>& /*outputRoles*/,
                   const V1_3::IDevice::allocate_cb& cb) -> hardware::Return<void> {
        cb(status, buffer, token);
        return hardware::Void();
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

TEST(DeviceTest, getVersionStringError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto ret = makeCallbackReturn(V1_0::ErrorStatus::GENERAL_FAILURE, "");
    EXPECT_CALL(*mockDevice, getVersionString(_)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getVersionStringTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getVersionString(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getVersionStringDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getVersionString(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, getTypeError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto ret =
            makeCallbackReturn(V1_0::ErrorStatus::GENERAL_FAILURE, V1_2::DeviceType::OTHER);
    EXPECT_CALL(*mockDevice, getType(_)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getTypeTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getType(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getTypeDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getType(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, getSupportedExtensionsError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto ret =
            makeCallbackReturn(V1_0::ErrorStatus::GENERAL_FAILURE, hidl_vec<V1_2::Extension>{});
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getSupportedExtensionsTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getSupportedExtensionsDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, getNumberOfCacheFilesNeededError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto ret = makeCallbackReturn(V1_0::ErrorStatus::GENERAL_FAILURE,
                                        nn::kMaxNumberOfCacheFiles, nn::kMaxNumberOfCacheFiles);
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, dataCacheFilesExceedsSpecifiedMax) {
    // setup test
    const auto mockDevice = createMockDevice();
    const auto ret = makeCallbackReturn(V1_0::ErrorStatus::NONE, nn::kMaxNumberOfCacheFiles + 1,
                                        nn::kMaxNumberOfCacheFiles);
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, modelCacheFilesExceedsSpecifiedMax) {
    // setup test
    const auto mockDevice = createMockDevice();
    const auto ret = makeCallbackReturn(V1_0::ErrorStatus::NONE, nn::kMaxNumberOfCacheFiles,
                                        nn::kMaxNumberOfCacheFiles + 1);
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getNumberOfCacheFilesNeededTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getNumberOfCacheFilesNeededDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, getCapabilitiesError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto ret = makeCallbackReturn(
            V1_3::ErrorStatus::GENERAL_FAILURE,
            V1_3::Capabilities{
                    .relaxedFloat32toFloat16PerformanceScalar = kNoPerformanceInfo,
                    .relaxedFloat32toFloat16PerformanceTensor = kNoPerformanceInfo,
                    .ifPerformance = kNoPerformanceInfo,
                    .whilePerformance = kNoPerformanceInfo,
            });
    EXPECT_CALL(*mockDevice, getCapabilities_1_3(_)).Times(1).WillOnce(Invoke(ret));

    // run test
    const auto result = Device::create(kName, mockDevice);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, getCapabilitiesTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getCapabilities_1_3(_))
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
    EXPECT_CALL(*mockDevice, getCapabilities_1_3(_))
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
    EXPECT_EQ(featureLevel, nn::Version::ANDROID_R);
}

TEST(DeviceTest, getCachedData) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getVersionString(_)).Times(1);
    EXPECT_CALL(*mockDevice, getType(_)).Times(1);
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_)).Times(1);
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_)).Times(1);
    EXPECT_CALL(*mockDevice, getCapabilities_1_3(_)).Times(1);

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
        cb(V1_3::ErrorStatus::NONE, std::vector<bool>(model.main.operations.size(), true));
        return hardware::Void();
    };
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_3(_, _)).Times(1).WillOnce(Invoke(ret));

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
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, {});
        return hardware::Void();
    };
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_3(_, _)).Times(1).WillOnce(Invoke(ret));

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
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_3(_, _))
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
    EXPECT_CALL(*mockDevice, getSupportedOperations_1_3(_, _))
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
    const auto mockPreparedModel = MockPreparedModel::create();
    EXPECT_CALL(*mockDevice, prepareModel_1_3(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(V1_3::ErrorStatus::NONE,
                                                     V1_3::ErrorStatus::NONE, mockPreparedModel)));

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
    EXPECT_CALL(*mockDevice, prepareModel_1_3(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(V1_3::ErrorStatus::GENERAL_FAILURE,
                                                     V1_3::ErrorStatus::GENERAL_FAILURE, nullptr)));

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
    EXPECT_CALL(*mockDevice, prepareModel_1_3(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(V1_3::ErrorStatus::NONE,
                                                     V1_3::ErrorStatus::GENERAL_FAILURE, nullptr)));

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
    EXPECT_CALL(*mockDevice, prepareModel_1_3(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(V1_3::ErrorStatus::NONE,
                                                     V1_3::ErrorStatus::NONE, nullptr)));

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
    EXPECT_CALL(*mockDevice, prepareModel_1_3(_, _, _, _, _, _, _, _))
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
    EXPECT_CALL(*mockDevice, prepareModel_1_3(_, _, _, _, _, _, _, _))
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
    const auto ret = [&mockDevice]() -> hardware::Return<V1_3::ErrorStatus> {
        mockDevice->simulateCrash();
        return V1_3::ErrorStatus::NONE;
    };
    EXPECT_CALL(*mockDevice, prepareModel_1_3(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, prepareModelFromCache) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    const auto mockPreparedModel = MockPreparedModel::create();
    EXPECT_CALL(*mockDevice, prepareModelFromCache_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelFromCacheReturn(
                    V1_3::ErrorStatus::NONE, V1_3::ErrorStatus::NONE, mockPreparedModel)));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST(DeviceTest, prepareModelFromCacheLaunchError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelFromCacheReturn(V1_3::ErrorStatus::GENERAL_FAILURE,
                                                              V1_3::ErrorStatus::GENERAL_FAILURE,
                                                              nullptr)));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, prepareModelFromCacheReturnError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelFromCacheReturn(
                    V1_3::ErrorStatus::NONE, V1_3::ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, prepareModelFromCacheNullptrError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelFromCacheReturn(V1_3::ErrorStatus::NONE,
                                                              V1_3::ErrorStatus::NONE, nullptr)));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, prepareModelFromCacheTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, prepareModelFromCacheDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, prepareModelFromCacheAsyncCrash) {
    // setup test
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    const auto ret = [&mockDevice]() -> hardware::Return<V1_3::ErrorStatus> {
        mockDevice->simulateCrash();
        return V1_3::ErrorStatus::NONE;
    };
    EXPECT_CALL(*mockDevice, prepareModelFromCache_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(DeviceTest, allocate) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    const auto mockBuffer = MockBuffer::create();
    constexpr uint32_t token = 1;
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeAllocateReturn(ErrorStatus::NONE, mockBuffer, token)));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST(DeviceTest, allocateError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeAllocateReturn(ErrorStatus::GENERAL_FAILURE, nullptr, 0)));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, allocateTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(DeviceTest, allocateDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice).value();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
