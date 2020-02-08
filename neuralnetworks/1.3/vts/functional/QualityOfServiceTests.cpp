/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "1.0/Utils.h"
#include "1.3/Callbacks.h"
#include "1.3/Utils.h"
#include "GeneratedTestHarness.h"
#include "Utils.h"

namespace android::hardware::neuralnetworks::V1_3::vts::functional {

using implementation::ExecutionCallback;
using implementation::PreparedModelCallback;
using test_helper::TestBuffer;
using test_helper::TestModel;
using V1_1::ExecutionPreference;
using V1_2::MeasureTiming;
using V1_2::OutputShape;
using V1_2::Timing;

using HidlToken =
        hidl_array<uint8_t, static_cast<uint32_t>(V1_2::Constant::BYTE_SIZE_OF_CACHE_TOKEN)>;

enum class DeadlineBoundType { NOW, UNLIMITED };
constexpr std::array<DeadlineBoundType, 2> deadlineBounds = {DeadlineBoundType::NOW,
                                                             DeadlineBoundType::UNLIMITED};
std::string toString(DeadlineBoundType type) {
    switch (type) {
        case DeadlineBoundType::NOW:
            return "NOW";
        case DeadlineBoundType::UNLIMITED:
            return "UNLIMITED";
    }
    LOG(FATAL) << "Unrecognized DeadlineBoundType: " << static_cast<int>(type);
    return {};
}

using Results = std::tuple<ErrorStatus, hidl_vec<OutputShape>, Timing>;
using MaybeResults = std::optional<Results>;

using ExecutionFunction =
        std::function<MaybeResults(const sp<IPreparedModel>& preparedModel, const Request& request,
                                   DeadlineBoundType deadlineBound)>;

static OptionalTimePoint makeOptionalTimePoint(DeadlineBoundType deadlineBoundType) {
    OptionalTimePoint deadline;
    switch (deadlineBoundType) {
        case DeadlineBoundType::NOW: {
            const auto currentTime = std::chrono::steady_clock::now();
            const auto currentTimeInNanoseconds =
                    std::chrono::time_point_cast<std::chrono::nanoseconds>(currentTime);
            const uint64_t nanosecondsSinceEpoch =
                    currentTimeInNanoseconds.time_since_epoch().count();
            deadline.nanosecondsSinceEpoch(nanosecondsSinceEpoch);
        } break;
        case DeadlineBoundType::UNLIMITED: {
            uint64_t unlimited = std::numeric_limits<uint64_t>::max();
            deadline.nanosecondsSinceEpoch(unlimited);
        } break;
    }
    return deadline;
}

void runPrepareModelTest(const sp<IDevice>& device, const Model& model, Priority priority,
                         std::optional<DeadlineBoundType> deadlineBound) {
    OptionalTimePoint deadline;
    if (deadlineBound.has_value()) {
        deadline = makeOptionalTimePoint(deadlineBound.value());
    }

    // see if service can handle model
    bool fullySupportsModel = false;
    const Return<void> supportedCall = device->getSupportedOperations_1_3(
            model, [&fullySupportsModel](ErrorStatus status, const hidl_vec<bool>& supported) {
                ASSERT_EQ(ErrorStatus::NONE, status);
                ASSERT_NE(0ul, supported.size());
                fullySupportsModel = std::all_of(supported.begin(), supported.end(),
                                                 [](bool valid) { return valid; });
            });
    ASSERT_TRUE(supportedCall.isOk());

    // launch prepare model
    const sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
    const Return<ErrorStatus> prepareLaunchStatus = device->prepareModel_1_3(
            model, ExecutionPreference::FAST_SINGLE_ANSWER, priority, deadline,
            hidl_vec<hidl_handle>(), hidl_vec<hidl_handle>(), HidlToken(), preparedModelCallback);
    ASSERT_TRUE(prepareLaunchStatus.isOk());
    ASSERT_EQ(ErrorStatus::NONE, static_cast<ErrorStatus>(prepareLaunchStatus));

    // retrieve prepared model
    preparedModelCallback->wait();
    const ErrorStatus prepareReturnStatus = preparedModelCallback->getStatus();
    const sp<V1_0::IPreparedModel> preparedModelV1_0 = preparedModelCallback->getPreparedModel();
    const sp<IPreparedModel> preparedModel =
            IPreparedModel::castFrom(preparedModelV1_0).withDefault(nullptr);

    // The getSupportedOperations_1_3 call returns a list of operations that are
    // guaranteed not to fail if prepareModel_1_3 is called, and
    // 'fullySupportsModel' is true i.f.f. the entire model is guaranteed.
    // If a driver has any doubt that it can prepare an operation, it must
    // return false. So here, if a driver isn't sure if it can support an
    // operation, but reports that it successfully prepared the model, the test
    // can continue.
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
                // If the execution was launched with a deadline of NOW, the
                // deadline has already passed when the driver would launch the
                // execution. In this case, the driver must return
                // MISSED_DEADLINE_*.
                EXPECT_TRUE(prepareReturnStatus == ErrorStatus::MISSED_DEADLINE_TRANSIENT ||
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

void runPrepareModelTests(const sp<IDevice>& device, const Model& model,
                          bool supportsPrepareModelDeadline) {
    // test priority
    for (auto priority : hidl_enum_range<Priority>{}) {
        SCOPED_TRACE("priority: " + toString(priority));
        if (priority == kDefaultPriority) continue;
        runPrepareModelTest(device, model, priority, {});
    }

    // test deadline
    if (supportsPrepareModelDeadline) {
        for (auto deadlineBound : deadlineBounds) {
            SCOPED_TRACE("deadlineBound: " + toString(deadlineBound));
            runPrepareModelTest(device, model, kDefaultPriority, deadlineBound);
        }
    }
}

static MaybeResults executeAsynchronously(const sp<IPreparedModel>& preparedModel,
                                          const Request& request, DeadlineBoundType deadlineBound) {
    SCOPED_TRACE("asynchronous");
    const MeasureTiming measure = MeasureTiming::NO;
    const OptionalTimePoint deadline = makeOptionalTimePoint(deadlineBound);

    // launch execution
    const sp<ExecutionCallback> callback = new ExecutionCallback();
    Return<ErrorStatus> ret = preparedModel->execute_1_3(request, measure, deadline, callback);
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(ErrorStatus::NONE, ret.withDefault(ErrorStatus::GENERAL_FAILURE));
    if (!ret.isOk() || ret != ErrorStatus::NONE) return std::nullopt;

    // retrieve execution results
    callback->wait();
    const ErrorStatus status = callback->getStatus();
    hidl_vec<OutputShape> outputShapes = callback->getOutputShapes();
    const Timing timing = callback->getTiming();

    // return results
    return Results{status, std::move(outputShapes), timing};
}

static MaybeResults executeSynchronously(const sp<IPreparedModel>& preparedModel,
                                         const Request& request, DeadlineBoundType deadlineBound) {
    SCOPED_TRACE("synchronous");
    const MeasureTiming measure = MeasureTiming::NO;
    const OptionalTimePoint deadline = makeOptionalTimePoint(deadlineBound);

    // configure results callback
    MaybeResults results;
    const auto cb = [&results](const auto&... args) { *results = {args...}; };

    // run execution
    const Return<void> ret =
            preparedModel->executeSynchronously_1_3(request, measure, deadline, cb);
    EXPECT_TRUE(ret.isOk());
    if (!ret.isOk()) return std::nullopt;

    // return results
    return results;
}

void runExecutionTest(const sp<IPreparedModel>& preparedModel, const TestModel& testModel,
                      const Request& request, bool synchronous, DeadlineBoundType deadlineBound) {
    const ExecutionFunction execute = synchronous ? executeSynchronously : executeAsynchronously;

    // Perform execution and unpack results.
    const auto results = execute(preparedModel, request, deadlineBound);
    if (!results.has_value()) return;
    const auto& [status, outputShapes, timing] = results.value();

    // Verify no timing information was returned
    EXPECT_EQ(UINT64_MAX, timing.timeOnDevice);
    EXPECT_EQ(UINT64_MAX, timing.timeInDriver);

    // Validate deadline information if applicable.
    switch (deadlineBound) {
        case DeadlineBoundType::NOW:
            // If the execution was launched with a deadline of NOW, the
            // deadline has already passed when the driver would launch the
            // execution. In this case, the driver must return
            // MISSED_DEADLINE_*.
            ASSERT_TRUE(status == ErrorStatus::MISSED_DEADLINE_TRANSIENT ||
                        status == ErrorStatus::MISSED_DEADLINE_PERSISTENT);
            return;
        case DeadlineBoundType::UNLIMITED:
            // If an unlimited deadline is supplied, we expect the execution to
            // proceed normally. In this case, check it normally by breaking out
            // of the switch statement.
            ASSERT_EQ(ErrorStatus::NONE, status);
            break;
    }

    // If the model output operands are fully specified, outputShapes must be either
    // either empty, or have the same number of elements as the number of outputs.
    ASSERT_TRUE(outputShapes.size() == 0 || outputShapes.size() == testModel.outputIndexes.size());

    // Go through all outputs, check returned output shapes.
    for (uint32_t i = 0; i < outputShapes.size(); i++) {
        EXPECT_TRUE(outputShapes[i].isSufficient);
        const auto& expect = testModel.operands[testModel.outputIndexes[i]].dimensions;
        const std::vector<uint32_t> actual = outputShapes[i].dimensions;
        EXPECT_EQ(expect, actual);
    }

    // Retrieve execution results.
    ASSERT_TRUE(nn::compliantWithV1_0(request));
    const V1_0::Request request10 = nn::convertToV1_0(request);
    const std::vector<TestBuffer> outputs = getOutputBuffers(request10);

    // We want "close-enough" results.
    checkResults(testModel, outputs);
}

void runExecutionTests(const sp<IPreparedModel>& preparedModel, const TestModel& testModel,
                       const Request& request) {
    for (bool synchronous : {false, true}) {
        for (auto deadlineBound : deadlineBounds) {
            runExecutionTest(preparedModel, testModel, request, synchronous, deadlineBound);
        }
    }
}

void runTests(const sp<IDevice>& device, const TestModel& testModel,
              std::pair<bool, bool> supportsDeadlines) {
    // setup
    const auto [supportsPrepareModelDeadline, supportsExecutionDeadline] = supportsDeadlines;
    if (!supportsPrepareModelDeadline && !supportsExecutionDeadline) return;
    const Model model = createModel(testModel);

    // run prepare model tests
    runPrepareModelTests(device, model, supportsPrepareModelDeadline);

    if (supportsExecutionDeadline) {
        // prepare model
        sp<IPreparedModel> preparedModel;
        createPreparedModel(device, model, &preparedModel);
        if (preparedModel == nullptr) return;

        // run execution tests
        const Request request = nn::convertToV1_3(createRequest(testModel));
        runExecutionTests(preparedModel, testModel, request);
    }
}

class DeadlineTest : public GeneratedTestBase {};

TEST_P(DeadlineTest, Test) {
    runTests(kDevice, kTestModel, mSupportsDeadlines);
}

INSTANTIATE_GENERATED_TEST(DeadlineTest,
                           [](const TestModel& testModel) { return !testModel.expectFailure; });

}  // namespace android::hardware::neuralnetworks::V1_3::vts::functional
