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

#include "bluetooth_hal/extensions/thread/thread_daemon.h"

#include <sys/inotify.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/test/common/test_helper.h"
#include "bluetooth_hal/test/mock/mock_socket_processor.h"
#include "bluetooth_hal/test/mock/mock_system_call_wrapper.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace thread {
namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Between;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::Test;

using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::util::MatcherFactory;
using ::bluetooth_hal::util::MockPacketHandler;
using ::bluetooth_hal::util::MockSystemCallWrapper;

constexpr int kNotificationFd = 1;
constexpr int kSocketMonitorFd = 2;
constexpr int kServerFd = 3;
constexpr int kClientFd = 4;

constexpr int kSelectMockSleepTimeMs = 1000;
constexpr int kDaemonThreadInitWaitTimeMs = 5;
constexpr int kFullDaemonThreadSelectProcessWaitTimeMs = 2000;

class ThreadDaemonTest : public Test {
 protected:
  void SetUp() override {
    MockSystemCallWrapper::SetMockWrapper(&mock_system_call_wrapper_);
    MockSocketProcessor::SetMockProcessor(&mock_socket_processor_);

    // Default action for mocking functions.
    ON_CALL(mock_system_call_wrapper_, Select(_, _, _, _, _))
        .WillByDefault(Invoke(
            []([[maybe_unused]] int nfds, [[maybe_unused]] fd_set* readfds,
               [[maybe_unused]] fd_set* writefds,
               [[maybe_unused]] fd_set* errorfds,
               [[maybe_unused]] struct timeval* timeout) -> int {
              std::this_thread::sleep_for(
                  std::chrono::milliseconds(kSelectMockSleepTimeMs));
              return 1;
            }));

    ON_CALL(mock_socket_processor_, GetServerSocket())
        .WillByDefault(Return(kServerFd));

    ON_CALL(mock_socket_processor_, OpenSocketFileMonitor())
        .WillByDefault(Return(kSocketMonitorFd));

    ON_CALL(mock_socket_processor_, GetClientSocket())
        .WillByDefault(Return(kClientFd));

    ON_CALL(mock_socket_processor_, GetSocketFileMonitor())
        .WillByDefault(Return(kSocketMonitorFd));

    TestInit();
  }

  void TearDown() override { TestCleanUp(); }

  void TestInit() {
    EXPECT_CALL(mock_socket_processor_, Initialize(_, _)).Times(1);
    EXPECT_CALL(mock_socket_processor_, SetSocketMode(_)).Times(1);

    thread_daemon_ = std::make_unique<ThreadDaemon>(
        std::bind(&MockPacketHandler::HalPacketCallback, &mock_packet_handler_,
                  std::placeholders::_1));
  }

  void TestCleanUp() {
    SetUpDaemonStopExpectations();
    EXPECT_CALL(mock_socket_processor_, Cleanup()).Times(1);

    thread_daemon_ = nullptr;
  }

  void SetUpDaemonStartExpectations() {
    EXPECT_CALL(mock_system_call_wrapper_, CreatePipe(_, _))
        .Times(AtLeast(1))
        .WillRepeatedly(
            DoAll(Invoke([](int pipefd[2], [[maybe_unused]] int flags) -> void {
                    pipefd[0] = kNotificationFd;
                    pipefd[1] = kNotificationFd;
                  }),
                  Return(0)));

    EXPECT_CALL(mock_socket_processor_, OpenServer())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
  }

  void SetUpDaemonStopExpectations() {
    if (thread_daemon_->IsDaemonRunning()) {
      // Main thread to notify daemon thread.
      EXPECT_CALL(mock_system_call_wrapper_, Write(_, _, 1))
          .Times(1)
          .WillOnce(Return(1));

      EXPECT_CALL(mock_system_call_wrapper_, Close(_)).Times(2);

      // Simulate daemon is notified to stop.
      EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kNotificationFd, _))
          .Times(Between(0, 1))
          .WillOnce(Return(1));

