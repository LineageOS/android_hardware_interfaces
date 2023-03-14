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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_RecurrentTimer_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_RecurrentTimer_H_

#include <android-base/thread_annotations.h>

#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// A thread-safe recurrent timer.
class RecurrentTimer final {
  public:
    // The class for the function that would be called recurrently.
    using Callback = std::function<void()>;

    RecurrentTimer();

    ~RecurrentTimer();

    // Registers a recurrent callback for a given interval.
    // Registering the same callback twice will override the interval provided before.
    void registerTimerCallback(int64_t intervalInNano, std::shared_ptr<Callback> callback);

    // Unregisters a previously registered recurrent callback.
    void unregisterTimerCallback(std::shared_ptr<Callback> callback);

  private:
    // friend class for unit testing.
    friend class RecurrentTimerTest;

    struct CallbackInfo {
        std::shared_ptr<Callback> callback;
        int64_t interval;
        int64_t nextTime;
        // A flag to indicate whether this CallbackInfo is already outdated and should be ignored.
        // The reason we need this flag is because we cannot easily remove an element from a heap.
        bool outdated = false;

        static bool cmp(const std::unique_ptr<CallbackInfo>& lhs,
                        const std::unique_ptr<CallbackInfo>& rhs);
    };

    std::mutex mLock;
    std::thread mThread;
    std::condition_variable mCond;
    bool mStopRequested GUARDED_BY(mLock) = false;
    // A map to map each callback to its current active CallbackInfo in the mCallbackQueue.
    std::unordered_map<std::shared_ptr<Callback>, CallbackInfo*> mCallbacks GUARDED_BY(mLock);
    // A min-heap sorted by nextTime. Note that because we cannot remove arbitrary element from the
    // heap, a single Callback can have multiple entries in this queue, all but one should be valid.
    // The rest should be mark as outdated. The valid one is one stored in mCallbacks.
    std::vector<std::unique_ptr<CallbackInfo>> mCallbackQueue GUARDED_BY(mLock);

    void loop();

    // Mark the callbackInfo as outdated and should be ignored when popped from the heap.
    void markOutdatedLocked(CallbackInfo* callback) REQUIRES(mLock);
    // Remove all outdated callbackInfos from the top of the heap. This function must be called
    // each time we might introduce outdated elements to the top. We must make sure the heap is
    // always valid from the top.
    void removeInvalidCallbackLocked() REQUIRES(mLock);
    // Gets the next calblack to run (must be valid) from the heap, update its nextTime and put
    // it back to the heap.
    std::shared_ptr<Callback> getNextCallbackLocked(int64_t now) REQUIRES(mLock);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_RecurrentTimer_H_
