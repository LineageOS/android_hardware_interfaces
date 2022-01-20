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
    HANDLE_STATUS_AIDL(status) << "fenced execution callback info failed with " << toString(status);
    return std::make_pair(NN_TRY(nn::convert(timingLaunched)), NN_TRY(nn::convert(timingFenced)));
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> handleExecutionResult(
        const ExecutionResult& result, const hal::utils::RequestRelocation& relocation) {
    if (!result.outputSufficientSize) {
        auto canonicalOutputShapes =
                nn::convert(result.outputShapes).value_or(std::vector<nn::OutputShape>{});
        return NN_ERROR(nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE, std::move(canonicalOutputShapes))
               << "execution failed with " << nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;
    }
    auto [outputShapes, timing] =
            NN_TRY(convertExecutionResults(result.outputShapes, result.timing));

    if (relocation.output) {
        relocation.output->flush();
    }
    return std::make_pair(std::move(outputShapes), timing);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
handleFencedExecutionResult(const FencedExecutionResult& result,
                            const hal::utils::RequestRelocation& relocation) {
    auto resultSyncFence = nn::SyncFence::createAsSignaled();
    if (result.syncFence.get() != -1) {
        resultSyncFence = nn::SyncFence::create(NN_TRY(nn::convert(result.syncFence))).value();
    }

    auto callback = result.callback;
    if (callback == nullptr) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "callback is null";
    }

    // If computeFenced required the request memory to be moved into shared memory, block here until
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

}  // namespace

nn::GeneralResult<std::shared_ptr<const PreparedModel>> PreparedModel::create(
        std::shared_ptr<aidl_hal::IPreparedModel> preparedModel, nn::Version featureLevel) {
    if (preparedModel == nullptr) {
        return NN_ERROR()
               << "aidl_hal::utils::PreparedModel::create must have non-null preparedModel";
    }

    return std::make_shared<const PreparedModel>(PrivateConstructorTag{}, std::move(preparedModel),
                                                 featureLevel);
}

