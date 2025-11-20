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

#include "bluetooth_hal/util/system_call_wrapper.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>

namespace bluetooth_hal {
namespace util {

class SystemCallWrapperImpl : public SystemCallWrapper {
 public:
  int Select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* errorfds,
             struct timeval* timeout) override {
    return select(nfds, readfds, writefds, errorfds, timeout);
  }

  ssize_t Send(int fd, const void* buffer, size_t length, int flags) override {
    return send(fd, buffer, length, flags);
  }

  ssize_t Recv(int fd, void* buffer, size_t length, int flags) override {
    return recv(fd, buffer, length, flags);
  }

  ssize_t Write(int fd, const void* buffer, size_t count) override {
    return write(fd, buffer, count);
  }

  ssize_t Writev(int fd, const struct iovec* iov, int iovcnt) override {
    return writev(fd, iov, iovcnt);
  }

  ssize_t Read(int fd, void* buffer, size_t count) override {
    return read(fd, buffer, count);
  }

  int Accept(int fd, struct sockaddr* address,
             socklen_t* address_len) override {
    return accept(fd, address, address_len);
  }

  int Open(const char* pathname, int flags) override {
    return open(pathname, flags);
  }

  void Close(int fd) override { close(fd); }

  void Unlink(const char* path) override { unlink(path); }

  int InotifyInit() override { return inotify_init(); }

  int InotifyAddWatch(int fd, const char* pathname, uint32_t mask) override {
    return inotify_add_watch(fd, pathname, mask);
  }

  int Socket(int domain, int type, int protocol) override {
    return socket(domain, type, protocol);
  }

  int Bind(int fd, const struct sockaddr* address,
           socklen_t address_len) override {
    return bind(fd, address, address_len);
  }

  int Listen(int fd, int backlog) override { return listen(fd, backlog); }

  int Stat(const char* path, struct stat* sb) override {
    return stat(path, sb);
  }

  bool IsSocketFile(int st_mode) override { return S_ISSOCK(st_mode); }

  int CreatePipe(int pipefd[2], int flags) override {
    return pipe2(pipefd, flags);
  }

  int FdIsSet(int fd, fd_set* set) override { return FD_ISSET(fd, set); }

  void FdSet(int fd, fd_set* set) override { FD_SET(fd, set); }

  void FdZero(fd_set* set) override { FD_ZERO(set); }

  int Kill(pid_t pid, int signal) override { return kill(pid, signal); }
};

SystemCallWrapper& SystemCallWrapper::GetWrapper() {
  static SystemCallWrapperImpl wrapper;
  return wrapper;
}

}  // namespace util
}  // namespace bluetooth_hal
