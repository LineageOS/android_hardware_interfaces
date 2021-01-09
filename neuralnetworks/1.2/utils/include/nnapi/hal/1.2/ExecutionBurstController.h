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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_EXECUTION_BURST_CONTROLLER_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_EXECUTION_BURST_CONTROLLER_H

#include "ExecutionBurstUtils.h"

#include <android-base/macros.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IBurstCallback.h>
#include <android/hardware/neuralnetworks/1.2/IBurstContext.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <stack>
#include <tuple>
#include <utility>
#include <vector>

namespace android::nn {

/**
 * The ExecutionBurstController class manages both the serialization and
 * deserialization of data across FMQ, making it appear to the runtime as a
 * regular synchronous inference. Additionally, this class manages the burst's
 * memory cache.
 */
class ExecutionBurstController {
    DISALLOW_IMPLICIT_CONSTRUCTORS(ExecutionBurstController);

  public:
    /**
     * NN runtime burst callback object and memory cache.
     *
     * ExecutionBurstCallback associates a hidl_memory object with a slot number
     * to be passed across FMQ. The ExecutionBurstServer can use this callback
     * to retrieve this hidl_memory corresponding to the slot via HIDL.
     *
     * Whenever a hidl_memory object is copied, it will duplicate the underlying
     * file descriptor. Because the NN runtime currently copies the hidl_memory
     * on each execution, it is difficult to associate hidl_memory objects with
     * previously cached hidl_memory objects. For this reason, callers of this
     * class must pair each hidl_memory object with an associated key. For
     * efficiency, if two hidl_memory objects represent the same underlying
     * buffer, they must use the same key.
     */
    class ExecutionBurstCallback : public hardware::neuralnetworks::V1_2::IBurstCallback {
        DISALLOW_COPY_AND_ASSIGN(ExecutionBurstCallback);

      public:
        ExecutionBurstCallback() = default;

        hardware::Return<void> getMemories(const hardware::hidl_vec<int32_t>& slots,
                                           getMemories_cb cb) override;

        /**
         * This function performs one of two different actions:
         * 1) If a key corresponding to a memory resource is unrecognized by the
         *    ExecutionBurstCallback object, the ExecutionBurstCallback object
         *    will allocate a slot, bind the memory to the slot, and return the
         *    slot identifier.
         * 2) If a key corresponding to a memory resource is recognized by the
         *    ExecutionBurstCallback object, the ExecutionBurstCallback object
         *    will return the existing slot identifier.
         *
         * @param memories Memory resources used in an inference.
         * @param keys Unique identifiers where each element corresponds to a
         *     memory resource element in "memories".
         * @return Unique slot identifiers where each returned slot element
         *     corresponds to a memory resource element in "memories".
         */
        std::vector<int32_t> getSlots(const hardware::hidl_vec<hardware::hidl_memory>& memories,
                                      const std::vector<intptr_t>& keys);

        /*
         * This function performs two different actions:
         * 1) Removes an entry from the cache (if present), including the local
         *    storage of the hidl_memory object. Note that this call does not
         *    free any corresponding hidl_memory object in ExecutionBurstServer,
         *    which is separately freed via IBurstContext::freeMemory.
         * 2) Return whether a cache entry was removed and which slot was removed if
         *    found. If the key did not to correspond to any entry in the cache, a
         *    slot number of 0 is returned. The slot number and whether the entry
         *    existed is useful so the same slot can be freed in the
         *    ExecutionBurstServer's cache via IBurstContext::freeMemory.
         */
        std::pair<bool, int32_t> freeMemory(intptr_t key);

      private:
        int32_t getSlotLocked(const hardware::hidl_memory& memory, intptr_t key);
        int32_t allocateSlotLocked();

        std::mutex mMutex;
        std::stack<int32_t, std::vector<int32_t>> mFreeSlots;
        std::map<intptr_t, int32_t> mMemoryIdToSlot;
        std::vector<hardware::hidl_memory> mMemoryCache;
    };

    /**
     * Creates a burst controller on a prepared model.
     *
     * Prefer this over ExecutionBurstController's constructor.
     *
     * @param preparedModel Model prepared for execution to execute on.
     * @param pollingTimeWindow How much time (in microseconds) the
     *     ExecutionBurstController is allowed to poll the FMQ before waiting on
     *     the blocking futex. Polling may result in lower latencies at the
     *     potential cost of more power usage.
     * @return ExecutionBurstController Execution burst controller object.
     */
    static std::unique_ptr<ExecutionBurstController> create(
            const sp<hardware::neuralnetworks::V1_2::IPreparedModel>& preparedModel,
            std::chrono::microseconds pollingTimeWindow);

    // prefer calling ExecutionBurstController::create
    ExecutionBurstController(const std::shared_ptr<RequestChannelSender>& requestChannelSender,
                             const std::shared_ptr<ResultChannelReceiver>& resultChannelReceiver,
                             const sp<hardware::neuralnetworks::V1_2::IBurstContext>& burstContext,
                             const sp<ExecutionBurstCallback>& callback,
                             const sp<hardware::hidl_death_recipient>& deathHandler = nullptr);

    // explicit destructor to unregister the death recipient
    ~ExecutionBurstController();

    /**
     * Execute a request on a model.
     *
     * @param request Arguments to be executed on a model.
     * @param measure Whether to collect timing measurements, either YES or NO
     * @param memoryIds Identifiers corresponding to each memory object in the
     *     request's pools.
     * @return A tuple of:
     *     - result code of the execution
     *     - dynamic output shapes from the execution
     *     - any execution time measurements of the execution
     *     - whether or not a failed burst execution should be re-run using a
     *       different path (e.g., IPreparedModel::executeSynchronously)
     */
    std::tuple<int, std::vector<hardware::neuralnetworks::V1_2::OutputShape>,
               hardware::neuralnetworks::V1_2::Timing, bool>
    compute(const hardware::neuralnetworks::V1_0::Request& request,
            hardware::neuralnetworks::V1_2::MeasureTiming measure,
            const std::vector<intptr_t>& memoryIds);

    /**
     * Propagate a user's freeing of memory to the service.
     *
     * @param key Key corresponding to the memory object.
     */
    void freeMemory(intptr_t key);

  private:
    std::mutex mMutex;
    const std::shared_ptr<RequestChannelSender> mRequestChannelSender;
    const std::shared_ptr<ResultChannelReceiver> mResultChannelReceiver;
    const sp<hardware::neuralnetworks::V1_2::IBurstContext> mBurstContext;
    const sp<ExecutionBurstCallback> mMemoryCache;
    const sp<hardware::hidl_death_recipient> mDeathHandler;
};

}  // namespace android::nn

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_EXECUTION_BURST_CONTROLLER_H
