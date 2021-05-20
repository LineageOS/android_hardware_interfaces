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

#include "Conversions.h"
#include "Utils.h"

#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IBurst.h>
#include <nnapi/IExecution.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/HandleError.h>

#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

class BurstExecution final : public nn::IExecution,
                             public std::enable_shared_from_this<BurstExecution> {
    struct PrivateConstructorTag {};

  public:
    static nn::GeneralResult<std::shared_ptr<const BurstExecution>> create(
            std::shared_ptr<const Burst> burst, Request request,
            std::vector<int64_t> memoryIdentifierTokens, bool measure, int64_t loopTimeoutDuration,
            hal::utils::RequestRelocation relocation,
            std::vector<Burst::OptionalCacheHold> cacheHolds);

    BurstExecution(PrivateConstructorTag tag, std::shared_ptr<const Burst> burst, Request request,
                   std::vector<int64_t> memoryIdentifierTokens, bool measure,
                   int64_t loopTimeoutDuration, hal::utils::RequestRelocation relocation,
                   std::vector<Burst::OptionalCacheHold> cacheHolds);

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> compute(
            const nn::OptionalTimePoint& deadline) const override;

    nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>> computeFenced(
            const std::vector<nn::SyncFence>& waitFor, const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& timeoutDurationAfterFence) const override;

  private:
    const std::shared_ptr<const Burst> kBurst;
    const Request kRequest;
    const std::vector<int64_t> kMemoryIdentifierTokens;
    const bool kMeasure;
    const int64_t kLoopTimeoutDuration;
    const hal::utils::RequestRelocation kRelocation;
    const std::vector<Burst::OptionalCacheHold> kCacheHolds;
};

nn::GeneralResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> convertExecutionResults(
        const std::vector<OutputShape>& outputShapes, const Timing& timing) {
    return std::make_pair(NN_TRY(nn::convert(outputShapes)), NN_TRY(nn::convert(timing)));
}

}  // namespace

Burst::MemoryCache::MemoryCache(std::shared_ptr<aidl_hal::IBurst> burst)
    : kBurst(std::move(burst)) {}

std::pair<int64_t, Burst::MemoryCache::SharedCleanup> Burst::MemoryCache::getOrCacheMemory(
        const nn::SharedMemory& memory) {
    std::lock_guard lock(mMutex);

    // Get the cache payload or create it (with default values) if it does not exist.
    auto& cachedPayload = mCache[memory];
    {
        const auto& [identifier, maybeCleaner] = cachedPayload;
        // If cache payload already exists, reuse it.
        if (auto cleaner = maybeCleaner.lock()) {
            return std::make_pair(identifier, std::move(cleaner));
        }
    }

    // If the code reaches this point, the cached payload either did not exist or expired prior to
    // this call.

    // Allocate a new identifier.
    CHECK_LT(mUnusedIdentifier, std::numeric_limits<int64_t>::max());
    const int64_t identifier = mUnusedIdentifier++;

    // Create reference-counted self-cleaning cache object.
    auto self = weak_from_this();
    Task cleanup = [memory, identifier, maybeMemoryCache = std::move(self)] {
        if (const auto memoryCache = maybeMemoryCache.lock()) {
            memoryCache->tryFreeMemory(memory, identifier);
        }
    };
    auto cleaner = std::make_shared<const Cleanup>(std::move(cleanup));

    // Store the result in the cache and return it.
    auto result = std::make_pair(identifier, std::move(cleaner));
    cachedPayload = result;
    return result;
}

std::optional<std::pair<int64_t, Burst::MemoryCache::SharedCleanup>>
Burst::MemoryCache::getMemoryIfAvailable(const nn::SharedMemory& memory) {
    std::lock_guard lock(mMutex);

    // Get the existing cached entry if it exists.
    const auto iter = mCache.find(memory);
    if (iter != mCache.end()) {
        const auto& [identifier, maybeCleaner] = iter->second;
        if (auto cleaner = maybeCleaner.lock()) {
            return std::make_pair(identifier, std::move(cleaner));
        }
    }

    // If the code reaches this point, the cached payload did not exist or was actively being
    // deleted.
    return std::nullopt;
}

void Burst::MemoryCache::tryFreeMemory(const nn::SharedMemory& memory, int64_t identifier) {
    {
        std::lock_guard guard(mMutex);
        // Remove the cached memory and payload if it is present but expired. Note that it may not
        // be present or may not be expired because another thread may have removed or cached the
        // same memory object before the current thread locked mMutex in tryFreeMemory.
        const auto iter = mCache.find(memory);
        if (iter != mCache.end()) {
            if (std::get<WeakCleanup>(iter->second).expired()) {
                mCache.erase(iter);
            }
        }
    }
    kBurst->releaseMemoryResource(identifier);
}

nn::GeneralResult<std::shared_ptr<const Burst>> Burst::create(
        std::shared_ptr<aidl_hal::IBurst> burst) {
    if (burst == nullptr) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
               << "aidl_hal::utils::Burst::create must have non-null burst";
    }

    return std::make_shared<const Burst>(PrivateConstructorTag{}, std::move(burst));
}

Burst::Burst(PrivateConstructorTag /*tag*/, std::shared_ptr<aidl_hal::IBurst> burst)
    : kBurst(std::move(burst)), kMemoryCache(std::make_shared<MemoryCache>(kBurst)) {
    CHECK(kBurst != nullptr);
}

