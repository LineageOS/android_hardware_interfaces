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

#include "Callbacks.h"
#include "Conversions.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <nnapi/IExecution.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <memory>
#include <utility>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_0::utils {

nn::GeneralResult<std::shared_ptr<const Execution>> Execution::create(
        std::shared_ptr<const PreparedModel> preparedModel, Request request,
        hal::utils::RequestRelocation relocation) {
    if (preparedModel == nullptr) {
        return NN_ERROR() << "V1_0::utils::Execution::create must have non-null preparedModel";
    }

    return std::make_shared<const Execution>(PrivateConstructorTag{}, std::move(preparedModel),
                                             std::move(request), std::move(relocation));
}

Execution::Execution(PrivateConstructorTag /*tag*/,
                     std::shared_ptr<const PreparedModel> preparedModel, Request request,
                     hal::utils::RequestRelocation relocation)
    : kPreparedModel(std::move(preparedModel)),
      kRequest(std::move(request)),
      kRelocation(std::move(relocation)) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> Execution::compute(
        const nn::OptionalTimePoint& /*deadline*/) const {
    return kPreparedModel->executeInternal(kRequest, kRelocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> Execution::computeFenced(
        const std::vector<nn::SyncFence>& /*waitFor*/, const nn::OptionalTimePoint& /*deadline*/,
        const nn::OptionalDuration& /*timeoutDurationAfterFence*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IExecution::computeFenced is not supported on 1.0 HAL service";
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
