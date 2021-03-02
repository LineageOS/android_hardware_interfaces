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

#define LOG_TAG "neuralnetworks_aidl_hal_test"

#include <android/binder_auto_utils.h>

#include <chrono>

#include <TestHarness.h>
#include <nnapi/hal/aidl/Utils.h>

#include "Callbacks.h"
#include "GeneratedTestHarness.h"
#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

namespace aidl::android::hardware::neuralnetworks::vts::functional {

using ExecutionMutation = std::function<void(Request*)>;

///////////////////////// UTILITY FUNCTIONS /////////////////////////

// Primary validation function. This function will take a valid request, apply a
// mutation to it to invalidate the request, then pass it to interface calls
// that use the request.
static void validate(const std::shared_ptr<IPreparedModel>& preparedModel,
                     const std::string& message, const Request& originalRequest,
                     const ExecutionMutation& mutate) {
    Request request = utils::clone(originalRequest).value();
    mutate(&request);

    // We'd like to test both with timing requested and without timing
    // requested. Rather than running each test both ways, we'll decide whether
    // to request timing by hashing the message. We do not use std::hash because
    // it is not guaranteed stable across executions.
    char hash = 0;
    for (auto c : message) {
        hash ^= c;
    };
    bool measure = (hash & 1);

    // synchronous
    {
        SCOPED_TRACE(message + " [executeSynchronously]");
        ExecutionResult executionResult;
        const auto executeStatus = preparedModel->executeSynchronously(
                request, measure, kNoDeadline, kOmittedTimeoutDuration, &executionResult);
        ASSERT_FALSE(executeStatus.isOk());
        ASSERT_EQ(executeStatus.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(static_cast<ErrorStatus>(executeStatus.getServiceSpecificError()),
                  ErrorStatus::INVALID_ARGUMENT);
    }

    // fenced
    {
        SCOPED_TRACE(message + " [executeFenced]");
        FencedExecutionResult executionResult;
        const auto executeStatus = preparedModel->executeFenced(request, {}, false, kNoDeadline,
                                                                kOmittedTimeoutDuration,
                                                                kNoDuration, &executionResult);
        ASSERT_FALSE(executeStatus.isOk());
        ASSERT_EQ(executeStatus.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(static_cast<ErrorStatus>(executeStatus.getServiceSpecificError()),
                  ErrorStatus::INVALID_ARGUMENT);
    }
}

///////////////////////// REMOVE INPUT ////////////////////////////////////

static void removeInputTest(const std::shared_ptr<IPreparedModel>& preparedModel,
                            const Request& request) {
    for (size_t input = 0; input < request.inputs.size(); ++input) {
        const std::string message = "removeInput: removed input " + std::to_string(input);
        validate(preparedModel, message, request, [input](Request* request) {
            request->inputs.erase(request->inputs.begin() + input);
        });
    }
}

///////////////////////// REMOVE OUTPUT ////////////////////////////////////

static void removeOutputTest(const std::shared_ptr<IPreparedModel>& preparedModel,
                             const Request& request) {
    for (size_t output = 0; output < request.outputs.size(); ++output) {
        const std::string message = "removeOutput: removed Output " + std::to_string(output);
        validate(preparedModel, message, request, [output](Request* request) {
            request->outputs.erase(request->outputs.begin() + output);
        });
    }
}

///////////////////////////// ENTRY POINT //////////////////////////////////

void validateRequest(const std::shared_ptr<IPreparedModel>& preparedModel, const Request& request) {
    removeInputTest(preparedModel, request);
    removeOutputTest(preparedModel, request);
}

void validateRequestFailure(const std::shared_ptr<IPreparedModel>& preparedModel,
                            const Request& request) {
    SCOPED_TRACE("Expecting request to fail [executeSynchronously]");
    ExecutionResult executionResult;
    const auto executeStatus = preparedModel->executeSynchronously(
            request, false, kNoDeadline, kOmittedTimeoutDuration, &executionResult);

    ASSERT_FALSE(executeStatus.isOk());
    ASSERT_EQ(executeStatus.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_NE(static_cast<ErrorStatus>(executeStatus.getServiceSpecificError()), ErrorStatus::NONE);
}

}  // namespace aidl::android::hardware::neuralnetworks::vts::functional
