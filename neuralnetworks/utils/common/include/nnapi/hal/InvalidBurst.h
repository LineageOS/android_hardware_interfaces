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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_INVALID_BURST_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_INVALID_BURST_H

#include <nnapi/IBurst.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <optional>
#include <utility>

namespace android::hardware::neuralnetworks::utils {

class InvalidBurst final : public nn::IBurst {
  public:
    OptionalCacheHold cacheMemory(const nn::SharedMemory& memory) const override;

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> execute(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

    nn::GeneralResult<nn::SharedExecution> createReusableExecution(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalDuration& loopTimeoutDuration) const override;
};

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_INVALID_BURST_H
