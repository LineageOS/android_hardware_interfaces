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

#include <android-base/chrono_utils.h>
#include <android/binder_enums.h>
#include <android/binder_interface_utils.h>
#include <android/binder_status.h>
#include <nnapi/hal/aidl/Conversions.h>

#include "Callbacks.h"
#include "GeneratedTestHarness.h"
#include "Utils.h"

namespace aidl::android::hardware::neuralnetworks::vts::functional {

using implementation::PreparedModelCallback;
using test_helper::TestBuffer;
using test_helper::TestModel;

enum class DeadlineBoundType { NOW, UNLIMITED, SHORT };
constexpr std::array<DeadlineBoundType, 3> deadlineBounds = {
        DeadlineBoundType::NOW, DeadlineBoundType::UNLIMITED, DeadlineBoundType::SHORT};
std::string toString(DeadlineBoundType type) {
    switch (type) {
        case DeadlineBoundType::NOW:
            return "NOW";
        case DeadlineBoundType::UNLIMITED:
            return "UNLIMITED";
        case DeadlineBoundType::SHORT:
            return "SHORT";
    }
    LOG(FATAL) << "Unrecognized DeadlineBoundType: " << static_cast<int>(type);
    return {};
}

constexpr auto kShortDuration = std::chrono::milliseconds{5};

using Results = std::tuple<ErrorStatus, std::vector<OutputShape>, Timing>;
using MaybeResults = std::optional<Results>;

using ExecutionFunction =
        std::function<MaybeResults(const std::shared_ptr<IPreparedModel>& preparedModel,
                                   const Request& request, int64_t deadlineNs)>;

static int64_t makeDeadline(DeadlineBoundType deadlineBoundType) {
    const auto getNanosecondsSinceEpoch = [](const auto& time) -> int64_t {
        const auto timeSinceEpoch = time.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(timeSinceEpoch).count();
    };

    ::android::base::boot_clock::time_point timePoint;
    switch (deadlineBoundType) {
        case DeadlineBoundType::NOW:
            timePoint = ::android::base::boot_clock::now();
            break;
        case DeadlineBoundType::UNLIMITED:
            timePoint = ::android::base::boot_clock::time_point::max();
            break;
        case DeadlineBoundType::SHORT:
            timePoint = ::android::base::boot_clock::now() + kShortDuration;
            break;
    }

    return getNanosecondsSinceEpoch(timePoint);
}

void runPrepareModelTest(const std::shared_ptr<IDevice>& device, const Model& model,
                         Priority priority, std::optional<DeadlineBoundType> deadlineBound) {
    int64_t deadlineNs = kNoDeadline;
    if (deadlineBound.has_value()) {
        deadlineNs = makeDeadline(deadlineBound.value());
    }

    // see if service can handle model
    std::vector<bool> supportedOps;
    const auto supportedCallStatus = device->getSupportedOperations(model, &supportedOps);
    ASSERT_TRUE(supportedCallStatus.isOk());
    ASSERT_NE(0ul, supportedOps.size());
    const bool fullySupportsModel =
            std::all_of(supportedOps.begin(), supportedOps.end(), [](bool valid) { return valid; });

    // launch prepare model
    const std::shared_ptr<PreparedModelCallback> preparedModelCallback =
            ndk::SharedRefBase::make<PreparedModelCallback>();
    const auto prepareLaunchStatus =
            device->prepareModel(model, ExecutionPreference::FAST_SINGLE_ANSWER, priority,
                                 deadlineNs, {}, {}, kEmptyCacheToken, preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk())
            << "prepareLaunchStatus: " << prepareLaunchStatus.getDescription();

    // retrieve prepared model
    preparedModelCallback->wait();
    const ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    const std::shared_ptr<IPreparedModel> preparedModel = preparedModelCallback->getPreparedModel();

    // The getSupportedOperations call returns a list of operations that are guaranteed not to fail
    // if prepareModel is called, and 'fullySupportsModel' is true i.f.f. the entire model is
    // guaranteed. If a driver has any doubt that it can prepare an operation, it must return false.
    // So here, if a driver isn't sure if it can support an operation, but reports that it
    // successfully prepared the model, the test can continue.
    if (!fullySupportsModel && prepareReturnStatus != ErrorStatus::NONE) {
        ASSERT_EQ(nullptr, preparedModel.get());
        return;
    }

    // verify return status
    if (!deadlineBound.has_value()) {
        EXPECT_EQ(ErrorStatus::NONE, prepareReturnStatus);
    } else {
        switch (deadlineBound.value()) {
            case DeadlineBoundType::NOW:
            case DeadlineBoundType::SHORT:
                // Either the driver successfully completed the task or it
                // aborted and returned MISSED_DEADLINE_*.
                EXPECT_TRUE(prepareReturnStatus == ErrorStatus::NONE ||
                            prepareReturnStatus == ErrorStatus::MISSED_DEADLINE_TRANSIENT ||
                            prepareReturnStatus == ErrorStatus::MISSED_DEADLINE_PERSISTENT);
                break;
            case DeadlineBoundType::UNLIMITED:
                // If an unlimited deadline is supplied, we expect the execution to
                // proceed normally. In this case, check it normally by breaking out
                // of the switch statement.
                EXPECT_EQ(ErrorStatus::NONE, prepareReturnStatus);
                break;
        }
    }
    ASSERT_EQ(prepareReturnStatus == ErrorStatus::NONE, preparedModel.get() != nullptr);
}

void runPrepareModelTests(const std::shared_ptr<IDevice>& device, const Model& model) {
    // test priority
    for (auto priority : ndk::enum_range<Priority>{}) {
        SCOPED_TRACE("priority: " + toString(priority));
        if (priority == kDefaultPriority) continue;
        runPrepareModelTest(device, model, priority, {});
    }

    // test deadline
    for (auto deadlineBound : deadlineBounds) {
        SCOPED_TRACE("deadlineBound: " + toString(deadlineBound));
        runPrepareModelTest(device, model, kDefaultPriority, deadlineBound);
    }
}

static MaybeResults executeSynchronously(const std::shared_ptr<IPreparedModel>& preparedModel,
                                         const Request& request, int64_t deadlineNs) {
    SCOPED_TRACE("synchronous");
    const bool measure = false;

    // run execution
    ExecutionResult executionResult;
    const auto ret = preparedModel->executeSynchronously(request, measure, deadlineNs,
                                                         kOmittedTimeoutDuration, &executionResult);
    EXPECT_TRUE(ret.isOk() || ret.getExceptionCode() == EX_SERVICE_SPECIFIC)
            << ret.getDescription();
    if (!ret.isOk()) {
        if (ret.getExceptionCode() != EX_SERVICE_SPECIFIC) {
            return std::nullopt;
        }
        return MaybeResults(
                {static_cast<ErrorStatus>(ret.getServiceSpecificError()), {}, kNoTiming});
    }

    // return results
    return MaybeResults({executionResult.outputSufficientSize
                                 ? ErrorStatus::NONE
                                 : ErrorStatus::OUTPUT_INSUFFICIENT_SIZE,
                         std::move(executionResult.outputShapes), executionResult.timing});
}

static MaybeResults executeBurst(const std::shared_ptr<IPreparedModel>& preparedModel,
                                 const Request& request, int64_t deadlineNs) {
    SCOPED_TRACE("burst");
    const bool measure = false;

    // create burst
    std::shared_ptr<IBurst> burst;
    auto ret = preparedModel->configureExecutionBurst(&burst);
    EXPECT_TRUE(ret.isOk()) << ret.getDescription();
    EXPECT_NE(nullptr, burst.get());
    if (!ret.isOk() || burst.get() == nullptr) {
        return std::nullopt;
    }

    // use -1 for all memory identifier tokens
    const std::vector<int64_t> slots(request.pools.size(), -1);

    // run execution
    ExecutionResult executionResult;
    ret = burst->executeSynchronously(request, slots, measure, deadlineNs, kOmittedTimeoutDuration,
                                      &executionResult);
    EXPECT_TRUE(ret.isOk() || ret.getExceptionCode() == EX_SERVICE_SPECIFIC)
            << ret.getDescription();
    if (!ret.isOk()) {
        if (ret.getExceptionCode() != EX_SERVICE_SPECIFIC) {
            return std::nullopt;
        }
        return MaybeResults(
                {static_cast<ErrorStatus>(ret.getServiceSpecificError()), {}, kNoTiming});
    }

    // return results
    return MaybeResults({executionResult.outputSufficientSize
                                 ? ErrorStatus::NONE
                                 : ErrorStatus::OUTPUT_INSUFFICIENT_SIZE,
                         std::move(executionResult.outputShapes), executionResult.timing});
}

void runExecutionTest(const std::shared_ptr<IPreparedModel>& preparedModel,
                      const TestModel& testModel, const Request& request,
                      const ExecutionContext& context, bool synchronous,
                      DeadlineBoundType deadlineBound) {
    const ExecutionFunction execute = synchronous ? executeSynchronously : executeBurst;
    const auto deadlineNs = makeDeadline(deadlineBound);

    // Perform execution and unpack results.
    const auto results = execute(preparedModel, request, deadlineNs);
    if (!results.has_value()) return;
    const auto& [status, outputShapes, timing] = results.value();

    // Verify no timing information was returned
    EXPECT_EQ(timing, kNoTiming);

    // Validate deadline information if applicable.
    switch (deadlineBound) {
        case DeadlineBoundType::NOW:
        case DeadlineBoundType::SHORT:
            // Either the driver successfully completed the task or it
            // aborted and returned MISSED_DEADLINE_*.
            ASSERT_TRUE(status == ErrorStatus::NONE ||
                        status == ErrorStatus::MISSED_DEADLINE_TRANSIENT ||
                        status == ErrorStatus::MISSED_DEADLINE_PERSISTENT);
            break;
        case DeadlineBoundType::UNLIMITED:
            // If an unlimited deadline is supplied, we expect the execution to
            // proceed normally. In this case, check it normally by breaking out
            // of the switch statement.
            ASSERT_EQ(ErrorStatus::NONE, status);
            break;
    }

    // If the model output operands are fully specified, outputShapes must be either
    // either empty, or have the same number of elements as the number of outputs.
    ASSERT_TRUE(outputShapes.size() == 0 ||
                outputShapes.size() == testModel.main.outputIndexes.size());

    // Go through all outputs, check returned output shapes.
    for (uint32_t i = 0; i < outputShapes.size(); i++) {
        EXPECT_TRUE(outputShapes[i].isSufficient);
        const auto expect =
                utils::toSigned(testModel.main.operands[testModel.main.outputIndexes[i]].dimensions)
                        .value();
        const std::vector<int32_t>& actual = outputShapes[i].dimensions;
        EXPECT_EQ(expect, actual);
    }

    // Retrieve execution results.
    const std::vector<TestBuffer> outputs = context.getOutputBuffers(request);

    // We want "close-enough" results.
    if (status == ErrorStatus::NONE) {
        checkResults(testModel, outputs);
    }
}

void runExecutionTests(const std::shared_ptr<IPreparedModel>& preparedModel,
                       const TestModel& testModel, const Request& request,
                       const ExecutionContext& context) {
    for (bool synchronous : {false, true}) {
        for (auto deadlineBound : deadlineBounds) {
            runExecutionTest(preparedModel, testModel, request, context, synchronous,
                             deadlineBound);
        }
    }
}

void runTests(const std::shared_ptr<IDevice>& device, const TestModel& testModel) {
    // setup
    const Model model = createModel(testModel);

    // run prepare model tests
    runPrepareModelTests(device, model);

    // prepare model
    std::shared_ptr<IPreparedModel> preparedModel;
    createPreparedModel(device, model, &preparedModel);
    if (preparedModel == nullptr) return;

    // run execution tests
    ExecutionContext context;
    const Request request = context.createRequest(testModel);
    runExecutionTests(preparedModel, testModel, request, context);
}

class DeadlineTest : public GeneratedTestBase {};

TEST_P(DeadlineTest, Test) {
    runTests(kDevice, kTestModel);
}

INSTANTIATE_GENERATED_TEST(DeadlineTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

}  // namespace aidl::android::hardware::neuralnetworks::vts::functional
