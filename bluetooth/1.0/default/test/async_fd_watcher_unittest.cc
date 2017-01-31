//
// Copyright 2017 The Android Open Source Project
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
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

class AsyncFdWatcherSocketTest : public ::testing::Test {
 public:
  static const uint16_t kPort = 6111;
  static const size_t kBufferSize = 16;

  bool CheckBufferEquals() {
    return strcmp(server_buffer_, client_buffer_) == 0;
  }

 protected:
  int StartServer() {
    ALOGD("%s", __func__);
    struct sockaddr_in serv_addr;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_FALSE(fd < 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(kPort);
    int reuse_flag = 1;
    EXPECT_FALSE(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_flag,
                            sizeof(reuse_flag)) < 0);
    EXPECT_FALSE(bind(fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) <
                 0);

    ALOGD("%s before listen", __func__);
    listen(fd, 1);
    return fd;
  }

  int AcceptConnection(int fd) {
    ALOGD("%s", __func__);
    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(cli_addr));
    socklen_t clilen = sizeof(cli_addr);

    int connection_fd = accept(fd, (struct sockaddr*)&cli_addr, &clilen);
    EXPECT_FALSE(connection_fd < 0);

    return connection_fd;
  }

  void ReadIncomingMessage(int fd) {
    ALOGD("%s", __func__);
    int n = TEMP_FAILURE_RETRY(read(fd, server_buffer_, kBufferSize - 1));
    EXPECT_FALSE(n < 0);

    if (n == 0)  // got EOF
      ALOGD("%s: EOF", __func__);
    else
      ALOGD("%s: Got something", __func__);
      n = write(fd, "1", 1);
  }

  void SetUp() override {
    ALOGD("%s", __func__);
    memset(server_buffer_, 0, kBufferSize);
    memset(client_buffer_, 0, kBufferSize);
  }

  void ConfigureServer() {
    socket_fd_ = StartServer();

    conn_watcher_.WatchFdForNonBlockingReads(socket_fd_, [this](int fd) {
      int connection_fd = AcceptConnection(fd);
      ALOGD("%s: Conn_watcher fd = %d", __func__, fd);

      conn_watcher_.ConfigureTimeout(std::chrono::seconds(0), [this]() { bool connection_timeout_cleared = false; ASSERT_TRUE(connection_timeout_cleared); });

      ALOGD("%s: 3", __func__);
      async_fd_watcher_.WatchFdForNonBlockingReads(
          connection_fd, [this](int fd) { ReadIncomingMessage(fd); });

      // Time out if it takes longer than a second.
      SetTimeout(std::chrono::seconds(1));
    });
    conn_watcher_.ConfigureTimeout(std::chrono::seconds(1), [this]() { bool connection_timeout = true; ASSERT_FALSE(connection_timeout); });
  }

  void CleanUpServer() {
    async_fd_watcher_.StopWatchingFileDescriptor();
    conn_watcher_.StopWatchingFileDescriptor();
    close(socket_fd_);
  }

  void TearDown() override {
    ALOGD("%s 3", __func__);
    EXPECT_TRUE(CheckBufferEquals());
  }

  void OnTimeout() {
    ALOGD("%s", __func__);
    timed_out_ = true;
  }

  void ClearTimeout() {
    ALOGD("%s", __func__);
    timed_out_ = false;
  }

  bool TimedOut() {
    ALOGD("%s %d", __func__, timed_out_? 1 : 0);
    return timed_out_;
  }

  void SetTimeout(std::chrono::milliseconds timeout_ms) {
    ALOGD("%s", __func__);
    async_fd_watcher_.ConfigureTimeout(timeout_ms, [this]() { OnTimeout(); });
    ClearTimeout();
  }

  int ConnectClient() {
    ALOGD("%s", __func__);
    int socket_cli_fd = socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_FALSE(socket_cli_fd < 0);

    struct sockaddr_in serv_addr;
    memset((void*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(kPort);

    int result =
        connect(socket_cli_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    EXPECT_FALSE(result < 0);

    return socket_cli_fd;
  }

  void WriteFromClient(int socket_cli_fd) {
    ALOGD("%s", __func__);
    strcpy(client_buffer_, "1");
    int n = write(socket_cli_fd, client_buffer_, strlen(client_buffer_));
    EXPECT_TRUE(n > 0);
  }

  void AwaitServerResponse(int socket_cli_fd) {
    ALOGD("%s", __func__);
    int n = read(socket_cli_fd, client_buffer_, 1);
    ALOGD("%s done", __func__);
    EXPECT_TRUE(n > 0);
  }

 private:
  AsyncFdWatcher async_fd_watcher_;
  AsyncFdWatcher conn_watcher_;
  int socket_fd_;
  char server_buffer_[kBufferSize];
  char client_buffer_[kBufferSize];
  bool timed_out_;
};

// Use a single AsyncFdWatcher to signal a connection to the server socket.
TEST_F(AsyncFdWatcherSocketTest, Connect) {
  int socket_fd = StartServer();

  AsyncFdWatcher conn_watcher;
  conn_watcher.WatchFdForNonBlockingReads(socket_fd, [this](int fd) {
    int connection_fd = AcceptConnection(fd);
    close(connection_fd);
  });

  // Fail if the client doesn't connect within 1 second.
  conn_watcher.ConfigureTimeout(std::chrono::seconds(1), [this]() {
     bool connection_timeout = true;
     ASSERT_FALSE(connection_timeout);
  });

  ConnectClient();
  conn_watcher.StopWatchingFileDescriptor();
  close(socket_fd);
}

// Use a single AsyncFdWatcher to signal a connection to the server socket.
TEST_F(AsyncFdWatcherSocketTest, TimedOutConnect) {
  int socket_fd = StartServer();
  bool timed_out = false;
  bool* timeout_ptr = &timed_out;

  AsyncFdWatcher conn_watcher;
  conn_watcher.WatchFdForNonBlockingReads(socket_fd, [this](int fd) {
    int connection_fd = AcceptConnection(fd);
    close(connection_fd);
  });

  // Set the timeout flag after 100ms.
  conn_watcher.ConfigureTimeout(std::chrono::milliseconds(100), [this, timeout_ptr]() { *timeout_ptr = true; });
  EXPECT_FALSE(timed_out);
  sleep(1);
  EXPECT_TRUE(timed_out);
  conn_watcher.StopWatchingFileDescriptor();
  close(socket_fd);
}

// Use two AsyncFdWatchers to set up a server socket.
TEST_F(AsyncFdWatcherSocketTest, ClientServer) {
  ConfigureServer();
  int socket_cli_fd = ConnectClient();

  WriteFromClient(socket_cli_fd);

  AwaitServerResponse(socket_cli_fd);

  close(socket_cli_fd);
  CleanUpServer();
}

// Use two AsyncFdWatchers to set up a server socket, which times out.
TEST_F(AsyncFdWatcherSocketTest, TimeOutTest) {
  ConfigureServer();
  int socket_cli_fd = ConnectClient();

  while (!TimedOut()) sleep(1);

  close(socket_cli_fd);
  CleanUpServer();
}

// Use two AsyncFdWatchers to set up a server socket, which times out.
TEST_F(AsyncFdWatcherSocketTest, RepeatedTimeOutTest) {
  ConfigureServer();
  int socket_cli_fd = ConnectClient();
  ClearTimeout();

  // Time out when there are no writes.
  EXPECT_FALSE(TimedOut());
  sleep(2);
  EXPECT_TRUE(TimedOut());
  ClearTimeout();

  // Don't time out when there is a write.
  WriteFromClient(socket_cli_fd);
  AwaitServerResponse(socket_cli_fd);
  EXPECT_FALSE(TimedOut());
  ClearTimeout();

  // Time out when the write is late.
  sleep(2);
  WriteFromClient(socket_cli_fd);
  AwaitServerResponse(socket_cli_fd);
  EXPECT_TRUE(TimedOut());
  ClearTimeout();

  // Time out when there is a pause after a write.
  WriteFromClient(socket_cli_fd);
  sleep(2);
  AwaitServerResponse(socket_cli_fd);
  EXPECT_TRUE(TimedOut());
  ClearTimeout();

  close(socket_cli_fd);
  CleanUpServer();
}

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
