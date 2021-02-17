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

using aidl::android::hardware::biometrics::fingerprint::WorkerThread;
using namespace std::chrono_literals;

TEST(WorkerThreadTest, ScheduleReturnsTrueWhenQueueHasSpace) {
    WorkerThread worker(1 /*maxQueueSize*/);
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(worker.schedule([] {}));
        // Allow enough time for the previous task to be processed.
        std::this_thread::sleep_for(2ms);
    }
}

TEST(WorkerThreadTest, ScheduleReturnsFalseWhenQueueIsFull) {
    WorkerThread worker(2 /*maxQueueSize*/);
    // Add a long-running task.
    worker.schedule([] { std::this_thread::sleep_for(1s); });

    // Allow enough time for the worker to start working on the previous task.
    std::this_thread::sleep_for(2ms);

    // Fill the worker's queue to the maximum.
    worker.schedule([] {});
    worker.schedule([] {});

    EXPECT_FALSE(worker.schedule([] {}));
}

TEST(WorkerThreadTest, TasksExecuteInOrder) {
    constexpr int NUM_TASKS = 10000;
    WorkerThread worker(NUM_TASKS);

    std::vector<int> results;
    for (int i = 0; i < NUM_TASKS; ++i) {
        worker.schedule([&results, i] {
            // Delay tasks differently to provoke races.
            std::this_thread::sleep_for(std::chrono::nanoseconds(100 - i % 100));
            // Unguarded write to results to provoke races.
            results.push_back(i);
        });
    }

    std::promise<void> promise;
    auto future = promise.get_future();

    // Schedule a special task to signal when all of the tasks are finished.
    worker.schedule([&promise] { promise.set_value(); });
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

    {
        WorkerThread worker(2 /*maxQueueSize*/);
        worker.schedule([&promise1] {
            promise1.set_value();
            std::this_thread::sleep_for(200ms);
        });
        worker.schedule([&promise2] { promise2.set_value(); });

        // Make sure the first task is executing.
        auto status1 = future1.wait_for(1s);
        ASSERT_EQ(status1, std::future_status::ready);
    }

    // The second task should never execute.
    auto status2 = future2.wait_for(1s);
    EXPECT_EQ(status2, std::future_status::timeout);
}

}  // namespace
