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

#include "MockExecution.h"
#include "MockFencedExecutionCallback.h"

#include <aidl/android/hardware/neuralnetworks/IFencedExecutionCallback.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IExecution.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/aidl/Execution.h>

#include <functional>
#include <memory>

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::SetArgPointee;

const std::shared_ptr<IExecution> kInvalidExecution;
constexpr auto kNoTiming = Timing{.timeOnDeviceNs = -1, .timeInDriverNs = -1};

constexpr auto makeStatusOk = [] { return ndk::ScopedAStatus::ok(); };

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

auto makeFencedExecutionResult(const std::shared_ptr<MockFencedExecutionCallback>& callback) {
    return [callback](const std::vector<ndk::ScopedFileDescriptor>& /*waitFor*/,
                      int64_t /*deadline*/, int64_t /*duration*/,
                      FencedExecutionResult* fencedExecutionResult) {
        *fencedExecutionResult = FencedExecutionResult{.callback = callback,
                                                       .syncFence = ndk::ScopedFileDescriptor(-1)};
        return ndk::ScopedAStatus::ok();
    };
}

}  // namespace

TEST(ExecutionTest, invalidExecution) {
    // run test
    const auto result = Execution::create(kInvalidExecution, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ExecutionTest, executeSync) {
    // setup call
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    const auto mockExecutionResult = ExecutionResult{
            .outputSufficientSize = true,
            .outputShapes = {},
            .timing = kNoTiming,
    };
    EXPECT_CALL(*mockExecution, executeSynchronously(_, _))
            .Times(1)
            .WillOnce(
                    DoAll(SetArgPointee<1>(mockExecutionResult), InvokeWithoutArgs(makeStatusOk)));

    // run test
    const auto result = execution->compute({});

    // verify result
    EXPECT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(ExecutionTest, executeSyncError) {
    // setup test
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    EXPECT_CALL(*mockExecution, executeSynchronously(_, _))
            .Times(1)
            .WillOnce(Invoke(makeGeneralFailure));

    // run test
    const auto result = execution->compute({});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ExecutionTest, executeSyncTransportFailure) {
    // setup test
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    EXPECT_CALL(*mockExecution, executeSynchronously(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = execution->compute({});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ExecutionTest, executeSyncDeadObject) {
    // setup test
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    EXPECT_CALL(*mockExecution, executeSynchronously(_, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = execution->compute({});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(ExecutionTest, executeFenced) {
    // setup call
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<0>(kNoTiming), SetArgPointee<1>(kNoTiming),
                            SetArgPointee<2>(ErrorStatus::NONE), Invoke(makeStatusOk)));
    EXPECT_CALL(*mockExecution, executeFenced(_, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeFencedExecutionResult(mockCallback)));

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    const auto& [syncFence, callback] = result.value();
    EXPECT_EQ(syncFence.syncWait({}), nn::SyncFence::FenceState::SIGNALED);
    ASSERT_NE(callback, nullptr);

    // get results from callback
    const auto callbackResult = callback();
    ASSERT_TRUE(callbackResult.has_value()) << "Failed with " << callbackResult.error().code << ": "
                                            << callbackResult.error().message;
}

TEST(ExecutionTest, executeFencedCallbackError) {
    // setup call
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_, _, _))
            .Times(1)
            .WillOnce(Invoke(DoAll(SetArgPointee<0>(kNoTiming), SetArgPointee<1>(kNoTiming),
                                   SetArgPointee<2>(ErrorStatus::GENERAL_FAILURE),
                                   Invoke(makeStatusOk))));
    EXPECT_CALL(*mockExecution, executeFenced(_, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeFencedExecutionResult(mockCallback)));

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    const auto& [syncFence, callback] = result.value();
    EXPECT_NE(syncFence.syncWait({}), nn::SyncFence::FenceState::ACTIVE);
    ASSERT_NE(callback, nullptr);

    // verify callback failure
    const auto callbackResult = callback();
    ASSERT_FALSE(callbackResult.has_value());
    EXPECT_EQ(callbackResult.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ExecutionTest, executeFencedError) {
    // setup test
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    EXPECT_CALL(*mockExecution, executeFenced(_, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ExecutionTest, executeFencedTransportFailure) {
    // setup test
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    EXPECT_CALL(*mockExecution, executeFenced(_, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(ExecutionTest, executeFencedDeadObject) {
    // setup test
    const auto mockExecution = MockExecution::create();
    const auto execution = Execution::create(mockExecution, {}).value();
    EXPECT_CALL(*mockExecution, executeFenced(_, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = execution->computeFenced({}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
