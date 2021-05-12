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
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.2/Conversions.h>
#include <nnapi/hal/1.2/ExecutionBurstController.h>
#include <nnapi/hal/1.2/ExecutionBurstUtils.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

nn::GeneralResult<std::pair<nn::Timing, nn::Timing>> convertFencedExecutionCallbackResults(
        ErrorStatus status, const V1_2::Timing& timingLaunched, const V1_2::Timing& timingFenced) {
    HANDLE_HAL_STATUS(status) << "fenced execution callback info failed with " << toString(status);
    return std::make_pair(NN_TRY(nn::convert(timingLaunched)), NN_TRY(nn::convert(timingFenced)));
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> fencedExecutionCallback(
        ErrorStatus status, const hidl_handle& syncFence,
        const sp<IFencedExecutionCallback>& callback) {
    HANDLE_HAL_STATUS(status) << "fenced execution failed with " << toString(status);

    auto resultSyncFence = nn::SyncFence::createAsSignaled();
    if (syncFence.getNativeHandle() != nullptr) {
        auto sharedHandle = NN_TRY(nn::convert(syncFence));
        resultSyncFence = NN_TRY(hal::utils::makeGeneralFailure(
                nn::SyncFence::create(std::move(sharedHandle)), nn::ErrorStatus::GENERAL_FAILURE));
    }

    if (callback == nullptr) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "callback is null";
    }

    // Create callback which can be used to retrieve the execution error status and timings.
    nn::ExecuteFencedInfoCallback resultCallback =
            [callback]() -> nn::GeneralResult<std::pair<nn::Timing, nn::Timing>> {
        auto cb = hal::utils::CallbackValue(convertFencedExecutionCallbackResults);

        const auto ret = callback->getExecutionInfo(cb);
        HANDLE_TRANSPORT_FAILURE(ret);

        return cb.take();
    };

    return std::make_pair(std::move(resultSyncFence), std::move(resultCallback));
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const PreparedModel>> PreparedModel::create(
        sp<V1_3::IPreparedModel> preparedModel, bool executeSynchronously) {
    if (preparedModel == nullptr) {
        return NN_ERROR() << "V1_3::utils::PreparedModel::create must have non-null preparedModel";
    }

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(preparedModel));
    return std::make_shared<const PreparedModel>(PrivateConstructorTag{}, executeSynchronously,
                                                 std::move(preparedModel), std::move(deathHandler));
}

