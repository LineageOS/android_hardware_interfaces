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

#include <gmock/gmock.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/ResilientDevice.h>
#include <tuple>
#include <utility>
#include "MockBuffer.h"
#include "MockDevice.h"
#include "MockPreparedModel.h"

namespace android::hardware::neuralnetworks::utils {
namespace {

using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

using SharedMockDevice = std::shared_ptr<const nn::MockDevice>;
using MockDeviceFactory = ::testing::MockFunction<nn::GeneralResult<nn::SharedDevice>(bool)>;

const std::string kName = "Google-MockV1";
const std::string kVersionString = "version1";
const auto kExtensions = std::vector<nn::Extension>{};
constexpr auto kNoInfo = std::numeric_limits<float>::max();
constexpr auto kNoPerformanceInfo =
        nn::Capabilities::PerformanceInfo{.execTime = kNoInfo, .powerUsage = kNoInfo};
const auto kCapabilities = nn::Capabilities{
        .relaxedFloat32toFloat16PerformanceScalar = kNoPerformanceInfo,
        .relaxedFloat32toFloat16PerformanceTensor = kNoPerformanceInfo,
        .operandPerformance = nn::Capabilities::OperandPerformanceTable::create({}).value(),
        .ifPerformance = kNoPerformanceInfo,
        .whilePerformance = kNoPerformanceInfo};
constexpr auto kNumberOfCacheFilesNeeded = std::pair<uint32_t, uint32_t>(5, 3);

SharedMockDevice createConfiguredMockDevice() {
    auto mockDevice = std::make_shared<const nn::MockDevice>();

    // Setup default actions for each relevant call.
    constexpr auto getName_ret = []() -> const std::string& { return kName; };
    constexpr auto getVersionString_ret = []() -> const std::string& { return kVersionString; };
    constexpr auto kFeatureLevel = nn::Version::ANDROID_OC_MR1;
    constexpr auto kDeviceType = nn::DeviceType::ACCELERATOR;
    constexpr auto getSupportedExtensions_ret = []() -> const std::vector<nn::Extension>& {
        return kExtensions;
    };
    constexpr auto getCapabilities_ret = []() -> const nn::Capabilities& { return kCapabilities; };

    // Setup default actions for each relevant call.
    ON_CALL(*mockDevice, getName()).WillByDefault(getName_ret);
    ON_CALL(*mockDevice, getVersionString()).WillByDefault(getVersionString_ret);
    ON_CALL(*mockDevice, getFeatureLevel()).WillByDefault(Return(kFeatureLevel));
    ON_CALL(*mockDevice, getType()).WillByDefault(Return(kDeviceType));
    ON_CALL(*mockDevice, getSupportedExtensions()).WillByDefault(getSupportedExtensions_ret);
    ON_CALL(*mockDevice, getCapabilities()).WillByDefault(getCapabilities_ret);
    ON_CALL(*mockDevice, getNumberOfCacheFilesNeeded())
            .WillByDefault(Return(kNumberOfCacheFilesNeeded));

    // These EXPECT_CALL(...).Times(testing::AnyNumber()) calls are to suppress warnings on the
    // uninteresting methods calls.
    EXPECT_CALL(*mockDevice, getName()).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getVersionString()).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getFeatureLevel()).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getType()).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getSupportedExtensions()).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getCapabilities()).Times(testing::AnyNumber());
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded()).Times(testing::AnyNumber());

    return mockDevice;
}

std::tuple<SharedMockDevice, std::unique_ptr<MockDeviceFactory>,
           std::shared_ptr<const ResilientDevice>>
setup() {
    auto mockDevice = createConfiguredMockDevice();

    auto mockDeviceFactory = std::make_unique<MockDeviceFactory>();
    EXPECT_CALL(*mockDeviceFactory, Call(true)).Times(1).WillOnce(Return(mockDevice));

    auto device = ResilientDevice::create(mockDeviceFactory->AsStdFunction()).value();
    return std::make_tuple(std::move(mockDevice), std::move(mockDeviceFactory), std::move(device));
}

constexpr auto makeError = [](nn::ErrorStatus status) {
    return [status](const auto&... /*args*/) { return nn::error(status); };
};
const auto kReturnGeneralFailure = makeError(nn::ErrorStatus::GENERAL_FAILURE);
const auto kReturnDeadObject = makeError(nn::ErrorStatus::DEAD_OBJECT);

}  // namespace

TEST(ResilientDeviceTest, invalidDeviceFactory) {
    // setup call
    const auto invalidDeviceFactory = ResilientDevice::Factory{};

    // run test
    const auto result = ResilientDevice::create(invalidDeviceFactory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::INVALID_ARGUMENT);
}

TEST(ResilientDeviceTest, preparedModelFactoryFailure) {
    // setup call
    const auto invalidDeviceFactory = kReturnGeneralFailure;

    // run test
    const auto result = ResilientDevice::create(invalidDeviceFactory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientDeviceTest, cachedData) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();

    // run test and verify results
    EXPECT_EQ(device->getName(), kName);
    EXPECT_EQ(device->getVersionString(), kVersionString);
    EXPECT_EQ(device->getSupportedExtensions(), kExtensions);
    EXPECT_EQ(device->getCapabilities(), kCapabilities);
}

