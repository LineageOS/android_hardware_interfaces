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

#include <chrono>
#include <memory>
#include <mutex>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

class RecurrentTimerTest : public testing::Test {
  public:
    std::shared_ptr<RecurrentTimer::Callback> getCallback(size_t token) {
        return std::make_shared<RecurrentTimer::Callback>([this, token] {
            std::scoped_lock<std::mutex> lockGuard(mLock);

            mCallbacks.push_back(token);
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

    size_t countTimerCallbackQueue(RecurrentTimer* timer) {
        std::scoped_lock<std::mutex> lockGuard(timer->mLock);
        return timer->mCallbackQueue.size();
    }

  private:
    std::mutex mLock;
    std::vector<size_t> mCallbacks GUARDED_BY(mLock);
};

TEST_F(RecurrentTimerTest, testRegisterCallback) {
    RecurrentTimer timer;
    // 0.1s
    int64_t interval = 100000000;

    auto action = getCallback(0);
    timer.registerTimerCallback(interval, action);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    timer.unregisterTimerCallback(action);

    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    ASSERT_GE(getCalledCallbacks().size(), static_cast<size_t>(9));
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

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    ASSERT_GE(getCalledCallbacks().size(), static_cast<size_t>(9));
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

    ASSERT_TRUE(getCalledCallbacks().empty());
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

    std::this_thread::sleep_for(std::chrono::seconds(1));

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
    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    ASSERT_GE(action1Count, static_cast<size_t>(9));
    // Theoretically trigger 20 times, but check for at least 15 times to be stable.
    ASSERT_GE(action2Count, static_cast<size_t>(15));
    // Theoretically trigger 33 times, but check for at least 25 times to be stable.
    ASSERT_GE(action3Count, static_cast<size_t>(25));
}

TEST_F(RecurrentTimerTest, testRegisterSameCallbackMultipleTimes) {
    RecurrentTimer timer;
    // 0.02s
    int64_t interval1 = 20000000;
    // 0.01s
    int64_t interval2 = 10000000;

    auto action = getCallback(0);
    for (int i = 0; i < 10; i++) {
        timer.registerTimerCallback(interval1, action);
        timer.registerTimerCallback(interval2, action);
    }

    clearCalledCallbacks();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    ASSERT_GE(getCalledCallbacks().size(), static_cast<size_t>(9));

    timer.unregisterTimerCallback(action);

    // Make sure there is no item in the callback queue.
    ASSERT_EQ(countTimerCallbackQueue(&timer), static_cast<size_t>(0));
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
