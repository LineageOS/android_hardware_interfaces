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

#include "InvalidBurst.h"

#include <nnapi/IBurst.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <optional>
#include <utility>

namespace android::hardware::neuralnetworks::utils {

InvalidBurst::OptionalCacheHold InvalidBurst::cacheMemory(
        const nn::SharedMemory& /*memory*/) const {
    return nullptr;
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> InvalidBurst::execute(
        const nn::Request& /*request*/, nn::MeasureTiming /*measure*/,
        const nn::OptionalTimePoint& /*deadline*/,
        const nn::OptionalDuration& /*loopTimeoutDuration*/) const {
    return NN_ERROR() << "InvalidBurst";
}

nn::GeneralResult<nn::SharedExecution> InvalidBurst::createReusableExecution(
        const nn::Request& /*request*/, nn::MeasureTiming /*measure*/,
        const nn::OptionalDuration& /*loopTimeoutDuration*/) const {
    return NN_ERROR() << "InvalidBurst";
}

}  // namespace android::hardware::neuralnetworks::utils
