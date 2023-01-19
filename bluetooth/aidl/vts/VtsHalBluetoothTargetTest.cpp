/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/BnBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHci.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/Status.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using aidl::android::hardware::bluetooth::IBluetoothHci;
using aidl::android::hardware::bluetooth::IBluetoothHciCallbacks;
using aidl::android::hardware::bluetooth::Status;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

// Bluetooth Core Specification 3.0 + HS
static constexpr uint8_t kHciMinimumHciVersion = 5;
// Bluetooth Core Specification 3.0 + HS
static constexpr uint8_t kHciMinimumLmpVersion = 5;

static constexpr std::chrono::milliseconds kWaitForInitTimeout(2000);
static constexpr std::chrono::milliseconds kWaitForHciEventTimeout(2000);
static constexpr std::chrono::milliseconds kInterfaceCloseDelayMs(200);

static constexpr uint8_t kCommandHciShouldBeUnknown[] = {
    0xff, 0x3B, 0x08, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
static constexpr uint8_t kCommandHciReadLocalVersionInformation[] = {0x01, 0x10,
                                                                     0x00};
static constexpr uint8_t kCommandHciReadBufferSize[] = {0x05, 0x10, 0x00};
static constexpr uint8_t kCommandHciReset[] = {0x03, 0x0c, 0x00};
static constexpr uint8_t kHciStatusSuccess = 0x00;
static constexpr uint8_t kHciStatusUnknownHciCommand = 0x01;

static constexpr uint8_t kEventCommandComplete = 0x0e;
static constexpr uint8_t kEventCommandStatus = 0x0f;
static constexpr uint8_t kEventNumberOfCompletedPackets = 0x13;

static constexpr size_t kEventCodeByte = 0;
static constexpr size_t kEventCommandStatusStatusByte = 2;
static constexpr size_t kEventCommandStatusOpcodeLsByte = 4;    // Bytes 4 and 5
static constexpr size_t kEventCommandCompleteOpcodeLsByte = 3;  // Bytes 3 and 4
static constexpr size_t kEventCommandCompleteStatusByte = 5;
static constexpr size_t kEventCommandCompleteFirstParamByte = 6;
static constexpr size_t kEventLocalHciVersionByte =
    kEventCommandCompleteFirstParamByte;
static constexpr size_t kEventLocalLmpVersionByte =
    kEventLocalHciVersionByte + 3;

static constexpr size_t kEventNumberOfCompletedPacketsNumHandles = 2;

// To discard Qualcomm ACL debugging
static constexpr uint16_t kAclHandleQcaDebugMessage = 0xedc;

class ThroughputLogger {
 public:
  ThroughputLogger(std::string task)
      : task_(task), start_time_(std::chrono::steady_clock::now()) {}

  ~ThroughputLogger() {
    if (total_bytes_ == 0) {
      return;
    }
    std::chrono::duration<double> duration =
        std::chrono::steady_clock::now() - start_time_;
    double s = duration.count();
    if (s == 0) {
      return;
    }
    double rate_kb = (static_cast<double>(total_bytes_) / s) / 1024;
    ALOGD("%s %.1f KB/s (%zu bytes in %.3fs)", task_.c_str(), rate_kb,
          total_bytes_, s);
  }

  void setTotalBytes(size_t total_bytes) { total_bytes_ = total_bytes; }

 private:
  size_t total_bytes_;
  std::string task_;
  std::chrono::steady_clock::time_point start_time_;
};

// The main test class for Bluetooth HAL.
class BluetoothAidlTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    // currently test passthrough mode only
    hci = IBluetoothHci::fromBinder(
        SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(hci, nullptr);
    ALOGI("%s: getService() for bluetooth hci is %s", __func__,
          hci->isRemote() ? "remote" : "local");

    // Lambda function
    auto on_binder_death = [](void* /*cookie*/) { FAIL(); };

    bluetooth_hci_death_recipient =
        AIBinder_DeathRecipient_new(on_binder_death);
    ASSERT_NE(bluetooth_hci_death_recipient, nullptr);
    ASSERT_EQ(STATUS_OK,
              AIBinder_linkToDeath(hci->asBinder().get(),
                                   bluetooth_hci_death_recipient, 0));

    hci_cb = ndk::SharedRefBase::make<BluetoothHciCallbacks>(*this);
    ASSERT_NE(hci_cb, nullptr);

    max_acl_data_packet_length = 0;
    max_sco_data_packet_length = 0;
    max_acl_data_packets = 0;
    max_sco_data_packets = 0;

    event_cb_count = 0;
    acl_cb_count = 0;
    sco_cb_count = 0;

    ASSERT_TRUE(hci->initialize(hci_cb).isOk());
    auto future = initialized_promise.get_future();
    auto timeout_status = future.wait_for(kWaitForInitTimeout);
    ASSERT_EQ(timeout_status, std::future_status::ready);
    ASSERT_TRUE(future.get());
  }

  virtual void TearDown() override {
    ALOGI("TearDown");
    // Should not be checked in production code
    ASSERT_TRUE(hci->close().isOk());
    std::this_thread::sleep_for(kInterfaceCloseDelayMs);
    handle_no_ops();
    EXPECT_EQ(static_cast<size_t>(0), event_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), sco_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), acl_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), iso_queue.size());
  }

  void setBufferSizes();

  // Helper functions to try to get a handle on verbosity
  void handle_no_ops();
  void wait_for_event(bool timeout_is_error);
  void wait_for_command_complete_event(std::vector<uint8_t> cmd);
  int wait_for_completed_packets_event(uint16_t handle);

  // A simple test implementation of BluetoothHciCallbacks.
  class BluetoothHciCallbacks
      : public aidl::android::hardware::bluetooth::BnBluetoothHciCallbacks {
    BluetoothAidlTest& parent_;

   public:
    BluetoothHciCallbacks(BluetoothAidlTest& parent) : parent_(parent){};

    virtual ~BluetoothHciCallbacks() = default;

    ndk::ScopedAStatus initializationComplete(Status status) {
      parent_.initialized_promise.set_value(status == Status::SUCCESS);
      ALOGV("%s (status = %d)", __func__, static_cast<int>(status));
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus hciEventReceived(const std::vector<uint8_t>& event) {
      parent_.event_cb_count++;
      parent_.event_queue.push(event);
      ALOGV("Event received (length = %d)", static_cast<int>(event.size()));
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus aclDataReceived(const std::vector<uint8_t>& data) {
      parent_.acl_cb_count++;
      parent_.acl_queue.push(data);
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus scoDataReceived(const std::vector<uint8_t>& data) {
      parent_.sco_cb_count++;
      parent_.sco_queue.push(data);
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus isoDataReceived(const std::vector<uint8_t>& data) {
      parent_.iso_cb_count++;
      parent_.iso_queue.push(data);
      return ScopedAStatus::ok();
    };
  };

  template <class T>
  class WaitQueue {
   public:
    WaitQueue(){};

    virtual ~WaitQueue() = default;

    bool empty() const {
      std::lock_guard<std::mutex> lock(m_);
      return q_.empty();
    };

    size_t size() const {
      std::lock_guard<std::mutex> lock(m_);
      return q_.size();
    };

    void push(const T& v) {
      std::lock_guard<std::mutex> lock(m_);
      q_.push(v);
      ready_.notify_one();
    };

    bool pop(T& v) {
      std::lock_guard<std::mutex> lock(m_);
      if (q_.empty()) {
        return false;
      }
      v = std::move(q_.front());
      q_.pop();
      return true;
    };

    bool front(T& v) {
      std::lock_guard<std::mutex> lock(m_);
      if (q_.empty()) {
        return false;
      }
      v = q_.front();
      return true;
    };

    void wait() {
      std::unique_lock<std::mutex> lock(m_);
      while (q_.empty()) {
        ready_.wait(lock);
      }
    };

    bool waitWithTimeout(std::chrono::milliseconds timeout) {
      std::unique_lock<std::mutex> lock(m_);
      while (q_.empty()) {
        if (ready_.wait_for(lock, timeout) == std::cv_status::timeout) {
          return false;
        }
      }
      return true;
    };

    bool tryPopWithTimeout(T& v, std::chrono::milliseconds timeout) {
      std::unique_lock<std::mutex> lock(m_);
      while (q_.empty()) {
        if (ready_.wait_for(lock, timeout) == std::cv_status::timeout) {
          return false;
        }
      }
      v = std::move(q_.front());
      q_.pop();
      return true;
    };

   private:
    mutable std::mutex m_;
    std::queue<T> q_;
    std::condition_variable_any ready_;
  };

  std::shared_ptr<IBluetoothHci> hci;
  std::shared_ptr<BluetoothHciCallbacks> hci_cb;
  AIBinder_DeathRecipient* bluetooth_hci_death_recipient;
  WaitQueue<std::vector<uint8_t>> event_queue;
  WaitQueue<std::vector<uint8_t>> acl_queue;
  WaitQueue<std::vector<uint8_t>> sco_queue;
  WaitQueue<std::vector<uint8_t>> iso_queue;

  std::promise<bool> initialized_promise;
  int event_cb_count;
  int sco_cb_count;
  int acl_cb_count;
  int iso_cb_count;

  int max_acl_data_packet_length;
  int max_sco_data_packet_length;
  int max_acl_data_packets;
  int max_sco_data_packets;
};

// Discard NO-OPs from the event queue.
void BluetoothAidlTest::handle_no_ops() {
  while (!event_queue.empty()) {
    std::vector<uint8_t> event;
    event_queue.front(event);
    ASSERT_GE(event.size(),
              static_cast<size_t>(kEventCommandCompleteStatusByte));
    bool event_is_no_op =
        (event[kEventCodeByte] == kEventCommandComplete) &&
        (event[kEventCommandCompleteOpcodeLsByte] == 0x00) &&
        (event[kEventCommandCompleteOpcodeLsByte + 1] == 0x00);
    event_is_no_op |= (event[kEventCodeByte] == kEventCommandStatus) &&
                      (event[kEventCommandStatusOpcodeLsByte] == 0x00) &&
                      (event[kEventCommandStatusOpcodeLsByte + 1] == 0x00);
    if (event_is_no_op) {
      event_queue.pop(event);
    } else {
      break;
    }
  }
  // Discard Qualcomm ACL debugging
  while (!acl_queue.empty()) {
    std::vector<uint8_t> acl_packet;
    acl_queue.front(acl_packet);
    uint16_t connection_handle = acl_packet[1] & 0xF;
    connection_handle <<= 8;
    connection_handle |= acl_packet[0];
    bool packet_is_no_op = connection_handle == kAclHandleQcaDebugMessage;
    if (packet_is_no_op) {
      acl_queue.pop(acl_packet);
    } else {
      break;
    }
  }
}

// Receive an event, discarding NO-OPs.
void BluetoothAidlTest::wait_for_event(bool timeout_is_error = true) {
  if (timeout_is_error) {
    ASSERT_TRUE(event_queue.waitWithTimeout(kWaitForHciEventTimeout));
  } else {
    event_queue.wait();
  }
  ASSERT_LT(static_cast<size_t>(0), event_queue.size());
  if (event_queue.empty()) {
    // waitWithTimeout timed out
    return;
  }
  handle_no_ops();
}

// Wait until a command complete is received.
void BluetoothAidlTest::wait_for_command_complete_event(
    std::vector<uint8_t> cmd) {
  wait_for_event();
  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));

  ASSERT_GT(event.size(), static_cast<size_t>(kEventCommandCompleteStatusByte));
  ASSERT_EQ(kEventCommandComplete, event[kEventCodeByte]);
  ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
  ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
  ASSERT_EQ(kHciStatusSuccess, event[kEventCommandCompleteStatusByte]);
}

// Send the command to read the controller's buffer sizes.
void BluetoothAidlTest::setBufferSizes() {
  std::vector<uint8_t> cmd{
      kCommandHciReadBufferSize,
      kCommandHciReadBufferSize + sizeof(kCommandHciReadBufferSize)};
  hci->sendHciCommand(cmd);

  wait_for_event();
  if (event_queue.empty()) {
    return;
  }
  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));

  ASSERT_EQ(kEventCommandComplete, event[kEventCodeByte]);
  ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
  ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
  ASSERT_EQ(kHciStatusSuccess, event[kEventCommandCompleteStatusByte]);

  max_acl_data_packet_length =
      event[kEventCommandCompleteStatusByte + 1] +
      (event[kEventCommandCompleteStatusByte + 2] << 8);
  max_sco_data_packet_length = event[kEventCommandCompleteStatusByte + 3];
  max_acl_data_packets = event[kEventCommandCompleteStatusByte + 4] +
                         (event[kEventCommandCompleteStatusByte + 5] << 8);
  max_sco_data_packets = event[kEventCommandCompleteStatusByte + 6] +
                         (event[kEventCommandCompleteStatusByte + 7] << 8);

  ALOGD("%s: ACL max %d num %d SCO max %d num %d", __func__,
        static_cast<int>(max_acl_data_packet_length),
        static_cast<int>(max_acl_data_packets),
        static_cast<int>(max_sco_data_packet_length),
        static_cast<int>(max_sco_data_packets));
}

