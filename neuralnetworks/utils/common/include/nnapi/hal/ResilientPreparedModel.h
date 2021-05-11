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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_PREPARED_MODEL_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_PREPARED_MODEL_H

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

class ResilientPreparedModel final : public nn::IPreparedModel,
                                     public std::enable_shared_from_this<ResilientPreparedModel> {
    struct PrivateConstructorTag {};

  public:
    using Factory = std::function<nn::GeneralResult<nn::SharedPreparedModel>()>;

    static nn::GeneralResult<std::shared_ptr<const ResilientPreparedModel>> create(
            Factory makePreparedModel);

    explicit ResilientPreparedModel(PrivateConstructorTag tag, Factory makePreparedModel,
                                    nn::SharedPreparedModel preparedModel);

    nn::SharedPreparedModel getPreparedModel() const;
    nn::GeneralResult<nn::SharedPreparedModel> recover(
            const nn::IPreparedModel* failingPreparedModel) const;

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> execute(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

    nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> executeFenced(
            const nn::Request& request, const std::vector<nn::SyncFence>& waitFor,
            nn::MeasureTiming measure, const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& loopTimeoutDuration,
            const nn::OptionalDuration& timeoutDurationAfterFence) const override;

    nn::GeneralResult<nn::SharedExecution> createReusableExecution(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

    nn::GeneralResult<nn::SharedBurst> configureExecutionBurst() const override;

    std::any getUnderlyingResource() const override;

  private:
    bool isValidInternal() const EXCLUDES(mMutex);
    nn::GeneralResult<nn::SharedExecution> createReusableExecutionInternal(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalDuration& loopTimeoutDuration) const;
    nn::GeneralResult<nn::SharedBurst> configureExecutionBurstInternal() const;

    const Factory kMakePreparedModel;
    mutable std::mutex mMutex;
    mutable nn::SharedPreparedModel mPreparedModel GUARDED_BY(mMutex);
};

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_PREPARED_MODEL_H
