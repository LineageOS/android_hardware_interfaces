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

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "bluetooth_hal/util/system_call_wrapper.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace util {

class MockSystemCallWrapper : public SystemCallWrapper {
 public:
  MOCK_METHOD(int, Select,
              (int nfds, fd_set* readfds, fd_set* writefds, fd_set* errorfds,
               struct timeval* timeout),
              (override));

  MOCK_METHOD(ssize_t, Send,
              (int fd, const void* buffer, size_t length, int flags),
              (override));

  MOCK_METHOD(ssize_t, Recv, (int fd, void* buffer, size_t length, int flags),
              (override));

  MOCK_METHOD(ssize_t, Write, (int fd, const void* buffer, size_t count),
              (override));

  MOCK_METHOD(ssize_t, Writev, (int fd, const struct iovec* iov, int iovcnt),
              (override));

  MOCK_METHOD(ssize_t, Read, (int fd, void* buffer, size_t count), (override));

  MOCK_METHOD(int, Accept,
              (int fd, struct sockaddr* address, socklen_t* address_len),
              (override));

  MOCK_METHOD(int, Open, (const char* pathname, int flags), (override));

  MOCK_METHOD(void, Close, (int fd), (override));

  MOCK_METHOD(void, Unlink, (const char* path), (override));

  MOCK_METHOD(int, InotifyInit, (), (override));

  MOCK_METHOD(int, InotifyAddWatch,
              (int fd, const char* pathname, uint32_t mask), (override));

  MOCK_METHOD(int, Socket, (int domain, int type, int protocol), (override));

  MOCK_METHOD(int, Bind,
              (int fd, const struct sockaddr* address, socklen_t address_len),
              (override));

  MOCK_METHOD(int, Listen, (int fd, int backlog), (override));

  MOCK_METHOD(int, Stat, (const char* path, struct stat* sb), (override));

  MOCK_METHOD(bool, IsSocketFile, (int st_mode), (override));

  MOCK_METHOD(int, CreatePipe, (int pipefd[2], int flags), (override));

  MOCK_METHOD(int, FdIsSet, (int fd, fd_set* set), (override));

  MOCK_METHOD(void, FdSet, (int fd, fd_set* set), (override));

  MOCK_METHOD(void, FdZero, (fd_set * set), (override));

  MOCK_METHOD(int, Kill, (pid_t pid, int signal), (override));

  static void SetMockWrapper(MockSystemCallWrapper* wrapper);

  static inline MockSystemCallWrapper* mock_system_call_wrapper_{nullptr};
};

}  // namespace util
}  // namespace bluetooth_hal
