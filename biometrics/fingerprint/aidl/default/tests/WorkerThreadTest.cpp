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

#include <algorithm>
#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>

#include "WorkerThread.h"

namespace {

using aidl::android::hardware::biometrics::fingerprint::Callable;
using aidl::android::hardware::biometrics::fingerprint::WorkerThread;
using namespace std::chrono_literals;

TEST(WorkerThreadTest, ScheduleReturnsTrueWhenQueueHasSpace) {
    WorkerThread worker(1 /*maxQueueSize*/);
    for (int i = 0; i < 100; ++i) {
        std::promise<void> promise;
        auto future = promise.get_future();

        ASSERT_TRUE(worker.schedule(Callable::from([promise = std::move(promise)]() mutable {
            // Notify that the task has started.
            promise.set_value();
        })));

        auto status = future.wait_for(1s);
        EXPECT_EQ(status, std::future_status::ready);
    }
}

TEST(WorkerThreadTest, ScheduleReturnsFalseWhenQueueIsFull) {
    WorkerThread worker(2 /*maxQueueSize*/);

    std::promise<void> promise;
    auto future = promise.get_future();

    // Schedule a long-running task.
    ASSERT_TRUE(worker.schedule(Callable::from([promise = std::move(promise)]() mutable {
        // Notify that the task has started.
        promise.set_value();
        // Block for a "very long" time.
        std::this_thread::sleep_for(2s);
    })));

    // Make sure the long-running task began executing.
    auto status = future.wait_for(1s);
    ASSERT_EQ(status, std::future_status::ready);

    // The first task is already being worked on, which means the queue must be empty.
    // Fill the worker's queue to the maximum.
    ASSERT_TRUE(worker.schedule(Callable::from([] {})));
    ASSERT_TRUE(worker.schedule(Callable::from([] {})));

    EXPECT_FALSE(worker.schedule(Callable::from([] {})));
}

TEST(WorkerThreadTest, TasksExecuteInOrder) {
    constexpr int NUM_TASKS = 10000;
    WorkerThread worker(NUM_TASKS);

    std::vector<int> results;
    for (int i = 0; i < NUM_TASKS; ++i) {
        worker.schedule(Callable::from([&results, i] {
            // Delay tasks differently to provoke races.
            std::this_thread::sleep_for(std::chrono::nanoseconds(100 - i % 100));
            // Unguarded write to results to provoke races.
            results.push_back(i);
        }));
    }

    std::promise<void> promise;
    auto future = promise.get_future();

    // Schedule a special task to signal when all of the tasks are finished.
    worker.schedule(
            Callable::from([promise = std::move(promise)]() mutable { promise.set_value(); }));
    auto status = future.wait_for(1s);
    ASSERT_EQ(status, std::future_status::ready);

    ASSERT_EQ(results.size(), NUM_TASKS);
    EXPECT_TRUE(std::is_sorted(results.begin(), results.end()));
}

TEST(WorkerThreadTest, ExecutionStopsAfterWorkerIsDestroyed) {
    std::promise<void> promise1;
    std::promise<void> promise2;
    auto future1 = promise1.get_future();
    auto future2 = promise2.get_future();
    std::atomic<bool> value;

    // Local scope for the worker to test its destructor when it goes out of scope.
    {
        WorkerThread worker(2 /*maxQueueSize*/);

        ASSERT_TRUE(worker.schedule(Callable::from([promise = std::move(promise1)]() mutable {
            promise.set_value();
            std::this_thread::sleep_for(200ms);
        })));

        // The first task should start executing.
        auto status = future1.wait_for(1s);
        ASSERT_EQ(status, std::future_status::ready);

        // The second task should schedule successfully.
        ASSERT_TRUE(
                worker.schedule(Callable::from([promise = std::move(promise2), &value]() mutable {
                    // The worker should destruct before it gets a chance to execute this.
                    value = true;
                    promise.set_value();
                })));
    }

    // The second task should never execute.
    auto status = future2.wait_for(1s);
    ASSERT_EQ(status, std::future_status::ready);
    // The future is expected to be ready but contain an exception.
    // Cannot use ASSERT_THROW because exceptions are disabled in this codebase.
    // ASSERT_THROW(future2.get(), std::future_error);
    EXPECT_FALSE(value);
}

}  // namespace
