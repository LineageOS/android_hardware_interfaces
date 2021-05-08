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

#include "MockBurstContext.h"
#include "MockFencedExecutionCallback.h"
#include "MockPreparedModel.h"

#include <android/hardware/neuralnetworks/1.3/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.3/IFencedExecutionCallback.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IExecution.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.3/PreparedModel.h>

#include <functional>
#include <memory>

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;

const sp<V1_3::IPreparedModel> kInvalidPreparedModel;
constexpr auto kNoTiming = V1_2::Timing{.timeOnDevice = std::numeric_limits<uint64_t>::max(),
                                        .timeInDriver = std::numeric_limits<uint64_t>::max()};

sp<MockPreparedModel> createMockPreparedModel() {
    const auto mockPreparedModel = MockPreparedModel::create();

    // Ensure that older calls are not used.
    EXPECT_CALL(*mockPreparedModel, execute(_, _)).Times(0);
    EXPECT_CALL(*mockPreparedModel, execute_1_2(_, _, _)).Times(0);
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _)).Times(0);

    return mockPreparedModel;
}

auto makeExecuteSynchronously(V1_3::ErrorStatus status,
                              const std::vector<V1_2::OutputShape>& outputShapes,
                              const V1_2::Timing& timing) {
    return [status, outputShapes, timing](
                   const V1_3::Request& /*request*/, V1_2::MeasureTiming /*measureTiming*/,
                   const V1_3::OptionalTimePoint& /*deadline*/,
                   const V1_3::OptionalTimeoutDuration& /*loopTimeoutDuration*/,
                   const V1_3::IPreparedModel::executeSynchronously_1_3_cb& cb) {
        cb(status, outputShapes, timing);
        return hardware::Void();
    };
}
auto makeExecuteAsynchronously(V1_3::ErrorStatus launchStatus, V1_3::ErrorStatus returnStatus,
                               const std::vector<V1_2::OutputShape>& outputShapes,
                               const V1_2::Timing& timing) {
    return [launchStatus, returnStatus, outputShapes, timing](
                   const V1_3::Request& /*request*/, V1_2::MeasureTiming /*measureTiming*/,
                   const V1_3::OptionalTimePoint& /*deadline*/,
                   const V1_3::OptionalTimeoutDuration& /*loopTimeoutDuration*/,
                   const sp<V1_3::IExecutionCallback>& cb) -> Return<V1_3::ErrorStatus> {
        cb->notify_1_3(returnStatus, outputShapes, timing);
        return launchStatus;
    };
}
auto makeExecuteFencedReturn(V1_3::ErrorStatus status, const hardware::hidl_handle& syncFence,
                             const sp<V1_3::IFencedExecutionCallback>& dispatchCallback) {
    return [status, syncFence, dispatchCallback](
                   const V1_3::Request& /*request*/,
                   const hardware::hidl_vec<hardware::hidl_handle>& /*waitFor*/,
                   V1_2::MeasureTiming /*measure*/, const V1_3::OptionalTimePoint& /*deadline*/,
                   const V1_3::OptionalTimeoutDuration& /*loopTimeoutDuration*/,
                   const V1_3::OptionalTimeoutDuration& /*duration*/,
                   const V1_3::IPreparedModel::executeFenced_cb& cb) {
        cb(status, syncFence, dispatchCallback);
        return hardware::Void();
    };
}
auto makeExecuteFencedCallbackReturn(V1_3::ErrorStatus status, const V1_2::Timing& timingA,
                                     const V1_2::Timing& timingB) {
    return [status, timingA,
            timingB](const V1_3::IFencedExecutionCallback::getExecutionInfo_cb& cb) {
        cb(status, timingA, timingB);
        return hardware::Void();
    };
}
auto makeConfigureExecutionBurstReturn(V1_0::ErrorStatus status,
                                       const sp<MockBurstContext>& burstContext) {
    return [status, burstContext](
                   const sp<V1_2::IBurstCallback>& /*callback*/,
                   const MQDescriptorSync<V1_2::FmqRequestDatum>& /*requestChannel*/,
                   const MQDescriptorSync<V1_2::FmqResultDatum>& /*resultChannel*/,
                   V1_2::IPreparedModel::configureExecutionBurst_cb cb) -> hardware::Return<void> {
        cb(status, burstContext);
        return hardware::Void();
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
    const auto result = PreparedModel::create(kInvalidPreparedModel, /*executeSynchronously=*/true);

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
    const auto result = PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true);

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
    const auto result = PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true);

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
    const auto result = PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, executeSync) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteSynchronously(V1_3::ErrorStatus::NONE, {}, kNoTiming)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    EXPECT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(PreparedModelTest, executeSyncError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(
                    makeExecuteSynchronously(V1_3::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeSyncTransportFailure) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeSyncDeadObject) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, executeAsync) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteAsynchronously(V1_3::ErrorStatus::NONE,
                                                       V1_3::ErrorStatus::NONE, {}, kNoTiming)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    EXPECT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(PreparedModelTest, executeAsyncLaunchError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteAsynchronously(V1_3::ErrorStatus::GENERAL_FAILURE,
                                                       V1_3::ErrorStatus::GENERAL_FAILURE, {},
                                                       kNoTiming)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeAsyncReturnError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteAsynchronously(
                    V1_3::ErrorStatus::NONE, V1_3::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeAsyncTransportFailure) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeAsyncDeadObject) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, executeAsyncCrash) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    const auto ret = [&mockPreparedModel]() -> hardware::Return<V1_3::ErrorStatus> {
        mockPreparedModel->simulateCrash();
        return V1_3::ErrorStatus::NONE;
    };
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(ret));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, executeFenced) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_))
            .Times(1)
            .WillOnce(Invoke(makeExecuteFencedCallbackReturn(V1_3::ErrorStatus::NONE, kNoTiming,
                                                             kNoTiming)));
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteFencedReturn(V1_3::ErrorStatus::NONE, {}, mockCallback)));

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

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

