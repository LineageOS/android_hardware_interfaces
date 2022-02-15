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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_BURST_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_BURST_H

#include <android-base/thread_annotations.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.2/IBurstCallback.h>
#include <android/hardware/neuralnetworks/1.2/IBurstContext.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <nnapi/IBurst.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/ProtectCallback.h>
#include <nnapi/hal/1.2/BurstUtils.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include <tuple>
#include <vector>

namespace android::hardware::neuralnetworks::adapter {

/**
 * The Burst class is responsible for waiting for and deserializing a request object from a FMQ,
 * performing the inference, and serializing the result back across another FMQ.
 */
class Burst : public V1_2::IBurstContext {
    struct PrivateConstructorTag {};

  public:
    /**
     * Class to cache the memory objects for a burst object.
     *
     * This class is thread-safe.
     */
    class MemoryCache {
      public:
        // Precondition: burstExecutor != nullptr
        // Precondition: burstCallback != nullptr
        MemoryCache(nn::SharedBurst burstExecutor, sp<V1_2::IBurstCallback> burstCallback);

        /**
         * Get the cached memory objects corresponding to provided slot identifiers.
         *
         * If the slot entry is not present in the cache, this class will use V1_2::IBurstCallback
         * to retrieve those entries that are not present in the cache, then cache them.
         *
         * @param slots Identifiers of memory objects to be retrieved.
         * @return A vector where each element is the memory object and a ref-counted cache "hold"
         *     object to preserve the cache entry of the IBurst object as long as the "hold" object
         *     is alive, otherwise GeneralError. Each element of the vector corresponds to the
         *     element of slot.
         */
        nn::GeneralResult<std::vector<std::pair<nn::SharedMemory, nn::IBurst::OptionalCacheHold>>>
        getCacheEntries(const std::vector<int32_t>& slots);

        /**
         * Remove an entry from the cache.
         *
         * @param slot Identifier of the memory object to be removed from the cache.
         */
        void removeCacheEntry(int32_t slot);

      private:
        nn::GeneralResult<void> ensureCacheEntriesArePresentLocked(
                const std::vector<int32_t>& slots) REQUIRES(mMutex);
        nn::GeneralResult<std::pair<nn::SharedMemory, nn::IBurst::OptionalCacheHold>>
        getCacheEntryLocked(int32_t slot) REQUIRES(mMutex);
        void addCacheEntryLocked(int32_t slot, nn::SharedMemory memory) REQUIRES(mMutex);

        std::mutex mMutex;
        std::map<int32_t, std::pair<nn::SharedMemory, nn::IBurst::OptionalCacheHold>> mCache
                GUARDED_BY(mMutex);
        nn::SharedBurst kBurstExecutor;
        const sp<V1_2::IBurstCallback> kBurstCallback;
    };

    /**
     * Create automated context to manage FMQ-based executions.
     *
     * This function is intended to be used by a service to automatically:
     * 1) Receive data from a provided FMQ
     * 2) Execute a model with the given information
     * 3) Send the result to the created FMQ
     *
     * @param callback Callback used to retrieve memories corresponding to unrecognized slots.
     * @param requestChannel Input FMQ channel through which the client passes the request to the
     *     service.
     * @param resultChannel Output FMQ channel from which the client can retrieve the result of the
     *     execution.
     * @param burstExecutor Object which maintains a local cache of the memory pools and executes
     *     using the cached memory pools.
     * @param pollingTimeWindow How much time (in microseconds) the Burst is allowed to poll the FMQ
     *     before waiting on the blocking futex. Polling may result in lower latencies at the
     *     potential cost of more power usage.
     * @return V1_2::IBurstContext Handle to the burst context.
     */
    static nn::GeneralResult<sp<Burst>> create(
            const sp<V1_2::IBurstCallback>& callback,
            const MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
            const MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
            nn::SharedBurst burstExecutor,
            std::chrono::microseconds pollingTimeWindow = std::chrono::microseconds{0});

    Burst(PrivateConstructorTag tag, const sp<V1_2::IBurstCallback>& callback,
          std::unique_ptr<V1_2::utils::RequestChannelReceiver> requestChannel,
          std::unique_ptr<V1_2::utils::ResultChannelSender> resultChannel,
          nn::SharedBurst burstExecutor);
    ~Burst();

    // Used by the NN runtime to preemptively remove any stored memory. See
    // V1_2::IBurstContext::freeMemory for more information.
    Return<void> freeMemory(int32_t slot) override;

  private:
    // Work loop that will continue processing execution requests until the Burst object is freed.
    void task();

    nn::ExecutionResult<std::pair<hidl_vec<V1_2::OutputShape>, V1_2::Timing>> execute(
            const V1_0::Request& requestWithoutPools, const std::vector<int32_t>& slotsOfPools,
            V1_2::MeasureTiming measure);

    std::thread mWorker;
    std::atomic<bool> mTeardown{false};
    const sp<V1_2::IBurstCallback> mCallback;
    const std::unique_ptr<V1_2::utils::RequestChannelReceiver> mRequestChannelReceiver;
    const std::unique_ptr<V1_2::utils::ResultChannelSender> mResultChannelSender;
    const nn::SharedBurst mBurstExecutor;
    MemoryCache mMemoryCache;
};

}  // namespace android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_BURST_H
