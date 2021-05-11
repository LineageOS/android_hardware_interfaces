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

#include "MockPreparedModel.h"

#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IExecution.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/PreparedModel.h>

#include <functional>
#include <memory>

namespace android::hardware::neuralnetworks::V1_0::utils {
namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;

const sp<V1_0::IPreparedModel> kInvalidPreparedModel;

sp<MockPreparedModel> createMockPreparedModel() {
    return MockPreparedModel::create();
}

auto makeExecute(V1_0::ErrorStatus launchStatus, V1_0::ErrorStatus returnStatus) {
    return [launchStatus, returnStatus](
                   const V1_0::Request& /*request*/,
                   const sp<V1_0::IExecutionCallback>& cb) -> Return<V1_0::ErrorStatus> {
        cb->notify(returnStatus);
        return launchStatus;
    };
}

std::function<hardware::Status()> makeTransportFailure(status_t status) {
    return [status] { return hardware::Status::fromStatusT(status); };
}

const auto makeGeneralTransportFailure = makeTransportFailure(NO_MEMORY);
const auto makeDeadObjectFailure = makeTransportFailure(DEAD_OBJECT);

}  // namespace

TEST(PreparedModelTest, invalidPreparedModel) {
    // run test
    const auto result = PreparedModel::create(kInvalidPreparedModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, linkToDeathError) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    const auto ret = []() -> Return<bool> { return false; };
    EXPECT_CALL(*mockPreparedModel, linkToDeathRet()).Times(1).WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = PreparedModel::create(mockPreparedModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, linkToDeathTransportFailure) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    EXPECT_CALL(*mockPreparedModel, linkToDeathRet())
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = PreparedModel::create(mockPreparedModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, linkToDeathDeadObject) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    EXPECT_CALL(*mockPreparedModel, linkToDeathRet())
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = PreparedModel::create(mockPreparedModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, execute) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(Invoke(makeExecute(V1_0::ErrorStatus::NONE, V1_0::ErrorStatus::NONE)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    EXPECT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(PreparedModelTest, executeLaunchError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(Invoke(makeExecute(V1_0::ErrorStatus::GENERAL_FAILURE,
                                         V1_0::ErrorStatus::GENERAL_FAILURE)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeReturnError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(Invoke(
                    makeExecute(V1_0::ErrorStatus::NONE, V1_0::ErrorStatus::GENERAL_FAILURE)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeTransportFailure) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeDeadObject) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, executeCrash) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    const auto ret = [&mockPreparedModel]() -> hardware::Return<V1_0::ErrorStatus> {
        mockPreparedModel->simulateCrash();
        return V1_0::ErrorStatus::NONE;
    };
    EXPECT_CALL(*mockPreparedModel, execute(_, _)).Times(1).WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, executeFencedNotSupported) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, reusableExecute) {
    // setup call
    const uint32_t kNumberOfComputations = 2;
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(kNumberOfComputations)
            .WillRepeatedly(Invoke(makeExecute(V1_0::ErrorStatus::NONE, V1_0::ErrorStatus::NONE)));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute repeatedly
    for (uint32_t i = 0; i < kNumberOfComputations; i++) {
        const auto computeResult = createResult.value()->compute({});
        EXPECT_TRUE(computeResult.has_value()) << "Failed with " << computeResult.error().code
                                               << ": " << computeResult.error().message;
    }
}

TEST(PreparedModelTest, reusableExecuteLaunchError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(Invoke(makeExecute(V1_0::ErrorStatus::GENERAL_FAILURE,
                                         V1_0::ErrorStatus::GENERAL_FAILURE)));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute
    const auto computeResult = createResult.value()->compute({});
    ASSERT_FALSE(computeResult.has_value());
    EXPECT_EQ(computeResult.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, reusableExecuteReturnError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(Invoke(
                    makeExecute(V1_0::ErrorStatus::NONE, V1_0::ErrorStatus::GENERAL_FAILURE)));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute
    const auto computeResult = createResult.value()->compute({});
    ASSERT_FALSE(computeResult.has_value());
    EXPECT_EQ(computeResult.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, reusableExecuteTransportFailure) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute
    const auto computeResult = createResult.value()->compute({});
    ASSERT_FALSE(computeResult.has_value());
    EXPECT_EQ(computeResult.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, reusableExecuteDeadObject) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, execute(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute
    const auto computeResult = createResult.value()->compute({});
    ASSERT_FALSE(computeResult.has_value());
    EXPECT_EQ(computeResult.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, reusableExecuteCrash) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    const auto ret = [&mockPreparedModel]() -> hardware::Return<V1_0::ErrorStatus> {
        mockPreparedModel->simulateCrash();
        return V1_0::ErrorStatus::NONE;
    };
    EXPECT_CALL(*mockPreparedModel, execute(_, _)).Times(1).WillOnce(InvokeWithoutArgs(ret));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute
    const auto computeResult = createResult.value()->compute({});
    ASSERT_FALSE(computeResult.has_value());
    EXPECT_EQ(computeResult.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, reusableExecuteFencedNotSupported) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute
    const auto computeResult = createResult.value()->computeFenced({}, {}, {});
    ASSERT_FALSE(computeResult.has_value());
    EXPECT_EQ(computeResult.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, configureExecutionBurst) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

    // run test
    const auto result = preparedModel->configureExecutionBurst();

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST(PreparedModelTest, getUnderlyingResource) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

    // run test
    const auto resource = preparedModel->getUnderlyingResource();

    // verify resource
    const sp<V1_0::IPreparedModel>* maybeMock = std::any_cast<sp<V1_0::IPreparedModel>>(&resource);
    ASSERT_NE(maybeMock, nullptr);
    EXPECT_EQ(maybeMock->get(), mockPreparedModel.get());
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
