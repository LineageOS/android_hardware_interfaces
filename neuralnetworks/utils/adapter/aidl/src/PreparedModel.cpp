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
#include "Execution.h"

#include <aidl/android/hardware/neuralnetworks/BnFencedExecutionCallback.h>
#include <aidl/android/hardware/neuralnetworks/BnPreparedModel.h>
#include <aidl/android/hardware/neuralnetworks/ExecutionResult.h>
#include <aidl/android/hardware/neuralnetworks/FencedExecutionResult.h>
#include <aidl/android/hardware/neuralnetworks/IBurst.h>
#include <aidl/android/hardware/neuralnetworks/Request.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IExecution.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/aidl/Conversions.h>
#include <nnapi/hal/aidl/Utils.h>

#include <memory>
#include <utility>
#include <vector>

namespace aidl::android::hardware::neuralnetworks::adapter {
namespace {

class FencedExecutionCallback : public BnFencedExecutionCallback {
  public:
    FencedExecutionCallback(nn::ExecuteFencedInfoCallback callback)
        : kCallback(std::move(callback)) {}

    ndk::ScopedAStatus getExecutionInfo(Timing* timingLaunched, Timing* timingFenced,
                                        ErrorStatus* errorStatus) override {
        const auto result = kCallback();
        if (result.ok()) {
            const auto& [nnTimingLaunched, nnTimingFenced] = result.value();
            *timingLaunched = utils::convert(nnTimingLaunched).value();
            *timingFenced = utils::convert(nnTimingFenced).value();
            *errorStatus = ErrorStatus::NONE;
        } else {
            constexpr auto kNoTiming = Timing{.timeOnDeviceNs = -1, .timeInDriverNs = -1};
            const auto& [message, code] = result.error();
            LOG(ERROR) << "getExecutionInfo failed with " << code << ": " << message;
            const auto aidlStatus = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
            *timingLaunched = kNoTiming;
            *timingFenced = kNoTiming;
            *errorStatus = aidlStatus;
        }
        return ndk::ScopedAStatus::ok();
    }

  private:
    const nn::ExecuteFencedInfoCallback kCallback;
};

template <typename Type>
auto convertInput(const Type& object) -> decltype(nn::convert(std::declval<Type>())) {
    auto result = nn::convert(object);
    if (!result.has_value()) {
        result.error().code = nn::ErrorStatus::INVALID_ARGUMENT;
    }
    return result;
}

nn::GeneralResult<std::vector<nn::SyncFence>> convertSyncFences(
        const std::vector<ndk::ScopedFileDescriptor>& waitFor) {
    auto handles = NN_TRY(convertInput(waitFor));

    constexpr auto valid = [](const nn::SharedHandle& handle) {
        return handle != nullptr && handle->ok();
    };
    if (!std::all_of(handles.begin(), handles.end(), valid)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid sync fence";
    }

    std::vector<nn::SyncFence> syncFences;
    syncFences.reserve(waitFor.size());
    for (auto& handle : handles) {
        syncFences.push_back(nn::SyncFence::create(std::move(handle)).value());
    }
    return syncFences;
}

nn::Duration makeDuration(int64_t durationNs) {
    return nn::Duration(std::chrono::nanoseconds(durationNs));
}

nn::GeneralResult<nn::OptionalDuration> makeOptionalDuration(int64_t durationNs) {
    if (durationNs < -1) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid duration " << durationNs;
    }
    return durationNs < 0 ? nn::OptionalDuration{} : makeDuration(durationNs);
}

nn::GeneralResult<nn::OptionalTimePoint> makeOptionalTimePoint(int64_t durationNs) {
    if (durationNs < -1) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid time point " << durationNs;
    }
    return durationNs < 0 ? nn::OptionalTimePoint{} : nn::TimePoint(makeDuration(durationNs));
}

nn::ExecutionResult<ExecutionResult> executeSynchronously(
        const nn::IPreparedModel& preparedModel, const Request& request, bool measureTiming,
        int64_t deadlineNs, int64_t loopTimeoutDurationNs, const std::vector<TokenValuePair>& hints,
        const std::vector<ExtensionNameAndPrefix>& extensionNameToPrefix) {
    const auto nnRequest = NN_TRY(convertInput(request));
    const auto nnMeasureTiming = measureTiming ? nn::MeasureTiming::YES : nn::MeasureTiming::NO;
    const auto nnDeadline = NN_TRY(makeOptionalTimePoint(deadlineNs));
    const auto nnLoopTimeoutDuration = NN_TRY(makeOptionalDuration(loopTimeoutDurationNs));
    auto nnHints = NN_TRY(convertInput(hints));
    auto nnExtensionNameToPrefix = NN_TRY(convertInput(extensionNameToPrefix));

    const auto result =
            preparedModel.execute(nnRequest, nnMeasureTiming, nnDeadline, nnLoopTimeoutDuration,
                                  nnHints, nnExtensionNameToPrefix);

    if (!result.ok() && result.error().code == nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        const auto& [message, code, outputShapes] = result.error();
        LOG(ERROR) << "executeSynchronously failed with " << code << ": " << message;
        return ExecutionResult{.outputSufficientSize = false,
                               .outputShapes = utils::convert(outputShapes).value(),
                               .timing = {.timeInDriverNs = -1, .timeOnDeviceNs = -1}};
    }

    const auto& [outputShapes, timing] = NN_TRY(result);
    return ExecutionResult{.outputSufficientSize = true,
                           .outputShapes = utils::convert(outputShapes).value(),
                           .timing = utils::convert(timing).value()};
}

