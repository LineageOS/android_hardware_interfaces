/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "ExecutionBurstServer"

#include "ExecutionBurstServer.h"
#include "Conversions.h"
#include "ExecutionBurstUtils.h"

#include <android-base/logging.h>
#include <nnapi/IBurst.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>
#include <nnapi/hal/TransferValue.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "Tracing.h"

namespace android::hardware::neuralnetworks::V1_2::utils {
namespace {

using neuralnetworks::utils::makeExecutionFailure;

constexpr V1_2::Timing kNoTiming = {std::numeric_limits<uint64_t>::max(),
                                    std::numeric_limits<uint64_t>::max()};

nn::GeneralResult<std::vector<nn::SharedMemory>> getMemoriesCallback(
        V1_0::ErrorStatus status, const hidl_vec<hidl_memory>& memories) {
    HANDLE_HAL_STATUS(status) << "getting burst memories failed with " << toString(status);
    std::vector<nn::SharedMemory> canonicalMemories;
    canonicalMemories.reserve(memories.size());
    for (const auto& memory : memories) {
        canonicalMemories.push_back(NN_TRY(nn::convert(memory)));
    }
    return canonicalMemories;
}

}  // anonymous namespace

ExecutionBurstServer::MemoryCache::MemoryCache(nn::SharedBurst burstExecutor,
                                               sp<IBurstCallback> burstCallback)
    : kBurstExecutor(std::move(burstExecutor)), kBurstCallback(std::move(burstCallback)) {
    CHECK(kBurstExecutor != nullptr);
    CHECK(kBurstCallback != nullptr);
}

nn::GeneralResult<std::vector<std::pair<nn::SharedMemory, nn::IBurst::OptionalCacheHold>>>
ExecutionBurstServer::MemoryCache::getCacheEntries(const std::vector<int32_t>& slots) {
    std::lock_guard guard(mMutex);
    NN_TRY(ensureCacheEntriesArePresentLocked(slots));

    std::vector<std::pair<nn::SharedMemory, nn::IBurst::OptionalCacheHold>> results;
    results.reserve(slots.size());
    for (int32_t slot : slots) {
        results.push_back(NN_TRY(getCacheEntryLocked(slot)));
    }

    return results;
}

nn::GeneralResult<void> ExecutionBurstServer::MemoryCache::ensureCacheEntriesArePresentLocked(
        const std::vector<int32_t>& slots) {
    const auto slotIsKnown = [this](int32_t slot)
                                     REQUIRES(mMutex) { return mCache.count(slot) > 0; };

    // find unique unknown slots
    std::vector<int32_t> unknownSlots = slots;
    std::sort(unknownSlots.begin(), unknownSlots.end());
    auto unknownSlotsEnd = std::unique(unknownSlots.begin(), unknownSlots.end());
    unknownSlotsEnd = std::remove_if(unknownSlots.begin(), unknownSlotsEnd, slotIsKnown);
    unknownSlots.erase(unknownSlotsEnd, unknownSlots.end());

    // quick-exit if all slots are known
    if (unknownSlots.empty()) {
        return {};
    }

    auto cb = neuralnetworks::utils::CallbackValue(getMemoriesCallback);

    const auto ret = kBurstCallback->getMemories(unknownSlots, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    auto returnedMemories = NN_TRY(cb.take());

    if (returnedMemories.size() != unknownSlots.size()) {
        return NN_ERROR()
               << "ExecutionBurstServer::MemoryCache::ensureCacheEntriesArePresentLocked: Error "
                  "retrieving memories -- count mismatch between requested memories ("
               << unknownSlots.size() << ") and returned memories (" << returnedMemories.size()
               << ")";
    }

    // add memories to unknown slots
    for (size_t i = 0; i < unknownSlots.size(); ++i) {
        addCacheEntryLocked(unknownSlots[i], std::move(returnedMemories[i]));
    }

    return {};
}

nn::GeneralResult<std::pair<nn::SharedMemory, nn::IBurst::OptionalCacheHold>>
ExecutionBurstServer::MemoryCache::getCacheEntryLocked(int32_t slot) {
    if (const auto iter = mCache.find(slot); iter != mCache.end()) {
        return iter->second;
    }
    return NN_ERROR()
           << "ExecutionBurstServer::MemoryCache::getCacheEntryLocked failed because slot " << slot
           << " is not present in the cache";
}

void ExecutionBurstServer::MemoryCache::addCacheEntryLocked(int32_t slot, nn::SharedMemory memory) {
    auto hold = kBurstExecutor->cacheMemory(memory);
    mCache.emplace(slot, std::make_pair(std::move(memory), std::move(hold)));
}

void ExecutionBurstServer::MemoryCache::removeCacheEntry(int32_t slot) {
    std::lock_guard guard(mMutex);
    mCache.erase(slot);
}

// ExecutionBurstServer methods

nn::GeneralResult<sp<ExecutionBurstServer>> ExecutionBurstServer::create(
        const sp<IBurstCallback>& callback, const MQDescriptorSync<FmqRequestDatum>& requestChannel,
        const MQDescriptorSync<FmqResultDatum>& resultChannel, nn::SharedBurst burstExecutor,
        std::chrono::microseconds pollingTimeWindow) {
    // check inputs
    if (callback == nullptr || burstExecutor == nullptr) {
        return NN_ERROR() << "ExecutionBurstServer::create passed a nullptr";
    }

    // create FMQ objects
    auto requestChannelReceiver =
            NN_TRY(RequestChannelReceiver::create(requestChannel, pollingTimeWindow));
    auto resultChannelSender = NN_TRY(ResultChannelSender::create(resultChannel));

    // check FMQ objects
    CHECK(requestChannelReceiver != nullptr);
    CHECK(resultChannelSender != nullptr);

    // make and return context
    return sp<ExecutionBurstServer>::make(PrivateConstructorTag{}, callback,
                                          std::move(requestChannelReceiver),
                                          std::move(resultChannelSender), std::move(burstExecutor));
}

ExecutionBurstServer::ExecutionBurstServer(PrivateConstructorTag /*tag*/,
                                           const sp<IBurstCallback>& callback,
                                           std::unique_ptr<RequestChannelReceiver> requestChannel,
                                           std::unique_ptr<ResultChannelSender> resultChannel,
                                           nn::SharedBurst burstExecutor)
    : mCallback(callback),
      mRequestChannelReceiver(std::move(requestChannel)),
      mResultChannelSender(std::move(resultChannel)),
      mBurstExecutor(std::move(burstExecutor)),
      mMemoryCache(mBurstExecutor, mCallback) {
    // TODO: highly document the threading behavior of this class
    mWorker = std::thread([this] { task(); });
}

ExecutionBurstServer::~ExecutionBurstServer() {
    // set teardown flag
    mTeardown = true;
    mRequestChannelReceiver->invalidate();

    // wait for task thread to end
    mWorker.join();
}

Return<void> ExecutionBurstServer::freeMemory(int32_t slot) {
    mMemoryCache.removeCacheEntry(slot);
    return Void();
}

void ExecutionBurstServer::task() {
    // loop until the burst object is being destroyed
    while (!mTeardown) {
        // receive request
        auto arguments = mRequestChannelReceiver->getBlocking();

        // if the request packet was not properly received, return a generic error and skip the
        // execution
        //
        // if the burst is being torn down, skip the execution so the "task" function can end
        if (!arguments.has_value()) {
            if (!mTeardown) {
                mResultChannelSender->send(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);
            }
            continue;
        }

        // unpack the arguments; types are Request, std::vector<int32_t>, and MeasureTiming,
        // respectively
        const auto [requestWithoutPools, slotsOfPools, measure] = std::move(arguments).value();

        auto result = execute(requestWithoutPools, slotsOfPools, measure);

        // return result
        if (result.has_value()) {
            const auto& [outputShapes, timing] = result.value();
            mResultChannelSender->send(V1_0::ErrorStatus::NONE, outputShapes, timing);
        } else {
            const auto& [message, code, outputShapes] = result.error();
            LOG(ERROR) << "IBurst::execute failed with " << code << ": " << message;
            mResultChannelSender->send(convert(code).value(), convert(outputShapes).value(),
                                       kNoTiming);
        }
    }
}

nn::ExecutionResult<std::pair<hidl_vec<OutputShape>, Timing>> ExecutionBurstServer::execute(
        const V1_0::Request& requestWithoutPools, const std::vector<int32_t>& slotsOfPools,
        MeasureTiming measure) {
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION,
                 "ExecutionBurstServer getting memory, executing, and returning results");

    // ensure executor with cache has required memory
    const auto cacheEntries =
            NN_TRY(makeExecutionFailure(mMemoryCache.getCacheEntries(slotsOfPools)));

    // convert request, populating its pools
    // This code performs an unvalidated convert because the request object without its pools is
    // invalid because it is incomplete. Instead, the validation is performed after the memory pools
    // have been added to the request.
    auto canonicalRequest =
            NN_TRY(makeExecutionFailure(nn::unvalidatedConvert(requestWithoutPools)));
    CHECK(canonicalRequest.pools.empty());
    std::transform(cacheEntries.begin(), cacheEntries.end(),
                   std::back_inserter(canonicalRequest.pools),
                   [](const auto& cacheEntry) { return cacheEntry.first; });
    NN_TRY(makeExecutionFailure(validate(canonicalRequest)));

    nn::MeasureTiming canonicalMeasure = NN_TRY(makeExecutionFailure(nn::convert(measure)));

    const auto [outputShapes, timing] =
            NN_TRY(mBurstExecutor->execute(canonicalRequest, canonicalMeasure, {}, {}));

    return std::make_pair(NN_TRY(makeExecutionFailure(convert(outputShapes))),
                          NN_TRY(makeExecutionFailure(convert(timing))));
}

}  // namespace android::hardware::neuralnetworks::V1_2::utils
