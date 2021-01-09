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

#include <android-base/logging.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "ExecutionBurstUtils.h"
#include "HalInterfaces.h"
#include "Tracing.h"

namespace android::nn {
namespace {

// DefaultBurstExecutorWithCache adapts an IPreparedModel so that it can be
// used as an IBurstExecutorWithCache. Specifically, the cache simply stores the
// hidl_memory object, and the execution forwards calls to the provided
// IPreparedModel's "executeSynchronously" method. With this class, hidl_memory
// must be mapped and unmapped for each execution.
class DefaultBurstExecutorWithCache : public ExecutionBurstServer::IBurstExecutorWithCache {
  public:
    DefaultBurstExecutorWithCache(V1_2::IPreparedModel* preparedModel)
        : mpPreparedModel(preparedModel) {}

    bool isCacheEntryPresent(int32_t slot) const override {
        const auto it = mMemoryCache.find(slot);
        return (it != mMemoryCache.end()) && it->second.valid();
    }

    void addCacheEntry(const hardware::hidl_memory& memory, int32_t slot) override {
        mMemoryCache[slot] = memory;
    }

    void removeCacheEntry(int32_t slot) override { mMemoryCache.erase(slot); }

    std::tuple<V1_0::ErrorStatus, hardware::hidl_vec<V1_2::OutputShape>, V1_2::Timing> execute(
            const V1_0::Request& request, const std::vector<int32_t>& slots,
            V1_2::MeasureTiming measure) override {
        // convert slots to pools
        hardware::hidl_vec<hardware::hidl_memory> pools(slots.size());
        std::transform(slots.begin(), slots.end(), pools.begin(),
                       [this](int32_t slot) { return mMemoryCache[slot]; });

        // create full request
        V1_0::Request fullRequest = request;
        fullRequest.pools = std::move(pools);

        // setup execution
        V1_0::ErrorStatus returnedStatus = V1_0::ErrorStatus::GENERAL_FAILURE;
        hardware::hidl_vec<V1_2::OutputShape> returnedOutputShapes;
        V1_2::Timing returnedTiming;
        auto cb = [&returnedStatus, &returnedOutputShapes, &returnedTiming](
                          V1_0::ErrorStatus status,
                          const hardware::hidl_vec<V1_2::OutputShape>& outputShapes,
                          const V1_2::Timing& timing) {
            returnedStatus = status;
            returnedOutputShapes = outputShapes;
            returnedTiming = timing;
        };

        // execute
        const hardware::Return<void> ret =
                mpPreparedModel->executeSynchronously(fullRequest, measure, cb);
        if (!ret.isOk() || returnedStatus != V1_0::ErrorStatus::NONE) {
            LOG(ERROR) << "IPreparedModelAdapter::execute -- Error executing";
            return {returnedStatus, std::move(returnedOutputShapes), kNoTiming};
        }

        return std::make_tuple(returnedStatus, std::move(returnedOutputShapes), returnedTiming);
    }