// Return the number of completed packets reported by the controller.
int BluetoothAidlTest::wait_for_completed_packets_event(uint16_t handle) {
  int packets_processed = 0;
  wait_for_event(false);
  if (event_queue.empty()) {
    ALOGW("%s: waitForBluetoothCallback timed out.", __func__);
    return packets_processed;
  }
  while (!event_queue.empty()) {
    std::vector<uint8_t> event;
    EXPECT_TRUE(event_queue.pop(event));

    EXPECT_EQ(kEventNumberOfCompletedPackets, event[kEventCodeByte]);
    EXPECT_EQ(1, event[kEventNumberOfCompletedPacketsNumHandles]);

    uint16_t event_handle = event[3] + (event[4] << 8);
    EXPECT_EQ(handle, event_handle);

    packets_processed += event[5] + (event[6] << 8);
  }
  return packets_processed;
}

// Empty test: Initialize()/Close() are called in SetUp()/TearDown().
TEST_P(BluetoothAidlTest, InitializeAndClose) {}

// Send an HCI Reset with sendHciCommand and wait for a command complete event.
TEST_P(BluetoothAidlTest, HciReset) {
  std::vector<uint8_t> reset{kCommandHciReset,
                             kCommandHciReset + sizeof(kCommandHciReset)};
  hci->sendHciCommand(reset);

  wait_for_command_complete_event(reset);
}

