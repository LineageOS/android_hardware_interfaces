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

#include <cstddef>
#include <cstdint>

namespace bluetooth_hal {
namespace util {

/**
 * @class SystemCallWrapper
 * @brief A wrapper class providing an interface to system calls.
 *
 * This class abstracts the underlying system calls, allowing for
 * potential mocking or customization in testing or different environments.
 */
class SystemCallWrapper {
 public:
  /**
   * @brief Virtual destructor.
   *
   * Allows derived classes to clean up resources properly.
   *
   */
  virtual ~SystemCallWrapper() = default;

  /**
   * @brief Gets a reference to the singleton instance of this class.
   *
   * @return A reference to the singleton SystemCallWrapper instance.
   *
   */
  static SystemCallWrapper& GetWrapper();

  /**
   * @brief Monitors multiple file descriptors for readiness.
   *
   * @param nfds The highest-numbered file descriptor in any of the three sets,
   * plus 1.
   * @param readfds A pointer to an fd_set structure, or NULL. On return,
   * readfds contains the file descriptors that are ready for reading.
   * @param writefds A pointer to an fd_set structure, or NULL. On return,
   * writefds contains the file descriptors that are ready for writing.
   * @param errorfds A pointer to an fd_set structure, or NULL. On return,
   * errorfds contains the file descriptors that have an error condition.
   * @param timeout A pointer to a timeval structure, or NULL. If timeout is
   * NULL, select() blocks indefinitely.
   *
   * @return On success, select() returns the number of file descriptors
   * contained in the three returned descriptor sets. On error, -1 is returned,
   * and errno is set to indicate the error.
   *
   */
  virtual int Select(int nfds, fd_set* readfds, fd_set* writefds,
                     fd_set* errorfds, struct timeval* timeout) = 0;

  /**
   * @brief Sends data over a connected socket.
   *
   * @param fd The socket file descriptor.
   * @param buffer A pointer to the buffer containing the data to send.
   * @param length The number of bytes to send.
   * @param flags Specifies the type of message transmission.
   *
   * @return On success, the number of bytes sent is returned. On error, -1 is
   * returned, and errno is set appropriately.
   *
   */
  virtual ssize_t Send(int fd, const void* buffer, size_t length,
                       int flags) = 0;

  /**
   * @brief Receives data from a connected socket.
   *
   * @param fd The socket file descriptor.
   * @param buffer A pointer to the buffer to store the received data.
   * @param length The maximum number of bytes to receive.
   * @param flags Specifies the type of message reception.
   *
   * @return On success, the number of bytes received is returned. On error, -1
   * is returned, and errno is set appropriately.
   *
   */
  virtual ssize_t Recv(int fd, void* buffer, size_t length, int flags) = 0;

  /**
   * @brief Writes data to a file descriptor.
   *
   * @param fd The file descriptor to write to.
   * @param buffer A pointer to the buffer containing the data to write.
   * @param count The number of bytes to write.
   *
   * @return On success, the number of bytes written is returned. On error, -1
   * is returned, and errno is set appropriately.
   *
   */
  virtual ssize_t Write(int fd, const void* buffer, size_t count) = 0;

  /**
   * @brief Writes data to a file descriptor with multiple buffers.
   *
   * The `writev` system call writes data from multiple buffers to a file
   * descriptor. The buffers are specified by an array of `iovec` structures.
   *
   * @param fd The file descriptor to write to.
   * @param iov A pointer to an array of `iovec` structures. Each `iovec`
   *            structure specifies a buffer to write data from.
   * @param iovcnt The number of elements in the `iov` array.
   *
   * @return On success, returns the total number of bytes written. This should
   * be the sum of the `iov_len` fields of all the `iovec` structures in the
   * `iov` array, unless an error occurs. On error, returns -1 and sets `errno`
   * appropriately.
   *
   */
  virtual ssize_t Writev(int fd, const struct iovec* iov, int iovcnt) = 0;

  /**
   * @brief Reads data from a file descriptor.
   *
   * @param fd The file descriptor to read from.
   * @param buffer A pointer to the buffer to store the read data.
   * @param count The maximum number of bytes to read.
   *
   * @return On success, the number of bytes read is returned. On error, -1 is
   * returned, and errno is set appropriately.
   *
   */
  virtual ssize_t Read(int fd, void* buffer, size_t count) = 0;

  /**
   * @brief Accepts a new connection on a listening socket.
   *
   * @param fd The listening socket file descriptor.
   * @param address A pointer to a sockaddr structure to store the address of
   * the connecting client.
   * @param address_len A pointer to a socklen_t variable that initially
   * contains the size of the structure pointed to by address. On return, it
   * contains the actual size of the client address.
   *
   * @return On success, a new file descriptor for the accepted connection is
   * returned. On error, -1 is returned, and errno is set appropriately.
   *
   */
  virtual int Accept(int fd, struct sockaddr* address,
                     socklen_t* address_len) = 0;

