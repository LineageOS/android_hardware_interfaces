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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_INVALID_EXECUTION_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_INVALID_EXECUTION_H

#include <nnapi/IExecution.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

class InvalidExecution final : public nn::IExecution {
  public:
    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> compute(
            const nn::OptionalTimePoint& deadline) const override;

    nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> computeFenced(
            const std::vector<nn::SyncFence>& waitFor, const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& timeoutDurationAfterFence) const override;
};

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_INVALID_EXECUTION_H