TEST(PreparedModelTest, executeFencedCallbackError) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_))
            .Times(1)
            .WillOnce(Invoke(makeExecuteFencedCallbackReturn(V1_3::ErrorStatus::GENERAL_FAILURE,
                                                             kNoTiming, kNoTiming)));
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteFencedReturn(V1_3::ErrorStatus::NONE, {}, mockCallback)));

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

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

TEST(PreparedModelTest, executeFencedError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(
                    makeExecuteFencedReturn(V1_3::ErrorStatus::GENERAL_FAILURE, {}, nullptr)));

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeFencedTransportFailure) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeFencedDeadObject) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, reusableExecuteSync) {
    // setup call
    const uint32_t kNumberOfComputations = 2;
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously_1_3(_, _, _, _, _))
            .Times(kNumberOfComputations)
            .WillRepeatedly(
                    Invoke(makeExecuteSynchronously(V1_3::ErrorStatus::NONE, {}, kNoTiming)));

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

TEST(PreparedModelTest, reusableExecuteSyncError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(
                    makeExecuteSynchronously(V1_3::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming)));

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

TEST(PreparedModelTest, reusableExecuteSyncTransportFailure) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously_1_3(_, _, _, _, _))
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

TEST(PreparedModelTest, reusableExecuteSyncDeadObject) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously_1_3(_, _, _, _, _))
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

TEST(PreparedModelTest, reusableExecuteAsync) {
    // setup call
    const uint32_t kNumberOfComputations = 2;
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(kNumberOfComputations)
            .WillRepeatedly(Invoke(makeExecuteAsynchronously(
                    V1_3::ErrorStatus::NONE, V1_3::ErrorStatus::NONE, {}, kNoTiming)));

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

TEST(PreparedModelTest, reusableExecuteAsyncLaunchError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteAsynchronously(V1_3::ErrorStatus::GENERAL_FAILURE,
                                                       V1_3::ErrorStatus::GENERAL_FAILURE, {},
                                                       kNoTiming)));

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

TEST(PreparedModelTest, reusableExecuteAsyncReturnError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteAsynchronously(
                    V1_3::ErrorStatus::NONE, V1_3::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, reusableExecuteAsyncTransportFailure) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
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

TEST(PreparedModelTest, reusableExecuteAsyncDeadObject) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
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

TEST(PreparedModelTest, reusableExecuteAsyncCrash) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/false).value();
    const auto ret = [&mockPreparedModel]() -> hardware::Return<V1_3::ErrorStatus> {
        mockPreparedModel->simulateCrash();
        return V1_3::ErrorStatus::NONE;
    };
    EXPECT_CALL(*mockPreparedModel, execute_1_3(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(ret));

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

TEST(PreparedModelTest, reusableExecuteFenced) {
    // setup call
    const uint32_t kNumberOfComputations = 2;
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_))
            .Times(kNumberOfComputations)
            .WillRepeatedly(Invoke(makeExecuteFencedCallbackReturn(V1_3::ErrorStatus::NONE,
                                                                   kNoTiming, kNoTiming)));
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(kNumberOfComputations)
            .WillRepeatedly(
                    Invoke(makeExecuteFencedReturn(V1_3::ErrorStatus::NONE, {}, mockCallback)));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute repeatedly
    for (uint32_t i = 0; i < kNumberOfComputations; i++) {
        const auto computeResult = createResult.value()->computeFenced({}, {}, {});
        ASSERT_TRUE(computeResult.has_value()) << "Failed with " << computeResult.error().code
                                               << ": " << computeResult.error().message;
        const auto& [syncFence, callback] = computeResult.value();
        EXPECT_EQ(syncFence.syncWait({}), nn::SyncFence::FenceState::SIGNALED);
        ASSERT_NE(callback, nullptr);

        // get results from callback
        const auto callbackResult = callback();
        ASSERT_TRUE(callbackResult.has_value()) << "Failed with " << callbackResult.error().code
                                                << ": " << callbackResult.error().message;
    }
}