Burst::OptionalCacheHold Burst::cacheMemory(const nn::SharedMemory& memory) const {
    auto [identifier, hold] = kMemoryCache->getOrCacheMemory(memory);
    return hold;
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> Burst::execute(
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

    std::vector<int64_t> memoryIdentifierTokens;
    std::vector<OptionalCacheHold> holds;
    memoryIdentifierTokens.reserve(requestInShared.pools.size());
    holds.reserve(requestInShared.pools.size());
    for (const auto& memoryPool : requestInShared.pools) {
        if (const auto* memory = std::get_if<nn::SharedMemory>(&memoryPool)) {
            if (auto cached = kMemoryCache->getMemoryIfAvailable(*memory)) {
                auto& [identifier, hold] = *cached;
                memoryIdentifierTokens.push_back(identifier);
                holds.push_back(std::move(hold));
                continue;
            }
        }
        memoryIdentifierTokens.push_back(-1);
    }
    CHECK_EQ(requestInShared.pools.size(), memoryIdentifierTokens.size());

    return executeInternal(aidlRequest, memoryIdentifierTokens, aidlMeasure, aidlDeadline,
                           aidlLoopTimeoutDuration, relocation);
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> Burst::executeInternal(
        const Request& request, const std::vector<int64_t>& memoryIdentifierTokens, bool measure,
        int64_t deadline, int64_t loopTimeoutDuration,
        const hal::utils::RequestRelocation& relocation) const {
    // Ensure that at most one execution is in flight at any given time.
    const bool alreadyInFlight = mExecutionInFlight.test_and_set();
    if (alreadyInFlight) {
        return NN_ERROR() << "IBurst already has an execution in flight";
    }
    const auto guard = ::android::base::make_scope_guard([this] { mExecutionInFlight.clear(); });

    if (relocation.input) {
        relocation.input->flush();
    }

    ExecutionResult executionResult;
    const auto ret = kBurst->executeSynchronously(request, memoryIdentifierTokens, measure,
                                                  deadline, loopTimeoutDuration, &executionResult);
    HANDLE_ASTATUS(ret) << "execute failed";
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

nn::GeneralResult<nn::SharedExecution> Burst::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared = NN_TRY(hal::utils::convertRequestFromPointerToShared(
            &request, nn::kDefaultRequestMemoryAlignment, nn::kDefaultRequestMemoryPadding,
            &maybeRequestInShared, &relocation));

    auto aidlRequest = NN_TRY(convert(requestInShared));
    const auto aidlMeasure = NN_TRY(convert(measure));
    const auto aidlLoopTimeoutDuration = NN_TRY(convert(loopTimeoutDuration));

    std::vector<int64_t> memoryIdentifierTokens;
    std::vector<OptionalCacheHold> holds;
    memoryIdentifierTokens.reserve(requestInShared.pools.size());
    holds.reserve(requestInShared.pools.size());
    for (const auto& memoryPool : requestInShared.pools) {
        if (const auto* memory = std::get_if<nn::SharedMemory>(&memoryPool)) {
            if (auto cached = kMemoryCache->getMemoryIfAvailable(*memory)) {
                auto& [identifier, hold] = *cached;
                memoryIdentifierTokens.push_back(identifier);
                holds.push_back(std::move(hold));
                continue;
            }
        }
        memoryIdentifierTokens.push_back(-1);
    }
    CHECK_EQ(requestInShared.pools.size(), memoryIdentifierTokens.size());

    return BurstExecution::create(shared_from_this(), std::move(aidlRequest),
                                  std::move(memoryIdentifierTokens), aidlMeasure,
                                  aidlLoopTimeoutDuration, std::move(relocation), std::move(holds));
}

nn::GeneralResult<std::shared_ptr<const BurstExecution>> BurstExecution::create(
        std::shared_ptr<const Burst> burst, Request request,
        std::vector<int64_t> memoryIdentifierTokens, bool measure, int64_t loopTimeoutDuration,
        hal::utils::RequestRelocation relocation,
        std::vector<Burst::OptionalCacheHold> cacheHolds) {
    if (burst == nullptr) {
        return NN_ERROR() << "aidl::utils::BurstExecution::create must have non-null burst";
    }

    return std::make_shared<const BurstExecution>(
            PrivateConstructorTag{}, std::move(burst), std::move(request),
            std::move(memoryIdentifierTokens), measure, loopTimeoutDuration, std::move(relocation),
            std::move(cacheHolds));
}

BurstExecution::BurstExecution(PrivateConstructorTag /*tag*/, std::shared_ptr<const Burst> burst,
                               Request request, std::vector<int64_t> memoryIdentifierTokens,
                               bool measure, int64_t loopTimeoutDuration,
                               hal::utils::RequestRelocation relocation,
                               std::vector<Burst::OptionalCacheHold> cacheHolds)
    : kBurst(std::move(burst)),
      kRequest(std::move(request)),
      kMemoryIdentifierTokens(std::move(memoryIdentifierTokens)),
      kMeasure(measure),
      kLoopTimeoutDuration(loopTimeoutDuration),
      kRelocation(std::move(relocation)),
      kCacheHolds(std::move(cacheHolds)) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> BurstExecution::compute(
        const nn::OptionalTimePoint& deadline) const {
    const auto aidlDeadline = NN_TRY(hal::utils::makeExecutionFailure(convert(deadline)));
    return kBurst->executeInternal(kRequest, kMemoryIdentifierTokens, kMeasure, aidlDeadline,
                                   kLoopTimeoutDuration, kRelocation);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
BurstExecution::computeFenced(const std::vector<nn::SyncFence>& /*waitFor*/,
                              const nn::OptionalTimePoint& /*deadline*/,
                              const nn::OptionalDuration& /*timeoutDurationAfterFence*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IExecution::computeFenced is not supported on burst object";
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
