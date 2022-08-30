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

#pragma once

#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <android-base/thread_annotations.h>
#include <system/thread_defs.h>

namespace android::hardware::audio::common {

template <typename Impl>
class StreamWorker {
    enum class WorkerState { STOPPED, RUNNING, PAUSE_REQUESTED, PAUSED, RESUME_REQUESTED };

  public:
    enum class WorkerStatus { ABORT, CONTINUE, EXIT };

    StreamWorker() = default;
    ~StreamWorker() { stop(); }
    // Note that 'priority' here is what is known as the 'nice number' in *nix systems.
    // The nice number is used with the default scheduler. For threads that
    // need to use a specialized scheduler (e.g. SCHED_FIFO) and set the priority within it,
    // it is recommended to implement an appropriate configuration sequence within `workerInit`.
    bool start(const std::string& name = "", int priority = ANDROID_PRIORITY_DEFAULT) {
        mThreadName = name;
        mThreadPriority = priority;
        mWorker = std::thread(&StreamWorker::workerThread, this);
        std::unique_lock<std::mutex> lock(mWorkerLock);
        android::base::ScopedLockAssertion lock_assertion(mWorkerLock);
        mWorkerCv.wait(lock, [&]() {
            android::base::ScopedLockAssertion lock_assertion(mWorkerLock);
            return mWorkerState == WorkerState::RUNNING || !mError.empty();
        });
        mWorkerStateChangeRequest = false;
        return mWorkerState == WorkerState::RUNNING;
    }
    void pause() { switchWorkerStateSync(WorkerState::RUNNING, WorkerState::PAUSE_REQUESTED); }
    void resume() { switchWorkerStateSync(WorkerState::PAUSED, WorkerState::RESUME_REQUESTED); }
    bool hasError() {
        std::lock_guard<std::mutex> lock(mWorkerLock);
        return !mError.empty();
    }
    std::string getError() {
        std::lock_guard<std::mutex> lock(mWorkerLock);
        return mError;
    }
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mWorkerLock);
            if (mWorkerState != WorkerState::STOPPED) {
                mWorkerState = WorkerState::STOPPED;
                mWorkerStateChangeRequest = true;
            }
        }
        if (mWorker.joinable()) {
            mWorker.join();
        }
    }
    bool waitForAtLeastOneCycle() {
        WorkerState newState;
        switchWorkerStateSync(WorkerState::RUNNING, WorkerState::PAUSE_REQUESTED, &newState);
        if (newState != WorkerState::PAUSED) return false;
        switchWorkerStateSync(newState, WorkerState::RESUME_REQUESTED, &newState);
        return newState == WorkerState::RUNNING;
    }
    // Only used by unit tests.
    void testLockUnlockMutex(bool lock) NO_THREAD_SAFETY_ANALYSIS {
        lock ? mWorkerLock.lock() : mWorkerLock.unlock();
    }
    std::thread::native_handle_type testGetThreadNativeHandle() { return mWorker.native_handle(); }

    // Methods that need to be provided by subclasses:
    //
    // /* Called once at the beginning of the thread loop. Must return
    //  * an empty string to enter the thread loop, otherwise the thread loop
    //  * exits and the worker switches into the 'error' state, setting
    //  * the error to the returned value.
    //  */
    // std::string workerInit();
    //
    // /* Called for each thread loop unless the thread is in 'paused' state.
    //  * Must return 'CONTINUE' to continue running, otherwise the thread loop
    //  * exits. If the result from worker cycle is 'ABORT' then the worker switches
    //  * into the 'error' state with a generic error message. It is recommended that
    //  * the subclass reports any problems via logging facilities. Returning the 'EXIT'
    //  * status is equivalent to calling 'stop()' method. This is just a way of
    //  * of stopping the worker by its own initiative.
    //  */
    // WorkerStatus workerCycle();

  private:
    void switchWorkerStateSync(WorkerState oldState, WorkerState newState,
                               WorkerState* finalState = nullptr) {
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
    void workerThread() {
        std::string error = static_cast<Impl*>(this)->workerInit();
        if (error.empty() && !mThreadName.empty()) {
            std::string compliantName(mThreadName.substr(0, 15));
            if (int errCode = pthread_setname_np(pthread_self(), compliantName.c_str());
                errCode != 0) {
                error.append("Failed to set thread name: ").append(strerror(errCode));
            }
        }
        if (error.empty() && mThreadPriority != ANDROID_PRIORITY_DEFAULT) {
            if (int result = setpriority(PRIO_PROCESS, 0, mThreadPriority); result != 0) {
                int errCode = errno;
                error.append("Failed to set thread priority: ").append(strerror(errCode));
            }
        }
        {
            std::lock_guard<std::mutex> lock(mWorkerLock);
            mWorkerState = error.empty() ? WorkerState::RUNNING : WorkerState::STOPPED;
            mError = error;
        }
        mWorkerCv.notify_one();
        if (!error.empty()) return;

        for (WorkerState state = WorkerState::RUNNING; state != WorkerState::STOPPED;) {
            bool needToNotify = false;
            if (WorkerStatus status = state != WorkerState::PAUSED
                                              ? static_cast<Impl*>(this)->workerCycle()
                                              : (sched_yield(), WorkerStatus::CONTINUE);
                status == WorkerStatus::CONTINUE) {
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
                if (status == WorkerStatus::ABORT) {
                    mError = "workerCycle aborted";
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

    std::string mThreadName;
    int mThreadPriority = ANDROID_PRIORITY_DEFAULT;
    std::thread mWorker;
    std::mutex mWorkerLock;
    std::condition_variable mWorkerCv;
    WorkerState mWorkerState GUARDED_BY(mWorkerLock) = WorkerState::STOPPED;
    std::string mError GUARDED_BY(mWorkerLock);
    // The atomic lock-free variable is used to prevent priority inversions
    // that can occur when a high priority worker tries to acquire the lock
    // which has been taken by a lower priority control thread which in its turn
    // got preempted. To prevent a PI under normal operating conditions, that is,
    // when there are no errors or state changes, the worker does not attempt
    // taking `mWorkerLock` unless `mWorkerStateChangeRequest` is set.
    // To make sure that updates to `mWorkerState` and `mWorkerStateChangeRequest`
    // are serialized, they are always made under a lock.
    static_assert(std::atomic<bool>::is_always_lock_free);
    std::atomic<bool> mWorkerStateChangeRequest GUARDED_BY(mWorkerLock) = false;
};

}  // namespace android::hardware::audio::common
