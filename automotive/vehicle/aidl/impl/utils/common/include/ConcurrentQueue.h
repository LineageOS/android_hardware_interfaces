/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_aidl_impl_utils_common_include_ConcurrentQueue_H_
#define android_hardware_automotive_vehicle_aidl_impl_utils_common_include_ConcurrentQueue_H_

#include <android-base/thread_annotations.h>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

template <typename T>
class ConcurrentQueue {
  public:
    bool waitForItems() {
        std::unique_lock<std::mutex> lockGuard(mLock);
        android::base::ScopedLockAssertion lockAssertion(mLock);
        while (mQueue.empty() && mIsActive) {
            mCond.wait(lockGuard);
        }
        return mIsActive;
    }

    std::vector<T> flush() {
        std::vector<T> items;

        std::scoped_lock<std::mutex> lockGuard(mLock);
        if (mQueue.empty()) {
            return items;
        }
        while (!mQueue.empty()) {
            // Even if the queue is deactivated, we should still flush all the remaining values
            // in the queue.
            items.push_back(std::move(mQueue.front()));
            mQueue.pop();
        }
        return items;
    }

    void push(T&& item) {
        {
            std::scoped_lock<std::mutex> lockGuard(mLock);
            if (!mIsActive) {
                return;
            }
            mQueue.push(std::move(item));
        }
        mCond.notify_one();
    }

    // Deactivates the queue, thus no one can push items to it, also notifies all waiting thread.
    // The items already in the queue could still be flushed even after the queue is deactivated.
    void deactivate() {
        {
            std::scoped_lock<std::mutex> lockGuard(mLock);
            mIsActive = false;
        }
        // To unblock all waiting consumers.
        mCond.notify_all();
    }

    ConcurrentQueue() = default;

    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

  private:
    mutable std::mutex mLock;
    bool mIsActive GUARDED_BY(mLock) = true;
    std::condition_variable mCond;
    std::queue<T> mQueue GUARDED_BY(mLock);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_ConcurrentQueue_H_
