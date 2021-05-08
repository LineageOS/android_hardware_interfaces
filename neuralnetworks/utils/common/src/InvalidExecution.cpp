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

#include "InvalidExecution.h"

#include <nnapi/IExecution.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> InvalidExecution::compute(
        const nn::OptionalTimePoint& /*deadline*/) const {
    return NN_ERROR() << "InvalidExecution";
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
InvalidExecution::computeFenced(const std::vector<nn::SyncFence>& /*waitFor*/,
                                const nn::OptionalTimePoint& /*deadline*/,
                                const nn::OptionalDuration& /*timeoutDurationAfterFence*/) const {
    return NN_ERROR() << "InvalidExecution";
}

}  // namespace android::hardware::neuralnetworks::utils