TEST(ResilientDeviceTest, getFeatureLevel) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    constexpr auto kFeatureLevel = nn::Version::ANDROID_OC_MR1;
    EXPECT_CALL(*mockDevice, getFeatureLevel()).Times(1).WillOnce(Return(kFeatureLevel));

    // run test
    const auto featureLevel = device->getFeatureLevel();

    // verify results
    EXPECT_EQ(featureLevel, kFeatureLevel);
}

TEST(ResilientDeviceTest, getType) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    constexpr auto kDeviceType = nn::DeviceType::ACCELERATOR;
    EXPECT_CALL(*mockDevice, getType()).Times(1).WillOnce(Return(kDeviceType));

    // run test
    const auto type = device->getType();

    // verify results
    EXPECT_EQ(type, kDeviceType);
}

TEST(ResilientDeviceTest, getNumberOfCacheFilesNeeded) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, getNumberOfCacheFilesNeeded())
            .Times(1)
            .WillOnce(Return(kNumberOfCacheFilesNeeded));

    // run test
    const auto numberOfCacheFilesNeeded = device->getNumberOfCacheFilesNeeded();

    // verify results
    EXPECT_EQ(numberOfCacheFilesNeeded, kNumberOfCacheFilesNeeded);
}

TEST(ResilientDeviceTest, getDevice) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();

    // run test
    const auto result = device->getDevice();

    // verify result
    EXPECT_TRUE(result == mockDevice);
}

TEST(ResilientDeviceTest, wait) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, wait()).Times(1).WillOnce(Return(nn::GeneralResult<void>{}));

    // run test
    const auto result = device->wait();

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, waitError) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, wait()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->wait();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientDeviceTest, waitDeadObjectFailedRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, wait()).Times(1).WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockDeviceFactory, Call(true)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->wait();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientDeviceTest, waitDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, wait()).Times(1).WillOnce(kReturnDeadObject);
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*recoveredMockDevice, wait()).Times(1).WillOnce(Return(nn::GeneralResult<void>{}));
    EXPECT_CALL(*mockDeviceFactory, Call(true)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->wait();

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, getSupportedOperations) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, getSupportedOperations(_))
            .Times(1)
            .WillOnce(Return(nn::GeneralResult<std::vector<bool>>{}));

    // run test
    const auto result = device->getSupportedOperations({});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, getSupportedOperationsError) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, getSupportedOperations(_)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->getSupportedOperations({});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientDeviceTest, getSupportedOperationsDeadObjectFailedRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, getSupportedOperations(_)).Times(1).WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->getSupportedOperations({});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientDeviceTest, getSupportedOperationsDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, getSupportedOperations(_)).Times(1).WillOnce(kReturnDeadObject);
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*recoveredMockDevice, getSupportedOperations(_))
            .Times(1)
            .WillOnce(Return(nn::GeneralResult<std::vector<bool>>{}));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->getSupportedOperations({});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, prepareModel) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto mockPreparedModel = std::make_shared<const nn::MockPreparedModel>();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(mockPreparedModel));

    // run test
    const auto result = device->prepareModel({}, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, prepareModelError) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->prepareModel({}, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientDeviceTest, prepareModelDeadObjectFailedRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->prepareModel({}, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientDeviceTest, prepareModelDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, prepareModel(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(kReturnDeadObject);
    const auto recoveredMockDevice = createConfiguredMockDevice();
    const auto mockPreparedModel = std::make_shared<const nn::MockPreparedModel>();
    EXPECT_CALL(*recoveredMockDevice, prepareModel(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(mockPreparedModel));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->prepareModel({}, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, prepareModelFromCache) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto mockPreparedModel = std::make_shared<const nn::MockPreparedModel>();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _))
            .Times(1)
            .WillOnce(Return(mockPreparedModel));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, prepareModelFromCacheError) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _))
            .Times(1)
            .WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientDeviceTest, prepareModelFromCacheDeadObjectFailedRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _))
            .Times(1)
            .WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientDeviceTest, prepareModelFromCacheDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, prepareModelFromCache(_, _, _, _))
            .Times(1)
            .WillOnce(kReturnDeadObject);
    const auto recoveredMockDevice = createConfiguredMockDevice();
    const auto mockPreparedModel = std::make_shared<const nn::MockPreparedModel>();
    EXPECT_CALL(*recoveredMockDevice, prepareModelFromCache(_, _, _, _))
            .Times(1)
            .WillOnce(Return(mockPreparedModel));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, allocate) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto mockBuffer = std::make_shared<const nn::MockBuffer>();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _)).Times(1).WillOnce(Return(mockBuffer));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, allocateError) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientDeviceTest, allocateDeadObjectFailedRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _)).Times(1).WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientDeviceTest, allocateDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    EXPECT_CALL(*mockDevice, allocate(_, _, _, _)).Times(1).WillOnce(kReturnDeadObject);
    const auto recoveredMockDevice = createConfiguredMockDevice();
    const auto mockBuffer = std::make_shared<const nn::MockBuffer>();
    EXPECT_CALL(*recoveredMockDevice, allocate(_, _, _, _)).Times(1).WillOnce(Return(mockBuffer));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientDeviceTest, recover) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() == recoveredMockDevice);
}

