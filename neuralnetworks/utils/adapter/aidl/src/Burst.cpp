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

#include "Burst.h"

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IBurst.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/aidl/Conversions.h>
#include <nnapi/hal/aidl/Utils.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <variant>

namespace aidl::android::hardware::neuralnetworks::adapter {
namespace {

using Value = Burst::ThreadSafeMemoryCache::Value;

template <typename Type>
auto convertInput(const Type& object) -> decltype(nn::convert(std::declval<Type>())) {
    auto result = nn::convert(object);
    if (!result.has_value()) {
        result.error().code = nn::ErrorStatus::INVALID_ARGUMENT;
    }
    return result;
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

std::vector<nn::IBurst::OptionalCacheHold> ensureAllMemoriesAreCached(
        nn::Request* request, const std::vector<int64_t>& memoryIdentifierTokens,
        const nn::IBurst& burst, const Burst::ThreadSafeMemoryCache& cache) {
    std::vector<nn::IBurst::OptionalCacheHold> holds;
    holds.reserve(memoryIdentifierTokens.size());

    for (size_t i = 0; i < memoryIdentifierTokens.size(); ++i) {
        const auto& pool = request->pools[i];
        const auto token = memoryIdentifierTokens[i];
        constexpr int64_t kNoToken = -1;
        if (token == kNoToken || !std::holds_alternative<nn::SharedMemory>(pool)) {
            continue;
        }

        const auto& memory = std::get<nn::SharedMemory>(pool);
        auto [storedMemory, hold] = cache.add(token, memory, burst);

        request->pools[i] = std::move(storedMemory);
        holds.push_back(std::move(hold));
    }

    return holds;
}

nn::ExecutionResult<ExecutionResult> executeSynchronously(
        const nn::IBurst& burst, const Burst::ThreadSafeMemoryCache& cache, const Request& request,
        const std::vector<int64_t>& memoryIdentifierTokens, bool measureTiming, int64_t deadlineNs,
        int64_t loopTimeoutDurationNs, const std::vector<TokenValuePair>& hints,
        const std::vector<ExtensionNameAndPrefix>& extensionNameToPrefix) {
    if (request.pools.size() != memoryIdentifierTokens.size()) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "request.pools.size() != memoryIdentifierTokens.size()";
    }
    if (!std::all_of(memoryIdentifierTokens.begin(), memoryIdentifierTokens.end(),
                     [](int64_t token) { return token >= -1; })) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid memoryIdentifierTokens";
    }

    auto nnRequest = NN_TRY(convertInput(request));
    const auto nnMeasureTiming = measureTiming ? nn::MeasureTiming::YES : nn::MeasureTiming::NO;
    const auto nnDeadline = NN_TRY(makeOptionalTimePoint(deadlineNs));
    const auto nnLoopTimeoutDuration = NN_TRY(makeOptionalDuration(loopTimeoutDurationNs));
    auto nnHints = NN_TRY(convertInput(hints));
    auto nnExtensionNameToPrefix = NN_TRY(convertInput(extensionNameToPrefix));

    const auto hold = ensureAllMemoriesAreCached(&nnRequest, memoryIdentifierTokens, burst, cache);

    const auto result = burst.execute(nnRequest, nnMeasureTiming, nnDeadline, nnLoopTimeoutDuration,
                                      nnHints, nnExtensionNameToPrefix);

    if (!result.ok() && result.error().code == nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        const auto& [message, code, outputShapes] = result.error();
        return ExecutionResult{.outputSufficientSize = false,
                               .outputShapes = utils::convert(outputShapes).value(),
                               .timing = {.timeInDriverNs = -1, .timeOnDeviceNs = -1}};
    }

    const auto& [outputShapes, timing] = NN_TRY(result);
    return ExecutionResult{.outputSufficientSize = true,
                           .outputShapes = utils::convert(outputShapes).value(),
                           .timing = utils::convert(timing).value()};
}

}  // namespace

Value Burst::ThreadSafeMemoryCache::add(int64_t token, const nn::SharedMemory& memory,
                                        const nn::IBurst& burst) const {
    std::lock_guard guard(mMutex);
    if (const auto it = mCache.find(token); it != mCache.end()) {
        return it->second;
    }
    auto hold = burst.cacheMemory(memory);
    auto [it, _] = mCache.emplace(token, std::make_pair(memory, std::move(hold)));
    return it->second;
}

void Burst::ThreadSafeMemoryCache::remove(int64_t token) const {
    std::lock_guard guard(mMutex);
    mCache.erase(token);
}

Burst::Burst(nn::SharedBurst burst) : kBurst(std::move(burst)) {
    CHECK(kBurst != nullptr);
}

ndk::ScopedAStatus Burst::executeSynchronously(const Request& request,
                                               const std::vector<int64_t>& memoryIdentifierTokens,
                                               bool measureTiming, int64_t deadlineNs,
                                               int64_t loopTimeoutDurationNs,
                                               ExecutionResult* executionResult) {
    auto result =
            adapter::executeSynchronously(*kBurst, kMemoryCache, request, memoryIdentifierTokens,
                                          measureTiming, deadlineNs, loopTimeoutDurationNs, {}, {});
    if (!result.has_value()) {
        auto [message, code, _] = std::move(result).error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *executionResult = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Burst::executeSynchronouslyWithConfig(
        const Request& request, const std::vector<int64_t>& memoryIdentifierTokens,
        const ExecutionConfig& config, int64_t deadlineNs, ExecutionResult* executionResult) {
    auto result = adapter::executeSynchronously(
            *kBurst, kMemoryCache, request, memoryIdentifierTokens, config.measureTiming,
            deadlineNs, config.loopTimeoutDurationNs, config.executionHints,
            config.extensionNameToPrefix);
    if (!result.has_value()) {
        auto [message, code, _] = std::move(result).error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    *executionResult = std::move(result).value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Burst::releaseMemoryResource(int64_t memoryIdentifierToken) {
    if (memoryIdentifierToken < -1) {
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(ErrorStatus::INVALID_ARGUMENT),
                "Invalid memoryIdentifierToken");
    }
    kMemoryCache.remove(memoryIdentifierToken);
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::neuralnetworks::adapter
