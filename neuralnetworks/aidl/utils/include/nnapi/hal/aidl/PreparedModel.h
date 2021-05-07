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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_PREPARED_MODEL_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_PREPARED_MODEL_H

#include <aidl/android/hardware/neuralnetworks/IPreparedModel.h>
#include <aidl/android/hardware/neuralnetworks/Request.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::utils {

// Class that adapts aidl_hal::IPreparedModel to nn::IPreparedModel.
class PreparedModel final : public nn::IPreparedModel,
                            public std::enable_shared_from_this<PreparedModel> {
    struct PrivateConstructorTag {};

  public:
    static nn::GeneralResult<std::shared_ptr<const PreparedModel>> create(
            std::shared_ptr<aidl_hal::IPreparedModel> preparedModel);

    PreparedModel(PrivateConstructorTag tag,
                  std::shared_ptr<aidl_hal::IPreparedModel> preparedModel);

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

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> executeInternal(
            const Request& request, bool measure, int64_t deadline, int64_t loopTimeoutDuration,
            const hal::utils::RequestRelocation& relocation) const;

    nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
    executeFencedInternal(const Request& request,
                          const std::vector<ndk::ScopedFileDescriptor>& waitFor, bool measure,
                          int64_t deadline, int64_t loopTimeoutDuration,
                          int64_t timeoutDurationAfterFence,
                          const hal::utils::RequestRelocation& relocation) const;

  private:
    const std::shared_ptr<aidl_hal::IPreparedModel> kPreparedModel;
};

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_PREPARED_MODEL_H
