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

#include "bluetooth_hal/extensions/thread/socket_processor.h"

#include <cstdint>
#include <string>
#include <vector>

#include "bluetooth_hal/test/common/test_helper.h"
#include "bluetooth_hal/test/mock/mock_system_call_wrapper.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace thread {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::Test;

using ::bluetooth_hal::util::MatcherFactory;
using ::bluetooth_hal::util::MockPacketHandler;
using ::bluetooth_hal::util::MockSystemCallWrapper;

// Mocked data used for stream-based socket tests.
std::vector<uint8_t> send_data = {0, 1, 2, 3, 4};
constexpr uint8_t kHeaderBuffer[1] = {kSocketSpecificHeader};
constexpr uint16_t kDataSize = 5;
constexpr uint8_t kPayloadSizeBuffer[2] = {
    static_cast<uint8_t>(kDataSize & 0xFF),
    static_cast<uint8_t>((kDataSize >> 8) & 0xFF)};

// Basic test class for socket processor.
class SocketProcessorTestBase : public Test {
 protected:
  void SetUp() override {
    MockSystemCallWrapper::SetMockWrapper(&mock_system_call_wrapper_);

    SocketProcessor::Initialize(
        test_socket_path_,
        std::bind_front(&MockPacketHandler::HalPacketCallback,
                        &mock_packet_handler_));
    TestInit();
  }

  void TearDown() override { TestDeinit(); }

  void TestInit() {
    ASSERT_NE(SocketProcessor::GetProcessor(), nullptr);
    // Socket fd should be initialized when creating the instance.
    ASSERT_EQ(SocketProcessor::GetProcessor()->GetServerSocket(),
              kInvalidFileDescriptor);
    ASSERT_EQ(SocketProcessor::GetProcessor()->GetClientSocket(),
              kInvalidFileDescriptor);
  }

  void TestDeinit() {
    EXPECT_CALL(mock_system_call_wrapper_, Unlink(_)).Times(1);
    ASSERT_NE(SocketProcessor::GetProcessor(), nullptr);
    SocketProcessor::Cleanup();
    // Instance should be null if we do not initialize after cleanup.
    EXPECT_DEATH(SocketProcessor::GetProcessor(), "");
  }

  std::string test_socket_path_ = "/tmp/test/socket";
  MockPacketHandler mock_packet_handler_;
  MockSystemCallWrapper mock_system_call_wrapper_;
};

// Test class for sequence packet mode socket processor.
class SocketProcessorTestSeqPacket : public SocketProcessorTestBase {
 protected:
  void SetUp() override {
    SocketProcessorTestBase::SetUp();
    SocketProcessor::GetProcessor()->SetSocketMode(
        SocketMode::kSockModeSeqPacket);
  }
};

// Test class for stream mode socket processor.
class SocketProcessorTestStream : public SocketProcessorTestBase {
 protected:
  void SetUp() override {
    SocketProcessorTestBase::SetUp();
    SocketProcessor::GetProcessor()->SetSocketMode(SocketMode::kSockModeStream);
  }
};

