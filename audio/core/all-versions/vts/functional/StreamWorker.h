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

#include <sched.h>

#include <condition_variable>
#include <mutex>
#include <thread>

template <typename Impl>
class StreamWorker {
    enum class WorkerState { STOPPED, RUNNING, PAUSE_REQUESTED, PAUSED, RESUME_REQUESTED, ERROR };

  public:
    StreamWorker() = default;
    ~StreamWorker() { stop(); }
    bool start() {
        mWorker = std::thread(&StreamWorker::workerThread, this);
        std::unique_lock<std::mutex> lock(mWorkerLock);
        mWorkerCv.wait(lock, [&] { return mWorkerState != WorkerState::STOPPED; });
        return mWorkerState == WorkerState::RUNNING;
    }
    void pause() { switchWorkerStateSync(WorkerState::RUNNING, WorkerState::PAUSE_REQUESTED); }
    void resume() { switchWorkerStateSync(WorkerState::PAUSED, WorkerState::RESUME_REQUESTED); }
    bool hasError() {
        std::lock_guard<std::mutex> lock(mWorkerLock);
        return mWorkerState == WorkerState::ERROR;
    }
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mWorkerLock);
            if (mWorkerState == WorkerState::STOPPED) return;
            mWorkerState = WorkerState::STOPPED;
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

    // Methods that need to be provided by subclasses:
    //
    // Called once at the beginning of the thread loop. Must return
    // 'true' to enter the thread loop, otherwise the thread loop
    // exits and the worker switches into the 'error' state.
    // bool workerInit();
    //
    // Called for each thread loop unless the thread is in 'paused' state.
    // Must return 'true' to continue running, otherwise the thread loop
    // exits and the worker switches into the 'error' state.
    // bool workerCycle();

  private:
    void switchWorkerStateSync(WorkerState oldState, WorkerState newState,
                               WorkerState* finalState = nullptr) {
        std::unique_lock<std::mutex> lock(mWorkerLock);
        if (mWorkerState != oldState) {
            if (finalState) *finalState = mWorkerState;
            return;
        }
        mWorkerState = newState;
        mWorkerCv.wait(lock, [&] { return mWorkerState != newState; });
        if (finalState) *finalState = mWorkerState;
    }
    void workerThread() {
        bool success = static_cast<Impl*>(this)->workerInit();
        {
            std::lock_guard<std::mutex> lock(mWorkerLock);
            mWorkerState = success ? WorkerState::RUNNING : WorkerState::ERROR;
        }
        mWorkerCv.notify_one();
        if (!success) return;

        for (WorkerState state = WorkerState::RUNNING; state != WorkerState::STOPPED;) {
            bool needToNotify = false;
            if (state != WorkerState::PAUSED ? static_cast<Impl*>(this)->workerCycle()
                                             : (sched_yield(), true)) {
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
                if (state == WorkerState::RESUME_REQUESTED) {
                    needToNotify = true;
                }
                std::lock_guard<std::mutex> lock(mWorkerLock);
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
                mWorkerState = WorkerState::ERROR;
                state = WorkerState::STOPPED;
            }
            if (needToNotify) {
                mWorkerCv.notify_one();
            }
        }
    }

    std::thread mWorker;
    std::mutex mWorkerLock;
    std::condition_variable mWorkerCv;
    WorkerState mWorkerState = WorkerState::STOPPED;  // GUARDED_BY(mWorkerLock);
};