nn::GeneralResult<FencedExecutionResult> executeFenced(
        const nn::IPreparedModel& preparedModel, const Request& request,
        const std::vector<ndk::ScopedFileDescriptor>& waitFor, bool measureTiming,
        int64_t deadlineNs, int64_t loopTimeoutDurationNs, int64_t durationNs,
        const std::vector<TokenValuePair>& hints,
        const std::vector<ExtensionNameAndPrefix>& extensionNameToPrefix) {
    const auto nnRequest = NN_TRY(convertInput(request));
    const auto nnWaitFor = NN_TRY(convertSyncFences(waitFor));
    const auto nnMeasureTiming = measureTiming ? nn::MeasureTiming::YES : nn::MeasureTiming::NO;
    const auto nnDeadline = NN_TRY(makeOptionalTimePoint(deadlineNs));
    const auto nnLoopTimeoutDuration = NN_TRY(makeOptionalDuration(loopTimeoutDurationNs));
    const auto nnDuration = NN_TRY(makeOptionalDuration(durationNs));
    auto nnHints = NN_TRY(convertInput(hints));
    auto nnExtensionNameToPrefix = NN_TRY(convertInput(extensionNameToPrefix));

    auto [syncFence, executeFencedInfoCallback] = NN_TRY(preparedModel.executeFenced(
            nnRequest, nnWaitFor, nnMeasureTiming, nnDeadline, nnLoopTimeoutDuration, nnDuration,
            nnHints, nnExtensionNameToPrefix));

    ndk::ScopedFileDescriptor fileDescriptor;
    if (syncFence.hasFd()) {
        auto uniqueFd = NN_TRY(nn::dupFd(syncFence.getFd()));
        fileDescriptor = ndk::ScopedFileDescriptor(uniqueFd.release());
    }

    return FencedExecutionResult{.callback = ndk::SharedRefBase::make<FencedExecutionCallback>(
                                         std::move(executeFencedInfoCallback)),
                                 .syncFence = std::move(fileDescriptor)};
}

nn::GeneralResult<nn::SharedExecution> createReusableExecution(
        const nn::IPreparedModel& preparedModel, const Request& request, bool measureTiming,
        int64_t loopTimeoutDurationNs, const std::vector<TokenValuePair>& hints,
        const std::vector<ExtensionNameAndPrefix>& extensionNameToPrefix) {
    const auto nnRequest = NN_TRY(convertInput(request));
    const auto nnMeasureTiming = measureTiming ? nn::MeasureTiming::YES : nn::MeasureTiming::NO;
    const auto nnLoopTimeoutDuration = NN_TRY(makeOptionalDuration(loopTimeoutDurationNs));
    auto nnHints = NN_TRY(convertInput(hints));
    auto nnExtensionNameToPrefix = NN_TRY(convertInput(extensionNameToPrefix));

    return preparedModel.createReusableExecution(nnRequest, nnMeasureTiming, nnLoopTimeoutDuration,
                                                 nnHints, nnExtensionNameToPrefix);
}

nn::ExecutionResult<ExecutionResult> executeSynchronously(const nn::IExecution& execution,
                                                          int64_t deadlineNs) {
    const auto nnDeadline = NN_TRY(makeOptionalTimePoint(deadlineNs));

    const auto result = execution.compute(nnDeadline);

    if (!result.ok() && result.error().code == nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        const auto& [message, code, outputShapes] = result.error();
        LOG(ERROR) << "executeSynchronously failed with " << code << ": " << message;
        return ExecutionResult{.outputSufficientSize = false,
                               .outputShapes = utils::convert(outputShapes).value(),
                               .timing = {.timeInDriverNs = -1, .timeOnDeviceNs = -1}};
    }

    const auto& [outputShapes, timing] = NN_TRY(result);
    return ExecutionResult{.outputSufficientSize = true,
                           .outputShapes = utils::convert(outputShapes).value(),
                           .timing = utils::convert(timing).value()};
}