TEST_F(SocketProcessorTestBase, OpenServerReturnSuccess) {
  ON_CALL(mock_system_call_wrapper_, Socket(_, _, _)).WillByDefault(Return(1));
  ON_CALL(mock_system_call_wrapper_, Bind(_, _, _)).WillByDefault(Return(1));
  ON_CALL(mock_system_call_wrapper_, Listen(_, _)).WillByDefault(Return(1));

  EXPECT_CALL(mock_system_call_wrapper_, Socket(_, _, _)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Unlink(_)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Bind(_, _, _)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Listen(_, _)).Times(1);

  ASSERT_TRUE(SocketProcessor::GetProcessor()->OpenServer());
  ASSERT_NE(SocketProcessor::GetProcessor()->GetServerSocket(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, OpenServerCreateReturnFail) {
  // Setting return value -1 of Socket() indicates a system call error.
  ON_CALL(mock_system_call_wrapper_, Socket(_, _, _)).WillByDefault(Return(-1));

  EXPECT_CALL(mock_system_call_wrapper_, Socket(_, _, _)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Unlink(_)).Times(0);
  EXPECT_CALL(mock_system_call_wrapper_, Bind(_, _, _)).Times(0);
  EXPECT_CALL(mock_system_call_wrapper_, Listen(_, _)).Times(0);

  ASSERT_FALSE(SocketProcessor::GetProcessor()->OpenServer());
  ASSERT_EQ(SocketProcessor::GetProcessor()->GetServerSocket(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, OpenServerBindReturnFail) {
  ON_CALL(mock_system_call_wrapper_, Socket(_, _, _)).WillByDefault(Return(1));
  // Setting return value -1 of Bind() indicates a system call error.
  ON_CALL(mock_system_call_wrapper_, Bind(_, _, _)).WillByDefault(Return(-1));

  EXPECT_CALL(mock_system_call_wrapper_, Socket(_, _, _)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Unlink(_)).Times(2);
  EXPECT_CALL(mock_system_call_wrapper_, Bind(_, _, _)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Close(_)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Listen(_, _)).Times(0);

  ASSERT_FALSE(SocketProcessor::GetProcessor()->OpenServer());
  ASSERT_EQ(SocketProcessor::GetProcessor()->GetServerSocket(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, OpenServerListenReturnFail) {
  ON_CALL(mock_system_call_wrapper_, Socket(_, _, _)).WillByDefault(Return(1));
  ON_CALL(mock_system_call_wrapper_, Bind(_, _, _)).WillByDefault(Return(1));
  // Setting return value -1 of Listen() indicates a system call error.
  ON_CALL(mock_system_call_wrapper_, Listen(_, _)).WillByDefault(Return(-1));

  EXPECT_CALL(mock_system_call_wrapper_, Socket(_, _, _)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Unlink(_)).Times(2);
  EXPECT_CALL(mock_system_call_wrapper_, Bind(_, _, _)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Listen(_, _)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Close(_)).Times(1);

  ASSERT_FALSE(SocketProcessor::GetProcessor()->OpenServer());
  ASSERT_EQ(SocketProcessor::GetProcessor()->GetServerSocket(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, CloseServerCheckServerSocket) {
  EXPECT_CALL(mock_system_call_wrapper_, Unlink(_)).Times(1);
  EXPECT_CALL(mock_system_call_wrapper_, Close(1)).Times(1);

  SocketProcessor::GetProcessor()->SetServerSocket(1);
  SocketProcessor::GetProcessor()->CloseServer();

  ASSERT_EQ(SocketProcessor::GetProcessor()->GetServerSocket(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, CloseClientCheckClientSocket) {
  EXPECT_CALL(mock_system_call_wrapper_, Close(1)).Times(1);

  SocketProcessor::GetProcessor()->SetClientSocket(1);
  SocketProcessor::GetProcessor()->CloseClient();

  ASSERT_EQ(SocketProcessor::GetProcessor()->GetClientSocket(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, AcceptClientReturnSuccess) {
  ON_CALL(mock_system_call_wrapper_, Accept(_, _, _)).WillByDefault(Return(1));

  EXPECT_CALL(mock_system_call_wrapper_, Accept(_, _, _)).Times(1);

  SocketProcessor::GetProcessor()->SetServerSocket(1);
  ASSERT_NE(SocketProcessor::GetProcessor()->AcceptClient(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, AcceptClientWithInvalidServerSocketReturnFail) {
  // We cannot accept a client without creating the server.
  EXPECT_CALL(mock_system_call_wrapper_, Accept(_, _, _)).Times(0);

  ASSERT_EQ(SocketProcessor::GetProcessor()->AcceptClient(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, AcceptClientReturnConnectionFail) {
  // Setting return value -1 for Accept() indicates a system call error.
  ON_CALL(mock_system_call_wrapper_, Accept(_, _, _)).WillByDefault(Return(-1));

  EXPECT_CALL(mock_system_call_wrapper_, Accept(_, _, _)).Times(1);

  SocketProcessor::GetProcessor()->SetServerSocket(1);
  ASSERT_EQ(SocketProcessor::GetProcessor()->AcceptClient(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, SetServerSocketAndCheck) {
  SocketProcessor::GetProcessor()->SetServerSocket(1);
  ASSERT_EQ(SocketProcessor::GetProcessor()->GetServerSocket(), 1);
}

TEST_F(SocketProcessorTestBase, SetClientSocketAndCheck) {
  SocketProcessor::GetProcessor()->SetClientSocket(1);
  ASSERT_EQ(SocketProcessor::GetProcessor()->GetClientSocket(), 1);
}

TEST_F(SocketProcessorTestBase, SendWithUnknownSocketModeReturnFail) {
  std::vector<uint8_t> data;
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Send(data));
}

TEST_F(SocketProcessorTestBase, RecvWithUnknownSocketModeReturnFail) {
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestBase, OpenSocketFileMonitorReturnNotifyFail) {
  EXPECT_CALL(mock_system_call_wrapper_, InotifyInit())
      .Times(1)
      .WillOnce(Return(-1));
  EXPECT_CALL(mock_system_call_wrapper_, InotifyAddWatch(_, _, _)).Times(0);

  ASSERT_EQ(SocketProcessor::GetProcessor()->OpenSocketFileMonitor(),
            kInvalidFileDescriptor);
  ASSERT_EQ(SocketProcessor::GetProcessor()->GetSocketFileMonitor(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, OpenSocketFileMonitorReturnAddWatchFail) {
  EXPECT_CALL(mock_system_call_wrapper_, InotifyInit())
      .Times(1)
      .WillOnce(Return(1));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      InotifyAddWatch(
          _, MatcherFactory::CreateStringMatcher(kThreadDispatcherFolderPath),
          _))
      .Times(1)
      .WillOnce(Return(-1));

  ASSERT_EQ(SocketProcessor::GetProcessor()->OpenSocketFileMonitor(),
            kInvalidFileDescriptor);
  ASSERT_EQ(SocketProcessor::GetProcessor()->GetSocketFileMonitor(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, OpenSocketFileMonitorReturnSuccess) {
  EXPECT_CALL(mock_system_call_wrapper_, InotifyInit())
      .Times(1)
      .WillOnce(Return(1));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      InotifyAddWatch(
          _, MatcherFactory::CreateStringMatcher(kThreadDispatcherFolderPath),
          _))
      .Times(1)
      .WillOnce(Return(1));

  ASSERT_NE(SocketProcessor::GetProcessor()->OpenSocketFileMonitor(),
            kInvalidFileDescriptor);
  ASSERT_NE(SocketProcessor::GetProcessor()->GetSocketFileMonitor(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, OpenSocketFileMonitorTwice) {
  EXPECT_CALL(mock_system_call_wrapper_, InotifyInit())
      .Times(1)
      .WillOnce(Return(1));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      InotifyAddWatch(
          _, MatcherFactory::CreateStringMatcher(kThreadDispatcherFolderPath),
          _))
      .Times(1)
      .WillOnce(Return(1));

  ASSERT_NE(SocketProcessor::GetProcessor()->OpenSocketFileMonitor(),
            kInvalidFileDescriptor);
  ASSERT_NE(SocketProcessor::GetProcessor()->GetSocketFileMonitor(),
            kInvalidFileDescriptor);

  Mock::VerifyAndClearExpectations(&mock_system_call_wrapper_);

  // We already have notify fd, so all system calls should not be executed.
  EXPECT_CALL(mock_system_call_wrapper_, InotifyInit()).Times(0);
  EXPECT_CALL(mock_system_call_wrapper_, InotifyAddWatch(_, _, _)).Times(0);

  ASSERT_NE(SocketProcessor::GetProcessor()->OpenSocketFileMonitor(),
            kInvalidFileDescriptor);
  ASSERT_NE(SocketProcessor::GetProcessor()->GetSocketFileMonitor(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, CloseSocketFileMonitorReturnSuccess) {
  EXPECT_CALL(mock_system_call_wrapper_, InotifyInit())
      .Times(1)
      .WillOnce(Return(1));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      InotifyAddWatch(
          _, MatcherFactory::CreateStringMatcher(kThreadDispatcherFolderPath),
          _))
      .Times(1)
      .WillOnce(Return(1));

  ASSERT_NE(SocketProcessor::GetProcessor()->OpenSocketFileMonitor(),
            kInvalidFileDescriptor);
  ASSERT_NE(SocketProcessor::GetProcessor()->GetSocketFileMonitor(),
            kInvalidFileDescriptor);

  EXPECT_CALL(mock_system_call_wrapper_,
              Close(SocketProcessor::GetProcessor()->GetSocketFileMonitor()))
      .Times(1);

  SocketProcessor::GetProcessor()->CloseSocketFileMonitor();
  ASSERT_EQ(SocketProcessor::GetProcessor()->GetSocketFileMonitor(),
            kInvalidFileDescriptor);
}

TEST_F(SocketProcessorTestBase, CheckIfSocketFileExistedReturnFileExisted) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Stat(MatcherFactory::CreateStringMatcher(test_socket_path_), _))
      .Times(1)
      .WillOnce(Return(0));
  EXPECT_CALL(mock_system_call_wrapper_, IsSocketFile(_))
      .Times(1)
      .WillOnce(Return(true));

  ASSERT_TRUE(SocketProcessor::GetProcessor()->IsSocketFileExisted());
}

TEST_F(SocketProcessorTestBase, CheckIfSocketFileExistedReturnFileNotExisted) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Stat(MatcherFactory::CreateStringMatcher(test_socket_path_), _))
      .Times(1)
      .WillOnce(Return(1));
  EXPECT_CALL(mock_system_call_wrapper_, IsSocketFile(_)).Times(0);

  ASSERT_FALSE(SocketProcessor::GetProcessor()->IsSocketFileExisted());
}

TEST_F(SocketProcessorTestBase,
       CheckIfSocketFileExistedReturnFileExistedButNotSocket) {
  EXPECT_CALL(mock_system_call_wrapper_,
              Stat(MatcherFactory::CreateStringMatcher(test_socket_path_), _))
      .Times(1)
      .WillOnce(Return(0));
  EXPECT_CALL(mock_system_call_wrapper_, IsSocketFile(_))
      .Times(1)
      .WillOnce(Return(false));

  ASSERT_FALSE(SocketProcessor::GetProcessor()->IsSocketFileExisted());
}

TEST_F(SocketProcessorTestSeqPacket, SendPacketReturnSuccess) {
  // We mock Send() to return 3, indicating successful sending of three bytes.
  ON_CALL(mock_system_call_wrapper_, Send(_, _, _, _)).WillByDefault(Return(3));

  EXPECT_CALL(mock_system_call_wrapper_, Send(_, _, _, _)).Times(1);

  std::vector<uint8_t> data;
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Send(data));
}

TEST_F(SocketProcessorTestSeqPacket, SendPacketReturnConnectionFail) {
  // Setting return value of Send() <= 0 indicates a system call error.
  ON_CALL(mock_system_call_wrapper_, Send(_, _, _, _)).WillByDefault(Return(0));

  EXPECT_CALL(mock_system_call_wrapper_, Send(_, _, _, _)).Times(1);

  std::vector<uint8_t> data;
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Send(data));
}

TEST_F(SocketProcessorTestSeqPacket, RecvPacketReturnSuccess) {
  // We mock Recv() to return 5, indicating a successful receipt of five
  // bytes.
  ON_CALL(mock_system_call_wrapper_, Recv(_, _, _, _)).WillByDefault(Return(5));

  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, _, _)).Times(1);
  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(1);

  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestSeqPacket, RecvPacketReturnConnectionFail) {
  // Setting return value of Recv() <= 0 indicates a system call error.
  ON_CALL(mock_system_call_wrapper_, Recv(_, _, _, _)).WillByDefault(Return(0));

  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, _, _)).Times(1);
  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);

  ASSERT_FALSE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestStream, SendPayloadAtOneTimeSuccess) {
  // Mock that the buffer is idle, so we can send the payload in one step.
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _, _))
      .WillByDefault(Return(1));
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kPayloadSizeBuffer, 2),
           _, _))
      .WillByDefault(Return(2));
  ON_CALL(mock_system_call_wrapper_,
          Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data(), 5),
               _, _))
      .WillByDefault(Return(5));

  // Expect Send() to be called three times:
  // 1. Send the header.
  // 2. Send the payload size.
  // 3. Send the payload (5 bytes in total).
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _,
           _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kPayloadSizeBuffer, 2),
           _, _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data(), 5), _,
           _));

  ASSERT_TRUE(SocketProcessor::GetProcessor()->Send(send_data));
}

