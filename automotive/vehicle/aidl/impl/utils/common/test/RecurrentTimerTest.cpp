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

#include "RecurrentTimer.h"

#include <android-base/thread_annotations.h>
#include <gtest/gtest.h>
#include <condition_variable>

#include <chrono>
#include <memory>
#include <mutex>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::android::base::ScopedLockAssertion;

class RecurrentTimerTest : public testing::Test {
  public:
    std::shared_ptr<RecurrentTimer::Callback> getCallback(size_t token) {
        return std::make_shared<RecurrentTimer::Callback>([this, token] {
            std::scoped_lock<std::mutex> lockGuard(mLock);

            mCallbacks.push_back(token);
            mCond.notify_all();
        });
    }

    bool waitForCalledCallbacks(size_t count, size_t timeoutInMs) {
        std::unique_lock<std::mutex> uniqueLock(mLock);
        return mCond.wait_for(uniqueLock, std::chrono::milliseconds(timeoutInMs), [this, count] {
            ScopedLockAssertion lockAssertion(mLock);
            return mCallbacks.size() >= count;
        });
    }

    std::vector<size_t> getCalledCallbacks() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return mCallbacks;
    }

    void clearCalledCallbacks() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mCallbacks.clear();
    }

    size_t countCallbackInfoById(RecurrentTimer* timer) {
        std::scoped_lock<std::mutex> lockGuard(timer->mLock);
        return timer->mCallbackInfoById.size();
    }

    size_t countIdByCallback(RecurrentTimer* timer) {
        std::scoped_lock<std::mutex> lockGuard(timer->mLock);
        return timer->mIdByCallback.size();
    }

  private:
    std::condition_variable mCond;
    std::mutex mLock;
    std::vector<size_t> mCallbacks GUARDED_BY(mLock);
};

TEST_F(RecurrentTimerTest, testRegisterCallback) {
    RecurrentTimer timer;
    // 0.1s
    int64_t interval = 100000000;

    auto action = getCallback(0);
    timer.registerTimerCallback(interval, action);

    // Should only takes 1s, use 5s as timeout to be safe.
    ASSERT_TRUE(waitForCalledCallbacks(/* count= */ 10u, /* timeoutInMs= */ 5000))
            << "Not enough callbacks called before timeout";

    timer.unregisterTimerCallback(action);
}

TEST_F(RecurrentTimerTest, testRegisterUnregisterRegister) {
    RecurrentTimer timer;
    // 0.1s
    int64_t interval = 100000000;

    auto action = getCallback(0);
    timer.registerTimerCallback(interval, action);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    timer.unregisterTimerCallback(action);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    clearCalledCallbacks();

    timer.registerTimerCallback(interval, action);

    // Should only takes 1s, use 5s as timeout to be safe.
    ASSERT_TRUE(waitForCalledCallbacks(/* count= */ 10u, /* timeoutInMs= */ 5000))
            << "Not enough callbacks called before timeout";

    timer.unregisterTimerCallback(action);

    ASSERT_EQ(countCallbackInfoById(&timer), 0u);
    ASSERT_EQ(countIdByCallback(&timer), 0u);
}

TEST_F(RecurrentTimerTest, testDestroyTimerWithCallback) {
    std::unique_ptr<RecurrentTimer> timer = std::make_unique<RecurrentTimer>();
    // 0.1s
    int64_t interval = 100000000;

    auto action = getCallback(0);
    timer->registerTimerCallback(interval, action);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    timer.reset();

    clearCalledCallbacks();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Should be 0, but in rare cases there might be 1 events in the queue while the timer is
    // being destroyed.
    ASSERT_LE(getCalledCallbacks().size(), 1u);
}

TEST_F(RecurrentTimerTest, testRegisterMultipleCallbacks) {
    RecurrentTimer timer;
    // 0.1s
    int64_t interval1 = 100000000;
    auto action1 = getCallback(1);
    timer.registerTimerCallback(interval1, action1);
    // 0.05s
    int64_t interval2 = 50000000;
    auto action2 = getCallback(2);
    timer.registerTimerCallback(interval2, action2);
    // 0.03s
    int64_t interval3 = 30000000;
    auto action3 = getCallback(3);
    timer.registerTimerCallback(interval3, action3);

    // In 1s, we should generate 10 + 20 + 33 = 63 events.
    // Here we are waiting for more events to make sure we receive enough events for each actions.
    // Use 5s as timeout to be safe.
    ASSERT_TRUE(waitForCalledCallbacks(/* count= */ 70u, /* timeoutInMs= */ 5000))
            << "Not enough callbacks called before timeout";

    timer.unregisterTimerCallback(action1);
    timer.unregisterTimerCallback(action2);
    timer.unregisterTimerCallback(action3);

    size_t action1Count = 0;
    size_t action2Count = 0;
    size_t action3Count = 0;
    for (size_t token : getCalledCallbacks()) {
        if (token == 1) {
            action1Count++;
        }
        if (token == 2) {
            action2Count++;
        }
        if (token == 3) {
            action3Count++;
        }
    }

    ASSERT_GE(action1Count, static_cast<size_t>(10));
    ASSERT_GE(action2Count, static_cast<size_t>(20));
    ASSERT_GE(action3Count, static_cast<size_t>(33));
}

TEST_F(RecurrentTimerTest, testRegisterSameCallbackMultipleTimes) {
    RecurrentTimer timer;
    // 0.2s
    int64_t interval1 = 200'000'000;
    // 0.1s
    int64_t interval2 = 100'000'000;

    auto action = getCallback(0);
    for (int i = 0; i < 10; i++) {
        timer.registerTimerCallback(interval1, action);
        timer.registerTimerCallback(interval2, action);
    }

    clearCalledCallbacks();

    // Should only takes 1s, use 5s as timeout to be safe.
    ASSERT_TRUE(waitForCalledCallbacks(/* count= */ 10u, /* timeoutInMs= */ 5000))
            << "Not enough callbacks called before timeout";

    timer.unregisterTimerCallback(action);

    ASSERT_EQ(countCallbackInfoById(&timer), 0u);
    ASSERT_EQ(countIdByCallback(&timer), 0u);
}

TEST_F(RecurrentTimerTest, testRegisterCallbackMultipleTimesNoDeadLock) {
    // We want to avoid the following situation:
    // Caller holds a lock while calling registerTimerCallback, registerTimerCallback will try
    // to obtain an internal lock inside timer.
    // Meanwhile an recurrent action happens with timer holding an internal lock. The action
    // tries to obtain the lock currently hold by the caller.
    // The solution is that while calling recurrent actions, timer must not hold the internal lock.

    std::unique_ptr<RecurrentTimer> timer = std::make_unique<RecurrentTimer>();
    std::mutex lock;
    for (size_t i = 0; i < 1000; i++) {
        std::scoped_lock<std::mutex> lockGuard(lock);
        auto action = std::make_shared<RecurrentTimer::Callback>([&lock] {
            // While calling this function, the timer must not hold lock in order not to dead
            // lock.
            std::scoped_lock<std::mutex> lockGuard(lock);
        });
        // 10ms
        int64_t interval = 10'000'000;
        timer->registerTimerCallback(interval, action);
        // Sleep for a little while to let the recurrent actions begin.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // Make sure we stop the timer before we destroy lock.
    timer.reset();
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
