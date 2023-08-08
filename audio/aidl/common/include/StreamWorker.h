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

#include <sys/types.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <android-base/thread_annotations.h>
#include <system/thread_defs.h>

namespace android::hardware::audio::common {

class StreamLogic;

namespace internal {

class ThreadController {
    enum class WorkerState { INITIAL, STOPPED, RUNNING, PAUSE_REQUESTED, PAUSED, RESUME_REQUESTED };

  public:
    explicit ThreadController(StreamLogic* logic) : mLogic(logic) {}
    ~ThreadController() { stop(); }

    bool start(const std::string& name, int priority);
    // Note: 'pause' and 'resume' methods should only be used on the "driving" side.
    // In the case of audio HAL I/O, the driving side is the client, because the HAL
    // implementation always blocks on getting a command.
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
    pid_t getTid() {
        std::lock_guard<std::mutex> lock(mWorkerLock);
        return mTid;
    }
    void stop();
    // Direct use of 'join' assumes that the StreamLogic is not intended
    // to run forever, and is guaranteed to exit by itself. This normally
    // only happen in tests.
    void join();
    bool waitForAtLeastOneCycle();

    // Only used by unit tests.
    void lockUnlockMutex(bool lock) NO_THREAD_SAFETY_ANALYSIS {
        lock ? mWorkerLock.lock() : mWorkerLock.unlock();
    }
    std::thread::native_handle_type getThreadNativeHandle() { return mWorker.native_handle(); }

  private:
    void switchWorkerStateSync(WorkerState oldState, WorkerState newState,
                               WorkerState* finalState = nullptr);
    void workerThread();

    StreamLogic* const mLogic;
    std::string mThreadName;
    int mThreadPriority = ANDROID_PRIORITY_DEFAULT;
    std::thread mWorker;
    std::mutex mWorkerLock;
    std::condition_variable mWorkerCv;
    WorkerState mWorkerState GUARDED_BY(mWorkerLock) = WorkerState::INITIAL;
    std::string mError GUARDED_BY(mWorkerLock);
    pid_t mTid GUARDED_BY(mWorkerLock) = -1;
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

// A special thread name used in tests only.
static const std::string kTestSingleThread = "__testST__";

}  // namespace internal

class StreamLogic {
  public:
    friend class internal::ThreadController;

    virtual ~StreamLogic() = default;

  protected:
    enum class Status { ABORT, CONTINUE, EXIT };

    /* Called once at the beginning of the thread loop. Must return
     * an empty string to enter the thread loop, otherwise the thread loop
     * exits and the worker switches into the 'error' state, setting
     * the error to the returned value.
     */
    virtual std::string init() = 0;

    /* Called for each thread loop unless the thread is in 'paused' state.
     * Must return 'CONTINUE' to continue running, otherwise the thread loop
     * exits. If the result from worker cycle is 'ABORT' then the worker switches
     * into the 'error' state with a generic error message. It is recommended that
     * the subclass reports any problems via logging facilities. Returning the 'EXIT'
     * status is equivalent to calling 'stop()' method. This is just a way of
     * of stopping the worker by its own initiative.
     */
    virtual Status cycle() = 0;
};

template <class LogicImpl>
class StreamWorker : public LogicImpl {
  public:
    template <class... Args>
    explicit StreamWorker(Args&&... args) : LogicImpl(std::forward<Args>(args)...), mThread(this) {}

    // Methods of LogicImpl are available via inheritance.
    // Forwarded methods of ThreadController follow.

    // Note that 'priority' here is what is known as the 'nice number' in *nix systems.
    // The nice number is used with the default scheduler. For threads that
    // need to use a specialized scheduler (e.g. SCHED_FIFO) and set the priority within it,
    // it is recommended to implement an appropriate configuration sequence within
    // 'LogicImpl' or 'StreamLogic::init'.
    bool start(const std::string& name = "", int priority = ANDROID_PRIORITY_DEFAULT) {
        return mThread.start(name, priority);
    }
    void pause() { mThread.pause(); }
    void resume() { mThread.resume(); }
    bool hasError() { return mThread.hasError(); }
    std::string getError() { return mThread.getError(); }
    pid_t getTid() { return mThread.getTid(); }
    void stop() { mThread.stop(); }
    void join() { mThread.join(); }
    bool waitForAtLeastOneCycle() { return mThread.waitForAtLeastOneCycle(); }

    // Only used by unit tests.
    void testLockUnlockMutex(bool lock) { mThread.lockUnlockMutex(lock); }
    std::thread::native_handle_type testGetThreadNativeHandle() {
        return mThread.getThreadNativeHandle();
    }

  private:
    // The ThreadController gets destroyed before LogicImpl.
    // After the controller has been destroyed, it is guaranteed that
    // the thread was joined, thus the 'cycle' method of LogicImpl
    // will not be called anymore, and it is safe to destroy LogicImpl.
    internal::ThreadController mThread;
};

}  // namespace android::hardware::audio::common
