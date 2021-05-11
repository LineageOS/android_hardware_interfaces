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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_BURST_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_BURST_H

#include <nnapi/IBurst.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <optional>
#include <utility>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_0::utils {

// Class that adapts nn::IPreparedModel to nn::IBurst.
class Burst final : public nn::IBurst {
    struct PrivateConstructorTag {};

  public:
    static nn::GeneralResult<std::shared_ptr<const Burst>> create(
            nn::SharedPreparedModel preparedModel);

    Burst(PrivateConstructorTag tag, nn::SharedPreparedModel preparedModel);

    OptionalCacheHold cacheMemory(const nn::SharedMemory& memory) const override;

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> execute(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

    nn::GeneralResult<nn::SharedExecution> createReusableExecution(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

  private:
    const nn::SharedPreparedModel kPreparedModel;
};

}  // namespace android::hardware::neuralnetworks::V1_0::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_BURST_H