TEST(ResilientDeviceTest, recoverFailure) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*mockDeviceFactory, Call(_)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    EXPECT_FALSE(result.has_value());
}

TEST(ResilientDeviceTest, someoneElseRecovered) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));
    device->recover(mockDevice.get(), /*blocking=*/false);

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() == recoveredMockDevice);
}

TEST(ResilientDeviceTest, recoverCacheMismatchGetName) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    const std::string kDifferentName = "Google-DifferentName";
    const auto ret = [&kDifferentName]() -> const std::string& { return kDifferentName; };
    EXPECT_CALL(*recoveredMockDevice, getName()).Times(1).WillOnce(ret);
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
    EXPECT_TRUE(result.value() != mockDevice);
    EXPECT_TRUE(result.value() != recoveredMockDevice);
}

TEST(ResilientDeviceTest, recoverCacheMismatchGetVersionString) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    const std::string kDifferentVersionString = "differentversion";
    const auto ret = [&kDifferentVersionString]() -> const std::string& {
        return kDifferentVersionString;
    };
    EXPECT_CALL(*recoveredMockDevice, getVersionString()).Times(1).WillOnce(ret);
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
    EXPECT_TRUE(result.value() != mockDevice);
    EXPECT_TRUE(result.value() != recoveredMockDevice);
}

TEST(ResilientDeviceTest, recoverCacheMismatchGetFeatureLevel) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*recoveredMockDevice, getFeatureLevel())
            .Times(1)
            .WillOnce(Return(nn::Version::ANDROID_P));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
    EXPECT_TRUE(result.value() != mockDevice);
    EXPECT_TRUE(result.value() != recoveredMockDevice);
}

TEST(ResilientDeviceTest, recoverCacheMismatchGetType) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*recoveredMockDevice, getType()).Times(1).WillOnce(Return(nn::DeviceType::GPU));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
    EXPECT_TRUE(result.value() != mockDevice);
    EXPECT_TRUE(result.value() != recoveredMockDevice);
}

TEST(ResilientDeviceTest, recoverCacheMismatchGetSupportedExtensions) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    const auto kDifferentExtensions =
            std::vector<nn::Extension>{nn::Extension{.name = "", .operandTypes = {}}};
    const auto ret = [&kDifferentExtensions]() -> const std::vector<nn::Extension>& {
        return kDifferentExtensions;
    };
    EXPECT_CALL(*recoveredMockDevice, getSupportedExtensions()).Times(1).WillOnce(ret);
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
    EXPECT_TRUE(result.value() != mockDevice);
    EXPECT_TRUE(result.value() != recoveredMockDevice);
}

TEST(ResilientDeviceTest, recoverCacheMismatchGetCapabilities) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    const auto kDifferentCapabilities = nn::Capabilities{
            .relaxedFloat32toFloat16PerformanceTensor = {.execTime = 0.5f, .powerUsage = 0.5f},
            .operandPerformance = nn::Capabilities::OperandPerformanceTable::create({}).value()};
    const auto ret = [&kDifferentCapabilities]() -> const nn::Capabilities& {
        return kDifferentCapabilities;
    };
    EXPECT_CALL(*recoveredMockDevice, getCapabilities()).Times(1).WillOnce(ret);
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));

    // run test
    const auto result = device->recover(mockDevice.get(), /*blocking=*/false);

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
    EXPECT_TRUE(result.value() != mockDevice);
    EXPECT_TRUE(result.value() != recoveredMockDevice);
}

TEST(ResilientDeviceTest, recoverCacheMismatchInvalidPrepareModel) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*recoveredMockDevice, getType()).Times(1).WillOnce(Return(nn::DeviceType::GPU));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));
    device->recover(mockDevice.get(), /*blocking=*/false);

    // run test
    auto result = device->prepareModel({}, {}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
}

TEST(ResilientDeviceTest, recoverCacheMismatchInvalidPrepareModelFromCache) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*recoveredMockDevice, getType()).Times(1).WillOnce(Return(nn::DeviceType::GPU));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));
    device->recover(mockDevice.get(), /*blocking=*/false);

    // run test
    auto result = device->prepareModelFromCache({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
}

TEST(ResilientDeviceTest, recoverCacheMismatchInvalidAllocate) {
    // setup call
    const auto [mockDevice, mockDeviceFactory, device] = setup();
    const auto recoveredMockDevice = createConfiguredMockDevice();
    EXPECT_CALL(*recoveredMockDevice, getType()).Times(1).WillOnce(Return(nn::DeviceType::GPU));
    EXPECT_CALL(*mockDeviceFactory, Call(false)).Times(1).WillOnce(Return(recoveredMockDevice));
    device->recover(mockDevice.get(), /*blocking=*/false);

    // run test
    auto result = device->allocate({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() != nullptr);
}

}  // namespace android::hardware::neuralnetworks::utils
