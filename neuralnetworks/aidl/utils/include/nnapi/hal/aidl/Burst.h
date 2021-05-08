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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BURST_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BURST_H

#include <aidl/android/hardware/neuralnetworks/IBurst.h>
#include <android-base/scopeguard.h>
#include <android-base/thread_annotations.h>
#include <nnapi/IBurst.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::utils {

// Class that adapts aidl_hal::IBurst to nn::IBurst.
class Burst final : public nn::IBurst, public std::enable_shared_from_this<Burst> {
    struct PrivateConstructorTag {};

  public:
    /**
     * Thread-safe, self-cleaning cache that relates an nn::Memory object to a unique int64_t
     * identifier.
     */
    class MemoryCache : public std::enable_shared_from_this<MemoryCache> {
      public:
        using Task = std::function<void()>;
        using Cleanup = ::android::base::ScopeGuard<Task>;
        using SharedCleanup = std::shared_ptr<const Cleanup>;
        using WeakCleanup = std::weak_ptr<const Cleanup>;

        explicit MemoryCache(std::shared_ptr<aidl_hal::IBurst> burst);

        /**
         * Get or cache a memory object in the MemoryCache object.
         *
         * @param memory Memory object to be cached while the returned `SharedCleanup` is alive.
         * @return A pair of (1) a unique identifier for the cache entry and (2) a ref-counted
         *     "hold" object which preserves the cache as long as the hold object is alive.
         */
        std::pair<int64_t, SharedCleanup> getOrCacheMemory(const nn::SharedMemory& memory);

        /**
         * Get a cached memory object in the MemoryCache object if it exists, otherwise
         * std::nullopt.
         *
         * @param memory Memory object to be cached while the returned `SharedCleanup` is alive.
         * @return A pair of (1) a unique identifier for the cache entry and (2) a ref-counted
         *     "hold" object which preserves the cache as long as the hold object is alive. IF the
         *     cache entry is not present, std::nullopt is returned instead.
         */
        std::optional<std::pair<int64_t, SharedCleanup>> getMemoryIfAvailable(
                const nn::SharedMemory& memory);

      private:
        void tryFreeMemory(const nn::SharedMemory& memory, int64_t identifier);

        const std::shared_ptr<aidl_hal::IBurst> kBurst;
        std::mutex mMutex;
        int64_t mUnusedIdentifier GUARDED_BY(mMutex) = 0;
        std::unordered_map<nn::SharedMemory, std::pair<int64_t, WeakCleanup>> mCache
                GUARDED_BY(mMutex);
    };

    static nn::GeneralResult<std::shared_ptr<const Burst>> create(
            std::shared_ptr<aidl_hal::IBurst> burst);

    Burst(PrivateConstructorTag tag, std::shared_ptr<aidl_hal::IBurst> burst);

    // See IBurst::cacheMemory for information.
    OptionalCacheHold cacheMemory(const nn::SharedMemory& memory) const override;

    // See IBurst::execute for information.
    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> execute(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalTimePoint& deadline,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

    // See IBurst::createReusableExecution for information.
    nn::GeneralResult<nn::SharedExecution> createReusableExecution(
            const nn::Request& request, nn::MeasureTiming measure,
            const nn::OptionalDuration& loopTimeoutDuration) const override;

    nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> executeInternal(
            const aidl_hal::Request& request, const std::vector<int64_t>& memoryIdentifierTokens,
            bool measure, int64_t deadline, int64_t loopTimeoutDuration,
            const hal::utils::RequestRelocation& relocation) const;

  private:
    mutable std::atomic_flag mExecutionInFlight = ATOMIC_FLAG_INIT;
    const std::shared_ptr<aidl_hal::IBurst> kBurst;
    const std::shared_ptr<MemoryCache> kMemoryCache;
};

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BURST_H
