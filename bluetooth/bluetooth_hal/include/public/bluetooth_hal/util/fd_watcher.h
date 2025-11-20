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

#pragma once

#include <functional>

namespace bluetooth_hal {
namespace util {

class FdWatcherImpl;

class FdWatcher {
 public:
  FdWatcher();

  FdWatcher(const FdWatcher&) = delete;
  FdWatcher& operator=(const FdWatcher&) = delete;

  ~FdWatcher();

  /**
   * @brief Starts watching a file descriptor.
   *
   * This function adds the specified file descriptor to the list of monitored
   * file descriptors. When the file descriptor becomes ready for reading, the
   * provided callback function is invoked.
   *
   * @note This is not a blocking call. The callback will be invoked
   * asynchronously.
   *
   * @param fd The file descriptor to watch.
   * @param on_read_fd_ready_callback The callback function to be invoked when
   * the file descriptor is ready for reading.
   *
   * @return 0 on success, -1 on error.
   *
   */
  int StartWatching(int fd,
                    const std::function<void(int)>& on_read_fd_ready_callback);

  /**
   * @brief Stops watching all file descriptors and terminates the internal
   * thread.
   *
   * This function should be called when the FdWatcher object is no longer
   * needed.
   *
   */
  void StopWatching();

 private:
  std::unique_ptr<FdWatcherImpl> impl_;
};

}  // namespace util
}  // namespace bluetooth_hal
