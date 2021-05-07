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

#include "ResilientExecution.h"

#include "InvalidBurst.h"
#include "ResilientBurst.h"

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>
#include <nnapi/IExecution.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {
namespace {

template <typename FnType>
auto protect(const ResilientExecution& resilientExecution, const FnType& fn)
        -> decltype(fn(*resilientExecution.getExecution())) {
    auto execution = resilientExecution.getExecution();
    auto result = fn(*execution);

    // Immediately return if prepared model is not dead.
    if (result.has_value() || result.error().code != nn::ErrorStatus::DEAD_OBJECT) {
        return result;
    }

    // Attempt recovery and return if it fails.
    auto maybeExecution = resilientExecution.recover(execution.get());
    if (!maybeExecution.has_value()) {
        const auto& [message, code] = maybeExecution.error();
        std::ostringstream oss;
        oss << ", and failed to recover dead prepared model with error " << code << ": " << message;
        result.error().message += oss.str();
        return result;
    }
    execution = std::move(maybeExecution).value();

    return fn(*execution);
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const ResilientExecution>> ResilientExecution::create(
        Factory makeExecution) {
    if (makeExecution == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "utils::ResilientExecution::create must have non-empty makeExecution";
    }
    auto execution = NN_TRY(makeExecution());
    CHECK(execution != nullptr);
    return std::make_shared<ResilientExecution>(PrivateConstructorTag{}, std::move(makeExecution),
                                                std::move(execution));
}

ResilientExecution::ResilientExecution(PrivateConstructorTag /*tag*/, Factory makeExecution,
                                       nn::SharedExecution execution)
    : kMakeExecution(std::move(makeExecution)), mExecution(std::move(execution)) {
    CHECK(kMakeExecution != nullptr);
    CHECK(mExecution != nullptr);
}

nn::SharedExecution ResilientExecution::getExecution() const {
    std::lock_guard guard(mMutex);
    return mExecution;
}

nn::GeneralResult<nn::SharedExecution> ResilientExecution::recover(
        const nn::IExecution* failingExecution) const {
    std::lock_guard guard(mMutex);

    // Another caller updated the failing prepared model.
    if (mExecution.get() != failingExecution) {
        return mExecution;
    }

    mExecution = NN_TRY(kMakeExecution());
    return mExecution;
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
ResilientExecution::compute(const nn::OptionalTimePoint& deadline) const {
    const auto fn = [&deadline](const nn::IExecution& execution) {
        return execution.compute(deadline);
    };
    return protect(*this, fn);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
ResilientExecution::computeFenced(const std::vector<nn::SyncFence>& waitFor,
                                  const nn::OptionalTimePoint& deadline,
                                  const nn::OptionalDuration& timeoutDurationAfterFence) const {
    const auto fn = [&waitFor, &deadline,
                     &timeoutDurationAfterFence](const nn::IExecution& execution) {
        return execution.computeFenced(waitFor, deadline, timeoutDurationAfterFence);
    };
    return protect(*this, fn);
}

bool ResilientExecution::isValidInternal() const {
    return true;
}

}  // namespace android::hardware::neuralnetworks::utils
