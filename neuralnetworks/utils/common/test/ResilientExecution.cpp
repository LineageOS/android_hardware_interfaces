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

#include <gmock/gmock.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/ResilientExecution.h>
#include <utility>
#include "MockExecution.h"

namespace android::hardware::neuralnetworks::utils {
namespace {

using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

using SharedMockExecution = std::shared_ptr<const nn::MockExecution>;
using MockExecutionFactory = ::testing::MockFunction<nn::GeneralResult<nn::SharedExecution>()>;

SharedMockExecution createMockExecution() {
    return std::make_shared<const nn::MockExecution>();
}

std::tuple<SharedMockExecution, std::unique_ptr<MockExecutionFactory>,
           std::shared_ptr<const ResilientExecution>>
setup() {
    auto mockExecution = std::make_shared<const nn::MockExecution>();

    auto mockExecutionFactory = std::make_unique<MockExecutionFactory>();
    EXPECT_CALL(*mockExecutionFactory, Call()).Times(1).WillOnce(Return(mockExecution));

    auto buffer = ResilientExecution::create(mockExecutionFactory->AsStdFunction()).value();
    return std::make_tuple(std::move(mockExecution), std::move(mockExecutionFactory),
                           std::move(buffer));
}

constexpr auto makeError = [](nn::ErrorStatus status) {
    return [status](const auto&... /*args*/) { return nn::error(status); };
};
const auto kReturnGeneralFailure = makeError(nn::ErrorStatus::GENERAL_FAILURE);
const auto kReturnDeadObject = makeError(nn::ErrorStatus::DEAD_OBJECT);

const auto kNoExecutionError =
        nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>{};
const auto kNoFencedExecutionError =
        nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>(
                std::make_pair(nn::SyncFence::createAsSignaled(), nullptr));

}  // namespace

TEST(ResilientExecutionTest, invalidExecutionFactory) {
    // setup call
    const auto invalidExecutionFactory = ResilientExecution::Factory{};

    // run test
    const auto result = ResilientExecution::create(invalidExecutionFactory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::INVALID_ARGUMENT);
}

TEST(ResilientExecutionTest, executionFactoryFailure) {
    // setup call
    const auto invalidExecutionFactory = kReturnGeneralFailure;

    // run test
    const auto result = ResilientExecution::create(invalidExecutionFactory);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientExecutionTest, getExecution) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();

    // run test
    const auto result = execution->getExecution();

    // verify result
    EXPECT_TRUE(result == mockExecution);
}

TEST(ResilientExecutionTest, compute) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    EXPECT_CALL(*mockExecution, compute(_)).Times(1).WillOnce(Return(kNoExecutionError));

    // run test
    const auto result = execution->compute({});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientExecutionTest, computeError) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    EXPECT_CALL(*mockExecution, compute(_)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = execution->compute({});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientExecutionTest, computeDeadObjectFailedRecovery) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    EXPECT_CALL(*mockExecution, compute(_)).Times(1).WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockExecutionFactory, Call()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = execution->compute({});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientExecutionTest, computeDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    EXPECT_CALL(*mockExecution, compute(_)).Times(1).WillOnce(kReturnDeadObject);
    const auto recoveredMockExecution = createMockExecution();
    EXPECT_CALL(*recoveredMockExecution, compute(_)).Times(1).WillOnce(Return(kNoExecutionError));
    EXPECT_CALL(*mockExecutionFactory, Call()).Times(1).WillOnce(Return(recoveredMockExecution));

    // run test
    const auto result = execution->compute({});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientExecutionTest, computeFenced) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    EXPECT_CALL(*mockExecution, computeFenced(_, _, _))
            .Times(1)
            .WillOnce(Return(kNoFencedExecutionError));

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientExecutionTest, computeFencedError) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    EXPECT_CALL(*mockExecution, computeFenced(_, _, _)).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ResilientExecutionTest, computeFencedDeadObjectFailedRecovery) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    EXPECT_CALL(*mockExecution, computeFenced(_, _, _)).Times(1).WillOnce(kReturnDeadObject);
    EXPECT_CALL(*mockExecutionFactory, Call()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ResilientExecutionTest, computeFencedDeadObjectSuccessfulRecovery) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    EXPECT_CALL(*mockExecution, computeFenced(_, _, _)).Times(1).WillOnce(kReturnDeadObject);
    const auto recoveredMockExecution = createMockExecution();
    EXPECT_CALL(*recoveredMockExecution, computeFenced(_, _, _))
            .Times(1)
            .WillOnce(Return(kNoFencedExecutionError));
    EXPECT_CALL(*mockExecutionFactory, Call()).Times(1).WillOnce(Return(recoveredMockExecution));

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ResilientExecutionTest, recover) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    const auto recoveredMockExecution = createMockExecution();
    EXPECT_CALL(*mockExecutionFactory, Call()).Times(1).WillOnce(Return(recoveredMockExecution));

    // run test
    const auto result = execution->recover(mockExecution.get());

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() == recoveredMockExecution);
}

TEST(ResilientExecutionTest, recoverFailure) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    const auto recoveredMockExecution = createMockExecution();
    EXPECT_CALL(*mockExecutionFactory, Call()).Times(1).WillOnce(kReturnGeneralFailure);

    // run test
    const auto result = execution->recover(mockExecution.get());

    // verify result
    EXPECT_FALSE(result.has_value());
}

TEST(ResilientExecutionTest, someoneElseRecovered) {
    // setup call
    const auto [mockExecution, mockExecutionFactory, execution] = setup();
    const auto recoveredMockExecution = createMockExecution();
    EXPECT_CALL(*mockExecutionFactory, Call()).Times(1).WillOnce(Return(recoveredMockExecution));
    execution->recover(mockExecution.get());

    // run test
    const auto result = execution->recover(mockExecution.get());

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_TRUE(result.value() == recoveredMockExecution);
}

}  // namespace android::hardware::neuralnetworks::utils
