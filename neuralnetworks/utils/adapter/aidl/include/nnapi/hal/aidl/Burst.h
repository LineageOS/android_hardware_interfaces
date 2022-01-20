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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_BURST_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_BURST_H

#include <aidl/android/hardware/neuralnetworks/BnBurst.h>
#include <aidl/android/hardware/neuralnetworks/ExecutionResult.h>
#include <aidl/android/hardware/neuralnetworks/Request.h>
#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IBurst.h>
#include <nnapi/Types.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::adapter {

// Class that adapts nn::Burst to BnBurst.
class Burst : public BnBurst {
  public:
    // Precondition: burst != nullptr
    explicit Burst(::android::nn::SharedBurst burst);

    ndk::ScopedAStatus executeSynchronously(const Request& request,
                                            const std::vector<int64_t>& memoryIdentifierTokens,
                                            bool measureTiming, int64_t deadlineNs,
                                            int64_t loopTimeoutDurationNs,
                                            ExecutionResult* executionResult) override;
    ndk::ScopedAStatus executeSynchronouslyWithConfig(
            const Request& request, const std::vector<int64_t>& memoryIdentifierTokens,
            const ExecutionConfig& config, int64_t deadlineNs,
            ExecutionResult* executionResult) override;

    ndk::ScopedAStatus releaseMemoryResource(int64_t memoryIdentifierToken) override;

    class ThreadSafeMemoryCache {
      public:
        using Value =
                std::pair<::android::nn::SharedMemory, ::android::nn::IBurst::OptionalCacheHold>;

        Value add(int64_t token, const ::android::nn::SharedMemory& memory,
                  const ::android::nn::IBurst& burst) const;
        void remove(int64_t token) const;

      private:
        mutable std::mutex mMutex;
        mutable std::unordered_map<int64_t, Value> mCache GUARDED_BY(mMutex);
    };

  private:
    const ::android::nn::SharedBurst kBurst;
    const ThreadSafeMemoryCache kMemoryCache;
};

}  // namespace aidl::android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_BURST_H
