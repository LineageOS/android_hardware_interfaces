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

nn::GeneralResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
convertExecutionResultsHelper(const hidl_vec<V1_2::OutputShape>& outputShapes,
                              const V1_2::Timing& timing) {
    return std::make_pair(NN_TRY(nn::convert(outputShapes)), NN_TRY(nn::convert(timing)));
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> convertExecutionResults(
        const hidl_vec<V1_2::OutputShape>& outputShapes, const V1_2::Timing& timing) {
    return hal::utils::makeExecutionFailure(convertExecutionResultsHelper(outputShapes, timing));
}

nn::GeneralResult<std::pair<nn::Timing, nn::Timing>> convertFencedExecutionCallbackResults(
        const V1_2::Timing& timingLaunched, const V1_2::Timing& timingFenced) {
    return std::make_pair(NN_TRY(nn::convert(timingLaunched)), NN_TRY(nn::convert(timingFenced)));
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
convertExecuteFencedResults(const hidl_handle& syncFence,
                            const sp<IFencedExecutionCallback>& callback) {
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
        nn::GeneralResult<std::pair<nn::Timing, nn::Timing>> result =
                NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
        auto cb = [&result](ErrorStatus status, const V1_2::Timing& timingLaunched,
                            const V1_2::Timing& timingFenced) {
            if (status != ErrorStatus::NONE) {
                const auto canonical =
                        nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
                result = NN_ERROR(canonical) << "getExecutionInfo failed with " << toString(status);
            } else {
                result = convertFencedExecutionCallbackResults(timingLaunched, timingFenced);
            }
        };

        const auto ret = callback->getExecutionInfo(cb);
        HANDLE_TRANSPORT_FAILURE(ret);

        return result;
    };

    return std::make_pair(std::move(resultSyncFence), std::move(resultCallback));
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const PreparedModel>> PreparedModel::create(
        sp<V1_3::IPreparedModel> preparedModel) {
    if (preparedModel == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_3::utils::PreparedModel::create must have non-null preparedModel";
    }

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(preparedModel));
    return std::make_shared<const PreparedModel>(PrivateConstructorTag{}, std::move(preparedModel),
                                                 std::move(deathHandler));
}

PreparedModel::PreparedModel(PrivateConstructorTag /*tag*/, sp<V1_3::IPreparedModel> preparedModel,
                             hal::utils::DeathHandler deathHandler)
    : kPreparedModel(std::move(preparedModel)), kDeathHandler(std::move(deathHandler)) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeSynchronously(const Request& request, V1_2::MeasureTiming measure,
                                    const OptionalTimePoint& deadline,
                                    const OptionalTimeoutDuration& loopTimeoutDuration) const {
    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> result =
            NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
    const auto cb = [&result](ErrorStatus status, const hidl_vec<V1_2::OutputShape>& outputShapes,
                              const V1_2::Timing& timing) {
        if (status != ErrorStatus::NONE) {
            const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "executeSynchronously failed with " << toString(status);
        } else {
            result = convertExecutionResults(outputShapes, timing);
        }
    };

    const auto ret = kPreparedModel->executeSynchronously_1_3(request, measure, deadline,
                                                              loopTimeoutDuration, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    return result;
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
    if (status != ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        return NN_ERROR(canonical) << "executeAsynchronously failed with " << toString(status);
    }

    return cb->get();
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> PreparedModel::execute(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalTimePoint& deadline,
        const nn::OptionalTimeoutDuration& loopTimeoutDuration) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    const nn::Request& requestInShared = NN_TRY(hal::utils::makeExecutionFailure(
            hal::utils::flushDataFromPointerToShared(&request, &maybeRequestInShared)));

    const auto hidlRequest = NN_TRY(hal::utils::makeExecutionFailure(convert(requestInShared)));
    const auto hidlMeasure =
            NN_TRY(hal::utils::makeExecutionFailure(V1_2::utils::convert(measure)));
    const auto hidlDeadline = NN_TRY(hal::utils::makeExecutionFailure(convert(deadline)));
    const auto hidlLoopTimeoutDuration =
            NN_TRY(hal::utils::makeExecutionFailure(convert(loopTimeoutDuration)));

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> result =
            NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
    const bool preferSynchronous = true;

    // Execute synchronously if allowed.
    if (preferSynchronous) {
        result = executeSynchronously(hidlRequest, hidlMeasure, hidlDeadline,
                                      hidlLoopTimeoutDuration);
    }

    // Run asymchronous execution if execution has not already completed.
    if (!result.has_value()) {
        result = executeAsynchronously(hidlRequest, hidlMeasure, hidlDeadline,
                                       hidlLoopTimeoutDuration);
    }

    // Flush output buffers if suxcessful execution.
    if (result.has_value()) {
        NN_TRY(hal::utils::makeExecutionFailure(
                hal::utils::unflushDataFromSharedToPointer(request, maybeRequestInShared)));
    }

    return result;
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFenced(const nn::Request& request, const std::vector<nn::SyncFence>& waitFor,
                             nn::MeasureTiming measure, const nn::OptionalTimePoint& deadline,
                             const nn::OptionalTimeoutDuration& loopTimeoutDuration,
                             const nn::OptionalTimeoutDuration& timeoutDurationAfterFence) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    const nn::Request& requestInShared =
            NN_TRY(hal::utils::flushDataFromPointerToShared(&request, &maybeRequestInShared));

    const auto hidlRequest = NN_TRY(convert(requestInShared));
    const auto hidlWaitFor = NN_TRY(hal::utils::convertSyncFences(waitFor));
    const auto hidlMeasure = NN_TRY(V1_2::utils::convert(measure));
    const auto hidlDeadline = NN_TRY(convert(deadline));
    const auto hidlLoopTimeoutDuration = NN_TRY(convert(loopTimeoutDuration));
    const auto hidlTimeoutDurationAfterFence = NN_TRY(convert(timeoutDurationAfterFence));

    nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> result =
            NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "uninitialized";
    auto cb = [&result](ErrorStatus status, const hidl_handle& syncFence,
                        const sp<IFencedExecutionCallback>& callback) {
        if (status != ErrorStatus::NONE) {
            const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
            result = NN_ERROR(canonical) << "executeFenced failed with " << toString(status);
        } else {
            result = convertExecuteFencedResults(syncFence, callback);
        }
    };

    const auto ret = kPreparedModel->executeFenced(hidlRequest, hidlWaitFor, hidlMeasure,
                                                   hidlDeadline, hidlLoopTimeoutDuration,
                                                   hidlTimeoutDurationAfterFence, cb);
    HANDLE_TRANSPORT_FAILURE(ret);
    auto [syncFence, callback] = NN_TRY(std::move(result));

    // If executeFenced required the request memory to be moved into shared memory, block here until
    // the fenced execution has completed and flush the memory back.
    if (maybeRequestInShared.has_value()) {
        const auto state = syncFence.syncWait({});
        if (state != nn::SyncFence::FenceState::SIGNALED) {
            return NN_ERROR() << "syncWait failed with " << state;
        }
        NN_TRY(hal::utils::unflushDataFromSharedToPointer(request, maybeRequestInShared));
    }

    return std::make_pair(std::move(syncFence), std::move(callback));
}

std::any PreparedModel::getUnderlyingResource() const {
    sp<V1_3::IPreparedModel> resource = kPreparedModel;
    return resource;
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