TEST_F(SocketProcessorTestStream, SendPayloadInMultipleTimesSuccess) {
  // Mock that the buffer is busy, so we need to send the payload in multiple
  // steps.
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _, _))
      .WillByDefault(Return(1));
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kPayloadSizeBuffer, 2),
           _, _))
      .WillByDefault(Return(2));
  ON_CALL(mock_system_call_wrapper_,
          Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data(), 5),
               _, _))
      .WillByDefault(Return(2));
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data() + 2, 3),
           _, _))
      .WillByDefault(Return(2));
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data() + 4, 1),
           _, _))
      .WillByDefault(Return(1));

  // Expect Send() to be called five times:
  // 1. Send the header.
  // 2. Send the payload size.
  // 3. Send the first 2 bytes of the payload.
  // 4. Send the next 2 bytes of the payload.
  // 5. Send the last 1 byte of the payload.
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _,
           _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kPayloadSizeBuffer, 2),
           _, _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data(), 5), _,
           _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data() + 2, 3),
           _, _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data() + 4, 1),
           _, _));

  ASSERT_TRUE(SocketProcessor::GetProcessor()->Send(send_data));
}

TEST_F(SocketProcessorTestStream, SendHeaderReturnConnectionFail) {
  // Mock connection failure when sending header. Setting return value of
  // Send() <= 0 indicates a system call error.
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _, _))
      .WillByDefault(Return(0));

  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _, _))
      .Times(1);
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Send(send_data));
}