TEST(PreparedModelTest, reusableExecuteFencedCallbackError) {
    // setup call
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_))
            .Times(1)
            .WillOnce(Invoke(makeExecuteFencedCallbackReturn(V1_3::ErrorStatus::GENERAL_FAILURE,
                                                             kNoTiming, kNoTiming)));
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeExecuteFencedReturn(V1_3::ErrorStatus::NONE, {}, mockCallback)));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute
    const auto computeResult = createResult.value()->computeFenced({}, {}, {});
    ASSERT_TRUE(computeResult.has_value()) << "Failed with " << computeResult.error().code << ": "
                                           << computeResult.error().message;
    const auto& [syncFence, callback] = computeResult.value();
    EXPECT_NE(syncFence.syncWait({}), nn::SyncFence::FenceState::ACTIVE);
    ASSERT_NE(callback, nullptr);

    // verify callback failure
    const auto callbackResult = callback();
    ASSERT_FALSE(callbackResult.has_value());
    EXPECT_EQ(callbackResult.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, reusableExecuteFencedError) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(
                    makeExecuteFencedReturn(V1_3::ErrorStatus::GENERAL_FAILURE, {}, nullptr)));

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

TEST(PreparedModelTest, reusableExecuteFencedTransportFailure) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

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

TEST(PreparedModelTest, reusableExecuteFencedDeadObject) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // create execution
    const auto createResult = preparedModel->createReusableExecution({}, {}, {});
    ASSERT_TRUE(createResult.has_value())
            << "Failed with " << createResult.error().code << ": " << createResult.error().message;
    ASSERT_NE(createResult.value(), nullptr);

    // invoke compute
    const auto computeResult = createResult.value()->computeFenced({}, {}, {});
    ASSERT_FALSE(computeResult.has_value());
    EXPECT_EQ(computeResult.error().code, nn::ErrorStatus::DEAD_OBJECT);
}
TEST(PreparedModelTest, configureExecutionBurst) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto mockBurstContext = sp<MockBurstContext>::make();
    EXPECT_CALL(*mockPreparedModel, configureExecutionBurst(_, _, _, _))
            .Times(1)
            .WillOnce(makeConfigureExecutionBurstReturn(V1_0::ErrorStatus::NONE, mockBurstContext));
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();

    // run test
    const auto result = preparedModel->configureExecutionBurst();

    // verify result
    ASSERT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
    EXPECT_NE(result.value(), nullptr);
}

TEST(PreparedModelTest, configureExecutionBurstError) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, configureExecutionBurst(_, _, _, _))
            .Times(1)
            .WillOnce(
                    makeConfigureExecutionBurstReturn(V1_0::ErrorStatus::GENERAL_FAILURE, nullptr));

    // run test
    const auto result = preparedModel->configureExecutionBurst();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, configureExecutionBurstTransportFailure) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, configureExecutionBurst(_, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));

    // run test
    const auto result = preparedModel->configureExecutionBurst();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, configureExecutionBurstDeadObject) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();
    EXPECT_CALL(*mockPreparedModel, configureExecutionBurst(_, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = preparedModel->configureExecutionBurst();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, getUnderlyingResource) {
    // setup test
    const auto mockPreparedModel = createMockPreparedModel();
    const auto preparedModel =
            PreparedModel::create(mockPreparedModel, /*executeSynchronously=*/true).value();

    // run test
    const auto resource = preparedModel->getUnderlyingResource();

    // verify resource
    const sp<V1_3::IPreparedModel>* maybeMock = std::any_cast<sp<V1_3::IPreparedModel>>(&resource);
    ASSERT_NE(maybeMock, nullptr);
    EXPECT_EQ(maybeMock->get(), mockPreparedModel.get());
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
