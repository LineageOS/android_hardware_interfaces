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

#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace util {
namespace power {

class Wakelock {
 public:
  /**
   * @brief Vote for acquiring wakelock from the system.
   *
   * @param source The source of the requester.
   *
   */
  virtual void Acquire(WakeSource source) = 0;

  /**
   * @brief Un-vote for wakelock from the system.
   *
   * @param source The source of the requester.
   *
   */
  virtual void Release(WakeSource source) = 0;

  /**
   * @brief Check if the wakelock is acquired.
   *
   * @return true if the wakelock is acquired, otherwise false.
   *
   */
  virtual bool IsAcquired() = 0;

  /**
   * @brief Check if the wakelock is voted by a certain requester.
   *
   * @param source The source of the requester.
   * @return true if the wakelock is acquired by the requester, otherwise
   * false.
   *
   */
  virtual bool IsWakeSourceAcquired(WakeSource source) = 0;

  static Wakelock& GetWakelock();

 protected:
  virtual ~Wakelock() = default;
};

class ScopedWakelock {
 public:
  ScopedWakelock(WakeSource source) : source_(source) {
    Wakelock::GetWakelock().Acquire(source_);
  }

  ~ScopedWakelock() { Wakelock::GetWakelock().Release(source_); }

 private:
  WakeSource source_;
};

}  // namespace power
}  // namespace util
}  // namespace bluetooth_hal