  private:
    V1_2::IPreparedModel* const mpPreparedModel;
    std::map<int32_t, hardware::hidl_memory> mMemoryCache;
};

}  // anonymous namespace

// ExecutionBurstServer methods

sp<ExecutionBurstServer> ExecutionBurstServer::create(
        const sp<IBurstCallback>& callback, const MQDescriptorSync<FmqRequestDatum>& requestChannel,
        const MQDescriptorSync<FmqResultDatum>& resultChannel,
        std::shared_ptr<IBurstExecutorWithCache> executorWithCache,
        std::chrono::microseconds pollingTimeWindow) {
    // check inputs
    if (callback == nullptr || executorWithCache == nullptr) {
        LOG(ERROR) << "ExecutionBurstServer::create passed a nullptr";
        return nullptr;
    }

    // create FMQ objects
    std::unique_ptr<RequestChannelReceiver> requestChannelReceiver =
            RequestChannelReceiver::create(requestChannel, pollingTimeWindow);
    std::unique_ptr<ResultChannelSender> resultChannelSender =
            ResultChannelSender::create(resultChannel);

    // check FMQ objects
    if (!requestChannelReceiver || !resultChannelSender) {
        LOG(ERROR) << "ExecutionBurstServer::create failed to create FastMessageQueue";
        return nullptr;
    }

    // make and return context
    return new ExecutionBurstServer(callback, std::move(requestChannelReceiver),
                                    std::move(resultChannelSender), std::move(executorWithCache));
}

sp<ExecutionBurstServer> ExecutionBurstServer::create(
        const sp<IBurstCallback>& callback, const MQDescriptorSync<FmqRequestDatum>& requestChannel,
        const MQDescriptorSync<FmqResultDatum>& resultChannel, V1_2::IPreparedModel* preparedModel,
        std::chrono::microseconds pollingTimeWindow) {
    // check relevant input
    if (preparedModel == nullptr) {
        LOG(ERROR) << "ExecutionBurstServer::create passed a nullptr";
        return nullptr;
    }

    // adapt IPreparedModel to have caching
    const std::shared_ptr<DefaultBurstExecutorWithCache> preparedModelAdapter =
            std::make_shared<DefaultBurstExecutorWithCache>(preparedModel);

    // make and return context
    return ExecutionBurstServer::create(callback, requestChannel, resultChannel,
                                        preparedModelAdapter, pollingTimeWindow);
}

ExecutionBurstServer::ExecutionBurstServer(
        const sp<IBurstCallback>& callback, std::unique_ptr<RequestChannelReceiver> requestChannel,
        std::unique_ptr<ResultChannelSender> resultChannel,
        std::shared_ptr<IBurstExecutorWithCache> executorWithCache)
    : mCallback(callback),
      mRequestChannelReceiver(std::move(requestChannel)),
      mResultChannelSender(std::move(resultChannel)),
      mExecutorWithCache(std::move(executorWithCache)) {
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

hardware::Return<void> ExecutionBurstServer::freeMemory(int32_t slot) {
    std::lock_guard<std::mutex> hold(mMutex);
    mExecutorWithCache->removeCacheEntry(slot);
    return hardware::Void();
}

void ExecutionBurstServer::ensureCacheEntriesArePresentLocked(const std::vector<int32_t>& slots) {
    const auto slotIsKnown = [this](int32_t slot) {
        return mExecutorWithCache->isCacheEntryPresent(slot);
    };

    // find unique unknown slots
    std::vector<int32_t> unknownSlots = slots;
    auto unknownSlotsEnd = unknownSlots.end();
    std::sort(unknownSlots.begin(), unknownSlotsEnd);
    unknownSlotsEnd = std::unique(unknownSlots.begin(), unknownSlotsEnd);
    unknownSlotsEnd = std::remove_if(unknownSlots.begin(), unknownSlotsEnd, slotIsKnown);
    unknownSlots.erase(unknownSlotsEnd, unknownSlots.end());

    // quick-exit if all slots are known
    if (unknownSlots.empty()) {
        return;
    }

    V1_0::ErrorStatus errorStatus = V1_0::ErrorStatus::GENERAL_FAILURE;
    std::vector<hardware::hidl_memory> returnedMemories;
    auto cb = [&errorStatus, &returnedMemories](
                      V1_0::ErrorStatus status,
                      const hardware::hidl_vec<hardware::hidl_memory>& memories) {
        errorStatus = status;
        returnedMemories = memories;
    };

    const hardware::Return<void> ret = mCallback->getMemories(unknownSlots, cb);

    if (!ret.isOk() || errorStatus != V1_0::ErrorStatus::NONE ||
        returnedMemories.size() != unknownSlots.size()) {
        LOG(ERROR) << "Error retrieving memories";
        return;
    }

    // add memories to unknown slots
    for (size_t i = 0; i < unknownSlots.size(); ++i) {
        mExecutorWithCache->addCacheEntry(returnedMemories[i], unknownSlots[i]);
    }
}

void ExecutionBurstServer::task() {
    // loop until the burst object is being destroyed
    while (!mTeardown) {
        // receive request
        auto arguments = mRequestChannelReceiver->getBlocking();

        // if the request packet was not properly received, return a generic
        // error and skip the execution
        //
        // if the burst is being torn down, skip the execution so the "task"
        // function can end
        if (!arguments) {
            if (!mTeardown) {
                mResultChannelSender->send(V1_0::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);
            }
            continue;
        }

        // otherwise begin tracing execution
        NNTRACE_FULL(NNTRACE_LAYER_IPC, NNTRACE_PHASE_EXECUTION,
                     "ExecutionBurstServer getting memory, executing, and returning results");

        // unpack the arguments; types are Request, std::vector<int32_t>, and
        // MeasureTiming, respectively
        const auto [requestWithoutPools, slotsOfPools, measure] = std::move(*arguments);

        // ensure executor with cache has required memory
        std::lock_guard<std::mutex> hold(mMutex);
        ensureCacheEntriesArePresentLocked(slotsOfPools);

        // perform computation; types are ErrorStatus, hidl_vec<OutputShape>,
        // and Timing, respectively
        const auto [errorStatus, outputShapes, returnedTiming] =
                mExecutorWithCache->execute(requestWithoutPools, slotsOfPools, measure);

        // return result
        mResultChannelSender->send(errorStatus, outputShapes, returnedTiming);
    }
}

}  // namespace android::nn
