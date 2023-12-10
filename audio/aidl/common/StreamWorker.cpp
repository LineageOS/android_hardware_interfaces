/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>

#include "include/StreamWorker.h"

namespace android::hardware::audio::common::internal {

bool ThreadController::start(const std::string& name, int priority) {
    mThreadName = name;
    mThreadPriority = priority;
    if (kTestSingleThread != name) {
        mWorker = std::thread(&ThreadController::workerThread, this);
    } else {
        // Simulate the case when the workerThread completes prior
        // to the moment when we being waiting for its start.
        workerThread();
    }
    std::unique_lock<std::mutex> lock(mWorkerLock);
    android::base::ScopedLockAssertion lock_assertion(mWorkerLock);
    mWorkerCv.wait(lock, [&]() {
        android::base::ScopedLockAssertion lock_assertion(mWorkerLock);
        return mWorkerState != WorkerState::INITIAL || !mError.empty();
    });
    return mError.empty();
}

void ThreadController::stop() {
    {
        std::lock_guard<std::mutex> lock(mWorkerLock);
        if (mWorkerState != WorkerState::STOPPED) {
            mWorkerState = WorkerState::STOPPED;
            mWorkerStateChangeRequest = true;
        }
    }
    join();
}

void ThreadController::join() {
    if (mWorker.joinable()) {
        mWorker.join();
    }
}

bool ThreadController::waitForAtLeastOneCycle() {
    WorkerState newState;
    switchWorkerStateSync(WorkerState::RUNNING, WorkerState::PAUSE_REQUESTED, &newState);
    if (newState != WorkerState::PAUSED) return false;
    switchWorkerStateSync(newState, WorkerState::RESUME_REQUESTED, &newState);
    return newState == WorkerState::RUNNING;
}

void ThreadController::switchWorkerStateSync(WorkerState oldState, WorkerState newState,
                                             WorkerState* finalState) {
    std::unique_lock<std::mutex> lock(mWorkerLock);
    android::base::ScopedLockAssertion lock_assertion(mWorkerLock);
    if (mWorkerState != oldState) {
        if (finalState) *finalState = mWorkerState;
        return;
    }
    mWorkerState = newState;
    mWorkerStateChangeRequest = true;
    mWorkerCv.wait(lock, [&]() {
        android::base::ScopedLockAssertion lock_assertion(mWorkerLock);
        return mWorkerState != newState;
    });
    if (finalState) *finalState = mWorkerState;
}

void ThreadController::workerThread() {
    using Status = StreamLogic::Status;

    std::string error;
    if (!mThreadName.empty()) {
        std::string compliantName(mThreadName.substr(0, 15));
        if (int errCode = pthread_setname_np(pthread_self(), compliantName.c_str()); errCode != 0) {
            error.append("Failed to set thread name: ").append(strerror(errCode));
        }
    }
    if (error.empty() && mThreadPriority != ANDROID_PRIORITY_DEFAULT) {
        if (int result = setpriority(PRIO_PROCESS, 0, mThreadPriority); result != 0) {
            int errCode = errno;
            error.append("Failed to set thread priority: ").append(strerror(errCode));
        }
    }
    if (error.empty()) {
        error.append(mLogic->init());
    }
    {
        std::lock_guard<std::mutex> lock(mWorkerLock);
        mWorkerState = error.empty() ? WorkerState::RUNNING : WorkerState::STOPPED;
        mError = error;
#if defined(__ANDROID__)
        mTid = pthread_gettid_np(pthread_self());
#endif
    }
    mWorkerCv.notify_one();
    if (!error.empty()) return;

    for (WorkerState state = WorkerState::RUNNING; state != WorkerState::STOPPED;) {
        bool needToNotify = false;
        if (Status status = state != WorkerState::PAUSED ? mLogic->cycle()
                                                         : (sched_yield(), Status::CONTINUE);
            status == Status::CONTINUE) {
            {
                // See https://developer.android.com/training/articles/smp#nonracing
                android::base::ScopedLockAssertion lock_assertion(mWorkerLock);
                if (!mWorkerStateChangeRequest.load(std::memory_order_relaxed)) continue;
            }
            //
            // Pause and resume are synchronous. One worker cycle must complete
            // before the worker indicates a state change. This is how 'mWorkerState' and
            // 'state' interact:
            //
            // mWorkerState == RUNNING
            // client sets mWorkerState := PAUSE_REQUESTED
            // last workerCycle gets executed, state := mWorkerState := PAUSED by us
            //   (or the workers enters the 'error' state if workerCycle fails)
            // client gets notified about state change in any case
            // thread is doing a busy wait while 'state == PAUSED'
            // client sets mWorkerState := RESUME_REQUESTED
            // state := mWorkerState (RESUME_REQUESTED)
            // mWorkerState := RUNNING, but we don't notify the client yet
            // first workerCycle gets executed, the code below triggers a client notification
            //   (or if workerCycle fails, worker enters 'error' state and also notifies)
            // state := mWorkerState (RUNNING)
            std::lock_guard<std::mutex> lock(mWorkerLock);
            if (state == WorkerState::RESUME_REQUESTED) {
                needToNotify = true;
            }
            state = mWorkerState;
            if (mWorkerState == WorkerState::PAUSE_REQUESTED) {
                state = mWorkerState = WorkerState::PAUSED;
                needToNotify = true;
            } else if (mWorkerState == WorkerState::RESUME_REQUESTED) {
                mWorkerState = WorkerState::RUNNING;
            }
        } else {
            std::lock_guard<std::mutex> lock(mWorkerLock);
            if (state == WorkerState::RESUME_REQUESTED ||
                mWorkerState == WorkerState::PAUSE_REQUESTED) {
                needToNotify = true;
            }
            state = mWorkerState = WorkerState::STOPPED;
            if (status == Status::ABORT) {
                mError = "Received ABORT from the logic cycle";
            }
        }
        if (needToNotify) {
            {
                std::lock_guard<std::mutex> lock(mWorkerLock);
                mWorkerStateChangeRequest = false;
            }
            mWorkerCv.notify_one();
        }
    }
}

}  // namespace android::hardware::audio::common::internal
