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

#define LOG_TAG "bluetooth_hal.wakelock_watchdog"

#include "bluetooth_hal/util/power/wakelock_watchdog.h"

#include <chrono>
#include <functional>
#include <mutex>
#include <unordered_map>

#include "android-base/logging.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/power/wakelock_util.h"
#include "bluetooth_hal/util/timer_manager.h"

namespace bluetooth_hal {
namespace util {
namespace power {

class WatchdogBiteHandler {
 public:
  // Intentionally crash in different methods to create crash reason
  // individually.
  void TxTimeout() {
    LOG(FATAL) << __func__ << ": wakelock watchdog BITE due to TX timeout!";
  }

  void RxTimeout() {
    LOG(FATAL) << __func__
               << ": wakelock watchdog BITE due to unable to complete RX!";
  }

  void HciTimeout() {
    LOG(FATAL) << __func__ << ": wakelock watchdog BITE due to HCI timeout!";
  }

  void RouterTaskTimeout() {
    LOG(FATAL) << __func__
               << ": wakelock watchdog BITE due to Router Task timeout!";
  }

  void InitializeTimeout() {
    LOG(FATAL) << __func__
               << ": wakelock watchdog BITE due to initialize timeout!";
  }

  void CloseTimeout() {
    LOG(FATAL) << __func__ << ": wakelock watchdog BITE due to close timeout!";
  }
};

class WakelockWatchdogImpl : public WakelockWatchdog {
 public:
  void Start(WakeSource source) override;
  void Stop(WakeSource source) override;
  void Pause() override;
  void Resume() override;

 private:
  void WatchdogTimerExpired();
  void Bark(WakeSource source, int remain_time);
  void Bite(WakeSource source);

  std::recursive_mutex mutex_;
  Timer watchdog_timer_;
  std::unordered_map<WakeSource, int> watchdog_map_;
  static constexpr int kWatchdogBarkMs = 1000;
  static const std::unordered_map<WakeSource, int> kWatchdogMs;
  bool watchdog_paused_;
};

const std::unordered_map<WakeSource, int> WakelockWatchdogImpl::kWatchdogMs = {
    {WakeSource::kTx, 5000},           //  5 seconds for TX timeout.
    {WakeSource::kRx, 5000},           //  5 seconds for RX timeout.
    {WakeSource::kHciBusy, 10000},     // 10 seconds for HCI timeout.
    {WakeSource::kRouterTask, 5000},   // 5 seconds for Router Task timeout.
    {WakeSource::kTransport, 20000},   // 20 seconds for Transport timeout.
    {WakeSource::kInitialize, 20000},  // 20 seconds for HAL Initialization.
    {WakeSource::kClose, 20000},       // 20 seconds for HAL Closing.
};

void WakelockWatchdogImpl::Start(WakeSource source) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (!watchdog_timer_.IsScheduled()) {
    watchdog_timer_.Schedule(
        std::bind_front(&WakelockWatchdogImpl::WatchdogTimerExpired, this),
        std::chrono::milliseconds{kWatchdogBarkMs});
  }
  watchdog_map_[source] = kWatchdogMs.at(source);
}

void WakelockWatchdogImpl::Stop(WakeSource source) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  watchdog_map_.erase(source);

  if (watchdog_map_.empty() && watchdog_timer_.IsScheduled()) {
    watchdog_timer_.Cancel();
  }
}

void WakelockWatchdogImpl::Pause() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  HAL_LOG(WARNING) << __func__ << ": Watchdog is paused!";
  watchdog_paused_ = true;
}

void WakelockWatchdogImpl::Resume() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  HAL_LOG(INFO) << __func__ << ": Watchdog is resumed";
  watchdog_paused_ = false;
}

void WakelockWatchdogImpl::WatchdogTimerExpired() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (watchdog_map_.empty()) {
    return;
  }

  for (auto& it : watchdog_map_) {
    if (watchdog_paused_) break;
    WakeSource source = it.first;
    it.second -= kWatchdogBarkMs;
    int remain_time = it.second;

    if (remain_time <= 0) {
      Bite(source);
      continue;
    }
    if (source != WakeSource::kTransport &&
        remain_time <= (kWatchdogMs.at(source)) / 2) {
      // Watchdog bark and print warning log when it is close to bite.
      Bark(source, remain_time);
    }
  }

  watchdog_timer_.Schedule(
      std::bind_front(&WakelockWatchdogImpl::WatchdogTimerExpired, this),
      std::chrono::milliseconds{kWatchdogBarkMs});
}

void WakelockWatchdogImpl::Bark(WakeSource source, int remain_time) {
  ANCHOR_LOG_WARNING(AnchorType::kWatchdog)
      << ": Watchdog BARK! WakeSource = "
      << WakelockUtil::WakeSourceToString(source)
      << ", remain time = " << remain_time << "ms.";
}

void WakelockWatchdogImpl::Bite(WakeSource source) {
  WatchdogBiteHandler watchdog_bite_handler;
  switch (source) {
    case WakeSource::kTx:
      watchdog_bite_handler.TxTimeout();
      break;
    case WakeSource::kRx:
      watchdog_bite_handler.RxTimeout();
      break;
    case WakeSource::kHciBusy:
      watchdog_bite_handler.HciTimeout();
      break;
    case WakeSource::kRouterTask:
      watchdog_bite_handler.RouterTaskTimeout();
      break;
    case WakeSource::kTransport:
      // Long Transport wakelock can happen in heavy BT traffic, print log here
      // as a nice-to-have battery information instead of crash.
      ANCHOR_LOG(AnchorType::kWatchdog) << "Long transport wakelock detected.";
      Start(source);
      break;
    case WakeSource::kInitialize:
      watchdog_bite_handler.InitializeTimeout();
      break;
    case WakeSource::kClose:
      watchdog_bite_handler.CloseTimeout();
      break;
  }
}

WakelockWatchdog& WakelockWatchdog::GetWatchdog() {
  static WakelockWatchdogImpl watchdog;
  return watchdog;
}

}  // namespace power
}  // namespace util
}  // namespace bluetooth_hal
