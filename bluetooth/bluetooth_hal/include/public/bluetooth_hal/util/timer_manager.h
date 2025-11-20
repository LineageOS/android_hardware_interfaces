/*
 * Copyright 2025 The Android Open Source Project
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
#include <functional>

namespace bluetooth_hal {
namespace util {

class Timer;

class TimerManager {
 public:
  virtual ~TimerManager() = default;

 private:
  friend class Timer;

  virtual bool Schedule(Timer* timer, const std::function<void()>& task,
                        std::chrono::milliseconds delay) = 0;

  virtual bool Cancel(Timer* timer) = 0;

  virtual bool IsScheduled(Timer* timer) = 0;

  static TimerManager& GetManager();
};

class Timer {
 public:
  ~Timer() { Cancel(); }

  /**
   * @brief Schedule a timer with a task and a delay. If there's already a task
   * scheduled on this timer, then the previous task will be canceled.
   *
   * @param task The function to be run in the future.
   * @param delay The delay in milliseconds, should be greater than 0ms.
   * @return If true, the timer is scheduled successfully. Otherwise, false.
   */
  bool Schedule(const std::function<void()>& task,
                std::chrono::milliseconds delay) {
    if (delay <= std::chrono::milliseconds(0)) {
      return false;
    }
    if (IsScheduled()) {
      Cancel();
    }
    return TimerManager::GetManager().Schedule(this, task, delay);
  }

  /**
   * @brief Cancel a timer. If there's no task scheduled on this timer, this is
   * a no-op.
   *
   * @return If true, the timer is canceled successfully. Otherwise, false.
   */
  bool Cancel() { return TimerManager::GetManager().Cancel(this); }

  /**
   * @brief Check if the task on this timer is scheduled or not.
   *
   * @return If true, it means the task has been scheduled, and will be fired.
   * If false, it means there's no task scheduled on this timer, or the task has
   * been fired already.
   */
  bool IsScheduled() { return TimerManager::GetManager().IsScheduled(this); }
};

}  // namespace util
}  // namespace bluetooth_hal