PreparedModel::PreparedModel(PrivateConstructorTag /*tag*/, bool executeSynchronously,
                             sp<V1_3::IPreparedModel> preparedModel,
                             hal::utils::DeathHandler deathHandler)
    : kExecuteSynchronously(executeSynchronously),
      kPreparedModel(std::move(preparedModel)),
      kDeathHandler(std::move(deathHandler)) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeSynchronously(const Request& request, V1_2::MeasureTiming measure,
                                    const OptionalTimePoint& deadline,
                                    const OptionalTimeoutDuration& loopTimeoutDuration) const {
    auto cb = hal::utils::CallbackValue(executionCallback);

    const auto ret = kPreparedModel->executeSynchronously_1_3(request, measure, deadline,
                                                              loopTimeoutDuration, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return cb.take();
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeAsynchronously(const Request& request, V1_2::MeasureTiming measure,
                                     const OptionalTimePoint& deadline,
                                     const OptionalTimeoutDuration& loopTimeoutDuration) const {
    const auto cb = sp<ExecutionCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret =
            kPreparedModel->execute_1_3(request, measure, deadline, loopTimeoutDuration, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    if (status != ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        HANDLE_HAL_STATUS(status) << "execution failed with " << toString(status);
    }

    return cb->get();
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> PreparedModel::execute(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared =
            NN_TRY(hal::utils::makeExecutionFailure(hal::utils::convertRequestFromPointerToShared(
                    &request, nn::kDefaultRequestMemoryAlignment, nn::kMinMemoryPadding,
                    &maybeRequestInShared, &relocation)));

    const auto hidlRequest = NN_TRY(hal::utils::makeExecutionFailure(convert(requestInShared)));
    const auto hidlMeasure = NN_TRY(hal::utils::makeExecutionFailure(convert(measure)));
    const auto hidlDeadline = NN_TRY(hal::utils::makeExecutionFailure(convert(deadline)));
    const auto hidlLoopTimeoutDuration =
            NN_TRY(hal::utils::makeExecutionFailure(convert(loopTimeoutDuration)));

    return executeInternal(hidlRequest, hidlMeasure, hidlDeadline, hidlLoopTimeoutDuration,
                           relocation);
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeInternal(const Request& request, V1_2::MeasureTiming measure,
                               const OptionalTimePoint& deadline,
                               const OptionalTimeoutDuration& loopTimeoutDuration,
                               const hal::utils::RequestRelocation& relocation) const {
    if (relocation.input) {
        relocation.input->flush();
    }

    auto result = kExecuteSynchronously
                          ? executeSynchronously(request, measure, deadline, loopTimeoutDuration)
                          : executeAsynchronously(request, measure, deadline, loopTimeoutDuration);
    auto [outputShapes, timing] = NN_TRY(std::move(result));

    if (relocation.output) {
        relocation.output->flush();
    }
    return std::make_pair(std::move(outputShapes), timing);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFenced(const nn::Request& request, const std::vector<nn::SyncFence>& waitFor,
                             nn::MeasureTiming measure, const nn::OptionalTimePoint& deadline,
                             const nn::OptionalDuration& loopTimeoutDuration,
                             const nn::OptionalDuration& timeoutDurationAfterFence) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared = NN_TRY(hal::utils::convertRequestFromPointerToShared(
            &request, nn::kDefaultRequestMemoryAlignment, nn::kMinMemoryPadding,
            &maybeRequestInShared, &relocation));

    const auto hidlRequest = NN_TRY(convert(requestInShared));
    const auto hidlWaitFor = NN_TRY(hal::utils::convertSyncFences(waitFor));
    const auto hidlMeasure = NN_TRY(convert(measure));
    const auto hidlDeadline = NN_TRY(convert(deadline));
    const auto hidlLoopTimeoutDuration = NN_TRY(convert(loopTimeoutDuration));
    const auto hidlTimeoutDurationAfterFence = NN_TRY(convert(timeoutDurationAfterFence));

    return executeFencedInternal(hidlRequest, hidlWaitFor, hidlMeasure, hidlDeadline,
                                 hidlLoopTimeoutDuration, hidlTimeoutDurationAfterFence,
                                 relocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFencedInternal(const Request& request, const hidl_vec<hidl_handle>& waitFor,
                                     V1_2::MeasureTiming measure, const OptionalTimePoint& deadline,
                                     const OptionalTimeoutDuration& loopTimeoutDuration,
                                     const OptionalTimeoutDuration& timeoutDurationAfterFence,
                                     const hal::utils::RequestRelocation& relocation) const {
    if (relocation.input) {
        relocation.input->flush();
    }

    auto cb = hal::utils::CallbackValue(fencedExecutionCallback);

    const auto ret =
            kPreparedModel->executeFenced(request, waitFor, measure, deadline, loopTimeoutDuration,
                                          timeoutDurationAfterFence, cb);
    HANDLE_TRANSPORT_FAILURE(ret);
    auto [syncFence, callback] = NN_TRY(cb.take());

    // If executeFenced required the request memory to be moved into shared memory, block here until
    // the fenced execution has completed and flush the memory back.
    if (relocation.output) {
        const auto state = syncFence.syncWait({});
        if (state != nn::SyncFence::FenceState::SIGNALED) {
            return NN_ERROR() << "syncWait failed with " << state;
        }
        relocation.output->flush();
    }

    return std::make_pair(std::move(syncFence), std::move(callback));
}

nn::GeneralResult<nn::SharedExecution> PreparedModel::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared = NN_TRY(hal::utils::convertRequestFromPointerToShared(
            &request, nn::kDefaultRequestMemoryAlignment, nn::kMinMemoryPadding,
            &maybeRequestInShared, &relocation));

    auto hidlRequest = NN_TRY(convert(requestInShared));
    auto hidlMeasure = NN_TRY(convert(measure));
    auto hidlLoopTimeoutDuration = NN_TRY(convert(loopTimeoutDuration));
    return Execution::create(shared_from_this(), std::move(hidlRequest), std::move(relocation),
                             hidlMeasure, std::move(hidlLoopTimeoutDuration));
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
    const auto pollingTimeWindow = V1_2::utils::getBurstControllerPollingTimeWindow();
    return V1_2::utils::ExecutionBurstController::create(shared_from_this(), kPreparedModel,
                                                         pollingTimeWindow);
}

std::any PreparedModel::getUnderlyingResource() const {
    sp<V1_3::IPreparedModel> resource = kPreparedModel;
    return resource;
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