PreparedModel::PreparedModel(PrivateConstructorTag /*tag*/,
                             std::shared_ptr<aidl_hal::IPreparedModel> preparedModel,
                             nn::Version featureLevel)
    : kPreparedModel(std::move(preparedModel)), kFeatureLevel(featureLevel) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> PreparedModel::execute(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalTimePoint& deadline, const nn::OptionalDuration& loopTimeoutDuration,
        const std::vector<nn::TokenValuePair>& hints,
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared = NN_TRY(hal::utils::convertRequestFromPointerToShared(
            &request, nn::kDefaultRequestMemoryAlignment, nn::kDefaultRequestMemoryPadding,
            &maybeRequestInShared, &relocation));

    const auto aidlRequest = NN_TRY(convert(requestInShared));
    const auto aidlMeasure = NN_TRY(convert(measure));
    const auto aidlDeadline = NN_TRY(convert(deadline));
    const auto aidlLoopTimeoutDuration = NN_TRY(convert(loopTimeoutDuration));
    return executeInternal(aidlRequest, aidlMeasure, aidlDeadline, aidlLoopTimeoutDuration, hints,
                           extensionNameToPrefix, relocation);
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeInternal(const Request& request, bool measure, int64_t deadline,
                               int64_t loopTimeoutDuration,
                               const std::vector<nn::TokenValuePair>& hints,
                               const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix,
                               const hal::utils::RequestRelocation& relocation) const {
    if (relocation.input) {
        relocation.input->flush();
    }

    ExecutionResult executionResult;
    if (kFeatureLevel.level >= nn::Version::Level::FEATURE_LEVEL_8) {
        auto aidlHints = NN_TRY(convert(hints));
        auto aidlExtensionPrefix = NN_TRY(convert(extensionNameToPrefix));
        const auto ret = kPreparedModel->executeSynchronouslyWithConfig(
                request,
                {measure, loopTimeoutDuration, std::move(aidlHints),
                 std::move(aidlExtensionPrefix)},
                deadline, &executionResult);
        HANDLE_ASTATUS(ret) << "executeSynchronouslyWithConfig failed";
    } else {
        const auto ret = kPreparedModel->executeSynchronously(
                request, measure, deadline, loopTimeoutDuration, &executionResult);
        HANDLE_ASTATUS(ret) << "executeSynchronously failed";
    }
    return handleExecutionResult(executionResult, relocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFenced(
        const nn::Request& request, const std::vector<nn::SyncFence>& waitFor,
        nn::MeasureTiming measure, const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& loopTimeoutDuration,
        const nn::OptionalDuration& timeoutDurationAfterFence,
        const std::vector<nn::TokenValuePair>& hints,
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const {
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
                                 aidlLoopTimeoutDuration, aidlTimeoutDurationAfterFence, hints,
                                 extensionNameToPrefix, relocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFencedInternal(
        const Request& request, const std::vector<ndk::ScopedFileDescriptor>& waitFor, bool measure,
        int64_t deadline, int64_t loopTimeoutDuration, int64_t timeoutDurationAfterFence,
        const std::vector<nn::TokenValuePair>& hints,
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix,
        const hal::utils::RequestRelocation& relocation) const {
    if (relocation.input) {
        relocation.input->flush();
    }

    FencedExecutionResult result;
    if (kFeatureLevel.level >= nn::Version::Level::FEATURE_LEVEL_8) {
        auto aidlHints = NN_TRY(convert(hints));
        auto aidlExtensionPrefix = NN_TRY(convert(extensionNameToPrefix));
        const auto ret = kPreparedModel->executeFencedWithConfig(
                request, waitFor,
                {measure, loopTimeoutDuration, std::move(aidlHints),
                 std::move(aidlExtensionPrefix)},
                deadline, timeoutDurationAfterFence, &result);
        HANDLE_ASTATUS(ret) << "executeFencedWithConfig failed";
    } else {
        const auto ret = kPreparedModel->executeFenced(request, waitFor, measure, deadline,
                                                       loopTimeoutDuration,
                                                       timeoutDurationAfterFence, &result);
        HANDLE_ASTATUS(ret) << "executeFenced failed";
    }
    return handleFencedExecutionResult(result, relocation);
}

nn::GeneralResult<nn::SharedExecution> PreparedModel::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration,
        const std::vector<nn::TokenValuePair>& hints,
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared = NN_TRY(hal::utils::convertRequestFromPointerToShared(
            &request, nn::kDefaultRequestMemoryAlignment, nn::kDefaultRequestMemoryPadding,
            &maybeRequestInShared, &relocation));

    auto aidlRequest = NN_TRY(convert(requestInShared));
    auto aidlMeasure = NN_TRY(convert(measure));
    auto aidlLoopTimeoutDuration = NN_TRY(convert(loopTimeoutDuration));

    if (kFeatureLevel.level >= nn::Version::Level::FEATURE_LEVEL_8) {
        std::shared_ptr<IExecution> execution;
        auto aidlHints = NN_TRY(convert(hints));
        auto aidlExtensionPrefix = NN_TRY(convert(extensionNameToPrefix));

        const auto ret = kPreparedModel->createReusableExecution(
                aidlRequest,
                {aidlMeasure, aidlLoopTimeoutDuration, std::move(aidlHints),
                 std::move(aidlExtensionPrefix)},
                &execution);
        HANDLE_ASTATUS(ret) << "createReusableExecution failed";
        return Execution::create(std::move(execution), std::move(relocation));
    }

    return ExecutionWithCachedRequest::create(shared_from_this(), std::move(aidlRequest),
                                              std::move(relocation), aidlMeasure,
                                              aidlLoopTimeoutDuration);
}

nn::GeneralResult<nn::SharedBurst> PreparedModel::configureExecutionBurst() const {
    std::shared_ptr<IBurst> burst;
    const auto ret = kPreparedModel->configureExecutionBurst(&burst);
    HANDLE_ASTATUS(ret) << "configureExecutionBurst failed";
    return Burst::create(std::move(burst), kFeatureLevel);
}

std::any PreparedModel::getUnderlyingResource() const {
    std::shared_ptr<aidl_hal::IPreparedModel> resource = kPreparedModel;
    return resource;
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> Execution::compute(
        const nn::OptionalTimePoint& deadline) const {
    const auto aidlDeadline = NN_TRY(convert(deadline));

    if (kRelocation.input) {
        kRelocation.input->flush();
    }

    ExecutionResult executionResult;
    auto ret = kExecution->executeSynchronously(aidlDeadline, &executionResult);
    HANDLE_ASTATUS(ret) << "executeSynchronously failed";
    return handleExecutionResult(executionResult, kRelocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> Execution::computeFenced(
        const std::vector<nn::SyncFence>& waitFor, const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& timeoutDurationAfterFence) const {
    const auto aidlWaitFor = NN_TRY(convert(waitFor));
    const auto aidlDeadline = NN_TRY(convert(deadline));
    const auto aidlTimeoutDurationAfterFence = NN_TRY(convert(timeoutDurationAfterFence));

    if (kRelocation.input) {
        kRelocation.input->flush();
    }

    FencedExecutionResult result;
    const auto ret = kExecution->executeFenced(aidlWaitFor, aidlDeadline,
                                               aidlTimeoutDurationAfterFence, &result);
    HANDLE_ASTATUS(ret) << "executeFenced failed";
    return handleFencedExecutionResult(result, kRelocation);
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