TEST_F(SocketProcessorTestStream, SendPayloadSizeReturnConnectionFail) {
  // Mock connection failure when sending payload size.
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _, _))
      .WillByDefault(Return(1));

  // Setting return value of Send() <= 0 indicates a system call error.
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kPayloadSizeBuffer, 2),
           _, _))
      .WillByDefault(Return(0));

  // Expect Send() to be called twice:
  // 1. Send header successfully.
  // 2. Send payload size fails.
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _,
           _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kPayloadSizeBuffer, 2),
           _, _));
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Send(send_data));
}

TEST_F(SocketProcessorTestStream, SendPayloadReturnConnectionFail) {
  // Mock connection failure when sending payload.
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _, _))
      .WillByDefault(Return(1));
  ON_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kPayloadSizeBuffer, 2),
           _, _))
      .WillByDefault(Return(2));

  // Setting return value of Send() <= 0 indicates a system call error.
  ON_CALL(mock_system_call_wrapper_,
          Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data(), 5),
               _, _))
      .WillByDefault(Return(0));

  // Expect Send() to be called three times:
  // 1. Send header successfully.
  // 2. Send payload size successfully.
  // 3. Sending payload fails.
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kHeaderBuffer, 1), _,
           _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(kPayloadSizeBuffer, 2),
           _, _));
  EXPECT_CALL(
      mock_system_call_wrapper_,
      Send(_, MatcherFactory::CreateByteContentMatcher(send_data.data(), 5), _,
           _));
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Send(send_data));
}

