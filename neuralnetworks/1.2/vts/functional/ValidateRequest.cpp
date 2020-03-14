/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include <chrono>
#include "1.0/Utils.h"
#include "1.2/Callbacks.h"
#include "ExecutionBurstController.h"
#include "GeneratedTestHarness.h"
#include "TestHarness.h"
#include "VtsHalNeuralnetworks.h"

namespace android::hardware::neuralnetworks::V1_2::vts::functional {

using implementation::ExecutionCallback;
using V1_0::ErrorStatus;
using V1_0::Request;

using ExecutionMutation = std::function<void(Request*)>;

///////////////////////// UTILITY FUNCTIONS /////////////////////////

static bool badTiming(Timing timing) {
    return timing.timeOnDevice == UINT64_MAX && timing.timeInDriver == UINT64_MAX;
}

// Primary validation function. This function will take a valid request, apply a
// mutation to it to invalidate the request, then pass it to interface calls
// that use the request.
static void validate(const sp<IPreparedModel>& preparedModel, const std::string& message,
                     const Request& originalRequest, const ExecutionMutation& mutate) {
    Request request = originalRequest;
    mutate(&request);

    // We'd like to test both with timing requested and without timing
    // requested. Rather than running each test both ways, we'll decide whether
    // to request timing by hashing the message. We do not use std::hash because
    // it is not guaranteed stable across executions.
    char hash = 0;
    for (auto c : message) {
        hash ^= c;
    };
    MeasureTiming measure = (hash & 1) ? MeasureTiming::YES : MeasureTiming::NO;

    // asynchronous
    {
        SCOPED_TRACE(message + " [execute_1_2]");

        sp<ExecutionCallback> executionCallback = new ExecutionCallback();
        Return<ErrorStatus> executeLaunchStatus =
                preparedModel->execute_1_2(request, measure, executionCallback);
        ASSERT_TRUE(executeLaunchStatus.isOk());
        ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, static_cast<ErrorStatus>(executeLaunchStatus));

        executionCallback->wait();
        ErrorStatus executionReturnStatus = executionCallback->getStatus();
        const auto& outputShapes = executionCallback->getOutputShapes();
        Timing timing = executionCallback->getTiming();
        ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, executionReturnStatus);
        ASSERT_EQ(outputShapes.size(), 0);
        ASSERT_TRUE(badTiming(timing));
    }

    // synchronous
    {
        SCOPED_TRACE(message + " [executeSynchronously]");

        Return<void> executeStatus = preparedModel->executeSynchronously(
                request, measure,
                [](ErrorStatus error, const hidl_vec<OutputShape>& outputShapes,
                   const Timing& timing) {
                    ASSERT_EQ(ErrorStatus::INVALID_ARGUMENT, error);
                    EXPECT_EQ(outputShapes.size(), 0);
                    EXPECT_TRUE(badTiming(timing));
                });
        ASSERT_TRUE(executeStatus.isOk());
    }

    // burst
    {
        SCOPED_TRACE(message + " [burst]");

        // create burst
        std::shared_ptr<::android::nn::ExecutionBurstController> burst =
                android::nn::ExecutionBurstController::create(preparedModel,
                                                              std::chrono::microseconds{0});
        ASSERT_NE(nullptr, burst.get());

        // create memory keys
        std::vector<intptr_t> keys(request.pools.size());
        for (size_t i = 0; i < keys.size(); ++i) {
            keys[i] = reinterpret_cast<intptr_t>(&request.pools[i]);
        }

        // execute and verify
        const auto [n, outputShapes, timing, fallback] = burst->compute(request, measure, keys);
        const ErrorStatus status = nn::legacyConvertResultCodeToErrorStatus(n);
        EXPECT_EQ(ErrorStatus::INVALID_ARGUMENT, status);
        EXPECT_EQ(outputShapes.size(), 0);
        EXPECT_TRUE(badTiming(timing));
        EXPECT_FALSE(fallback);

        // additional burst testing
        if (request.pools.size() > 0) {
            // valid free
            burst->freeMemory(keys.front());

            // negative test: invalid free of unknown (blank) memory
            burst->freeMemory(intptr_t{});

            // negative test: double free of memory
            burst->freeMemory(keys.front());
        }
    }
}

///////////////////////// REMOVE INPUT ////////////////////////////////////

static void removeInputTest(const sp<IPreparedModel>& preparedModel, const Request& request) {
    for (size_t input = 0; input < request.inputs.size(); ++input) {
        const std::string message = "removeInput: removed input " + std::to_string(input);
        validate(preparedModel, message, request,
                 [input](Request* request) { hidl_vec_removeAt(&request->inputs, input); });
    }
}

///////////////////////// REMOVE OUTPUT ////////////////////////////////////

static void removeOutputTest(const sp<IPreparedModel>& preparedModel, const Request& request) {
    for (size_t output = 0; output < request.outputs.size(); ++output) {
        const std::string message = "removeOutput: removed Output " + std::to_string(output);
        validate(preparedModel, message, request,
                 [output](Request* request) { hidl_vec_removeAt(&request->outputs, output); });
    }
}

///////////////////////////// ENTRY POINT //////////////////////////////////

void validateRequest(const sp<IPreparedModel>& preparedModel, const Request& request) {
    removeInputTest(preparedModel, request);
    removeOutputTest(preparedModel, request);
}

void validateRequestFailure(const sp<IPreparedModel>& preparedModel, const Request& request) {
    SCOPED_TRACE("Expecting request to fail [executeSynchronously]");
    Return<void> executeStatus = preparedModel->executeSynchronously(
            request, MeasureTiming::NO,
            [](ErrorStatus error, const hidl_vec<OutputShape>& outputShapes, const Timing& timing) {
                ASSERT_NE(ErrorStatus::NONE, error);
                EXPECT_EQ(outputShapes.size(), 0);
                EXPECT_TRUE(badTiming(timing));
            });
    ASSERT_TRUE(executeStatus.isOk());
}

}  // namespace android::hardware::neuralnetworks::V1_2::vts::functional
