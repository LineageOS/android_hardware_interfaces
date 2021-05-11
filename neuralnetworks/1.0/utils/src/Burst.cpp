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

#include "Burst.h"

#include <android-base/logging.h>
#include <nnapi/IBurst.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>

#include <memory>
#include <optional>
#include <utility>

namespace android::hardware::neuralnetworks::V1_0::utils {

nn::GeneralResult<std::shared_ptr<const Burst>> Burst::create(
        nn::SharedPreparedModel preparedModel) {
    if (preparedModel == nullptr) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
               << "V1_0::utils::Burst::create must have non-null preparedModel";
    }

    return std::make_shared<const Burst>(PrivateConstructorTag{}, std::move(preparedModel));
}

Burst::Burst(PrivateConstructorTag /*tag*/, nn::SharedPreparedModel preparedModel)
    : kPreparedModel(std::move(preparedModel)) {
    CHECK(kPreparedModel != nullptr);
}

Burst::OptionalCacheHold Burst::cacheMemory(const nn::SharedMemory& /*memory*/) const {
    return nullptr;
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> Burst::execute(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    return kPreparedModel->execute(request, measure, deadline, loopTimeoutDuration);
}

nn::GeneralResult<nn::SharedExecution> Burst::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    return kPreparedModel->createReusableExecution(request, measure, loopTimeoutDuration);
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
