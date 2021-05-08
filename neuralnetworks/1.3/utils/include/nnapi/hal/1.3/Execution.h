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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_EXECUTION_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_EXECUTION_H

#include <nnapi/IExecution.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

#include "PreparedModel.h"

#include <memory>
#include <utility>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_3::utils {

class Execution final : public nn::IExecution, public std::enable_shared_from_this<Execution> {
    struct PrivateConstructorTag {};

  public:
    static nn::GeneralResult<std::shared_ptr<const Execution>> create(
            std::shared_ptr<const PreparedModel> preparedModel, Request request,
            hal::utils::RequestRelocation relocation, V1_2::MeasureTiming measure,
            OptionalTimeoutDuration loopTimeoutDuration);

    Execution(PrivateConstructorTag tag, std::shared_ptr<const PreparedModel> preparedModel,
              Request request, hal::utils::RequestRelocation relocation,
              V1_2::MeasureTiming measure, OptionalTimeoutDuration loopTimeoutDuration);

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> compute(
            const nn::OptionalTimePoint& deadline) const override;

    nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> computeFenced(
            const std::vector<nn::SyncFence>& waitFor, const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& timeoutDurationAfterFence) const override;

  private:
    const std::shared_ptr<const PreparedModel> kPreparedModel;
    const Request kRequest;
    const hal::utils::RequestRelocation kRelocation;
    const V1_2::MeasureTiming kMeasure;
    const OptionalTimeoutDuration kLoopTimeoutDuration;
};

}  // namespace android::hardware::neuralnetworks::V1_3::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_EXECUTION_H
