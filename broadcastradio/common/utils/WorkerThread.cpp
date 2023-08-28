/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <broadcastradio-utils/WorkerThread.h>

namespace android {

using std::function;
using std::lock_guard;
using std::mutex;
using std::unique_lock;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::this_thread::sleep_for;

bool operator<(const WorkerThread::Task& lhs, const WorkerThread::Task& rhs) {
    return lhs.when > rhs.when;
}

WorkerThread::WorkerThread() : mIsTerminating(false) {
    // putting mThread in constructor instead of initializer list
    // to ensure all class members are init before mThread starts
    mThread = std::thread(&WorkerThread::threadLoop, this);
}

WorkerThread::~WorkerThread() {
    {
        lock_guard<mutex> lk(mMut);
        mIsTerminating = true;
        mCond.notify_one();
    }
    mThread.join();
}

void WorkerThread::schedule(function<void()> task, milliseconds delay) {
    auto cancelTask = []() {};
    schedule(std::move(task), cancelTask, delay);
}

void WorkerThread::schedule(function<void()> task, function<void()> cancelTask,
                            milliseconds delay) {
    auto when = steady_clock::now() + delay;

    lock_guard<mutex> lk(mMut);
    mTasks.push(Task({when, std::move(task), std::move(cancelTask)}));
    mCond.notify_one();
}

void WorkerThread::cancelAll() {
    lock_guard<mutex> lk(mMut);
    while (!mTasks.empty()) {
        auto task = mTasks.top();
        task.onCanceled();
        mTasks.pop();
    }
}

void WorkerThread::threadLoop() {
    while (!mIsTerminating) {
        unique_lock<mutex> lk(mMut);
        if (mTasks.empty()) {
            mCond.wait(lk);
            continue;
        }

        auto task = mTasks.top();
        if (task.when > steady_clock::now()) {
            mCond.wait_until(lk, task.when);
            continue;
        }

        mTasks.pop();
        lk.unlock();  // what() might need to schedule another task
        task.what();
    }
}

}  // namespace android
