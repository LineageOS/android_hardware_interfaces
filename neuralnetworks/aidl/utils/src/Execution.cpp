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

#include <aidl/android/hardware/neuralnetworks/Request.h>
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

namespace aidl::android::hardware::neuralnetworks::utils {

nn::GeneralResult<std::shared_ptr<const Execution>> Execution::create(
        std::shared_ptr<const PreparedModel> preparedModel, Request request,
        hal::utils::RequestRelocation relocation, bool measure, int64_t loopTimeoutDuration) {
    if (preparedModel == nullptr) {
        return NN_ERROR() << "aidl::utils::Execution::create must have non-null preparedModel";
    }

    return std::make_shared<const Execution>(PrivateConstructorTag{}, std::move(preparedModel),
                                             std::move(request), std::move(relocation), measure,
                                             loopTimeoutDuration);
}

Execution::Execution(PrivateConstructorTag /*tag*/,
                     std::shared_ptr<const PreparedModel> preparedModel, Request request,
                     hal::utils::RequestRelocation relocation, bool measure,
                     int64_t loopTimeoutDuration)
    : kPreparedModel(std::move(preparedModel)),
      kRequest(std::move(request)),
      kRelocation(std::move(relocation)),
      kMeasure(measure),
      kLoopTimeoutDuration(loopTimeoutDuration) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> Execution::compute(
        const nn::OptionalTimePoint& deadline) const {
    const auto aidlDeadline = NN_TRY(hal::utils::makeExecutionFailure(convert(deadline)));
    return kPreparedModel->executeInternal(kRequest, kMeasure, aidlDeadline, kLoopTimeoutDuration,
                                           kRelocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> Execution::computeFenced(
        const std::vector<nn::SyncFence>& waitFor, const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& timeoutDurationAfterFence) const {
    const auto aidlWaitFor = NN_TRY(convert(waitFor));
    const auto aidlDeadline = NN_TRY(convert(deadline));
    const auto aidlTimeoutDurationAfterFence = NN_TRY(convert(timeoutDurationAfterFence));
    return kPreparedModel->executeFencedInternal(kRequest, aidlWaitFor, kMeasure, aidlDeadline,
                                                 kLoopTimeoutDuration,
                                                 aidlTimeoutDurationAfterFence, kRelocation);
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
