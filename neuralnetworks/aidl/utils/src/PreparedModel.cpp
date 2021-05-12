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

#include "PreparedModel.h"

#include "Burst.h"
#include "Callbacks.h"
#include "Conversions.h"
#include "Execution.h"
#include "ProtectCallback.h"
#include "Utils.h"

#include <aidl/android/hardware/neuralnetworks/Request.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

nn::GeneralResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> convertExecutionResults(
        const std::vector<OutputShape>& outputShapes, const Timing& timing) {
    return std::make_pair(NN_TRY(nn::convert(outputShapes)), NN_TRY(nn::convert(timing)));
}

nn::GeneralResult<std::pair<nn::Timing, nn::Timing>> convertFencedExecutionResults(
        ErrorStatus status, const aidl_hal::Timing& timingLaunched,
        const aidl_hal::Timing& timingFenced) {
    HANDLE_HAL_STATUS(status) << "fenced execution callback info failed with " << toString(status);
    return std::make_pair(NN_TRY(nn::convert(timingLaunched)), NN_TRY(nn::convert(timingFenced)));
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const PreparedModel>> PreparedModel::create(
        std::shared_ptr<aidl_hal::IPreparedModel> preparedModel) {
    if (preparedModel == nullptr) {
        return NN_ERROR()
               << "aidl_hal::utils::PreparedModel::create must have non-null preparedModel";
    }

    return std::make_shared<const PreparedModel>(PrivateConstructorTag{}, std::move(preparedModel));
}

PreparedModel::PreparedModel(PrivateConstructorTag /*tag*/,
                             std::shared_ptr<aidl_hal::IPreparedModel> preparedModel)
    : kPreparedModel(std::move(preparedModel)) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> PreparedModel::execute(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared =
            NN_TRY(hal::utils::makeExecutionFailure(hal::utils::convertRequestFromPointerToShared(
                    &request, nn::kDefaultRequestMemoryAlignment, nn::kDefaultRequestMemoryPadding,
                    &maybeRequestInShared, &relocation)));

    const auto aidlRequest = NN_TRY(hal::utils::makeExecutionFailure(convert(requestInShared)));
    const auto aidlMeasure = NN_TRY(hal::utils::makeExecutionFailure(convert(measure)));
    const auto aidlDeadline = NN_TRY(hal::utils::makeExecutionFailure(convert(deadline)));
    const auto aidlLoopTimeoutDuration =
            NN_TRY(hal::utils::makeExecutionFailure(convert(loopTimeoutDuration)));
    return executeInternal(aidlRequest, aidlMeasure, aidlDeadline, aidlLoopTimeoutDuration,
                           relocation);
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeInternal(const Request& request, bool measure, int64_t deadline,
                               int64_t loopTimeoutDuration,
                               const hal::utils::RequestRelocation& relocation) const {
    if (relocation.input) {
        relocation.input->flush();
    }

    ExecutionResult executionResult;
    const auto ret = kPreparedModel->executeSynchronously(request, measure, deadline,
                                                          loopTimeoutDuration, &executionResult);
    HANDLE_ASTATUS(ret) << "executeSynchronously failed";
    if (!executionResult.outputSufficientSize) {
        auto canonicalOutputShapes =
                nn::convert(executionResult.outputShapes).value_or(std::vector<nn::OutputShape>{});
        return NN_ERROR(nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE, std::move(canonicalOutputShapes))
               << "execution failed with " << nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;
    }
    auto [outputShapes, timing] = NN_TRY(hal::utils::makeExecutionFailure(
            convertExecutionResults(executionResult.outputShapes, executionResult.timing)));

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
            &request, nn::kDefaultRequestMemoryAlignment, nn::kDefaultRequestMemoryPadding,
            &maybeRequestInShared, &relocation));

    const auto aidlRequest = NN_TRY(convert(requestInShared));
    const auto aidlWaitFor = NN_TRY(convert(waitFor));
    const auto aidlMeasure = NN_TRY(convert(measure));
    const auto aidlDeadline = NN_TRY(convert(deadline));
    const auto aidlLoopTimeoutDuration = NN_TRY(convert(loopTimeoutDuration));
    const auto aidlTimeoutDurationAfterFence = NN_TRY(convert(timeoutDurationAfterFence));
    return executeFencedInternal(aidlRequest, aidlWaitFor, aidlMeasure, aidlDeadline,
                                 aidlLoopTimeoutDuration, aidlTimeoutDurationAfterFence,
                                 relocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFencedInternal(const Request& request,
                                     const std::vector<ndk::ScopedFileDescriptor>& waitFor,
                                     bool measure, int64_t deadline, int64_t loopTimeoutDuration,
                                     int64_t timeoutDurationAfterFence,
                                     const hal::utils::RequestRelocation& relocation) const {
    if (relocation.input) {
        relocation.input->flush();
    }

    FencedExecutionResult result;
    const auto ret =
            kPreparedModel->executeFenced(request, waitFor, measure, deadline, loopTimeoutDuration,
                                          timeoutDurationAfterFence, &result);
    HANDLE_ASTATUS(ret) << "executeFenced failed";

    auto resultSyncFence = nn::SyncFence::createAsSignaled();
    if (result.syncFence.get() != -1) {
        resultSyncFence = NN_TRY(nn::convert(result.syncFence));
    }

    auto callback = result.callback;
    if (callback == nullptr) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "callback is null";
    }

    // If executeFenced required the request memory to be moved into shared memory, block here until
    // the fenced execution has completed and flush the memory back.
    if (relocation.output) {
        const auto state = resultSyncFence.syncWait({});
        if (state != nn::SyncFence::FenceState::SIGNALED) {
            return NN_ERROR() << "syncWait failed with " << state;
        }
        relocation.output->flush();
    }

    // Create callback which can be used to retrieve the execution error status and timings.
    nn::ExecuteFencedInfoCallback resultCallback =
            [callback]() -> nn::GeneralResult<std::pair<nn::Timing, nn::Timing>> {
        ErrorStatus errorStatus;
        Timing timingLaunched;
        Timing timingFenced;
        const auto ret = callback->getExecutionInfo(&timingLaunched, &timingFenced, &errorStatus);
        HANDLE_ASTATUS(ret) << "fenced execution callback getExecutionInfo failed";
        return convertFencedExecutionResults(errorStatus, timingLaunched, timingFenced);
    };

    return std::make_pair(std::move(resultSyncFence), std::move(resultCallback));
}

nn::GeneralResult<nn::SharedExecution> PreparedModel::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared = NN_TRY(hal::utils::convertRequestFromPointerToShared(
            &request, nn::kDefaultRequestMemoryAlignment, nn::kDefaultRequestMemoryPadding,
            &maybeRequestInShared, &relocation));

    auto aidlRequest = NN_TRY(convert(requestInShared));
    auto aidlMeasure = NN_TRY(convert(measure));
    auto aidlLoopTimeoutDuration = NN_TRY(convert(loopTimeoutDuration));
    return Execution::create(shared_from_this(), std::move(aidlRequest), std::move(relocation),
                             aidlMeasure, aidlLoopTimeoutDuration);
}

nn::GeneralResult<nn::SharedBurst> PreparedModel::configureExecutionBurst() const {
    std::shared_ptr<IBurst> burst;
    const auto ret = kPreparedModel->configureExecutionBurst(&burst);
    HANDLE_ASTATUS(ret) << "configureExecutionBurst failed";
    return Burst::create(std::move(burst));
}

std::any PreparedModel::getUnderlyingResource() const {
    std::shared_ptr<aidl_hal::IPreparedModel> resource = kPreparedModel;
    return resource;
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
