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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_EXECUTION_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_EXECUTION_H

#include <android-base/thread_annotations.h>
#include <nnapi/IExecution.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

class ResilientExecution final : public nn::IExecution,
                                 public std::enable_shared_from_this<ResilientExecution> {
    struct PrivateConstructorTag {};

  public:
    using Factory = std::function<nn::GeneralResult<nn::SharedExecution>()>;

    static nn::GeneralResult<std::shared_ptr<const ResilientExecution>> create(
            Factory makeExecution);

    ResilientExecution(PrivateConstructorTag tag, Factory makeExecution,
                       nn::SharedExecution execution);

    nn::SharedExecution getExecution() const;
    nn::GeneralResult<nn::SharedExecution> recover(const nn::IExecution* failingExecution) const;

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> compute(
            const nn::OptionalTimePoint& deadline) const override;

    nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> computeFenced(
            const std::vector<nn::SyncFence>& waitFor, const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& timeoutDurationAfterFence) const override;

  private:
    bool isValidInternal() const EXCLUDES(mMutex);

    const Factory kMakeExecution;
    mutable std::mutex mMutex;
    mutable nn::SharedExecution mExecution GUARDED_BY(mMutex);
};

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_EXECUTION_H
