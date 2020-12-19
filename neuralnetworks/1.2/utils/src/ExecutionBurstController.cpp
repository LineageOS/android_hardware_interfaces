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

#define LOG_TAG "ExecutionBurstController"

#include "ExecutionBurstController.h"
#include "ExecutionBurstUtils.h"

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>
#include <nnapi/IBurst.h>
#include <nnapi/IPreparedModel.h>
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
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "Callbacks.h"
#include "Conversions.h"
#include "Tracing.h"
#include "Utils.h"

namespace android::hardware::neuralnetworks::V1_2::utils {
namespace {

nn::GeneralResult<sp<IBurstContext>> executionBurstResultCallback(
        V1_0::ErrorStatus status, const sp<IBurstContext>& burstContext) {
    HANDLE_HAL_STATUS(status) << "IPreparedModel::configureExecutionBurst failed with status "
                              << toString(status);
    if (burstContext == nullptr) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
               << "IPreparedModel::configureExecutionBurst returned nullptr for burst";
    }
    return burstContext;
}

nn::GeneralResult<hidl_vec<hidl_memory>> getMemoriesHelper(
        const hidl_vec<int32_t>& slots,
        const std::shared_ptr<ExecutionBurstController::MemoryCache>& memoryCache) {
    hidl_vec<hidl_memory> memories(slots.size());
    for (size_t i = 0; i < slots.size(); ++i) {
        const int32_t slot = slots[i];
        const auto memory = NN_TRY(memoryCache->getMemory(slot));
        memories[i] = NN_TRY(V1_0::utils::unvalidatedConvert(memory));
        if (!memories[i].valid()) {
            return NN_ERROR() << "memory at slot " << slot << " is invalid";
        }
    }
    return memories;
}

}  // namespace

// MemoryCache methods

ExecutionBurstController::MemoryCache::MemoryCache() {
    constexpr size_t kPreallocatedCount = 1024;
    std::vector<int32_t> freeSlotsSpace;
    freeSlotsSpace.reserve(kPreallocatedCount);
    mFreeSlots = std::stack<int32_t, std::vector<int32_t>>(std::move(freeSlotsSpace));
    mMemoryCache.reserve(kPreallocatedCount);
    mCacheCleaner.reserve(kPreallocatedCount);
}

void ExecutionBurstController::MemoryCache::setBurstContext(sp<IBurstContext> burstContext) {
    std::lock_guard guard(mMutex);
    mBurstContext = std::move(burstContext);
}

std::pair<int32_t, ExecutionBurstController::MemoryCache::SharedCleanup>
ExecutionBurstController::MemoryCache::cacheMemory(const nn::SharedMemory& memory) {
    std::unique_lock lock(mMutex);
    base::ScopedLockAssertion lockAssert(mMutex);

    // Use existing cache entry if (1) the Memory object is in the cache and (2) the cache entry is
    // not currently being freed.
    auto iter = mMemoryIdToSlot.find(memory);
    while (iter != mMemoryIdToSlot.end()) {
        const int32_t slot = iter->second;
        if (auto cleaner = mCacheCleaner.at(slot).lock()) {
            return std::make_pair(slot, std::move(cleaner));
        }

        // If the code reaches this point, the Memory object was in the cache, but is currently
        // being destroyed. This code waits until the cache entry has been freed, then loops to
        // ensure the cache entry has been freed or has been made present by another thread.
        mCond.wait(lock);
        iter = mMemoryIdToSlot.find(memory);
    }

    // Allocate a new cache entry.
    const int32_t slot = allocateSlotLocked();
    mMemoryIdToSlot[memory] = slot;
    mMemoryCache[slot] = memory;

    // Create reference-counted self-cleaning cache object.
    auto self = weak_from_this();
    Task cleanup = [memory, memoryCache = std::move(self)] {
        if (const auto lock = memoryCache.lock()) {
            lock->freeMemory(memory);
        }
    };
    auto cleaner = std::make_shared<const Cleanup>(std::move(cleanup));
    mCacheCleaner[slot] = cleaner;

    return std::make_pair(slot, std::move(cleaner));
}

nn::GeneralResult<nn::SharedMemory> ExecutionBurstController::MemoryCache::getMemory(int32_t slot) {
    std::lock_guard guard(mMutex);
    if (slot < 0 || static_cast<size_t>(slot) >= mMemoryCache.size()) {
        return NN_ERROR() << "Invalid slot: " << slot << " vs " << mMemoryCache.size();
    }
    return mMemoryCache[slot];
}

