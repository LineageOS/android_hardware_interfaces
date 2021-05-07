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

#include "Execution.h"

#include "Conversions.h"
#include "PreparedModel.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IExecution.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>

#include <memory>
#include <utility>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_3::utils {

nn::GeneralResult<std::shared_ptr<const Execution>> Execution::create(
        std::shared_ptr<const PreparedModel> preparedModel, Request request,
        hal::utils::RequestRelocation relocation, V1_2::MeasureTiming measure,
        OptionalTimeoutDuration loopTimeoutDuration) {
    if (preparedModel == nullptr) {
        return NN_ERROR() << "V1_3::utils::Execution::create must have non-null preparedModel";
    }

    return std::make_shared<const Execution>(PrivateConstructorTag{}, std::move(preparedModel),
                                             std::move(request), std::move(relocation), measure,
                                             std::move(loopTimeoutDuration));
}

Execution::Execution(PrivateConstructorTag /*tag*/,
                     std::shared_ptr<const PreparedModel> preparedModel, Request request,
                     hal::utils::RequestRelocation relocation, V1_2::MeasureTiming measure,
                     OptionalTimeoutDuration loopTimeoutDuration)
    : kPreparedModel(std::move(preparedModel)),
      kRequest(std::move(request)),
      kRelocation(std::move(relocation)),
      kMeasure(measure),
      kLoopTimeoutDuration(std::move(loopTimeoutDuration)) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> Execution::compute(
        const nn::OptionalTimePoint& deadline) const {
    const auto hidlDeadline = NN_TRY(hal::utils::makeExecutionFailure(convert(deadline)));
    return kPreparedModel->executeInternal(kRequest, kMeasure, hidlDeadline, kLoopTimeoutDuration,
                                           kRelocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> Execution::computeFenced(
        const std::vector<nn::SyncFence>& waitFor, const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& timeoutDurationAfterFence) const {
    const auto hidlWaitFor = NN_TRY(hal::utils::convertSyncFences(waitFor));
    const auto hidlDeadline = NN_TRY(convert(deadline));
    const auto hidlTimeoutDurationAfterFence = NN_TRY(convert(timeoutDurationAfterFence));
    return kPreparedModel->executeFencedInternal(kRequest, hidlWaitFor, kMeasure, hidlDeadline,
                                                 kLoopTimeoutDuration,
                                                 hidlTimeoutDurationAfterFence, kRelocation);
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