nn::GeneralResult<FencedExecutionResult> executeFenced(
        const nn::IExecution& execution, const std::vector<ndk::ScopedFileDescriptor>& waitFor,
        int64_t deadlineNs, int64_t durationNs) {
    const auto nnWaitFor = NN_TRY(convertSyncFences(waitFor));
    const auto nnDeadline = NN_TRY(makeOptionalTimePoint(deadlineNs));
    const auto nnDuration = NN_TRY(makeOptionalDuration(durationNs));

    auto [syncFence, executeFencedInfoCallback] =
            NN_TRY(execution.computeFenced(nnWaitFor, nnDeadline, nnDuration));

    ndk::ScopedFileDescriptor fileDescriptor;
    if (syncFence.hasFd()) {
        auto uniqueFd = NN_TRY(nn::dupFd(syncFence.getFd()));
        fileDescriptor = ndk::ScopedFileDescriptor(uniqueFd.release());
    }

    return FencedExecutionResult{.callback = ndk::SharedRefBase::make<FencedExecutionCallback>(
                                         std::move(executeFencedInfoCallback)),
                                 .syncFence = std::move(fileDescriptor)};
}

}  // namespace

PreparedModel::PreparedModel(nn::SharedPreparedModel preparedModel)
    : kPreparedModel(std::move(preparedModel)) {
    CHECK(kPreparedModel != nullptr);
}

ndk::ScopedAStatus PreparedModel::executeSynchronously(const Request& request, bool measureTiming,
                                                       int64_t deadlineNs,
                                                       int64_t loopTimeoutDurationNs,
                                                       ExecutionResult* executionResult) {
    auto result = adapter::executeSynchronously(*kPreparedModel, request, measureTiming, deadlineNs,
                                                loopTimeoutDurationNs, {}, {});
    if (!result.has_value()) {
        const auto& [message, code, _] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *executionResult = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PreparedModel::executeFenced(
        const Request& request, const std::vector<ndk::ScopedFileDescriptor>& waitFor,
        bool measureTiming, int64_t deadlineNs, int64_t loopTimeoutDurationNs, int64_t durationNs,
        FencedExecutionResult* executionResult) {
    auto result = adapter::executeFenced(*kPreparedModel, request, waitFor, measureTiming,
                                         deadlineNs, loopTimeoutDurationNs, durationNs, {}, {});
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *executionResult = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PreparedModel::executeSynchronouslyWithConfig(const Request& request,
                                                                 const ExecutionConfig& config,
                                                                 int64_t deadlineNs,
                                                                 ExecutionResult* executionResult) {
    auto result = adapter::executeSynchronously(
            *kPreparedModel, request, config.measureTiming, deadlineNs,
            config.loopTimeoutDurationNs, config.executionHints, config.extensionNameToPrefix);
    if (!result.has_value()) {
        const auto& [message, code, _] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *executionResult = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PreparedModel::executeFencedWithConfig(
        const Request& request, const std::vector<ndk::ScopedFileDescriptor>& waitFor,
        const ExecutionConfig& config, int64_t deadlineNs, int64_t durationNs,
        FencedExecutionResult* executionResult) {
    auto result = adapter::executeFenced(*kPreparedModel, request, waitFor, config.measureTiming,
                                         deadlineNs, config.loopTimeoutDurationNs, durationNs,
                                         config.executionHints, config.extensionNameToPrefix);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *executionResult = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PreparedModel::configureExecutionBurst(std::shared_ptr<IBurst>* burst) {
    auto result = kPreparedModel->configureExecutionBurst();
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *burst = ndk::SharedRefBase::make<Burst>(std::move(result).value());
    return ndk::ScopedAStatus::ok();
}

nn::SharedPreparedModel PreparedModel::getUnderlyingPreparedModel() const {
    return kPreparedModel;
}

ndk::ScopedAStatus PreparedModel::createReusableExecution(const Request& request,
                                                          const ExecutionConfig& config,
                                                          std::shared_ptr<IExecution>* execution) {
    auto result = adapter::createReusableExecution(
            *kPreparedModel, request, config.measureTiming, config.loopTimeoutDurationNs,
            config.executionHints, config.extensionNameToPrefix);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *execution = ndk::SharedRefBase::make<Execution>(std::move(result).value());
    return ndk::ScopedAStatus::ok();
}

Execution::Execution(nn::SharedExecution execution) : kExecution(std::move(execution)) {
    CHECK(kExecution != nullptr);
}

ndk::ScopedAStatus Execution::executeSynchronously(int64_t deadlineNs,
                                                   ExecutionResult* executionResult) {
    auto result = adapter::executeSynchronously(*kExecution, deadlineNs);
    if (!result.has_value()) {
        const auto& [message, code, _] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *executionResult = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Execution::executeFenced(const std::vector<ndk::ScopedFileDescriptor>& waitFor,
                                            int64_t deadlineNs, int64_t durationNs,
                                            FencedExecutionResult* executionResult) {
    auto result = adapter::executeFenced(*kExecution, waitFor, deadlineNs, durationNs);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *executionResult = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::neuralnetworks::adapter
