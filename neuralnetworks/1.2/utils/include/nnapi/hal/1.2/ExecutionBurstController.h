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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_EXECUTION_BURST_CONTROLLER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_EXECUTION_BURST_CONTROLLER_H

#include "ExecutionBurstUtils.h"

#include <android-base/thread_annotations.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.2/IBurstCallback.h>
#include <android/hardware/neuralnetworks/1.2/IBurstContext.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <nnapi/IBurst.h>
#include <nnapi/IExecution.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/ProtectCallback.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stack>
#include <tuple>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::V1_2::utils {

/**
 * The ExecutionBurstController class manages both the serialization and deserialization of data
 * across FMQ, making it appear to the runtime as a regular synchronous inference. Additionally,
 * this class manages the burst's memory cache.
 */
class ExecutionBurstController final
    : public nn::IBurst,
      public std::enable_shared_from_this<ExecutionBurstController> {
    struct PrivateConstructorTag {};

  public:
    using FallbackFunction = std::function<
            nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>()>;

    /**
     * NN runtime memory cache.
     *
     * MemoryCache associates a Memory object with a slot number to be passed across FMQ. The
     * ExecutionBurstServer can use this callback to retrieve a hidl_memory corresponding to the
     * slot via HIDL.
     *
     * Whenever a hidl_memory object is copied, it will duplicate the underlying file descriptor.
     * Because the NN runtime currently copies the hidl_memory on each execution, it is difficult to
     * associate hidl_memory objects with previously cached hidl_memory objects. For this reason,
     * callers of this class must pair each hidl_memory object with an associated key. For
     * efficiency, if two hidl_memory objects represent the same underlying buffer, they must use
     * the same key.
     *
     * This class is thread-safe.
     */
    class MemoryCache : public std::enable_shared_from_this<MemoryCache> {
        struct PrivateConstructorTag {};

      public:
        using Task = std::function<void()>;
        using Cleanup = base::ScopeGuard<Task>;
        using SharedCleanup = std::shared_ptr<const Cleanup>;
        using WeakCleanup = std::weak_ptr<const Cleanup>;

        // Custom constructor to pre-allocate cache sizes.
        MemoryCache();

        /**
         * Add a burst context to the MemoryCache object.
         *
         * If this method is called, it must be called before the MemoryCache::cacheMemory or
         * MemoryCache::getMemory is used.
         *
         * @param burstContext Burst context to be added to the MemoryCache object.
         */
        void setBurstContext(sp<IBurstContext> burstContext);

        /**
         * Cache a memory object in the MemoryCache object.
         *
         * @param memory Memory object to be cached while the returned `SharedCleanup` is alive.
         * @return A pair of (1) a unique identifier for the cache entry and (2) a ref-counted
         *     "hold" object which preserves the cache as long as the hold object is alive.
         */
        std::pair<int32_t, SharedCleanup> cacheMemory(const nn::SharedMemory& memory);

        /**
         * Get the memory object corresponding to a slot identifier.
         *
         * @param slot Slot which identifies the memory object to retrieve.
         * @return The memory object corresponding to slot, otherwise GeneralError.
         */
        nn::GeneralResult<nn::SharedMemory> getMemory(int32_t slot);

      private:
        void freeMemory(const nn::SharedMemory& memory);
        int32_t allocateSlotLocked() REQUIRES(mMutex);

        std::mutex mMutex;
        std::condition_variable mCond;
        sp<IBurstContext> mBurstContext GUARDED_BY(mMutex);
        std::stack<int32_t, std::vector<int32_t>> mFreeSlots GUARDED_BY(mMutex);
        std::map<nn::SharedMemory, int32_t> mMemoryIdToSlot GUARDED_BY(mMutex);
        std::vector<nn::SharedMemory> mMemoryCache GUARDED_BY(mMutex);
        std::vector<WeakCleanup> mCacheCleaner GUARDED_BY(mMutex);
    };

    /**
     * HIDL Callback class to pass memory objects to the Burst server when given corresponding
     * slots.
     */
    class ExecutionBurstCallback : public IBurstCallback {
      public:
        // Precondition: memoryCache must be non-null.
        explicit ExecutionBurstCallback(const std::shared_ptr<MemoryCache>& memoryCache);

        // See IBurstCallback::getMemories for information on this method.
        Return<void> getMemories(const hidl_vec<int32_t>& slots, getMemories_cb cb) override;

      private:
        const std::weak_ptr<MemoryCache> kMemoryCache;
    };

    /**
     * Creates a burst controller on a prepared model.
     *
     * @param preparedModel Model prepared for execution to execute on.
     * @param pollingTimeWindow How much time (in microseconds) the ExecutionBurstController is
     *     allowed to poll the FMQ before waiting on the blocking futex. Polling may result in lower
     *     latencies at the potential cost of more power usage.
     * @return ExecutionBurstController Execution burst controller object.
     */
    static nn::GeneralResult<std::shared_ptr<const ExecutionBurstController>> create(
            nn::SharedPreparedModel preparedModel, const sp<IPreparedModel>& hidlPreparedModel,
            std::chrono::microseconds pollingTimeWindow);

    ExecutionBurstController(PrivateConstructorTag tag, nn::SharedPreparedModel preparedModel,
                             std::unique_ptr<RequestChannelSender> requestChannelSender,
                             std::unique_ptr<ResultChannelReceiver> resultChannelReceiver,
                             sp<ExecutionBurstCallback> callback, sp<IBurstContext> burstContext,
                             std::shared_ptr<MemoryCache> memoryCache,
                             neuralnetworks::utils::DeathHandler deathHandler);

    // See IBurst::cacheMemory for information on this method.
    OptionalCacheHold cacheMemory(const nn::SharedMemory& memory) const override;

    // See IBurst::execute for information on this method.
    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> execute(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

    // See IBurst::createReusableExecution for information on this method.
    nn::GeneralResult<nn::SharedExecution> createReusableExecution(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

    // If fallback is not nullptr, this method will invoke the fallback function to try another
    // execution path if the packet could not be sent. Otherwise, failing to send the packet will
    // result in an error.
    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> executeInternal(
            const std::vector<FmqRequestDatum>& requestPacket,
            const hal::utils::RequestRelocation& relocation, FallbackFunction fallback) const;

  private:
    mutable std::atomic_flag mExecutionInFlight = ATOMIC_FLAG_INIT;
    const nn::SharedPreparedModel kPreparedModel;
    const std::unique_ptr<RequestChannelSender> mRequestChannelSender;
    const std::unique_ptr<ResultChannelReceiver> mResultChannelReceiver;
    const sp<ExecutionBurstCallback> mBurstCallback;
    const sp<IBurstContext> mBurstContext;
    const std::shared_ptr<MemoryCache> mMemoryCache;
    // `kDeathHandler` must come after `mRequestChannelSender` and `mResultChannelReceiver` because
    // it holds references to both objects.
    const neuralnetworks::utils::DeathHandler kDeathHandler;
};

}  // namespace android::hardware::neuralnetworks::V1_2::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_EXECUTION_BURST_CONTROLLER_H
