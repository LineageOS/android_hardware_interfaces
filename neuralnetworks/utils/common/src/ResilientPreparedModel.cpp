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

#include "ResilientPreparedModel.h"

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

nn::GeneralResult<std::shared_ptr<const ResilientPreparedModel>> ResilientPreparedModel::create(
        Factory makePreparedModel) {
    if (makePreparedModel == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "utils::ResilientPreparedModel::create must have non-empty makePreparedModel";
    }
    auto preparedModel = NN_TRY(makePreparedModel());
    CHECK(preparedModel != nullptr);
    return std::make_shared<ResilientPreparedModel>(
            PrivateConstructorTag{}, std::move(makePreparedModel), std::move(preparedModel));
}

ResilientPreparedModel::ResilientPreparedModel(PrivateConstructorTag /*tag*/,
                                               Factory makePreparedModel,
                                               nn::SharedPreparedModel preparedModel)
    : kMakePreparedModel(std::move(makePreparedModel)), mPreparedModel(std::move(preparedModel)) {
    CHECK(kMakePreparedModel != nullptr);
    CHECK(mPreparedModel != nullptr);
}

nn::SharedPreparedModel ResilientPreparedModel::getPreparedModel() const {
    std::lock_guard guard(mMutex);
    return mPreparedModel;
}

nn::SharedPreparedModel ResilientPreparedModel::recover(
        const nn::IPreparedModel* /*failingPreparedModel*/, bool /*blocking*/) const {
    std::lock_guard guard(mMutex);
    return mPreparedModel;
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
ResilientPreparedModel::execute(const nn::Request& request, nn::MeasureTiming measure,
                                const nn::OptionalTimePoint& deadline,
                                const nn::OptionalDuration& loopTimeoutDuration) const {
    return getPreparedModel()->execute(request, measure, deadline, loopTimeoutDuration);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
ResilientPreparedModel::executeFenced(const nn::Request& request,
                                      const std::vector<nn::SyncFence>& waitFor,
                                      nn::MeasureTiming measure,
                                      const nn::OptionalTimePoint& deadline,
                                      const nn::OptionalDuration& loopTimeoutDuration,
                                      const nn::OptionalDuration& timeoutDurationAfterFence) const {
    return getPreparedModel()->executeFenced(request, waitFor, measure, deadline,
                                             loopTimeoutDuration, timeoutDurationAfterFence);
}

std::any ResilientPreparedModel::getUnderlyingResource() const {
    return getPreparedModel()->getUnderlyingResource();
}

}  // namespace android::hardware::neuralnetworks::utils