      EXPECT_CALL(mock_socket_processor_, CloseClient()).Times(1);
      EXPECT_CALL(mock_socket_processor_, CloseServer()).Times(1);
      EXPECT_CALL(mock_socket_processor_, CloseSocketFileMonitor()).Times(1);
    }
  }

  void SetUpClientConnectExpectations() {
    // Simulate daemon thread receiving a client connect request.
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kNotificationFd, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kServerFd, _))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kSocketMonitorFd, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kClientFd, _))
        .WillRepeatedly(Return(0));
  }

  void SetupClientSignalReceptionExpectations() {
    // Simulate receiving signal from client fd.
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kNotificationFd, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kServerFd, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kSocketMonitorFd, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kClientFd, _))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));
  }

  void SetUpFileMonitorSignalReceptionExpectations() {
    // Simulate receiving signal from file monitor fd.
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kNotificationFd, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kServerFd, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kSocketMonitorFd, _))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kClientFd, _))
        .WillRepeatedly(Return(0));
  }

  MockSocketProcessor mock_socket_processor_;
  MockSystemCallWrapper mock_system_call_wrapper_;
  MockPacketHandler mock_packet_handler_;
  std::unique_ptr<ThreadDaemon> thread_daemon_;
};

TEST_F(ThreadDaemonTest, CheckDaemonReturnNotRunning) {
  // Daemon should not run before Start() is called.
  ASSERT_FALSE(thread_daemon_->IsDaemonRunning());
}

TEST_F(ThreadDaemonTest, StartDaemonReturnSuccess) {
  SetUpDaemonStartExpectations();

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Ensure the daemon thread reaches the blocking point.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kDaemonThreadInitWaitTimeMs));
}

TEST_F(ThreadDaemonTest, StartDaemonReturnFail) {
  EXPECT_CALL(mock_system_call_wrapper_, CreatePipe(_, _))
      .Times(1)
      .WillOnce(Return(1));
  EXPECT_CALL(mock_socket_processor_, OpenServer()).Times(0);

  EXPECT_FALSE(thread_daemon_->Start());
  ASSERT_FALSE(thread_daemon_->IsDaemonRunning());
}

TEST_F(ThreadDaemonTest, StartDaemonTwiceReturnFail) {
  SetUpDaemonStartExpectations();

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Ensure the daemon thread reaches the blocking point.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kDaemonThreadInitWaitTimeMs));

  EXPECT_FALSE(thread_daemon_->Start());
}

TEST_F(ThreadDaemonTest, StopDaemonReturnSuccess) {
  SetUpDaemonStartExpectations();

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Ensure the daemon thread reaches the blocking point.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kDaemonThreadInitWaitTimeMs));

  SetUpDaemonStopExpectations();

  EXPECT_TRUE(thread_daemon_->Stop());
  ASSERT_FALSE(thread_daemon_->IsDaemonRunning());
}

TEST_F(ThreadDaemonTest, StopDaemonReturnFail) {
  // Stop should fail if the daemon is not running.
  EXPECT_FALSE(thread_daemon_->Stop());
}

TEST_F(ThreadDaemonTest, AcceptClientReturnSuccess) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  // Simulate successful client acceptance.
  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kClientFd));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(1);

  // 1. kInvalidFileDescriptor in PrepareFdsForMonitor() as no client connected.
  // 2. kInvalidFileDescriptor in AcceptClient() for first connection.
  // 3. After that all are kClientFd as one client is connected.
  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillRepeatedly(Return(kClientFd));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Ensure the daemon thread finishes the tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  SetUpDaemonStopExpectations();

  EXPECT_TRUE(thread_daemon_->Stop());
  ASSERT_FALSE(thread_daemon_->IsDaemonRunning());
}

TEST_F(ThreadDaemonTest, AcceptClientReturnAcceptFail) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  // Simulate client acceptance failure.
  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kInvalidFileDescriptor));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(0);

  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillRepeatedly(Return(kInvalidFileDescriptor));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Ensure the daemon thread completes the tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  SetUpDaemonStopExpectations();

  EXPECT_TRUE(thread_daemon_->Stop());
  ASSERT_FALSE(thread_daemon_->IsDaemonRunning());
}