  /**
   * @brief Open a file and return a file descriptor.
   *
   * @param pathname The path to the file to open.
   * @param flags  bitwise OR combination of flags. The flags argument specifies
   * how the file should be opened. It must include one of the following access
   * modes: O_RDONLY, O_WRONLY, or O_RDWR.
   *
   * @return On success, a new file descriptor is returned. On error, -1 is
   * returned, and errno is set appropriately.
   *
   */
  virtual int Open(const char* pathname, int flags) = 0;

  /**
   * @brief Closes a file descriptor.
   *
   * @param fd The file descriptor to close.
   *
   */
  virtual void Close(int fd) = 0;

  /**
   * @brief Deletes a file or directory.
   *
   * @param path The path to the file or directory to delete.
   *
   */
  virtual void Unlink(const char* path) = 0;

  /**
   * @brief Initializes an inotify instance for monitoring file system events.
   *
   * @return On success, a new inotify file descriptor is returned. On error, -1
   * is returned, and errno is set appropriately.
   *
   */
  virtual int InotifyInit() = 0;

  /**
   * @brief Adds a watch to an inotify instance for a specific file or
   * directory.
   *
   * @param fd The inotify file descriptor.
   * @param pathname The path to the file or directory to watch.
   * @param mask A bitmask specifying the events to watch for.
   *
   * @return On success, a unique watch descriptor is returned. On error, -1 is
   * returned, and errno is set appropriately.
   *
   */
  virtual int InotifyAddWatch(int fd, const char* pathname, uint32_t mask) = 0;

  /**
   * @brief Creates a new socket.
   *
   * @param domain The communication domain (e.g., AF_INET for IPv4, AF_INET6
   * for IPv6).
   * @param type The socket type (e.g., SOCK_STREAM for TCP, SOCK_DGRAM for
   * UDP).
   * @param protocol The protocol (e.g., 0 for the default protocol).
   *
   * @return On success, a new socket file descriptor is returned. On error, -1
   * is returned, and errno is set appropriately.
   *
   */
  virtual int Socket(int domain, int type, int protocol) = 0;

  /**
   * @brief Binds a socket to a specific address.
   *
   * @param fd The socket file descriptor.
   * @param address A pointer to a sockaddr structure containing the address to
   * bind to.
   * @param address_len The size of the sockaddr structure.
   *
   * @return On success, 0 is returned. On error, -1 is returned, and errno is
   * set appropriately.
   *
   */
  virtual int Bind(int fd, const struct sockaddr* address,
                   socklen_t address_len) = 0;

  /**
   * @brief Marks a socket as a passive socket, ready to accept connections.
   *
   * @param fd The socket file descriptor.
   * @param backlog The maximum number of pending connections in the queue.
   *
   * @return On success, 0 is returned. On error, -1 is returned, and errno is
   * set appropriately.
   *
   */
  virtual int Listen(int fd, int backlog) = 0;

  /**
   * @brief Gets file status information.
   *
   * @param path The path to the file.
   * @param sb A pointer to a stat structure to store the file status
   * information.
   *
   * @return On success, 0 is returned. On error, -1 is returned, and errno is
   * set appropriately.
   *
   */
  virtual int Stat(const char* path, struct stat* sb) = 0;

  /**
   * @brief Checks if a file mode represents a socket file.
   *
   * @param st_mode The file mode.
   *
   * @return True if the file mode represents a socket file, false otherwise.
   *
   */
  virtual bool IsSocketFile(int st_mode) = 0;

  /**
   * @brief Creates a pipe (unidirectional data channel).
   *
   * @param pipefd An array of two integers to store the read and write file
   * descriptors of the pipe.
   * @param flags Flags that can be used to modify the behavior of the pipe
   * (e.g., O_NONBLOCK, O_CLOEXEC).
   *
   * @return On success, 0 is returned. On error, -1 is returned, and errno is
   * set appropriately.
   *
   */
  virtual int CreatePipe(int pipefd[2], int flags) = 0;

  /**
   * @brief Checks if a specific file descriptor is set in a file descriptor
   * set.
   *
   * @param fd The file descriptor to check.
   * @param set A pointer to the fd_set structure.
   *
   * @return 1 if the file descriptor is set, 0 otherwise.
   *
   */
  virtual int FdIsSet(int fd, fd_set* set) = 0;

  /**
   * @brief Sets a specific file descriptor in a file descriptor set.
   *
   * @param fd The file descriptor to set.
   * @param set A pointer to the fd_set structure.
   *
   */
  virtual void FdSet(int fd, fd_set* set) = 0;

  /**
   * @brief Clears all file descriptors in a file descriptor set.
   *
   * @param set A pointer to the fd_set structure.
   *
   */
  virtual void FdZero(fd_set* set) = 0;

  /**
   * @brief Sends a signal to a process.
   *
   * @param pid The process ID.
   * @param signal The signal number.
   *
   * @return On success, 0 is returned. On error, -1 is returned, and errno is
   * set appropriately.
   *
   */
  virtual int Kill(pid_t pid, int signal) = 0;
};

}  // namespace util
}  // namespace bluetooth_hal
