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

    void push(std::vector<T>&& items) {
        {
            std::scoped_lock<std::mutex> lockGuard(mLock);
            if (!mIsActive) {
                return;
            }
            for (T& item : items) {
                mQueue.push(std::move(item));
            }
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

template <typename T>
class BatchingConsumer {
  private:
    enum class State {
        INIT = 0,
        RUNNING = 1,
        STOP_REQUESTED = 2,
        STOPPED = 3,
    };

  public:
    BatchingConsumer() : mState(State::INIT) {}

    BatchingConsumer(const BatchingConsumer&) = delete;
    BatchingConsumer& operator=(const BatchingConsumer&) = delete;

    using OnBatchReceivedFunc = std::function<void(std::vector<T> vec)>;

    void run(ConcurrentQueue<T>* queue, std::chrono::nanoseconds batchInterval,
             const OnBatchReceivedFunc& func) {
        mQueue = queue;
        mBatchInterval = batchInterval;

        mWorkerThread = std::thread(&BatchingConsumer<T>::runInternal, this, func);
    }

    void requestStop() { mState = State::STOP_REQUESTED; }

    void waitStopped() {
        if (mWorkerThread.joinable()) {
            mWorkerThread.join();
        }
    }

  private:
    void runInternal(const OnBatchReceivedFunc& onBatchReceived) {
        if (mState.exchange(State::RUNNING) == State::INIT) {
            while (State::RUNNING == mState) {
                mQueue->waitForItems();
                if (State::STOP_REQUESTED == mState) break;

                std::this_thread::sleep_for(mBatchInterval);
                if (State::STOP_REQUESTED == mState) break;

                std::vector<T> items = mQueue->flush();

                if (items.size() > 0) {
                    onBatchReceived(std::move(items));
                }
            }
        }

        mState = State::STOPPED;
    }

  private:
    std::thread mWorkerThread;

    std::atomic<State> mState;
    std::chrono::nanoseconds mBatchInterval;
    ConcurrentQueue<T>* mQueue;
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_ConcurrentQueue_H_
