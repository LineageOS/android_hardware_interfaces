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

#define LOG_TAG "bthal.timer_manager"

#include "bluetooth_hal/util/timer_manager.h"

#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>

#include "android-base/logging.h"
#include "bluetooth_hal/util/worker.h"

constexpr clockid_t GetAlarmClock() {
#ifndef UNIT_TEST
  return CLOCK_BOOTTIME_ALARM;
#else
  return CLOCK_BOOTTIME;
#endif
}

constexpr long kMillisecondsPerSecond = 1000;
constexpr long kNanosecondsPerMillisecond = 1000000;
constexpr long kTearDownTimerInMillisecond = 10;
constexpr long kDisarmTimerInMillisecond = 0;

namespace bluetooth_hal {
namespace util {

class TimerManagerImpl : public TimerManager {
 public:
  TimerManagerImpl();
  ~TimerManagerImpl() override;
  bool Schedule(Timer* timer, const std::function<void()>& task,
                std::chrono::milliseconds delay) override;
  bool Cancel(Timer* timer) override;
  bool IsScheduled(Timer* timer) override;

 private:
  enum class TimerMessage : int {
    kWaitForExpiration = 1,
  };

  enum class TaskMessage : int {
    kOnTimerExpired = 1,
  };

  class TimerEvent {
   public:
    Timer* timer_;
    std::chrono::steady_clock::time_point expires_at_;
    std::function<void()> task_;

    bool operator<(const TimerEvent& other) const {
      if (expires_at_ == other.expires_at_) {
        return timer_ < other.timer_;
      }
      return expires_at_ < other.expires_at_;
    }
  };

  inline int RunSyscallUntilNoIntr(std::function<int()> fn);
  void OnTimerExpired();
  bool RescheduleTimer();
  bool WillBeTheFirstExpired(Timer* timer);
  void EpollWaitTimer();
  bool SetTimer(std::chrono::milliseconds delay);
  std::unique_ptr<Worker<TimerMessage>> timer_thread_;
  std::unique_ptr<Worker<TaskMessage>> task_thread_;
  int timer_fd_;
  int epoll_fd_;
  std::unordered_map<Timer*, TimerEvent> timer_events_;
  std::multiset<TimerEvent> ordered_timer_events_;
  std::mutex mutex_;
  std::atomic<bool> running_ = true;
};

TimerManagerImpl::TimerManagerImpl() {
  timer_fd_ = timerfd_create(GetAlarmClock(), 0);
  if (timer_fd_ < 0) {
    LOG(ERROR) << "Failed to create timerfd: " << strerror(errno);
    return;
  }

  epoll_fd_ = RunSyscallUntilNoIntr(std::bind(epoll_create1, 0));
  if (epoll_fd_ < 0) {
    LOG(ERROR) << "Failed to create epoll fd: " << strerror(errno);
    close(timer_fd_);
    return;
  }
  epoll_event event{.events = EPOLLIN, .data{.fd = timer_fd_}};
  int result = RunSyscallUntilNoIntr([this, &event]() -> int {
    return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &event);
  });
  if (result < 0) {
    LOG(ERROR) << "Failed to add timer fd to epoll: " << strerror(errno);
    close(timer_fd_);
    close(epoll_fd_);
    return;
  }

  timer_thread_ =
      std::make_unique<Worker<TimerMessage>>([this](TimerMessage message) {
        if (message == TimerMessage::kWaitForExpiration) {
          EpollWaitTimer();
        } else {
          LOG(ERROR) << "Unknown message: " << static_cast<int>(message);
        }
      });
  task_thread_ =
      std::make_unique<Worker<TaskMessage>>([this](TaskMessage message) {
        if (message == TaskMessage::kOnTimerExpired) {
          OnTimerExpired();
        } else {
          LOG(ERROR) << "Unknown message: " << static_cast<int>(message);
        }
      });
  timer_thread_->Post(TimerMessage::kWaitForExpiration);
}

TimerManagerImpl::~TimerManagerImpl() {
  std::lock_guard<std::mutex> lock(mutex_);
  running_.store(false);
  timer_events_.clear();
  ordered_timer_events_.clear();
  // TODO: b/419117083 - Fix this tricky SetTimer.
  // Manually set a timer here to unblock the epoll_wait, so that EpollWaitTimer
  // can proceed and exit.
  SetTimer(std::chrono::milliseconds(kTearDownTimerInMillisecond));
  timer_thread_.reset();
  task_thread_.reset();
  close(timer_fd_);
  close(epoll_fd_);
}

bool TimerManagerImpl::Schedule(Timer* timer, const std::function<void()>& task,
                                std::chrono::milliseconds delay) {
  std::lock_guard<std::mutex> lock(mutex_);
  TimerEvent timer_event{timer, std::chrono::steady_clock::now() + delay,
                         std::move(task)};
  timer_events_[timer] = timer_event;
  ordered_timer_events_.insert(timer_event);
  if (WillBeTheFirstExpired(timer)) {
    return RescheduleTimer();
  }
  return true;
}

bool TimerManagerImpl::Cancel(Timer* timer) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = timer_events_.find(timer);
  if (it == timer_events_.end()) {
    LOG(ERROR) << "Timer not found: " << timer;
    return false;
  }
  bool need_to_reschedule = WillBeTheFirstExpired(it->second.timer_);
  ordered_timer_events_.erase(it->second);
  timer_events_.erase(it);
  if (need_to_reschedule) {
    return RescheduleTimer();
  }
  return true;
}

