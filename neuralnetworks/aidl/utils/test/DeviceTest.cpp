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

#include "MockBuffer.h"
#include "MockDevice.h"
#include "MockPreparedModel.h"
#include "TestUtils.h"

#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <android/binder_auto_utils.h>
#include <android/binder_status.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IDevice.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/aidl/Device.h>

#include <functional>
#include <memory>
#include <string>

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

namespace nn = ::android::nn;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::SetArgPointee;

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
const std::shared_ptr<BnDevice> kInvalidDevice;
constexpr PerformanceInfo kNoPerformanceInfo = {.execTime = std::numeric_limits<float>::max(),
                                                .powerUsage = std::numeric_limits<float>::max()};
constexpr NumberOfCacheFiles kNumberOfCacheFiles = {.numModelCache = nn::kMaxNumberOfCacheFiles - 1,
                                                    .numDataCache = nn::kMaxNumberOfCacheFiles};
constexpr auto makeStatusOk = [] { return ndk::ScopedAStatus::ok(); };

std::shared_ptr<MockDevice> createMockDevice() {
    const auto mockDevice = MockDevice::create();

    // Setup default actions for each relevant call.
    ON_CALL(*mockDevice, getVersionString(_))
            .WillByDefault(DoAll(SetArgPointee<0>(kName), InvokeWithoutArgs(makeStatusOk)));
    ON_CALL(*mockDevice, getType(_))
            .WillByDefault(
                    DoAll(SetArgPointee<0>(DeviceType::OTHER), InvokeWithoutArgs(makeStatusOk)));
    ON_CALL(*mockDevice, getSupportedExtensions(_))
            .WillByDefault(DoAll(SetArgPointee<0>(std::vector<Extension>{}),
                                 InvokeWithoutArgs(makeStatusOk)));
    ON_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .WillByDefault(
                    DoAll(SetArgPointee<0>(kNumberOfCacheFiles), InvokeWithoutArgs(makeStatusOk)));
    ON_CALL(*mockDevice, getCapabilities(_))
            .WillByDefault(
                    DoAll(SetArgPointee<0>(Capabilities{
                                  .relaxedFloat32toFloat16PerformanceScalar = kNoPerformanceInfo,
                                  .relaxedFloat32toFloat16PerformanceTensor = kNoPerformanceInfo,
                                  .ifPerformance = kNoPerformanceInfo,
                                  .whilePerformance = kNoPerformanceInfo,
                          }),
                          InvokeWithoutArgs(makeStatusOk)));

    // These EXPECT_CALL(...).Times(testing::AnyNumber()) calls are to suppress warnings on the
    // uninteresting methods calls.
    EXPECT_CALL(*mockDevice, getVersionString(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getType(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getCapabilities(_)).Times(testing::AnyNumber());

    return mockDevice;
}

constexpr auto makePreparedModelReturnImpl =
        [](ErrorStatus launchStatus, ErrorStatus returnStatus,
           const std::shared_ptr<MockPreparedModel>& preparedModel,
           const std::shared_ptr<IPreparedModelCallback>& cb) {
            cb->notify(returnStatus, preparedModel);
            if (launchStatus == ErrorStatus::NONE) {
                return ndk::ScopedAStatus::ok();
            }
            return ndk::ScopedAStatus::fromServiceSpecificError(static_cast<int32_t>(launchStatus));
        };

auto makePreparedModelReturn(ErrorStatus launchStatus, ErrorStatus returnStatus,
                             const std::shared_ptr<MockPreparedModel>& preparedModel) {
    return [launchStatus, returnStatus, preparedModel](
                   const Model& /*model*/, ExecutionPreference /*preference*/,
                   Priority /*priority*/, const int64_t& /*deadline*/,
                   const std::vector<ndk::ScopedFileDescriptor>& /*modelCache*/,
                   const std::vector<ndk::ScopedFileDescriptor>& /*dataCache*/,
                   const std::vector<uint8_t>& /*token*/,
                   const std::shared_ptr<IPreparedModelCallback>& cb) -> ndk::ScopedAStatus {
        return makePreparedModelReturnImpl(launchStatus, returnStatus, preparedModel, cb);
    };
}

const std::vector<nn::TokenValuePair> kHints = {nn::TokenValuePair{.token = 0, .value = {1}}};
const std::vector<nn::ExtensionNameAndPrefix> kExtensionNameToPrefix = {
        nn::ExtensionNameAndPrefix{.name = "com.android.nn_test", .prefix = 1}};
auto makePreparedModelWithConfigReturn(ErrorStatus launchStatus, ErrorStatus returnStatus,
                                       const std::shared_ptr<MockPreparedModel>& preparedModel) {
    return [launchStatus, returnStatus, preparedModel](
                   const Model& /*model*/, const PrepareModelConfig& /*config*/,
                   const std::shared_ptr<IPreparedModelCallback>& cb) -> ndk::ScopedAStatus {
        return makePreparedModelReturnImpl(launchStatus, returnStatus, preparedModel, cb);
    };
}

auto makePreparedModelFromCacheReturn(ErrorStatus launchStatus, ErrorStatus returnStatus,
                                      const std::shared_ptr<MockPreparedModel>& preparedModel) {
    return [launchStatus, returnStatus, preparedModel](
                   const int64_t& /*deadline*/,
                   const std::vector<ndk::ScopedFileDescriptor>& /*modelCache*/,
                   const std::vector<ndk::ScopedFileDescriptor>& /*dataCache*/,
                   const std::vector<uint8_t>& /*token*/,
                   const std::shared_ptr<IPreparedModelCallback>& cb) {
        return makePreparedModelReturnImpl(launchStatus, returnStatus, preparedModel, cb);
    };
}

constexpr auto makeGeneralFailure = [] {
    return ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(ErrorStatus::GENERAL_FAILURE));
};
constexpr auto makeGeneralTransportFailure = [] {
    return ndk::ScopedAStatus::fromStatus(STATUS_NO_MEMORY);
};
constexpr auto makeDeadObjectFailure = [] {
    return ndk::ScopedAStatus::fromStatus(STATUS_DEAD_OBJECT);
};

class DeviceTest : public VersionedAidlUtilsTestBase {};

}  // namespace

