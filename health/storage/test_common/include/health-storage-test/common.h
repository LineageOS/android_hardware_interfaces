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

#include <chrono>
#include <string>

namespace android::hardware::health::storage::test {

// Dev GC timeout. This is the timeout used by vold.
const uint64_t kDevGcTimeoutSec = 120;
const std::chrono::seconds kDevGcTimeout{kDevGcTimeoutSec};
// Dev GC timeout tolerance. The HAL may not immediately return after the
// timeout, so include an acceptable tolerance.
const std::chrono::seconds kDevGcTolerance{3};
// Time accounted for RPC calls.
const std::chrono::milliseconds kRpcTime{1000};

template <typename R>
std::string to_string(std::chrono::duration<R, std::milli> time) {
    return std::to_string(time.count()) + "ms";
}

/** An atomic boolean flag that indicates whether a task has finished. */
class Flag {
  public:
    void OnFinish() {
        std::unique_lock<std::mutex> lock(mutex_);
        OnFinishLocked(&lock);
    }
    template <typename R, typename P>
    bool Wait(std::chrono::duration<R, P> duration) {
        std::unique_lock<std::mutex> lock(mutex_);
        return WaitLocked(&lock, duration);
    }

  protected:
    /** Will unlock. */
    void OnFinishLocked(std::unique_lock<std::mutex>* lock) {
        finished_ = true;
        lock->unlock();
        cv_.notify_all();
    }
    template <typename R, typename P>
    bool WaitLocked(std::unique_lock<std::mutex>* lock, std::chrono::duration<R, P> duration) {
        cv_.wait_for(*lock, duration, [this] { return finished_; });
        return finished_;
    }

    bool finished_{false};
    std::mutex mutex_;
    std::condition_variable cv_;
};

}  // namespace android::hardware::health::storage::test