TEST_F(SocketProcessorTestStream, RecvPayloadAtOneTimeSuccess) {
  // Header state: mock receiving header successfully, returning 1.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 1, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = kSocketSpecificHeader;
                      }),
                      Return(1)));
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());

  // Flag state: mock expected length size as 17.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 2, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = 17;
                      }),
                      Return(2)));
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());

  // Payload state: read full buffer at one time, returning 17 from recv().
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 17, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        std::fill(ptr, ptr + 17, 1);
                      }),
                      Return(17)));
  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(1);
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestStream, RecvPayloadInMultipleTimesSuccess) {
  // Header state: return 1 as we receive the header successfully.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 1, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = kSocketSpecificHeader;
                      }),
                      Return(1)));
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());

  // Flag state: mock expected length as 17.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 2, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = 17;
                      }),
                      Return(2)));
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());

  // Payload state: expect to call Recv() three times.
  // 1. Receive payload for the first 5 bytes.
  // 2. Receive payload for the next 5 bytes.
  // 3. Receive payload for the last 7 bytes.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 17, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        std::fill(ptr, ptr + 5, 1);
                      }),
                      Return(5)));

  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 12, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        std::fill(ptr, ptr + 5, 1);
                      }),
                      Return(5)));

  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 7, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        std::fill(ptr, ptr + 7, 1);
                      }),
                      Return(7)));

  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(1);
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestStream, RecvHeaderReturnConnectionFail) {
  // Header state: return value of Recv() <= 0 indicates a system call error.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 1, _)).WillOnce(Return(0));
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestStream, RecvHeaderReturnInvalidValueFail) {
  // Header state: mock receiving an invalid header value when calling Recv().
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 1, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = 1;
                      }),
                      Return(1)));
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestStream, RecvPayloadSizeReturnConnectionFail) {
  // Header state: return 1 as we receive the header successfully.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 1, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = kSocketSpecificHeader;
                      }),
                      Return(1)));
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());

  // Flag state: return value of Recv() <= 0 indicates a system call error.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 2, _)).WillOnce(Return(0));
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestStream, RecvPayloadSizeReturnZeroFail) {
  // Header state.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 1, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = kSocketSpecificHeader;
                      }),
                      Return(1)));
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());

  // Flag state: mock an invalid payload size of 0 when calling Recv().
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 2, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = 0;
                      }),
                      Return(2)));
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Recv());
}

TEST_F(SocketProcessorTestStream, RecvPayloadReturnConnectionFail) {
  // Header state.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 1, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = kSocketSpecificHeader;
                      }),
                      Return(1)));
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());

  // Flag state: mock an expected length size of 17.
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 2, _))
      .WillOnce(DoAll(Invoke([]([[maybe_unused]] int fd, void* buffer,
                                [[maybe_unused]] size_t length,
                                [[maybe_unused]] int flags) {
                        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                        *ptr = 17;
                      }),
                      Return(2)));
  ASSERT_TRUE(SocketProcessor::GetProcessor()->Recv());

  // Payload state: system call returns <= 0 when calling Recv().
  EXPECT_CALL(mock_system_call_wrapper_, Recv(_, _, 17, _)).WillOnce(Return(0));

  EXPECT_CALL(mock_packet_handler_, HalPacketCallback(_)).Times(0);
  ASSERT_FALSE(SocketProcessor::GetProcessor()->Recv());
}

}  // namespace
}  // namespace thread
}  // namespace bluetooth_hal
