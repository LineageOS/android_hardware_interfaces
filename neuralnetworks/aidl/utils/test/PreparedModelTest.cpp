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

#include "MockBurst.h"
#include "MockFencedExecutionCallback.h"
#include "MockPreparedModel.h"

#include <aidl/android/hardware/neuralnetworks/IFencedExecutionCallback.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IExecution.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/aidl/PreparedModel.h>

#include <functional>
#include <memory>

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::SetArgPointee;

const std::shared_ptr<IPreparedModel> kInvalidPreparedModel;
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
    return [callback](const Request& /*request*/,
                      const std::vector<ndk::ScopedFileDescriptor>& /*waitFor*/,
                      bool /*measureTiming*/, int64_t /*deadline*/, int64_t /*loopTimeoutDuration*/,
                      int64_t /*duration*/, FencedExecutionResult* fencedExecutionResult) {
        *fencedExecutionResult = FencedExecutionResult{.callback = callback,
                                                       .syncFence = ndk::ScopedFileDescriptor(-1)};
        return ndk::ScopedAStatus::ok();
    };
}

}  // namespace

TEST(PreparedModelTest, invalidPreparedModel) {
    // run test
    const auto result = PreparedModel::create(kInvalidPreparedModel);

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeSync) {
    // setup call
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    const auto mockExecutionResult = ExecutionResult{
            .outputSufficientSize = true,
            .outputShapes = {},
            .timing = kNoTiming,
    };
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _, _, _))
            .Times(1)
            .WillOnce(
                    DoAll(SetArgPointee<4>(mockExecutionResult), InvokeWithoutArgs(makeStatusOk)));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    EXPECT_TRUE(result.has_value())
            << "Failed with " << result.error().code << ": " << result.error().message;
}

TEST(PreparedModelTest, executeSyncError) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeGeneralFailure));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeSyncTransportFailure) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _, _, _))
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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));

    // run test
    const auto result = preparedModel->execute({}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, executeFenced) {
    // setup call
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<0>(kNoTiming), SetArgPointee<1>(kNoTiming),
                            SetArgPointee<2>(ErrorStatus::NONE), Invoke(makeStatusOk)));
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeFencedExecutionResult(mockCallback)));

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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_, _, _))
            .Times(1)
            .WillOnce(Invoke(DoAll(SetArgPointee<0>(kNoTiming), SetArgPointee<1>(kNoTiming),
                                   SetArgPointee<2>(ErrorStatus::GENERAL_FAILURE),
                                   Invoke(makeStatusOk))));
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeFencedExecutionResult(mockCallback)));

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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

    // run test
    const auto result = preparedModel->executeFenced({}, {}, {}, {}, {}, {});

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, executeFencedTransportFailure) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    const auto mockExecutionResult = ExecutionResult{
            .outputSufficientSize = true,
            .outputShapes = {},
            .timing = kNoTiming,
    };
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _, _, _))
            .Times(kNumberOfComputations)
            .WillRepeatedly(
                    DoAll(SetArgPointee<4>(mockExecutionResult), InvokeWithoutArgs(makeStatusOk)));

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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeGeneralFailure));

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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _, _, _))
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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, executeSynchronously(_, _, _, _, _))
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

TEST(PreparedModelTest, reusableExecuteFenced) {
    // setup call
    const uint32_t kNumberOfComputations = 2;
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_, _, _))
            .Times(kNumberOfComputations)
            .WillRepeatedly(DoAll(SetArgPointee<0>(kNoTiming), SetArgPointee<1>(kNoTiming),
                                  SetArgPointee<2>(ErrorStatus::NONE), Invoke(makeStatusOk)));
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(kNumberOfComputations)
            .WillRepeatedly(Invoke(makeFencedExecutionResult(mockCallback)));

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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    const auto mockCallback = MockFencedExecutionCallback::create();
    EXPECT_CALL(*mockCallback, getExecutionInfo(_, _, _))
            .Times(1)
            .WillOnce(Invoke(DoAll(SetArgPointee<0>(kNoTiming), SetArgPointee<1>(kNoTiming),
                                   SetArgPointee<2>(ErrorStatus::GENERAL_FAILURE),
                                   Invoke(makeStatusOk))));
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Invoke(makeFencedExecutionResult(mockCallback)));

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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
    EXPECT_CALL(*mockPreparedModel, executeFenced(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));

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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
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
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();
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
    const auto mockBurst = ndk::SharedRefBase::make<MockBurst>();
    EXPECT_CALL(*mockPreparedModel, configureExecutionBurst(_))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<0>(mockBurst), Invoke(makeStatusOk)));
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

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
    EXPECT_CALL(*mockPreparedModel, configureExecutionBurst(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralFailure));
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

    // run test
    const auto result = preparedModel->configureExecutionBurst();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, configureExecutionBurstTransportFailure) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    EXPECT_CALL(*mockPreparedModel, configureExecutionBurst(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeGeneralTransportFailure));
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

    // run test
    const auto result = preparedModel->configureExecutionBurst();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::GENERAL_FAILURE);
}

TEST(PreparedModelTest, configureExecutionBurstDeadObject) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    EXPECT_CALL(*mockPreparedModel, configureExecutionBurst(_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(makeDeadObjectFailure));
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

    // run test
    const auto result = preparedModel->configureExecutionBurst();

    // verify result
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, nn::ErrorStatus::DEAD_OBJECT);
}

TEST(PreparedModelTest, getUnderlyingResource) {
    // setup test
    const auto mockPreparedModel = MockPreparedModel::create();
    const auto preparedModel = PreparedModel::create(mockPreparedModel).value();

    // run test
    const auto resource = preparedModel->getUnderlyingResource();

    // verify resource
    const std::shared_ptr<IPreparedModel>* maybeMock =
            std::any_cast<std::shared_ptr<IPreparedModel>>(&resource);
    ASSERT_NE(maybeMock, nullptr);
    EXPECT_EQ(maybeMock->get(), mockPreparedModel.get());
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
