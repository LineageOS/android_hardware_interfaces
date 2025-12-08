/*
 * Copyright 2024 The Android Open Source Project
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

#define LOG_TAG "bluetooth_hal.fd_watcher"

#include "bluetooth_hal/util/fd_watcher.h"

#include <fcntl.h>
#include <sched.h>
#include <sys/select.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>

#include "android-base/logging.h"
#include "android-base/unique_fd.h"

namespace bluetooth_hal {
namespace util {
namespace {

using ::android::base::Pipe;
using ::android::base::unique_fd;

constexpr int kBtRtPriority = 1;

}  // namespace

class FdWatcherImpl {
 public:
  FdWatcherImpl() = default;
  ~FdWatcherImpl();

  int StartWatching(int fd,
                    const std::function<void(int)>& on_read_fd_ready_callback);
  void StopWatching();

 private:
  int StartThreadIfNeeded();
  void StopThread();
  void NotifyThread() const;
  void ThreadRoutine();

  std::atomic_bool running_{false};
  std::thread thread_;

  // Mutex to protect shared data related to watched FDs.
  std::mutex watched_fds_mutex_;
  std::unordered_map<int, std::function<void(int)>> watched_fds_;

  // File descriptors for inter-thread communication using a pipe.
  unique_fd notification_read_fd_;
  unique_fd notification_write_fd_;
};

FdWatcherImpl::~FdWatcherImpl() { StopThread(); }

int FdWatcherImpl::StartWatching(
    int fd, const std::function<void(int)>& on_read_fd_ready_callback) {
  {
    std::scoped_lock lock(watched_fds_mutex_);
    watched_fds_.emplace(fd, on_read_fd_ready_callback);
  }

  return StartThreadIfNeeded();
}

void FdWatcherImpl::StopWatching() { StopThread(); }

int FdWatcherImpl::StartThreadIfNeeded() {
  if (running_.exchange(true)) {
    return 0;
  }

  unique_fd read_fd;
  unique_fd write_fd;
  if (!Pipe(&read_fd, &write_fd, O_NONBLOCK)) {
    running_ = false;
    return -1;
  }

  notification_read_fd_ = std::move(read_fd);
  notification_write_fd_ = std::move(write_fd);

  thread_ = std::thread([this]() { ThreadRoutine(); });
  if (!thread_.joinable()) {
    return -1;
  }

  return 0;
}

void FdWatcherImpl::StopThread() {
  if (!running_.exchange(false)) {
    return;
  }

  NotifyThread();

  // Wait for the thread to finish if not the current thread.
  if (thread_.joinable() && std::this_thread::get_id() != thread_.get_id()) {
    thread_.join();
  }

  {
    std::scoped_lock lock(watched_fds_mutex_);
    watched_fds_.clear();
  }
}

void FdWatcherImpl::NotifyThread() const {
  uint8_t stub_buffer = 0;
  const ssize_t ret = TEMP_FAILURE_RETRY(
      write(notification_write_fd_.get(), &stub_buffer, sizeof(stub_buffer)));
  if (ret < 0) {
    LOG(ERROR) << __func__
               << ": Failed to write to notification pipe: " << strerror(errno)
               << ".";
  }
}

void FdWatcherImpl::ThreadRoutine() {
  sched_param rt_params{.sched_priority = kBtRtPriority};
  if (sched_setscheduler(0, SCHED_FIFO, &rt_params) != 0) {
    LOG(WARNING) << __func__
                 << ": Failed to set SCHED_FIFO: " << strerror(errno) << ".";
  }

  while (running_) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(notification_read_fd_, &read_fds);

    int max_fd = notification_read_fd_.get();
    {
      std::scoped_lock lock(watched_fds_mutex_);
      for (const auto& [fd, _] : watched_fds_) {
        FD_SET(fd, &read_fds);
        max_fd = std::max(max_fd, fd);
      }
    }

    const int num_ready_fds =
        select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

    if (num_ready_fds < 0) {
      if (errno == EINTR) {
        continue;
      }

      LOG(ERROR) << __func__ << ": select() failed: " << strerror(errno) << ".";
      break;
    }

    if (FD_ISSET(notification_read_fd_.get(), &read_fds)) {
      uint8_t stub_buffer;
      TEMP_FAILURE_RETRY(
          read(notification_read_fd_.get(), &stub_buffer, sizeof(stub_buffer)));
      continue;
    }

    // Invoke the data ready callbacks if appropriate.
    {
      // Hold the mutex to make sure that the callbacks are still valid.
      std::scoped_lock lock(watched_fds_mutex_);
      for (const auto& [fd, ready_callback] : watched_fds_) {
        if (FD_ISSET(fd, &read_fds)) {
          ready_callback(fd);
        }
      }
    }
  }
}

FdWatcher::FdWatcher() : impl_(std::make_unique<FdWatcherImpl>()) {}

FdWatcher::~FdWatcher() = default;

int FdWatcher::StartWatching(
    int fd, const std::function<void(int)>& on_read_fd_ready_callback) {
  return impl_->StartWatching(fd, on_read_fd_ready_callback);
}

void FdWatcher::StopWatching() { impl_->StopWatching(); }

}  // namespace util
}  // namespace bluetooth_hal