TEST_F(ThreadDaemonTest, AcceptClientReturnGetSocketFail) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  // Simulate client acceptance, then socket failure.
  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kClientFd));

  // 1. kClientFd in PrepareFdsForMonitor() for a connected client.
  // 2. kClientFd in AcceptClient() for the connected client.
  // 3. Detect invalid state and clean up client, so it's
  // kInvalidFileDescriptor.
  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kClientFd))
      .WillOnce(Return(kClientFd))
      .WillRepeatedly(Return(kInvalidFileDescriptor));

  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(0);

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Ensure the daemon thread completes the tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  SetUpDaemonStopExpectations();

  EXPECT_TRUE(thread_daemon_->Stop());
  ASSERT_FALSE(thread_daemon_->IsDaemonRunning());
}

TEST_F(ThreadDaemonTest, AcceptClientTwiceReturnFail) {
  SetUpDaemonStartExpectations();

  // Simulate daemon thread receiving two client connect requests.
  EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kNotificationFd, _))
      .WillRepeatedly(Return(0));
  EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kServerFd, _))
      .WillOnce(Return(1))
      .WillOnce(Return(1))
      .WillRepeatedly(Return(0));
  EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kSocketMonitorFd, _))
      .WillRepeatedly(Return(0));
  EXPECT_CALL(mock_system_call_wrapper_, FdIsSet(kClientFd, _))
      .WillRepeatedly(Return(0));

  // Simulate accepting a client while one is already connected.
  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(2)
      .WillOnce(Return(kClientFd))
      .WillOnce(Return(kClientFd));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(1);

  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillRepeatedly(Return(kClientFd));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Double the process time to account for two requests.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs) * 2);

  SetUpDaemonStopExpectations();

  EXPECT_TRUE(thread_daemon_->Stop());
  ASSERT_FALSE(thread_daemon_->IsDaemonRunning());
}

TEST_F(ThreadDaemonTest, ReceiveClientDisconnectSignal) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kClientFd));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(1);

  // 1. kInvalidFileDescriptor in PrepareFdsForMonitor() as no client connected.
  // 2. kInvalidFileDescriptor in AcceptClient() for first connection.
  // 3. After that, all are kClientFd as one client is connected.
  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillRepeatedly(Return(kClientFd));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Ensure daemon thread finishes the above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);
  Mock::VerifyAndClearExpectations(&mock_socket_processor_);

  SetupClientSignalReceptionExpectations();

  EXPECT_CALL(mock_socket_processor_, Recv()).Times(1).WillOnce(Return(false));
  EXPECT_CALL(mock_socket_processor_, CloseClient()).Times(1);

  // Ensure daemon thread finishes the above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);
  Mock::VerifyAndClearExpectations(&mock_socket_processor_);
}

TEST_F(ThreadDaemonTest, ReceivePacketFromClientReturnSuccess) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kClientFd));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(1);

  // 1. kInvalidFileDescriptor in PrepareFdsForMonitor() as no client connected.
  // 2. kInvalidFileDescriptor in AcceptClient() for first connection.
  // 3. After that, all are kClientFd as one client is connected.
  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillRepeatedly(Return(kClientFd));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Ensure daemon thread finishes the above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);
  Mock::VerifyAndClearExpectations(&mock_socket_processor_);

  SetupClientSignalReceptionExpectations();

  EXPECT_CALL(mock_socket_processor_, Recv()).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mock_socket_processor_, CloseClient()).Times(0);

  // Ensure daemon thread finishes the above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);
  Mock::VerifyAndClearExpectations(&mock_socket_processor_);
}

TEST_F(ThreadDaemonTest, SendUplinkWhenDaemonNotRunningReturnFail) {
  EXPECT_CALL(mock_socket_processor_, Send(_)).Times(0);

  std::vector<uint8_t> valid_hal_packet = {0x70, 0x00, 0x00, 0x05, 0x00,
                                           0x00, 0x01, 0x02, 0x03, 0x04};
  thread_daemon_->SendUplink(valid_hal_packet);
}

TEST_F(ThreadDaemonTest, SendUplinkWhenNoClientConnectedReturnFail) {
  SetUpDaemonStartExpectations();

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread reaches blocking point.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kDaemonThreadInitWaitTimeMs));

  EXPECT_CALL(mock_socket_processor_, Send(_)).Times(0);

  std::vector<uint8_t> valid_hal_packet = {0x70, 0x00, 0x00, 0x05, 0x00,
                                           0x00, 0x01, 0x02, 0x03, 0x04};
  thread_daemon_->SendUplink(valid_hal_packet);
}

