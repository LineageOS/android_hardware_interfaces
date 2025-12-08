/*
 * Copyright 2025 The Android Open Source Project
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

#include "bluetooth_hal/util/timer_manager.h"

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>

#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace util {
namespace {

using ::testing::Test;

class TimerManagerTest : public Test {
 protected:
  /**
   * @brief Returns a pair of promise and future. This is a helper for testing
   * if a scheduled task on a timer is executed.
   *
   * Example:
   *  auto [promise, future] = GetPromiseFuturePair();
   *  timer.Schedule([&promise]() { promise->set_value(); }, milliseconds(10));
   *  ASSERT_EQ(std::future_status::ready, future.wait_for(milliseconds(100)));
   *
   * @return A pair of promise and future.
   */
  std::pair<std::unique_ptr<std::promise<void>>, std::future<void>>
  GetPromiseFuturePair() {
    std::unique_ptr<std::promise<void>> promise =
        std::make_unique<std::promise<void>>();
    std::future<void> future = promise->get_future();
    return std::make_pair(std::move(promise), std::move(future));
  }
};

TEST_F(TimerManagerTest, ScheduleTask) {
  Timer timer;
  auto [promise, future] = GetPromiseFuturePair();
  ASSERT_TRUE(timer.Schedule([&promise]() { promise->set_value(); },
                             std::chrono::milliseconds(50)));
  // The task should not be executed yet.
  ASSERT_NE(std::future_status::ready,
            future.wait_for(std::chrono::milliseconds(10)));
  // The task should be executed after 50ms, set a timeout of 100ms.
  ASSERT_EQ(std::future_status::ready,
            future.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, CancelTask) {
  Timer timer;
  auto [promise, future] = GetPromiseFuturePair();
  ASSERT_TRUE(timer.Schedule([&promise]() { promise->set_value(); },
                             std::chrono::milliseconds(10)));
  ASSERT_TRUE(timer.Cancel());
  ASSERT_NE(std::future_status::ready,
            future.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, CancelNonExistingTask) {
  Timer timer;
  ASSERT_FALSE(timer.Cancel());
}

TEST_F(TimerManagerTest, ScheduleMultipleTasks) {
  Timer timer1, timer2;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer1.Schedule([&promise1]() { promise1->set_value(); },
                              std::chrono::milliseconds(50)));
  ASSERT_TRUE(timer2.Schedule([&promise2]() { promise2->set_value(); },
                              std::chrono::milliseconds(100)));
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(10)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(10)));
  ASSERT_EQ(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(150)));
}

TEST_F(TimerManagerTest, SecondTimerFiresFirst) {
  Timer timer1, timer2;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer1.Schedule([&promise1]() { promise1->set_value(); },
                              std::chrono::milliseconds(100)));
  ASSERT_TRUE(timer2.Schedule([&promise2]() { promise2->set_value(); },
                              std::chrono::milliseconds(50)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(10)));
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(10)));
  ASSERT_EQ(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(150)));
}