void ExecutionBurstController::MemoryCache::freeMemory(const nn::SharedMemory& memory) {
    {
        std::lock_guard guard(mMutex);
        const int32_t slot = mMemoryIdToSlot.at(memory);
        if (mBurstContext) {
            mBurstContext->freeMemory(slot);
        }
        mMemoryIdToSlot.erase(memory);
        mMemoryCache[slot] = {};
        mCacheCleaner[slot].reset();
        mFreeSlots.push(slot);
    }
    mCond.notify_all();
}

int32_t ExecutionBurstController::MemoryCache::allocateSlotLocked() {
    constexpr size_t kMaxNumberOfSlots = std::numeric_limits<int32_t>::max();

    // If there is a free slot, use it.
    if (!mFreeSlots.empty()) {
        const int32_t slot = mFreeSlots.top();
        mFreeSlots.pop();
        return slot;
    }

    // Use a slot for the first time.
    CHECK_LT(mMemoryCache.size(), kMaxNumberOfSlots) << "Exceeded maximum number of slots!";
    const int32_t slot = static_cast<int32_t>(mMemoryCache.size());
    mMemoryCache.emplace_back();
    mCacheCleaner.emplace_back();

    return slot;
}

// ExecutionBurstCallback methods

ExecutionBurstController::ExecutionBurstCallback::ExecutionBurstCallback(
        const std::shared_ptr<MemoryCache>& memoryCache)
    : kMemoryCache(memoryCache) {
    CHECK(memoryCache != nullptr);
}

Return<void> ExecutionBurstController::ExecutionBurstCallback::getMemories(
        const hidl_vec<int32_t>& slots, getMemories_cb cb) {
    const auto memoryCache = kMemoryCache.lock();
    if (memoryCache == nullptr) {
        LOG(ERROR) << "ExecutionBurstController::ExecutionBurstCallback::getMemories called after "
                      "the MemoryCache has been freed";
        cb(V1_0::ErrorStatus::GENERAL_FAILURE, {});
        return Void();
    }

    const auto maybeMemories = getMemoriesHelper(slots, memoryCache);
    if (!maybeMemories.has_value()) {
        const auto& [message, code] = maybeMemories.error();
        LOG(ERROR) << "ExecutionBurstController::ExecutionBurstCallback::getMemories failed with "
                   << code << ": " << message;
        cb(V1_0::ErrorStatus::INVALID_ARGUMENT, {});
        return Void();
    }

    cb(V1_0::ErrorStatus::NONE, maybeMemories.value());
    return Void();
}

// ExecutionBurstController methods

nn::GeneralResult<std::shared_ptr<const ExecutionBurstController>> ExecutionBurstController::create(
        const sp<V1_2::IPreparedModel>& preparedModel, FallbackFunction fallback,
        std::chrono::microseconds pollingTimeWindow) {
    // check inputs
    if (preparedModel == nullptr) {
        return NN_ERROR() << "ExecutionBurstController::create passed a nullptr";
    }

    // create FMQ objects
    auto [requestChannelSender, requestChannelDescriptor] =
            NN_TRY(RequestChannelSender::create(kExecutionBurstChannelLength));
    auto [resultChannelReceiver, resultChannelDescriptor] =
            NN_TRY(ResultChannelReceiver::create(kExecutionBurstChannelLength, pollingTimeWindow));

    // check FMQ objects
    CHECK(requestChannelSender != nullptr);
    CHECK(requestChannelDescriptor != nullptr);
    CHECK(resultChannelReceiver != nullptr);
    CHECK(resultChannelDescriptor != nullptr);

    // create memory cache
    auto memoryCache = std::make_shared<MemoryCache>();

    // create callback object
    auto burstCallback = sp<ExecutionBurstCallback>::make(memoryCache);
    auto cb = hal::utils::CallbackValue(executionBurstResultCallback);

    // configure burst
    const Return<void> ret = preparedModel->configureExecutionBurst(
            burstCallback, *requestChannelDescriptor, *resultChannelDescriptor, cb);
    HANDLE_TRANSPORT_FAILURE(ret);

    auto burstContext = NN_TRY(cb.take());
    memoryCache->setBurstContext(burstContext);

    // create death handler object
    auto deathHandler = NN_TRY(neuralnetworks::utils::DeathHandler::create(burstContext));
    deathHandler.protectCallbackForLifetimeOfDeathHandler(requestChannelSender.get());
    deathHandler.protectCallbackForLifetimeOfDeathHandler(resultChannelReceiver.get());

    // make and return controller
    return std::make_shared<const ExecutionBurstController>(
            PrivateConstructorTag{}, std::move(fallback), std::move(requestChannelSender),
            std::move(resultChannelReceiver), std::move(burstCallback), std::move(burstContext),
            std::move(memoryCache), std::move(deathHandler));
}

