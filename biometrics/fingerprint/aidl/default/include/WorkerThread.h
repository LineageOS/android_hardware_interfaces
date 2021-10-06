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

#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "Callable.h"

namespace aidl::android::hardware::biometrics::fingerprint {

// A class that encapsulates a worker thread and a task queue, and provides a convenient interface
// for a Session to schedule its tasks for asynchronous execution.
class WorkerThread final {
  public:
    // Internally creates a queue that cannot exceed maxQueueSize elements and a new thread that
    // polls the queue for tasks until this instance is destructed.
    explicit WorkerThread(size_t maxQueueSize);

    // Unblocks the internal queue and calls join on the internal thread allowing it to gracefully
    // exit.
    ~WorkerThread();

    // Disallow copying this class.
    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    // Also disable moving this class to simplify implementation.
    WorkerThread(WorkerThread&&) = delete;
    WorkerThread& operator=(WorkerThread&&) = delete;

    // If the internal queue is not full, pushes a task at the end of the queue and returns true.
    // Otherwise, returns false. If the queue is busy, blocks until it becomes available.
    // This method expects heap-allocated tasks because it's the simplest way to represent function
    // objects of any type. Stack-allocated std::function could be used instead, but it cannot
    // represent functions with move-only captures because std::function is inherently copyable.
    // Not being able to pass move-only lambdas is a major limitation for the HAL implementation,
    // so heap-allocated tasks that share a common interface (Callable) were chosen instead.
    bool schedule(std::unique_ptr<Callable> task);

  private:
    // The function that runs on the internal thread. Sequentially runs the available tasks from
    // the queue. If the queue is empty, waits until a new task is added. If the worker is being
    // destructed, finishes its current task and gracefully exits.
    void threadFunc();

    // The maximum size that the queue is allowed to expand to.
    size_t mMaxSize;

    // Whether the destructor was called. If true, tells threadFunc to exit as soon as possible, and
    // tells schedule to avoid doing any work.
    std::atomic<bool> mIsDestructing;

    // Queue that's guarded by mQueueMutex and mQueueCond.
    std::deque<std::unique_ptr<Callable>> mQueue;
    std::mutex mQueueMutex;
    std::condition_variable mQueueCond;

    // The internal thread that works on the tasks from the queue.
    std::thread mThread;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