// Read and check the HCI version of the controller.
TEST_P(BluetoothAidlTest, HciVersionTest) {
  std::vector<uint8_t> cmd{kCommandHciReadLocalVersionInformation,
                           kCommandHciReadLocalVersionInformation +
                               sizeof(kCommandHciReadLocalVersionInformation)};
  hci->sendHciCommand(cmd);

  wait_for_event();
  if (event_queue.empty()) {
    return;
  }

  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));
  ASSERT_GT(event.size(), static_cast<size_t>(kEventLocalLmpVersionByte));

  ASSERT_EQ(kEventCommandComplete, event[kEventCodeByte]);
  ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
  ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
  ASSERT_EQ(kHciStatusSuccess, event[kEventCommandCompleteStatusByte]);

  ASSERT_LE(kHciMinimumHciVersion, event[kEventLocalHciVersionByte]);
  ASSERT_LE(kHciMinimumLmpVersion, event[kEventLocalLmpVersionByte]);
}

// Send an unknown HCI command and wait for the error message.
TEST_P(BluetoothAidlTest, HciUnknownCommand) {
  std::vector<uint8_t> cmd{
      kCommandHciShouldBeUnknown,
      kCommandHciShouldBeUnknown + sizeof(kCommandHciShouldBeUnknown)};
  hci->sendHciCommand(cmd);

  wait_for_event();
  if (event_queue.empty()) {
    return;
  }

  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));

  ASSERT_GT(event.size(), static_cast<size_t>(kEventCommandCompleteStatusByte));
  if (event[kEventCodeByte] == kEventCommandComplete) {
    ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
    ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
    ASSERT_EQ(kHciStatusUnknownHciCommand,
              event[kEventCommandCompleteStatusByte]);
  } else {
    ASSERT_EQ(kEventCommandStatus, event[kEventCodeByte]);
    ASSERT_EQ(cmd[0], event[kEventCommandStatusOpcodeLsByte]);
    ASSERT_EQ(cmd[1], event[kEventCommandStatusOpcodeLsByte + 1]);
    ASSERT_EQ(kHciStatusUnknownHciCommand,
              event[kEventCommandStatusStatusByte]);
  }
}

// Set all bits in the event mask
TEST_P(BluetoothAidlTest, SetEventMask) {
  std::vector<uint8_t> set_event_mask{
      0x01, 0x0c, 0x08 /*parameter bytes*/, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff};
  hci->sendHciCommand({set_event_mask});
  wait_for_command_complete_event(set_event_mask);
}

// Set all bits in the LE event mask
TEST_P(BluetoothAidlTest, SetLeEventMask) {
  std::vector<uint8_t> set_event_mask{
      0x20, 0x0c, 0x08 /*parameter bytes*/, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff};
  hci->sendHciCommand({set_event_mask});
  wait_for_command_complete_event(set_event_mask);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BluetoothAidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothHci::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ABinderProcess_startThreadPool();
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