TEST_F(ThreadDaemonTest, SendUplinkWithEmptyPacketReturnFail) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kClientFd));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(1);

  // 1. kInvalidFileDescriptor in PrepareFdsForMonitor() as no client connected.
  // 2. kInvalidFileDescriptor in AcceptClient() for first connection.
  // 3. After that all are kClientFd as one client is connected.
  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillRepeatedly(Return(kClientFd));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread finishes above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  EXPECT_CALL(mock_socket_processor_, Send(_)).Times(0);

  HalPacket empty_hal_packet;
  thread_daemon_->SendUplink(empty_hal_packet);
}

TEST_F(ThreadDaemonTest, SendUplinkWithInvalidHeaderSizeReturnFail) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kClientFd));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(1);

  // 1. kInvalidFileDescriptor in PrepareFdsForMonitor() as no client connected.
  // 2. kInvalidFileDescriptor in AcceptClient() for first connection.
  // 3. After that all are kClientFd as one client is connected.
  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillRepeatedly(Return(kClientFd));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread finishes above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  std::vector<uint8_t> empty_packet;
  EXPECT_CALL(mock_socket_processor_,
              Send(MatcherFactory::CreateVectorMatcher(empty_packet)))
      .Times(1);

  std::vector<uint8_t> invalid_header_hal_packet = {0x00, 0x00, 0x05};
  thread_daemon_->SendUplink(invalid_header_hal_packet);
}

TEST_F(ThreadDaemonTest, SendUplinkWithInvalidPacketSizeReturnFail) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kClientFd));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(1);

  // 1. kInvalidFileDescriptor in PrepareFdsForMonitor() as no client connected.
  // 2. kInvalidFileDescriptor in AcceptClient() for first connection.
  // 3. After that all are kClientFd as one client is connected.
  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillRepeatedly(Return(kClientFd));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread finishes above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  std::vector<uint8_t> empty_packet;
  EXPECT_CALL(mock_socket_processor_,
              Send(MatcherFactory::CreateVectorMatcher(empty_packet)))
      .Times(1);

  std::vector<uint8_t> invalid_packet_size_hal_packet = {
      0x70, 0x00, 0x00, 0x05, 0x00, 0x00, 0x01, 0x02, 0x03};
  thread_daemon_->SendUplink(invalid_packet_size_hal_packet);
}

TEST_F(ThreadDaemonTest, SendUplinkReturnSuccess) {
  SetUpDaemonStartExpectations();
  SetUpClientConnectExpectations();

  EXPECT_CALL(mock_socket_processor_, AcceptClient())
      .Times(1)
      .WillOnce(Return(kClientFd));
  EXPECT_CALL(mock_socket_processor_, SetClientSocket(kClientFd)).Times(1);

  // 1. kInvalidFileDescriptor in PrepareFdsForMonitor() as no client connected.
  // 2. kInvalidFileDescriptor in AcceptClient() for first connection.
  // 3. After that all are kClientFd as one client is connected.
  EXPECT_CALL(mock_socket_processor_, GetClientSocket())
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillOnce(Return(kInvalidFileDescriptor))
      .WillRepeatedly(Return(kClientFd));

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread finishes above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  std::vector<uint8_t> valid_spinel_packet = {0x00, 0x01, 0x02, 0x03, 0x04};
  EXPECT_CALL(mock_socket_processor_,
              Send(MatcherFactory::CreateVectorMatcher(valid_spinel_packet)))
      .Times(1);

  std::vector<uint8_t> valid_hal_packet = {0x70, 0x00, 0x00, 0x05, 0x00,
                                           0x00, 0x01, 0x02, 0x03, 0x04};
  thread_daemon_->SendUplink(valid_hal_packet);
}

