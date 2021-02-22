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

#include "WorkerThread.h"

namespace aidl::android::hardware::biometrics::fingerprint {

// It's important that mThread is initialized after everything else because it runs a member
// function that may use any member of this class.
WorkerThread::WorkerThread(size_t maxQueueSize)
    : mMaxSize(maxQueueSize),
      mIsDestructing(false),
      mQueue(),
      mQueueMutex(),
      mQueueCond(),
      mThread(&WorkerThread::threadFunc, this) {}

WorkerThread::~WorkerThread() {
    // This is a signal for threadFunc to terminate as soon as possible, and a hint for schedule
    // that it doesn't need to do any work.
    mIsDestructing = true;
    mQueueCond.notify_all();
    mThread.join();
}

bool WorkerThread::schedule(std::unique_ptr<Callable> task) {
    if (mIsDestructing) {
        return false;
    }

    std::unique_lock<std::mutex> lock(mQueueMutex);
    if (mQueue.size() >= mMaxSize) {
        return false;
    }
    mQueue.push_back(std::move(task));
    lock.unlock();
    mQueueCond.notify_one();
    return true;
}

void WorkerThread::threadFunc() {
    while (!mIsDestructing) {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        mQueueCond.wait(lock, [this] { return !mQueue.empty() || mIsDestructing; });
        if (mIsDestructing) {
            return;
        }
        std::unique_ptr<Callable> task = std::move(mQueue.front());
        mQueue.pop_front();
        lock.unlock();
        (*task)();
    }
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
