//
// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "async_fd_watcher.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include "fcntl.h"
#include "sys/select.h"
#include "unistd.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

int AsyncFdWatcher::WatchFdForNonBlockingReads(
    int file_descriptor, const ReadCallback& on_read_fd_ready_callback) {
  // Add file descriptor and callback
  {
    std::unique_lock<std::mutex> guard(internal_mutex_);
    read_fd_ = file_descriptor;
    cb_ = on_read_fd_ready_callback;
  }

  // Start the thread if not started yet
  int started = tryStartThread();
  if (started != 0) {
    return started;
  }

  return 0;
}

int AsyncFdWatcher::ConfigureTimeout(
    const std::chrono::milliseconds timeout,
    const TimeoutCallback& on_timeout_callback) {
  // Add timeout and callback
  {
    std::unique_lock<std::mutex> guard(timeout_mutex_);
    timeout_cb_ = on_timeout_callback;
    timeout_ms_ = timeout;
  }

  notifyThread();
  return 0;
}

void AsyncFdWatcher::StopWatchingFileDescriptor() { stopThread(); }

AsyncFdWatcher::~AsyncFdWatcher() {}

// Make sure to call this with at least one file descriptor ready to be
// watched upon or the thread routine will return immediately
int AsyncFdWatcher::tryStartThread() {
  if (std::atomic_exchange(&running_, true)) return 0;

  // Set up the communication channel
  int pipe_fds[2];
  if (pipe2(pipe_fds, O_NONBLOCK)) return -1;

  notification_listen_fd_ = pipe_fds[0];
  notification_write_fd_ = pipe_fds[1];

  thread_ = std::thread([this]() { ThreadRoutine(); });
  if (!thread_.joinable()) return -1;

  return 0;
}

int AsyncFdWatcher::stopThread() {
  if (!std::atomic_exchange(&running_, false)) return 0;

  notifyThread();
  if (std::this_thread::get_id() != thread_.get_id()) {
    thread_.join();
  }

  {
    std::unique_lock<std::mutex> guard(internal_mutex_);
    cb_ = nullptr;
    read_fd_ = -1;
  }

  {
    std::unique_lock<std::mutex> guard(timeout_mutex_);
    timeout_cb_ = nullptr;
  }

  return 0;
}

int AsyncFdWatcher::notifyThread() {
  uint8_t buffer[] = {0};
  if (TEMP_FAILURE_RETRY(write(notification_write_fd_, &buffer, 1)) < 0) {
    return -1;
  }
  return 0;
}

void AsyncFdWatcher::ThreadRoutine() {
  while (running_) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(notification_listen_fd_, &read_fds);
    FD_SET(read_fd_, &read_fds);

    struct timeval timeout;
    struct timeval* timeout_ptr = NULL;
    if (timeout_ms_ > std::chrono::milliseconds(0)) {
      timeout.tv_sec = timeout_ms_.count() / 1000;
      timeout.tv_usec = (timeout_ms_.count() % 1000) * 1000;
      timeout_ptr = &timeout;
    }

    // Wait until there is data available to read on some FD.
    int nfds = std::max(notification_listen_fd_, read_fd_);
    int retval = select(nfds + 1, &read_fds, NULL, NULL, timeout_ptr);

    // There was some error.
    if (retval < 0) continue;

    // Timeout.
    if (retval == 0) {
      std::unique_lock<std::mutex> guard(timeout_mutex_);
      if (timeout_ms_ > std::chrono::milliseconds(0) && timeout_cb_)
        timeout_cb_();
      continue;
    }

    // Read data from the notification FD.
    if (FD_ISSET(notification_listen_fd_, &read_fds)) {
      char buffer[] = {0};
      TEMP_FAILURE_RETRY(read(notification_listen_fd_, buffer, 1));
      continue;
    }

    // Invoke the data ready callback if appropriate.
    if (FD_ISSET(read_fd_, &read_fds)) {
      std::unique_lock<std::mutex> guard(internal_mutex_);
      if (cb_) cb_(read_fd_);
    }
  }
}

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
