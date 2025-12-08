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

#define LOG_TAG "bluetooth_hal.wakelock"

#include "bluetooth_hal/util/power/wakelock.h"

#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>

#include "android-base/logging.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/power/power_interface.h"
#include "bluetooth_hal/util/power/wakelock_util.h"
#include "bluetooth_hal/util/power/wakelock_watchdog.h"
#include "bluetooth_hal/util/timer_manager.h"

namespace bluetooth_hal {
namespace util {
namespace power {

class WakelockImpl : public Wakelock {
 public:
  WakelockImpl() : wakelock_timeout_(kWakelockTimeMilliseconds) {};
  void Acquire(WakeSource source) override;
  void Release(WakeSource source) override;
  bool IsAcquired() override;
  bool IsWakeSourceAcquired(WakeSource source) override;
  void SetWakelockTimeout(const int timeout) override;

 private:
  void ReleaseWakelock();
  void AcquireWakelock();
  std::string ToString();

  bool wakelock_acquired_;
  std::recursive_mutex mutex_;
  std::unordered_set<WakeSource> acquired_sources_;
  Timer release_wakelock_timer_;

  // TODO: b/382605673 - Read it from the config manager.
  static constexpr int kWakelockTimeMilliseconds = 100;
  int wakelock_timeout_;
};

void WakelockImpl::Acquire(WakeSource source) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (acquired_sources_.count(source) > 0) {
    return;
  }
  WakelockWatchdog::GetWatchdog().Start(source);

  if (acquired_sources_.empty()) {
    if (release_wakelock_timer_.IsScheduled()) {
      // Stop the timer of releasing wakelock.
      release_wakelock_timer_.Cancel();
    }
    AcquireWakelock();
  }
  acquired_sources_.emplace(source);

  HAL_LOG(VERBOSE) << "Wakelock VOTE for: "
                   << WakelockUtil::WakeSourceToString(source)
                   << ", current wakelocks: " << ToString();
}

void WakelockImpl::Release(WakeSource source) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (acquired_sources_.erase(source) == 0) {
    return;
  }

  HAL_LOG(VERBOSE) << "Wakelock UNVOTE for: "
                   << WakelockUtil::WakeSourceToString(source)
                   << ", current wakelocks: " << ToString();

  if (acquired_sources_.empty()) {
    // The wakelock list is empty, schedule a timer to release the wakelock.
    release_wakelock_timer_.Schedule(
        std::bind_front(&WakelockImpl::ReleaseWakelock, this),
        std::chrono::milliseconds{wakelock_timeout_});
  }
  WakelockWatchdog::GetWatchdog().Stop(source);
}

bool WakelockImpl::IsAcquired() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  return !acquired_sources_.empty();
}

bool WakelockImpl::IsWakeSourceAcquired(WakeSource source) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  return (acquired_sources_.count(source) > 0);
}

void WakelockImpl::AcquireWakelock() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (!wakelock_acquired_) {
    HAL_LOG(DEBUG) << "Acuqire system wakelock";
    PowerInterface::GetInterface().AcquireWakelock();
    wakelock_acquired_ = true;
  }
}

void WakelockImpl::ReleaseWakelock() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (wakelock_acquired_) {
    HAL_LOG(DEBUG) << "Release system wakelock";
    PowerInterface::GetInterface().ReleaseWakelock();
    wakelock_acquired_ = false;
  }
}

void WakelockImpl::SetWakelockTimeout(const int timeout) {
  if (timeout == wakelock_timeout_) {
    return;
  }

  HAL_LOG(DEBUG) << "Wakelock timeout set to " << timeout;
  wakelock_timeout_ = timeout;
}

std::string WakelockImpl::ToString() {
  std::stringstream ss;
  ss << "[";
  bool first = true;
  for (auto source : acquired_sources_) {
    if (!first) {
      ss << ", ";
    }
    ss << WakelockUtil::WakeSourceToString(source);
    first = false;
  }
  ss << "]";
  return ss.str();
}

Wakelock& Wakelock::GetWakelock() {
  static WakelockImpl wakelock;
  return wakelock;
}

}  // namespace power
}  // namespace util
}  // namespace bluetooth_hal