TEST_P(DeviceTest, invalidName) {
    // run test
    const auto device = MockDevice::create();
    const auto result = Device::create(kInvalidName, device, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(DeviceTest, invalidDevice) {
    // run test
    const auto result = Device::create(kName, kInvalidDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::INVALID_ARGUMENT);
}

TEST_P(DeviceTest, getVersionStringError) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getVersionString(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getVersionStringTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getVersionString(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getVersionStringDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getVersionString(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, getTypeError) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getType(_)).Times(1).WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getTypeTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getType(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getTypeDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getType(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, getSupportedExtensionsError) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getSupportedExtensionsTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getSupportedExtensionsDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, getNumberOfCacheFilesNeeded) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_)).Times(1);

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_TRUE(result.has_value());
    constexpr auto kNumberOfCacheFilesPair = std::make_pair<uint32_t, uint32_t>(
            kNumberOfCacheFiles.numModelCache, kNumberOfCacheFiles.numDataCache);
    EXPECT_EQ(result.value()->getNumberOfCacheFilesNeeded(), kNumberOfCacheFilesPair);
}

TEST_P(DeviceTest, getNumberOfCacheFilesNeededError) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, dataCacheFilesExceedsSpecifiedMax) {
    // setup test
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<0>(NumberOfCacheFiles{
                                    .numModelCache = nn::kMaxNumberOfCacheFiles + 1,
                                    .numDataCache = nn::kMaxNumberOfCacheFiles}),
                            InvokeWithoutArgs(makeStatusOk)));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, modelCacheFilesExceedsSpecifiedMax) {
    // setup test
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<0>(NumberOfCacheFiles{
                                    .numModelCache = nn::kMaxNumberOfCacheFiles,
                                    .numDataCache = nn::kMaxNumberOfCacheFiles + 1}),
                            InvokeWithoutArgs(makeStatusOk)));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getNumberOfCacheFilesNeededTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getNumberOfCacheFilesNeededDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, getCapabilitiesError) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getCapabilities(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getCapabilitiesTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getCapabilities(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getCapabilitiesDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getCapabilities(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = Device::create(kName, mockDevice, kVersion);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, getName) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();

    // run test
    const auto& name = device->getName();

    // verify result
    EXPECT_EQ(name, kName);
}

TEST_P(DeviceTest, getFeatureLevel) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();

    // run test
    const auto featureLevel = device->getFeatureLevel();

    // verify result
    EXPECT_EQ(featureLevel, kVersion);
}

