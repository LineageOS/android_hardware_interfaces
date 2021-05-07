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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_PREPARED_MODEL_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_PREPARED_MODEL_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IPreparedModel.h>

namespace android::nn {

class MockPreparedModel final : public IPreparedModel {
  public:
    MOCK_METHOD((ExecutionResult<std::pair<std::vector<OutputShape>, Timing>>), execute,
                (const Request& request, MeasureTiming measure, const OptionalTimePoint& deadline,
                 const OptionalDuration& loopTimeoutDuration),
                (const, override));
    MOCK_METHOD((GeneralResult<std::pair<SyncFence, ExecuteFencedInfoCallback>>), executeFenced,
                (const Request& request, const std::vector<SyncFence>& waitFor,
                 MeasureTiming measure, const OptionalTimePoint& deadline,
                 const OptionalDuration& loopTimeoutDuration,
                 const OptionalDuration& timeoutDurationAfterFence),
                (const, override));
    MOCK_METHOD((GeneralResult<SharedExecution>), createReusableExecution,
                (const nn::Request& request, nn::MeasureTiming measure,
                 const nn::OptionalDuration& loopTimeoutDuration),
                (const, override));
    MOCK_METHOD(GeneralResult<SharedBurst>, configureExecutionBurst, (), (const, override));
    MOCK_METHOD(std::any, getUnderlyingResource, (), (const, override));
};

}  // namespace android::nn

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_PREPARED_MODEL_H
