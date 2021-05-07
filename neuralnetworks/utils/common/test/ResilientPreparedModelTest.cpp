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
#include <nnapi/hal/ResilientPreparedModel.h>
#include <utility>
#include "MockPreparedModel.h"

namespace android::hardware::neuralnetworks::utils {
namespace {

using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

using SharedMockPreparedModel = std::shared_ptr<const nn::MockPreparedModel>;
using MockPreparedModelFactory =
        ::testing::MockFunction<nn::GeneralResult<nn::SharedPreparedModel>()>;

SharedMockPreparedModel createConfiguredMockPreparedModel() {
    return std::make_shared<const nn::MockPreparedModel>();
}

std::tuple<std::shared_ptr<const nn::MockPreparedModel>, std::unique_ptr<MockPreparedModelFactory>,
           std::shared_ptr<const ResilientPreparedModel>>
setup() {
    auto mockPreparedModel = std::make_shared<const nn::MockPreparedModel>();

    auto mockPreparedModelFactory = std::make_unique<MockPreparedModelFactory>();
    EXPECT_CALL(*mockPreparedModelFactory, Call()).Times(1).WillOnce(Return(mockPreparedModel));

    auto buffer = ResilientPreparedModel::create(mockPreparedModelFactory->AsStdFunction()).value();
    return std::make_tuple(std::move(mockPreparedModel), std::move(mockPreparedModelFactory),
                           std::move(buffer));
}

constexpr auto makeError = [](nn::ErrorStatus status) {
    return [status](const auto&... /*args*/) { return nn::error(status); };
};
const auto kReturnGeneralFailure = makeError(nn::ErrorStatus::GENERAL_FAILURE);
const auto kReturnDeadObject = makeError(nn::ErrorStatus::DEAD_OBJECT);

const auto kNoCreateReusableExecutionError = nn::GeneralResult<nn::SharedExecution>{};
const auto kNoExecutionError =
        nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>{};
const auto kNoFencedExecutionError =
        nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>(
                std::make_pair(nn::SyncFence::createAsSignaled(), nullptr));

struct FakeResource {};

}  // namespace

TEST(ResilientPreparedModelTest, invalidPreparedModelFactory) {
    // setup call
    const auto invalidPreparedModelFactory = ResilientPreparedModel::Factory{};

    // run test
    const auto result = ResilientPreparedModel::create(invalidPreparedModelFactory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::INVALID_ARGUMENT);
}

TEST(ResilientPreparedModelTest, preparedModelFactoryFailure) {
    // setup call
    const auto invalidPreparedModelFactory = kReturnGeneralFailure;

    // run test
    const auto result = ResilientPreparedModel::create(invalidPreparedModelFactory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientPreparedModelTest, getPreparedModel) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();

    // run test
    const auto result = preparedModel->getPreparedModel();

    // verify result
    EXPECT_TRUE(result == mockPreparedModel);
}

TEST(ResilientPreparedModelTest, execute) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, execute(_, _, _, _))
            .Times(1)
            .WillOnce(Return(kNoExecutionError));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientPreparedModelTest, executeError) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, execute(_, _, _, _)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientPreparedModelTest, executeDeadObjectFailedRecovery) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, execute(_, _, _, _)).Times(1).WillOnce(kReturnDeadObject);
    constexpr auto ret = [] { return nn::error(nn::ErrorStatus::GENERAL_FAILURE); };
    EXPECT_CALL(*mockPreparedModelFactory, Call()).Times(1).WillOnce(ret);

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientPreparedModelTest, executeDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, execute(_, _, _, _)).Times(1).WillOnce(kReturnDeadObject);
    const auto recoveredMockPreparedModel = createConfiguredMockPreparedModel();
    EXPECT_CALL(*recoveredMockPreparedModel, execute(_, _, _, _))
            .Times(1)
            .WillOnce(Return(kNoExecutionError));
    EXPECT_CALL(*mockPreparedModelFactory, Call())
            .Times(1)
            .WillOnce(Return(recoveredMockPreparedModel));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientPreparedModelTest, executeFenced) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(kNoFencedExecutionError));

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientPreparedModelTest, executeFencedError) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientPreparedModelTest, executeFencedDeadObjectFailedRecovery) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockPreparedModelFactory, Call()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientPreparedModelTest, executeFencedDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(kReturnDeadObject);
    const auto recoveredMockPreparedModel = createConfiguredMockPreparedModel();
    EXPECT_CALL(*recoveredMockPreparedModel, executeFenced(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(kNoFencedExecutionError));
    EXPECT_CALL(*mockPreparedModelFactory, Call())
            .Times(1)
            .WillOnce(Return(recoveredMockPreparedModel));

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientPreparedModelTest, createReusableExecution) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, createReusableExecution(_, _, _))
            .Times(1)
            .WillOnce(Return(kNoCreateReusableExecutionError));

    // run test
    const auto result = preparedModel->createReusableExecution({}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientPreparedModelTest, createReusableExecutionError) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, createReusableExecution(_, _, _))
            .Times(1)
            .WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = preparedModel->createReusableExecution({}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientPreparedModelTest, getUnderlyingResource) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    EXPECT_CALL(*mockPreparedModel, getUnderlyingResource())
            .Times(1)
            .WillOnce(Return(FakeResource{}));

    // run test
    const auto resource = preparedModel->getUnderlyingResource();

    // verify resource
    const FakeResource* maybeFakeResource = std::any_cast<FakeResource>(&resource);
    EXPECT_NE(maybeFakeResource, nullptr);
}

TEST(ResilientPreparedModelTest, recover) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    const auto recoveredMockPreparedModel = createConfiguredMockPreparedModel();
    EXPECT_CALL(*mockPreparedModelFactory, Call())
            .Times(1)
            .WillOnce(Return(recoveredMockPreparedModel));

    // run test
    const auto result = preparedModel->recover(mockPreparedModel.get());

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() == recoveredMockPreparedModel);
}

TEST(ResilientPreparedModelTest, recoverFailure) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    const auto recoveredMockPreparedModel = createConfiguredMockPreparedModel();
    EXPECT_CALL(*mockPreparedModelFactory, Call()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = preparedModel->recover(mockPreparedModel.get());

    // verify result
    EXPECT_FALSE(result.has_value());
}

TEST(ResilientPreparedModelTest, someoneElseRecovered) {
    // setup call
    const auto [mockPreparedModel, mockPreparedModelFactory, preparedModel] = setup();
    const auto recoveredMockPreparedModel = createConfiguredMockPreparedModel();
    EXPECT_CALL(*mockPreparedModelFactory, Call())
            .Times(1)
            .WillOnce(Return(recoveredMockPreparedModel));
    preparedModel->recover(mockPreparedModel.get());

    // run test
    const auto result = preparedModel->recover(mockPreparedModel.get());

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() == recoveredMockPreparedModel);
}

}  // namespace android::hardware::neuralnetworks::utils
