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

#include "ResilientBurst.h"

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>
#include <nnapi/IBurst.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

#include "InvalidExecution.h"
#include "ResilientExecution.h"

namespace android::hardware::neuralnetworks::utils {
namespace {

template <typename FnType>
auto protect(const ResilientBurst& resilientBurst, const FnType& fn)
        -> decltype(fn(*resilientBurst.getBurst())) {
    auto burst = resilientBurst.getBurst();
    auto result = fn(*burst);

    // Immediately return if burst is not dead.
    if (result.has_value() || result.error().code != nn::ErrorStatus::DEAD_OBJECT) {
        return result;
    }

    // Attempt recovery and return if it fails.
    auto maybeBurst = resilientBurst.recover(burst.get());
    if (!maybeBurst.has_value()) {
        const auto& [message, code] = maybeBurst.error();
        std::ostringstream oss;
        oss << ", and failed to recover dead burst object with error " << code << ": " << message;
        result.error().message += oss.str();
        return result;
    }
    burst = std::move(maybeBurst).value();

    return fn(*burst);
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const ResilientBurst>> ResilientBurst::create(Factory makeBurst) {
    if (makeBurst == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "utils::ResilientBurst::create must have non-empty makeBurst";
    }
    auto burst = NN_TRY(makeBurst());
    CHECK(burst != nullptr);
    return std::make_shared<ResilientBurst>(PrivateConstructorTag{}, std::move(makeBurst),
                                            std::move(burst));
}

ResilientBurst::ResilientBurst(PrivateConstructorTag /*tag*/, Factory makeBurst,
                               nn::SharedBurst burst)
    : kMakeBurst(std::move(makeBurst)), mBurst(std::move(burst)) {
    CHECK(kMakeBurst != nullptr);
    CHECK(mBurst != nullptr);
}

nn::SharedBurst ResilientBurst::getBurst() const {
    std::lock_guard guard(mMutex);
    return mBurst;
}

nn::GeneralResult<nn::SharedBurst> ResilientBurst::recover(const nn::IBurst* failingBurst) const {
    std::lock_guard guard(mMutex);

    // Another caller updated the failing burst.
    if (mBurst.get() != failingBurst) {
        return mBurst;
    }

    mBurst = NN_TRY(kMakeBurst());
    return mBurst;
}

ResilientBurst::OptionalCacheHold ResilientBurst::cacheMemory(
        const nn::SharedMemory& memory) const {
    return getBurst()->cacheMemory(memory);
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> ResilientBurst::execute(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    const auto fn = [&request, measure, deadline, loopTimeoutDuration](const nn::IBurst& burst) {
        return burst.execute(request, measure, deadline, loopTimeoutDuration);
    };
    return protect(*this, fn);
}

nn::GeneralResult<nn::SharedExecution> ResilientBurst::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration) const {
#if 0
    auto self = shared_from_this();
    ResilientExecution::Factory makeExecution =
            [burst = std::move(self), request, measure, loopTimeoutDuration] {
        return burst->createReusableExecutionInternal(request, measure, loopTimeoutDuration);
    };
    return ResilientExecution::create(std::move(makeExecution));
#else
    return createReusableExecutionInternal(request, measure, loopTimeoutDuration);
#endif
}

nn::GeneralResult<nn::SharedExecution> ResilientBurst::createReusableExecutionInternal(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    if (!isValidInternal()) {
        return std::make_shared<const InvalidExecution>();
    }
    const auto fn = [&request, measure, &loopTimeoutDuration](const nn::IBurst& burst) {
        return burst.createReusableExecution(request, measure, loopTimeoutDuration);
    };
    return protect(*this, fn);
}

bool ResilientBurst::isValidInternal() const {
    return true;
}

}  // namespace android::hardware::neuralnetworks::utils