TEST_P(DeviceTest, getCachedData) {
    // setup call
    const auto mockDevice = createMockDevice();
    EXPECT_CALL(*mockDevice, getVersionString(_)).Times(1);
    EXPECT_CALL(*mockDevice, getType(_)).Times(1);
    EXPECT_CALL(*mockDevice, getSupportedExtensions(_)).Times(1);
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded(_)).Times(1);
    EXPECT_CALL(*mockDevice, getCapabilities(_)).Times(1);

    const auto result = Device::create(kName, mockDevice, kVersion);
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

TEST_P(DeviceTest, getSupportedOperations) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, getSupportedOperations(_, _))
            .Times(1)
            .WillOnce(DoAll(
                    SetArgPointee<1>(std::vector<bool>(kSimpleModel.main.operations.size(), true)),
                    InvokeWithoutArgs(makeStatusOk)));

    // run test
    const auto result = device->getSupportedOperations(kSimpleModel);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    const auto& supportedOperations = result.value();
    EXPECT_EQ(supportedOperations.size(), kSimpleModel.main.operations.size());
    EXPECT_THAT(supportedOperations, Each(testing::IsTrue()));
}

TEST_P(DeviceTest, getSupportedOperationsError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, getSupportedOperations(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = device->getSupportedOperations(kSimpleModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getSupportedOperationsTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, getSupportedOperations(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->getSupportedOperations(kSimpleModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, getSupportedOperationsDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, getSupportedOperations(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->getSupportedOperations(kSimpleModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, prepareModel) {
    if (kVersion.level > nn::Version::Level::FEATURE_LEVEL_7) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    const auto mockPreparedModel = MockPreparedModel::create();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(ErrorStatus::NONE, ErrorStatus::NONE,
                                                     mockPreparedModel)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST_P(DeviceTest, prepareModelLaunchError) {
    if (kVersion.level > nn::Version::Level::FEATURE_LEVEL_7) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(ErrorStatus::GENERAL_FAILURE,
                                                     ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelReturnError) {
    if (kVersion.level > nn::Version::Level::FEATURE_LEVEL_7) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelReturn(ErrorStatus::NONE,
                                                     ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelNullptrError) {
    if (kVersion.level > nn::Version::Level::FEATURE_LEVEL_7) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(
                    Invoke(makePreparedModelReturn(ErrorStatus::NONE, ErrorStatus::NONE, nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelTransportFailure) {
    if (kVersion.level > nn::Version::Level::FEATURE_LEVEL_7) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelDeadObject) {
    if (kVersion.level > nn::Version::Level::FEATURE_LEVEL_7) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, prepareModelAsyncCrash) {
    if (kVersion.level > nn::Version::Level::FEATURE_LEVEL_7) return;

    // setup test
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    const auto ret = [&device]() {
        DeathMonitor::serviceDied(
                reinterpret_cast<void*>(device->getDeathMonitor()->getCookieKey()));
        return ndk::ScopedAStatus::ok();
    };
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, prepareModelWithConfig) {
    if (kVersion.level < nn::Version::Level::FEATURE_LEVEL_8) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    const auto mockPreparedModel = MockPreparedModel::create();
    EXPECT_CALL(*mockDevice, prepareModelWithConfig(_, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelWithConfigReturn(ErrorStatus::NONE, ErrorStatus::NONE,
                                                               mockPreparedModel)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, kHints,
                                             kExtensionNameToPrefix);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST_P(DeviceTest, prepareModelWithConfigLaunchError) {
    if (kVersion.level < nn::Version::Level::FEATURE_LEVEL_8) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelWithConfig(_, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelWithConfigReturn(
                    ErrorStatus::GENERAL_FAILURE, ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, kHints,
                                             kExtensionNameToPrefix);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelWithConfigReturnError) {
    if (kVersion.level < nn::Version::Level::FEATURE_LEVEL_8) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelWithConfig(_, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelWithConfigReturn(
                    ErrorStatus::NONE, ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, kHints,
                                             kExtensionNameToPrefix);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelWithConfigNullptrError) {
    if (kVersion.level < nn::Version::Level::FEATURE_LEVEL_8) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelWithConfig(_, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelWithConfigReturn(ErrorStatus::NONE, ErrorStatus::NONE,
                                                               nullptr)));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, kHints,
                                             kExtensionNameToPrefix);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelWithConfigTransportFailure) {
    if (kVersion.level < nn::Version::Level::FEATURE_LEVEL_8) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelWithConfig(_, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, kHints,
                                             kExtensionNameToPrefix);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelWithConfigDeadObject) {
    if (kVersion.level < nn::Version::Level::FEATURE_LEVEL_8) return;

    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelWithConfig(_, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, kHints,
                                             kExtensionNameToPrefix);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, prepareModelWithConfigAsyncCrash) {
    if (kVersion.level < nn::Version::Level::FEATURE_LEVEL_8) return;

    // setup test
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    const auto ret = [&device]() {
        DeathMonitor::serviceDied(
                reinterpret_cast<void*>(device->getDeathMonitor()->getCookieKey()));
        return ndk::ScopedAStatus::ok();
    };
    EXPECT_CALL(*mockDevice, prepareModelWithConfig(_, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = device->prepareModel(kSimpleModel, nn::ExecutionPreference::DEFAULT,
                                             nn::Priority::DEFAULT, {}, {}, {}, {}, kHints,
                                             kExtensionNameToPrefix);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, prepareModelFromCache) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    const auto mockPreparedModel = MockPreparedModel::create();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelFromCacheReturn(ErrorStatus::NONE, ErrorStatus::NONE,
                                                              mockPreparedModel)));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST_P(DeviceTest, prepareModelFromCacheLaunchError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelFromCacheReturn(
                    ErrorStatus::GENERAL_FAILURE, ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelFromCacheReturnError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelFromCacheReturn(
                    ErrorStatus::NONE, ErrorStatus::GENERAL_FAILURE, nullptr)));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelFromCacheNullptrError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makePreparedModelFromCacheReturn(ErrorStatus::NONE, ErrorStatus::NONE,
                                                              nullptr)));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelFromCacheTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, prepareModelFromCacheDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, prepareModelFromCacheAsyncCrash) {
    // setup test
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    const auto ret = [&device]() {
        DeathMonitor::serviceDied(
                reinterpret_cast<void*>(device->getDeathMonitor()->getCookieKey()));
        return ndk::ScopedAStatus::ok();
    };
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST_P(DeviceTest, allocate) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    const auto mockBuffer = DeviceBuffer{.buffer = MockBuffer::create(), .token = 1};
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<4>(mockBuffer), InvokeWithoutArgs(makeStatusOk)));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST_P(DeviceTest, allocateError) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, allocateTransportFailure) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST_P(DeviceTest, allocateDeadObject) {
    // setup call
    const auto mockDevice = createMockDevice();
    const auto device = Device::create(kName, mockDevice, kVersion).value();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

INSTANTIATE_VERSIONED_AIDL_UTILS_TEST(DeviceTest, kAllAidlVersions);

}  // namespace aidl::android::hardware::neuralnetworks::utils
