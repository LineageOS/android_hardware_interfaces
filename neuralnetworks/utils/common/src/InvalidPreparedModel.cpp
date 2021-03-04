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

#include "InvalidPreparedModel.h"

#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
InvalidPreparedModel::execute(const nn::Request& /*request*/, nn::MeasureTiming /*measure*/,
                              const nn::OptionalTimePoint& /*deadline*/,
                              const nn::OptionalDuration& /*loopTimeoutDuration*/) const {
    return NN_ERROR() << "InvalidPreparedModel";
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
InvalidPreparedModel::executeFenced(
        const nn::Request& /*request*/, const std::vector<nn::SyncFence>& /*waitFor*/,
        nn::MeasureTiming /*measure*/, const nn::OptionalTimePoint& /*deadline*/,
        const nn::OptionalDuration& /*loopTimeoutDuration*/,
        const nn::OptionalDuration& /*timeoutDurationAfterFence*/) const {
    return NN_ERROR() << "InvalidPreparedModel";
}

nn::GeneralResult<nn::SharedExecution> InvalidPreparedModel::createReusableExecution(
        const nn::Request& /*request*/, nn::MeasureTiming /*measure*/,
        const nn::OptionalDuration& /*loopTimeoutDuration*/) const {
    return NN_ERROR() << "InvalidPreparedModel";
}

nn::GeneralResult<nn::SharedBurst> InvalidPreparedModel::configureExecutionBurst() const {
    return NN_ERROR() << "InvalidPreparedModel";
}

std::any InvalidPreparedModel::getUnderlyingResource() const {
    return {};
}

}  // namespace android::hardware::neuralnetworks::utils