ExecutionBurstController::ExecutionBurstController(
        PrivateConstructorTag /*tag*/, FallbackFunction fallback,
        std::unique_ptr<RequestChannelSender> requestChannelSender,
        std::unique_ptr<ResultChannelReceiver> resultChannelReceiver,
        sp<ExecutionBurstCallback> callback, sp<IBurstContext> burstContext,
        std::shared_ptr<MemoryCache> memoryCache, neuralnetworks::utils::DeathHandler deathHandler)
    : kFallback(std::move(fallback)),
      mRequestChannelSender(std::move(requestChannelSender)),
      mResultChannelReceiver(std::move(resultChannelReceiver)),
      mBurstCallback(std::move(callback)),
      mBurstContext(std::move(burstContext)),
      mMemoryCache(std::move(memoryCache)),
      kDeathHandler(std::move(deathHandler)) {}

ExecutionBurstController::OptionalCacheHold ExecutionBurstController::cacheMemory(
        const nn::SharedMemory& memory) const {
    auto [slot, hold] = mMemoryCache->cacheMemory(memory);
    return hold;
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
ExecutionBurstController::execute(const nn::Request& request, nn::MeasureTiming measure) const {
    // This is the first point when we know an execution is occurring, so begin to collect
    // systraces. Note that the first point we can begin collecting systraces in
    // ExecutionBurstServer is when the RequestChannelReceiver realizes there is data in the FMQ, so
    // ExecutionBurstServer collects systraces at different points in the code.
    NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION, "ExecutionBurstController::execute");

    // if the request is valid but of a higher version than what's supported in burst execution,
    // fall back to another execution path
    if (const auto version = NN_TRY(hal::utils::makeExecutionFailure(nn::validate(request)));
        version > nn::Version::ANDROID_Q) {
        // fallback to another execution path if the packet could not be sent
        if (kFallback) {
            return kFallback(request, measure);
        }
        return NN_ERROR() << "Request object has features not supported by IBurst::execute";
    }

    // clear pools field of request, as they will be provided via slots
    const auto requestWithoutPools =
            nn::Request{.inputs = request.inputs, .outputs = request.outputs, .pools = {}};
    auto hidlRequest = NN_TRY(
            hal::utils::makeExecutionFailure(V1_0::utils::unvalidatedConvert(requestWithoutPools)));
    const auto hidlMeasure = NN_TRY(hal::utils::makeExecutionFailure(convert(measure)));

    // Ensure that at most one execution is in flight at any given time.
    const bool alreadyInFlight = mExecutionInFlight.test_and_set();
    if (alreadyInFlight) {
        return NN_ERROR() << "IBurst already has an execution in flight";
    }
    const auto guard = base::make_scope_guard([this] { mExecutionInFlight.clear(); });

    std::vector<int32_t> slots;
    std::vector<OptionalCacheHold> holds;
    slots.reserve(request.pools.size());
    holds.reserve(request.pools.size());
    for (const auto& memoryPool : request.pools) {
        auto [slot, hold] = mMemoryCache->cacheMemory(std::get<nn::SharedMemory>(memoryPool));
        slots.push_back(slot);
        holds.push_back(std::move(hold));
    }

    // send request packet
    const auto sendStatus = mRequestChannelSender->send(hidlRequest, hidlMeasure, slots);
    if (!sendStatus.ok()) {
        // fallback to another execution path if the packet could not be sent
        if (kFallback) {
            return kFallback(request, measure);
        }
        return NN_ERROR() << "Error sending FMQ packet: " << sendStatus.error();
    }

    // get result packet
    const auto [status, outputShapes, timing] =
            NN_TRY(hal::utils::makeExecutionFailure(mResultChannelReceiver->getBlocking()));
    return executionCallback(status, outputShapes, timing);
}

}  // namespace android::hardware::neuralnetworks::V1_2::utils
