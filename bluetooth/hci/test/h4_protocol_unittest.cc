/*
 * Copyright 2022 The Android Open Source Project
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

#define LOG_TAG "bt_h4_unittest"

#include "h4_protocol.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <future>
#include <vector>

#include "async_fd_watcher.h"

using android::hardware::bluetooth::async::AsyncFdWatcher;
using namespace android::hardware::bluetooth::hci;
using ::testing::Eq;

static char sample_data1[100] = "A point is that which has no part.";
static char sample_data2[100] = "A line is breadthless length.";
static char sample_data3[100] = "The ends of a line are points.";
static char sample_data4[100] =
    "A plane surface is a surface which lies evenly with the straight ...";
static char acl_data[100] =
    "A straight line is a line which lies evenly with the points on itself.";
static char sco_data[100] =
    "A surface is that which has length and breadth only.";
static char event_data[100] = "The edges of a surface are lines.";
static char iso_data[100] =
    "A plane angle is the inclination to one another of two lines in a ...";
static char short_payload[10] = "12345";

// 5 seconds.  Just don't hang.
static constexpr size_t kTimeoutMs = 5000;

MATCHER_P3(PacketMatches, header_, header_length, payload,
           "Match header_length bytes of header and then the payload") {
  size_t payload_length = strlen(payload);
  if (header_length + payload_length != arg.size()) {
    return false;
  }

  if (memcmp(header_, arg.data(), header_length) != 0) {
    return false;
  }

  return memcmp(payload, arg.data() + header_length, payload_length) == 0;
};

ACTION_P(Notify, barrier) {
  ALOGD("%s", __func__);
  barrier->set_value();
}

class H4ProtocolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ALOGD("%s", __func__);

    int sockfd[2];
    socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
    chip_uart_fd_ = sockfd[1];
    stack_uart_fd_ = sockfd[0];
    h4_hci_ = std::make_shared<H4Protocol>(
        stack_uart_fd_, cmd_cb_.AsStdFunction(), acl_cb_.AsStdFunction(),
        sco_cb_.AsStdFunction(), event_cb_.AsStdFunction(),
        iso_cb_.AsStdFunction(), disconnect_cb_.AsStdFunction());
  }

  void TearDown() override {
    close(stack_uart_fd_);
    close(chip_uart_fd_);
  }

  virtual void CallDataReady() { h4_hci_->OnDataReady(); }

  void SendAndReadUartOutbound(PacketType type, char* data) {
    ALOGD("%s sending", __func__);
    int data_length = strlen(data);
    h4_hci_->Send(type, (uint8_t*)data, data_length);

    int uart_length = data_length + 1;  // + 1 for data type code
    int i;

    ALOGD("%s reading", __func__);
    for (i = 0; i < uart_length; i++) {
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(chip_uart_fd_, &read_fds);
      TEMP_FAILURE_RETRY(
          select(chip_uart_fd_ + 1, &read_fds, nullptr, nullptr, nullptr));

      char byte;
      TEMP_FAILURE_RETRY(read(chip_uart_fd_, &byte, 1));

      EXPECT_EQ(i == 0 ? static_cast<uint8_t>(type) : data[i - 1], byte);
    }

    EXPECT_EQ(i, uart_length);
  }

  void ExpectInboundAclData(char* payload, std::promise<void>* promise) {
    // h4 type[1] + handle[2] + size[2]
    header_[0] = static_cast<uint8_t>(PacketType::ACL_DATA);
    header_[1] = 19;
    header_[2] = 92;
    int length = strlen(payload);
    header_[3] = length & 0xFF;
    header_[4] = (length >> 8) & 0xFF;
    ALOGD("(%d bytes) %s", length, payload);

    EXPECT_CALL(acl_cb_,
                Call(PacketMatches(header_ + 1, kAclHeaderSize, payload)))
        .WillOnce(Notify(promise));
  }

  void WaitForTimeout(std::promise<void>* promise) {
    auto future = promise->get_future();
    auto status = future.wait_for(std::chrono::milliseconds(kTimeoutMs));
    EXPECT_EQ(status, std::future_status::ready);
  }

  void WriteInboundAclData(char* payload) {
    // Use the header_ computed in ExpectInboundAclData
    TEMP_FAILURE_RETRY(write(chip_uart_fd_, header_, kAclHeaderSize + 1));
    TEMP_FAILURE_RETRY(write(chip_uart_fd_, payload, strlen(payload)));
  }

  void ExpectInboundScoData(char* payload, std::promise<void>* promise) {
    // h4 type[1] + handle[2] + size[1]
    header_[0] = static_cast<uint8_t>(PacketType::SCO_DATA);
    header_[1] = 20;
    header_[2] = 17;
    header_[3] = strlen(payload) & 0xFF;
    EXPECT_CALL(sco_cb_,
                Call(PacketMatches(header_ + 1, kScoHeaderSize, payload)))
        .WillOnce(Notify(promise));
  }

  void WriteInboundScoData(char* payload) {
    // Use the header_ computed in ExpectInboundScoData
    ALOGD("%s writing", __func__);
    TEMP_FAILURE_RETRY(write(chip_uart_fd_, header_, kScoHeaderSize + 1));
    TEMP_FAILURE_RETRY(write(chip_uart_fd_, payload, strlen(payload)));
  }

  void ExpectInboundEvent(char* payload, std::promise<void>* promise) {
    // h4 type[1] + event_code[1] + size[1]
    header_[0] = static_cast<uint8_t>(PacketType::EVENT);
    header_[1] = 9;
    header_[2] = strlen(payload) & 0xFF;
    EXPECT_CALL(event_cb_,
                Call(PacketMatches(header_ + 1, kEventHeaderSize, payload)))
        .WillOnce(Notify(promise));
  }

  void WriteInboundEvent(char* payload) {
    // Use the header_ computed in ExpectInboundEvent
    char preamble[3] = {static_cast<uint8_t>(PacketType::EVENT), 9, 0};
    preamble[2] = strlen(payload) & 0xFF;
    ALOGD("%s writing", __func__);
    TEMP_FAILURE_RETRY(write(chip_uart_fd_, header_, kEventHeaderSize + 1));
    TEMP_FAILURE_RETRY(write(chip_uart_fd_, payload, strlen(payload)));
  }

  void ExpectInboundIsoData(char* payload, std::promise<void>* promise) {
    // h4 type[1] + handle[2] + size[1]
    header_[0] = static_cast<uint8_t>(PacketType::ISO_DATA);
    header_[1] = 19;
    header_[2] = 92;
    int length = strlen(payload);
    header_[3] = length & 0xFF;
    header_[4] = (length >> 8) & 0x3F;

    EXPECT_CALL(iso_cb_,
                Call(PacketMatches(header_ + 1, kIsoHeaderSize, payload)))
        .WillOnce(Notify(promise));
  }

  void WriteInboundIsoData(char* payload) {
    // Use the header_ computed in ExpectInboundIsoData
    ALOGD("%s writing", __func__);
    TEMP_FAILURE_RETRY(write(chip_uart_fd_, header_, kIsoHeaderSize + 1));
    TEMP_FAILURE_RETRY(write(chip_uart_fd_, payload, strlen(payload)));
  }

  void WriteAndExpectManyInboundAclDataPackets(char* payload) {
    size_t kNumPackets = 20;
    // h4 type[1] + handle[2] + size[2]
    char preamble[5] = {static_cast<uint8_t>(PacketType::ACL_DATA), 19, 92, 0,
                        0};
    int length = strlen(payload);
    preamble[3] = length & 0xFF;
    preamble[4] = (length >> 8) & 0xFF;

    EXPECT_CALL(acl_cb_, Call(PacketMatches(preamble + 1, sizeof(preamble) - 1,
                                            payload)))
        .Times(kNumPackets);

    for (size_t i = 0; i < kNumPackets; i++) {
      TEMP_FAILURE_RETRY(write(chip_uart_fd_, preamble, sizeof(preamble)));
      TEMP_FAILURE_RETRY(write(chip_uart_fd_, payload, strlen(payload)));
    }

    CallDataReady();
  }

  void WriteAndExpectManyAclDataPacketsDifferentOffsetsShort() {
    std::promise<void> last_packet_promise;
    size_t kNumPackets = 30;
    // h4 type[1] + handle[2] + size[2]
    char preamble[5] = {static_cast<uint8_t>(PacketType::ACL_DATA), 19, 92, 0,
                        0};
    int length = strlen(short_payload);
    preamble[3] = length & 0xFF;
    preamble[4] = 0;

    EXPECT_CALL(acl_cb_, Call(PacketMatches(preamble + 1, kAclHeaderSize,
                                            short_payload)))
        .Times(kNumPackets);
    ExpectInboundEvent(event_data, &last_packet_promise);

    char all_packets[kNumPackets * 10];
    size_t total_bytes = 0;

    for (size_t packet = 0; packet < kNumPackets; packet++) {
      for (size_t i = 0; i < sizeof(preamble); i++) {
        all_packets[total_bytes++] = preamble[i];
      }
      for (size_t i = 0; i < length; i++) {
        all_packets[total_bytes++] = short_payload[i];
      }
    }

    size_t written_bytes = 0;
    size_t partial_size = 1;
    while (written_bytes < total_bytes) {
      size_t to_write = std::min(partial_size, total_bytes - written_bytes);
      TEMP_FAILURE_RETRY(
          write(chip_uart_fd_, all_packets + written_bytes, to_write));
      written_bytes += to_write;
      CallDataReady();
      partial_size++;
      partial_size = partial_size % 5 + 1;
    }
    WriteInboundEvent(event_data);
    CallDataReady();
    WaitForTimeout(&last_packet_promise);
  }

  testing::MockFunction<void(const std::vector<uint8_t>&)> cmd_cb_;
  testing::MockFunction<void(const std::vector<uint8_t>&)> event_cb_;
  testing::MockFunction<void(const std::vector<uint8_t>&)> acl_cb_;
  testing::MockFunction<void(const std::vector<uint8_t>&)> sco_cb_;
  testing::MockFunction<void(const std::vector<uint8_t>&)> iso_cb_;
  testing::MockFunction<void(void)> disconnect_cb_;
  std::shared_ptr<H4Protocol> h4_hci_;
  int chip_uart_fd_;
  int stack_uart_fd_;

  char header_[5];
};

// Test sending data sends correct data onto the UART
TEST_F(H4ProtocolTest, TestSends) {
  SendAndReadUartOutbound(PacketType::COMMAND, sample_data1);
  SendAndReadUartOutbound(PacketType::ACL_DATA, sample_data2);
  SendAndReadUartOutbound(PacketType::SCO_DATA, sample_data3);
  SendAndReadUartOutbound(PacketType::ISO_DATA, sample_data4);
}

// Ensure we properly parse data coming from the UART
TEST_F(H4ProtocolTest, TestReads) {
  std::promise<void> acl_promise;
  std::promise<void> sco_promise;
  std::promise<void> event_promise;
  std::promise<void> iso_promise;

  ExpectInboundAclData(acl_data, &acl_promise);
  WriteInboundAclData(acl_data);
  CallDataReady();
  ExpectInboundScoData(sco_data, &sco_promise);
  WriteInboundScoData(sco_data);
  CallDataReady();
  ExpectInboundEvent(event_data, &event_promise);
  WriteInboundEvent(event_data);
  CallDataReady();
  ExpectInboundIsoData(iso_data, &iso_promise);
  WriteInboundIsoData(iso_data);
  CallDataReady();

  WaitForTimeout(&acl_promise);
  WaitForTimeout(&sco_promise);
  WaitForTimeout(&event_promise);
  WaitForTimeout(&iso_promise);
}

TEST_F(H4ProtocolTest, TestMultiplePackets) {
  WriteAndExpectManyInboundAclDataPackets(sco_data);
}

TEST_F(H4ProtocolTest, TestMultipleWritesPacketsShortWrites) {
  WriteAndExpectManyAclDataPacketsDifferentOffsetsShort();
}

TEST_F(H4ProtocolTest, TestDisconnect) {
  EXPECT_CALL(disconnect_cb_, Call());
  close(chip_uart_fd_);
  CallDataReady();
}

TEST_F(H4ProtocolTest, TestPartialWrites) {
  size_t payload_len = strlen(acl_data);
  const size_t kNumIntervals = payload_len + 1;
  // h4 type[1] + handle[2] + size[2]
  header_[0] = static_cast<uint8_t>(PacketType::ACL_DATA);
  header_[1] = 19;
  header_[2] = 92;
  header_[3] = payload_len & 0xFF;
  header_[4] = (payload_len >> 8) & 0xFF;

  EXPECT_CALL(acl_cb_,
              Call(PacketMatches(header_ + 1, sizeof(header_) - 1, acl_data)))
      .Times(kNumIntervals);

  for (size_t interval = 1; interval < kNumIntervals + 1; interval++) {
    // Use the header_ data that expect already set up.
    if (interval < kAclHeaderSize) {
      TEMP_FAILURE_RETRY(write(chip_uart_fd_, header_, interval));
      CallDataReady();
      TEMP_FAILURE_RETRY(write(chip_uart_fd_, header_ + interval,
                               kAclHeaderSize + 1 - interval));
      CallDataReady();
    } else {
      TEMP_FAILURE_RETRY(write(chip_uart_fd_, header_, kAclHeaderSize + 1));
      CallDataReady();
    }

    for (size_t bytes = 0; bytes + interval <= payload_len; bytes += interval) {
      TEMP_FAILURE_RETRY(write(chip_uart_fd_, acl_data + bytes, interval));
      CallDataReady();
    }
    size_t extra_bytes = payload_len % interval;
    if (extra_bytes) {
      TEMP_FAILURE_RETRY(write(
          chip_uart_fd_, acl_data + payload_len - extra_bytes, extra_bytes));
      CallDataReady();
    }
  }
}

class H4ProtocolAsyncTest : public H4ProtocolTest {
 protected:
  void SetUp() override {
    H4ProtocolTest::SetUp();
    fd_watcher_.WatchFdForNonBlockingReads(
        stack_uart_fd_, [this](int) { h4_hci_->OnDataReady(); });
  }

  void TearDown() override { fd_watcher_.StopWatchingFileDescriptors(); }

  // Calling CallDataReady() has no effect in the AsyncTest
  void CallDataReady() override {}

  void SendAndReadUartOutbound(PacketType type, char* data) {
    ALOGD("%s sending", __func__);
    int data_length = strlen(data);
    h4_hci_->Send(type, (uint8_t*)data, data_length);

    int uart_length = data_length + 1;  // + 1 for data type code
    int i;

    ALOGD("%s reading", __func__);
    for (i = 0; i < uart_length; i++) {
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(chip_uart_fd_, &read_fds);
      TEMP_FAILURE_RETRY(
          select(chip_uart_fd_ + 1, &read_fds, nullptr, nullptr, nullptr));

      char byte;
      TEMP_FAILURE_RETRY(read(chip_uart_fd_, &byte, 1));

      EXPECT_EQ(i == 0 ? static_cast<uint8_t>(type) : data[i - 1], byte);
    }

    EXPECT_EQ(i, uart_length);
  }

  void WriteAndExpectInboundAclData(char* payload) {
    std::promise<void> promise;
    ExpectInboundAclData(payload, &promise);
    WriteInboundAclData(payload);
    WaitForTimeout(&promise);
  }

  void WriteAndExpectInboundScoData(char* payload) {
    std::promise<void> promise;
    ExpectInboundScoData(payload, &promise);
    WriteInboundScoData(payload);
    WaitForTimeout(&promise);
  }

  void WriteAndExpectInboundEvent(char* payload) {
    std::promise<void> promise;
    ExpectInboundEvent(payload, &promise);
    WriteInboundEvent(payload);
    WaitForTimeout(&promise);
  }

  void WriteAndExpectInboundIsoData(char* payload) {
    std::promise<void> promise;
    ExpectInboundIsoData(payload, &promise);
    WriteInboundIsoData(payload);
    WaitForTimeout(&promise);
  }

  void WriteAndExpectManyInboundAclDataPackets(char* payload) {
    const size_t kNumPackets = 20;
    // h4 type[1] + handle[2] + size[2]
    char preamble[5] = {static_cast<uint8_t>(PacketType::ACL_DATA), 19, 92, 0,
                        0};
    int length = strlen(payload);
    preamble[3] = length & 0xFF;
    preamble[4] = (length >> 8) & 0xFF;

    EXPECT_CALL(acl_cb_, Call(PacketMatches(preamble + 1, sizeof(preamble) - 1,
                                            payload)))
        .Times(kNumPackets);

    for (size_t i = 0; i < kNumPackets; i++) {
      TEMP_FAILURE_RETRY(write(chip_uart_fd_, preamble, sizeof(preamble)));
      TEMP_FAILURE_RETRY(write(chip_uart_fd_, payload, strlen(payload)));
    }

    WriteAndExpectInboundEvent(event_data);
  }

  AsyncFdWatcher fd_watcher_;
};

// Test sending data sends correct data onto the UART
TEST_F(H4ProtocolAsyncTest, TestSends) {
  SendAndReadUartOutbound(PacketType::COMMAND, sample_data1);
  SendAndReadUartOutbound(PacketType::ACL_DATA, sample_data2);
  SendAndReadUartOutbound(PacketType::SCO_DATA, sample_data3);
  SendAndReadUartOutbound(PacketType::ISO_DATA, sample_data4);
}

// Ensure we properly parse data coming from the UART
TEST_F(H4ProtocolAsyncTest, TestReads) {
  WriteAndExpectInboundAclData(acl_data);
  WriteAndExpectInboundScoData(sco_data);
  WriteAndExpectInboundEvent(event_data);
  WriteAndExpectInboundIsoData(iso_data);
}

TEST_F(H4ProtocolAsyncTest, TestMultiplePackets) {
  WriteAndExpectManyInboundAclDataPackets(sco_data);
}

TEST_F(H4ProtocolAsyncTest, TestMultipleWritesPacketsShortWrites) {
  WriteAndExpectManyAclDataPacketsDifferentOffsetsShort();
}

TEST_F(H4ProtocolAsyncTest, TestDisconnect) {
  std::promise<void> promise;
  EXPECT_CALL(disconnect_cb_, Call()).WillOnce(Notify(&promise));
  close(chip_uart_fd_);

  WaitForTimeout(&promise);
}