TEST_F(TimerManagerTest, ScheduleMultipleTasksFireAtTheSameTime) {
  Timer timer1, timer2;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer1.Schedule([&promise1]() { promise1->set_value(); },
                              std::chrono::milliseconds(50)));
  ASSERT_TRUE(timer2.Schedule([&promise2]() { promise2->set_value(); },
                              std::chrono::milliseconds(50)));
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(10)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(10)));
  ASSERT_EQ(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, ScheduleMultipleTasksWithCancel) {
  Timer timer1, timer2;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer1.Schedule([&promise1]() { promise1->set_value(); },
                              std::chrono::milliseconds(50)));
  ASSERT_TRUE(timer2.Schedule([&promise2]() { promise2->set_value(); },
                              std::chrono::milliseconds(60)));
  ASSERT_TRUE(timer1.Cancel());
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(10)));
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, Reschedule) {
  Timer timer;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer.Schedule([&promise1]() { promise1->set_value(); },
                             std::chrono::milliseconds(50)));
  ASSERT_TRUE(timer.Schedule([&promise2]() { promise2->set_value(); },
                             std::chrono::milliseconds(50)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(10)));
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, WillNotCancelItself) {
  Timer timer;
  auto [promise, future] = GetPromiseFuturePair();
  ASSERT_TRUE(timer.Schedule(
      [&timer, &promise]() {
        // This cancel will be a no-op since the task is already fired.
        ASSERT_FALSE(timer.Cancel());
        promise->set_value();
      },
      std::chrono::milliseconds(50)));
  ASSERT_NE(std::future_status::ready,
            future.wait_for(std::chrono::milliseconds(10)));
  ASSERT_EQ(std::future_status::ready,
            future.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, CanScheduleAnotherTaskInTask) {
  Timer timer;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer.Schedule(
      [&timer, &promise1, &promise2]() {
        ASSERT_TRUE(timer.Schedule([&promise2]() { promise2->set_value(); },
                                   std::chrono::milliseconds(10)));
        promise1->set_value();
      },
      std::chrono::milliseconds(50)));
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(10)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(10)));
  ASSERT_EQ(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, ScheduleTaskOnAnotherTimerInTask) {
  Timer timer1, timer2;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer1.Schedule(
      [&timer2, &promise1, &promise2]() {
        ASSERT_TRUE(timer2.Schedule([&promise2]() { promise2->set_value(); },
                                    std::chrono::milliseconds(10)));
        promise1->set_value();
      },
      std::chrono::milliseconds(50)));
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(10)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(10)));
  ASSERT_EQ(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, CancelTaskWhichSchedulesAnotherTask) {
  Timer timer;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer.Schedule(
      [&timer, &promise1, &promise2]() {
        ASSERT_TRUE(timer.Schedule([&promise2]() { promise2->set_value(); },
                                   std::chrono::milliseconds(10)));
        promise1->set_value();
      },
      std::chrono::milliseconds(10)));
  ASSERT_TRUE(timer.Cancel());
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, CancelTaskScheduledInTask) {
  Timer timer;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer.Schedule(
      [&timer, &promise1, &promise2]() {
        ASSERT_TRUE(timer.Schedule([&promise2]() { promise2->set_value(); },
                                   std::chrono::milliseconds(20)));
        promise1->set_value();
      },
      std::chrono::milliseconds(50)));
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(10)));
  // First task should be fired after 50ms, and then we can cancel the second.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  ASSERT_TRUE(timer.Cancel());
  ASSERT_EQ(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, CancelTaskWhichCancelsAnotherTask) {
  Timer timer1, timer2;
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  ASSERT_TRUE(timer1.Schedule([&promise1]() { promise1->set_value(); },
                              std::chrono::milliseconds(50)));
  ASSERT_TRUE(timer2.Schedule(
      [&timer1, &promise2]() {
        FAIL();
        timer1.Cancel();
        promise2->set_value();
      },
      std::chrono::milliseconds(50)));
  ASSERT_TRUE(timer2.Cancel());
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(10)));
  ASSERT_EQ(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, DestroyTimerBeforeExpire) {
  auto [promise, future] = GetPromiseFuturePair();
  {
    Timer timer;
    ASSERT_TRUE(timer.Schedule([&promise]() { promise->set_value(); },
                               std::chrono::milliseconds(10)));
  }
  ASSERT_NE(std::future_status::ready,
            future.wait_for(std::chrono::milliseconds(100)));
}

TEST_F(TimerManagerTest, FireTimerAfterTheEarlyOneThatHasDestroyed) {
  auto [promise1, future1] = GetPromiseFuturePair();
  auto [promise2, future2] = GetPromiseFuturePair();
  Timer timer1;
  ASSERT_TRUE(timer1.Schedule([&promise1]() { promise1->set_value(); },
                              std::chrono::milliseconds(50)));
  {
    Timer timer2;
    ASSERT_TRUE(timer2.Schedule([&promise2]() { promise2->set_value(); },
                                std::chrono::milliseconds(5)));
  }
  ASSERT_NE(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(10)));
  ASSERT_EQ(std::future_status::ready,
            future1.wait_for(std::chrono::milliseconds(100)));
  ASSERT_NE(std::future_status::ready,
            future2.wait_for(std::chrono::milliseconds(100)));
}

}  // namespace
}  // namespace util
}  // namespace bluetooth_hal