TEST_F(ThreadDaemonTest, SendDownlinkWithHardwareResetReturnCrash) {
  SetUpDaemonStartExpectations();

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread reach blocking point.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kDaemonThreadInitWaitTimeMs));

  EXPECT_CALL(mock_socket_processor_, Cleanup()).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Kill(_, _)).Times(1);
  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);

  HalPacket valid_spinel_packet(
      {kSpinelHeader, kThreadCommandReset, kThreadCommandResetHardware});
  thread_daemon_->SendDownlink(valid_spinel_packet);
}

TEST_F(ThreadDaemonTest, SendDownlinkReturnSuccess) {
  SetUpDaemonStartExpectations();

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread reach blocking point.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kDaemonThreadInitWaitTimeMs));

  HalPacket valid_hal_packet(
      {0x70, 0x00, 0x00, 0x05, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04});
  EXPECT_CALL(mock_packet_handler_,
              HalPacketCallback(
                  MatcherFactory::CreateHalPacketMatcher(valid_hal_packet)))
      .Times(1);

  std::vector<uint8_t> valid_spinel_packet = {0x00, 0x01, 0x02, 0x03, 0x04};
  thread_daemon_->SendDownlink(valid_spinel_packet);
}

TEST_F(ThreadDaemonTest, ReceiveDeleteFileSignalButReadFail) {
  SetUpDaemonStartExpectations();
  SetUpFileMonitorSignalReceptionExpectations();

  // Simulate read fail.
  EXPECT_CALL(mock_system_call_wrapper_, Read(kSocketMonitorFd, _, _))
      .Times(1)
      .WillOnce(Return(0));
  EXPECT_CALL(mock_socket_processor_, CloseSocketFileMonitor()).Times(0);
  EXPECT_CALL(mock_socket_processor_, CloseClient()).Times(0);
  EXPECT_CALL(mock_socket_processor_, CloseServer()).Times(0);

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread finish above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);
  Mock::VerifyAndClearExpectations(&mock_socket_processor_);
}

TEST_F(ThreadDaemonTest, ReceiveDeleteFileSignalButNotSocketFile) {
  SetUpDaemonStartExpectations();
  SetUpFileMonitorSignalReceptionExpectations();

  // Simulate socket file is deleted but not socket file.
  EXPECT_CALL(mock_system_call_wrapper_, Read(kSocketMonitorFd, _, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t count) -> void {
                        struct inotify_event* event =
                            reinterpret_cast<struct inotify_event*>(buffer);
                        event->mask |= IN_DELETE;
                      }),
                      Return(sizeof(struct inotify_event))));
  EXPECT_CALL(mock_socket_processor_, IsSocketFileExisted())
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(mock_socket_processor_, CloseSocketFileMonitor()).Times(0);
  EXPECT_CALL(mock_socket_processor_, CloseClient()).Times(0);
  EXPECT_CALL(mock_socket_processor_, CloseServer()).Times(0);

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread finish above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);
  Mock::VerifyAndClearExpectations(&mock_socket_processor_);
}

TEST_F(ThreadDaemonTest, ReceiveDeleteSocketFileSignalReturnDaemonRestart) {
  SetUpDaemonStartExpectations();
  SetUpFileMonitorSignalReceptionExpectations();

  // Simulate socket file is deleted and daemon is restarted.
  EXPECT_CALL(mock_system_call_wrapper_, Read(kSocketMonitorFd, _, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t count) -> void {
                        struct inotify_event* event =
                            reinterpret_cast<struct inotify_event*>(buffer);
                        event->mask |= IN_DELETE;
                      }),
                      Return(sizeof(struct inotify_event))));
  EXPECT_CALL(mock_socket_processor_, IsSocketFileExisted())
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(mock_socket_processor_, CloseSocketFileMonitor()).Times(2);
  EXPECT_CALL(mock_socket_processor_, CloseClient()).Times(1);
  EXPECT_CALL(mock_socket_processor_, CloseServer()).Times(1);

  EXPECT_TRUE(thread_daemon_->Start());
  ASSERT_TRUE(thread_daemon_->IsDaemonRunning());

  // Make sure daemon thread finish above tasks.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kFullDaemonThreadSelectProcessWaitTimeMs));

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);
  Mock::VerifyAndClearExpectations(&mock_socket_processor_);
}

}  // namespace
}  // namespace thread
}  // namespace bluetooth_hal