bool TimerManagerImpl::IsScheduled(Timer* timer) {
  std::lock_guard<std::mutex> lock(mutex_);
  return timer_events_.find(timer) != timer_events_.end();
}

inline int TimerManagerImpl::RunSyscallUntilNoIntr(std::function<int()> fn) {
  int result = fn();
  while (result == -1 && errno == EINTR) {
    result = fn();
  }
  return result;
}

bool TimerManagerImpl::RescheduleTimer() {
  if (ordered_timer_events_.empty()) {
    SetTimer(std::chrono::milliseconds(kDisarmTimerInMillisecond));
    return true;
  }
  auto next_timer_event = ordered_timer_events_.begin();
  std::chrono::milliseconds delay =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          next_timer_event->expires_at_ - std::chrono::steady_clock::now());
  if (delay.count() <= 0) {
    task_thread_->Post(TaskMessage::kOnTimerExpired);
    return true;
  }
  return SetTimer(delay);
}

bool TimerManagerImpl::SetTimer(std::chrono::milliseconds delay) {
  itimerspec timer_spec{
      .it_value = {
          .tv_sec = static_cast<time_t>(delay.count() / kMillisecondsPerSecond),
          .tv_nsec =
              static_cast<long>((delay.count() % kMillisecondsPerSecond) *
                                kNanosecondsPerMillisecond)}};
  if (timerfd_settime(timer_fd_, 0, &timer_spec, nullptr) < 0) {
    LOG(ERROR) << "Failed to set timerfd: " << strerror(errno);
    return false;
  }
  return true;
}

bool TimerManagerImpl::WillBeTheFirstExpired(Timer* timer) {
  if (ordered_timer_events_.empty()) {
    return false;
  }
  return ordered_timer_events_.begin()->timer_ == timer;
}

void TimerManagerImpl::EpollWaitTimer() {
  if (!running_.load()) {
    return;
  }
  epoll_event event[1];
  int event_count = RunSyscallUntilNoIntr(
      [this, &event]() -> int { return epoll_wait(epoll_fd_, event, 1, -1); });
  if (event_count > 0) {
    uint64_t exp;
    ssize_t size = read(event[0].data.fd, &exp, sizeof(exp));
    if (size == sizeof(exp)) {
      task_thread_->Post(TaskMessage::kOnTimerExpired);
    }
  } else if (event_count < 0) {
    LOG(ERROR) << "epoll_wait error: " << strerror(errno);
  }
  timer_thread_->Post(TimerMessage::kWaitForExpiration);
}

void TimerManagerImpl::OnTimerExpired() {
  std::function<void()> expired_task;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ordered_timer_events_.empty()) {
      return;
    }
    auto it = ordered_timer_events_.begin();
    // Ensure the timer has actually expired.
    if (it->expires_at_ > std::chrono::steady_clock::now()) {
      RescheduleTimer();
      return;
    }
    // Take the expired task out and reschedule before executing them, and
    // then unlock to allow the task to call `Schedule()`.
    expired_task = std::move(it->task_);
    timer_events_.erase(it->timer_);
    ordered_timer_events_.erase(it);
    RescheduleTimer();
  }
  expired_task();
}

TimerManager& TimerManager::GetManager() {
  static TimerManagerImpl manager;
  return manager;
}

}  // namespace util
}  // namespace bluetooth_hal
