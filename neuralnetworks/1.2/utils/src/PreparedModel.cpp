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

#include "PreparedModel.h"

#include "Callbacks.h"
#include "Conversions.h"
#include "Execution.h"
#include "ExecutionBurstController.h"
#include "ExecutionBurstUtils.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <chrono>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_2::utils {

nn::GeneralResult<std::shared_ptr<const PreparedModel>> PreparedModel::create(
        sp<V1_2::IPreparedModel> preparedModel, bool executeSynchronously) {
    if (preparedModel == nullptr) {
        return NN_ERROR() << "V1_2::utils::PreparedModel::create must have non-null preparedModel";
    }

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(preparedModel));
    return std::make_shared<const PreparedModel>(PrivateConstructorTag{}, executeSynchronously,
                                                 std::move(preparedModel), std::move(deathHandler));
}

PreparedModel::PreparedModel(PrivateConstructorTag /*tag*/, bool executeSynchronously,
                             sp<V1_2::IPreparedModel> preparedModel,
                             hal::utils::DeathHandler deathHandler)
    : kExecuteSynchronously(executeSynchronously),
      kPreparedModel(std::move(preparedModel)),
      kDeathHandler(std::move(deathHandler)) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeSynchronously(const V1_0::Request& request, MeasureTiming measure) const {
    auto cb = hal::utils::CallbackValue(executionCallback);

    const auto ret = kPreparedModel->executeSynchronously(request, measure, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return cb.take();
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeAsynchronously(const V1_0::Request& request, MeasureTiming measure) const {
    const auto cb = sp<ExecutionCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kPreparedModel->execute_1_2(request, measure, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    if (status != V1_0::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        HANDLE_HAL_STATUS(status) << "execution failed with " << toString(status);
    }

    return cb->get();
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> PreparedModel::execute(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalTimePoint& /*deadline*/,
        const nn::OptionalDuration& /*loopTimeoutDuration*/) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared =
            NN_TRY(hal::utils::makeExecutionFailure(hal::utils::convertRequestFromPointerToShared(
                    &request, nn::kDefaultRequestMemoryAlignment, nn::kMinMemoryPadding,
                    &maybeRequestInShared, &relocation)));

    const auto hidlRequest = NN_TRY(hal::utils::makeExecutionFailure(convert(requestInShared)));
    const auto hidlMeasure = NN_TRY(hal::utils::makeExecutionFailure(convert(measure)));

    return executeInternal(hidlRequest, hidlMeasure, relocation);
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeInternal(const V1_0::Request& request, MeasureTiming measure,
                               const hal::utils::RequestRelocation& relocation) const {
    if (relocation.input) {
        relocation.input->flush();
    }

    auto result = kExecuteSynchronously ? executeSynchronously(request, measure)
                                        : executeAsynchronously(request, measure);
    auto [outputShapes, timing] = NN_TRY(std::move(result));

    if (relocation.output) {
        relocation.output->flush();
    }
    return std::make_pair(std::move(outputShapes), timing);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFenced(const nn::Request& /*request*/,
                             const std::vector<nn::SyncFence>& /*waitFor*/,
                             nn::MeasureTiming /*measure*/,
                             const nn::OptionalTimePoint& /*deadline*/,
                             const nn::OptionalDuration& /*loopTimeoutDuration*/,
                             const nn::OptionalDuration& /*timeoutDurationAfterFence*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IPreparedModel::executeFenced is not supported on 1.2 HAL service";
}

nn::GeneralResult<nn::SharedExecution> PreparedModel::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& /*loopTimeoutDuration*/) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared = NN_TRY(hal::utils::convertRequestFromPointerToShared(
            &request, nn::kDefaultRequestMemoryAlignment, nn::kMinMemoryPadding,
            &maybeRequestInShared, &relocation));

    auto hidlRequest = NN_TRY(convert(requestInShared));
    auto hidlMeasure = NN_TRY(convert(measure));
    return Execution::create(shared_from_this(), std::move(hidlRequest), std::move(relocation),
                             hidlMeasure);
}

nn::GeneralResult<nn::SharedBurst> PreparedModel::configureExecutionBurst() const {
    auto self = shared_from_this();
    auto fallback = [preparedModel = std::move(self)](
                            const nn::Request& request, nn::MeasureTiming measure,
                            const nn::OptionalTimePoint& deadline,
                            const nn::OptionalDuration& loopTimeoutDuration)
            -> nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> {
        return preparedModel->execute(request, measure, deadline, loopTimeoutDuration);
    };
    const auto pollingTimeWindow = getBurstControllerPollingTimeWindow();
    return ExecutionBurstController::create(shared_from_this(), kPreparedModel, pollingTimeWindow);
}

std::any PreparedModel::getUnderlyingResource() const {
    sp<V1_2::IPreparedModel> resource = kPreparedModel;
    return resource;
}

}  // namespace android::hardware::neuralnetworks::V1_2::utils
