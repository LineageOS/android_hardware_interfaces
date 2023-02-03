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

static constexpr size_t kNumHciCommandsBandwidth = 100;
static constexpr size_t kNumScoPacketsBandwidth = 100;
static constexpr size_t kNumAclPacketsBandwidth = 100;
static constexpr std::chrono::milliseconds kWaitForInitTimeout(2000);
static constexpr std::chrono::milliseconds kWaitForHciEventTimeout(2000);
static constexpr std::chrono::milliseconds kWaitForScoDataTimeout(1000);
static constexpr std::chrono::milliseconds kWaitForAclDataTimeout(1000);
static constexpr std::chrono::milliseconds kInterfaceCloseDelayMs(200);

static constexpr uint8_t kCommandHciShouldBeUnknown[] = {
    0xff, 0x3B, 0x08, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
static constexpr uint8_t kCommandHciReadLocalVersionInformation[] = {0x01, 0x10,
                                                                     0x00};
static constexpr uint8_t kCommandHciReadBufferSize[] = {0x05, 0x10, 0x00};
static constexpr uint8_t kCommandHciWriteLoopbackModeLocal[] = {0x02, 0x18,
                                                                0x01, 0x01};
static constexpr uint8_t kCommandHciReset[] = {0x03, 0x0c, 0x00};
static constexpr uint8_t kCommandHciSynchronousFlowControlEnable[] = {
    0x2f, 0x0c, 0x01, 0x01};
static constexpr uint8_t kCommandHciWriteLocalName[] = {0x13, 0x0c, 0xf8};
static constexpr uint8_t kHciStatusSuccess = 0x00;
static constexpr uint8_t kHciStatusUnknownHciCommand = 0x01;

static constexpr uint8_t kEventConnectionComplete = 0x03;
static constexpr uint8_t kEventCommandComplete = 0x0e;
static constexpr uint8_t kEventCommandStatus = 0x0f;
static constexpr uint8_t kEventNumberOfCompletedPackets = 0x13;
static constexpr uint8_t kEventLoopbackCommand = 0x19;

static constexpr size_t kEventCodeByte = 0;
static constexpr size_t kEventLengthByte = 1;
static constexpr size_t kEventFirstPayloadByte = 2;
static constexpr size_t kEventCommandStatusStatusByte = 2;
static constexpr size_t kEventCommandStatusOpcodeLsByte = 4;    // Bytes 4 and 5
static constexpr size_t kEventCommandCompleteOpcodeLsByte = 3;  // Bytes 3 and 4
static constexpr size_t kEventCommandCompleteStatusByte = 5;
static constexpr size_t kEventCommandCompleteFirstParamByte = 6;
static constexpr size_t kEventLocalHciVersionByte =
    kEventCommandCompleteFirstParamByte;
static constexpr size_t kEventLocalLmpVersionByte =
    kEventLocalHciVersionByte + 3;

static constexpr size_t kEventConnectionCompleteParamLength = 11;
static constexpr size_t kEventConnectionCompleteType = 11;
static constexpr size_t kEventConnectionCompleteTypeSco = 0;
static constexpr size_t kEventConnectionCompleteTypeAcl = 1;
static constexpr size_t kEventConnectionCompleteHandleLsByte = 3;

static constexpr size_t kEventNumberOfCompletedPacketsNumHandles = 2;

static constexpr size_t kAclBroadcastFlagOffset = 6;
static constexpr uint8_t kAclBroadcastFlagPointToPoint = 0x0;
static constexpr uint8_t kAclBroadcastPointToPoint =
    (kAclBroadcastFlagPointToPoint << kAclBroadcastFlagOffset);

static constexpr uint8_t kAclPacketBoundaryFlagOffset = 4;
static constexpr uint8_t kAclPacketBoundaryFlagFirstAutoFlushable = 0x2;
static constexpr uint8_t kAclPacketBoundaryFirstAutoFlushable =
    kAclPacketBoundaryFlagFirstAutoFlushable << kAclPacketBoundaryFlagOffset;

// To discard Qualcomm ACL debugging
static constexpr uint16_t kAclHandleQcaDebugMessage = 0xedc;

class ThroughputLogger {
 public:
  ThroughputLogger(std::string task)
      : total_bytes_(0),
        task_(task),
        start_time_(std::chrono::steady_clock::now()) {}

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
    discard_qca_debugging();
    EXPECT_EQ(static_cast<size_t>(0), event_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), sco_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), acl_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), iso_queue.size());
  }

  void setBufferSizes();
  void setSynchronousFlowControlEnable();

  // Functions called from within tests in loopback mode
  void sendAndCheckHci(int num_packets);
  void sendAndCheckSco(int num_packets, size_t size, uint16_t handle);
  void sendAndCheckAcl(int num_packets, size_t size, uint16_t handle);

  // Helper functions to try to get a handle on verbosity
  void enterLoopbackMode();
  void handle_no_ops();
  void discard_qca_debugging();
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

  std::vector<uint16_t> sco_connection_handles;
  std::vector<uint16_t> acl_connection_handles;
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
}

// Discard Qualcomm ACL debugging
void BluetoothAidlTest::discard_qca_debugging() {
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
  // Wait until we get something that's not a no-op.
  while (true) {
    bool event_ready = event_queue.waitWithTimeout(kWaitForHciEventTimeout);
    ASSERT_TRUE(event_ready || !timeout_is_error);
    if (event_queue.empty()) {
      // waitWithTimeout timed out
      return;
    }
    handle_no_ops();
    if (!event_queue.empty()) {
      // There's an event in the queue that's not a no-op.
      return;
    }
  }
}

// Wait until a command complete is received.
void BluetoothAidlTest::wait_for_command_complete_event(
    std::vector<uint8_t> cmd) {
  ASSERT_NO_FATAL_FAILURE(wait_for_event());
  std::vector<uint8_t> event;
  ASSERT_FALSE(event_queue.empty());
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

  ASSERT_NO_FATAL_FAILURE(wait_for_event());
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

// Enable flow control packets for SCO
void BluetoothAidlTest::setSynchronousFlowControlEnable() {
  std::vector<uint8_t> cmd{kCommandHciSynchronousFlowControlEnable,
                           kCommandHciSynchronousFlowControlEnable +
                               sizeof(kCommandHciSynchronousFlowControlEnable)};
  hci->sendHciCommand(cmd);

  wait_for_command_complete_event(cmd);
}

// Send an HCI command (in Loopback mode) and check the response.
void BluetoothAidlTest::sendAndCheckHci(int num_packets) {
  ThroughputLogger logger = {__func__};
  int command_size = 0;
  for (int n = 0; n < num_packets; n++) {
    // Send an HCI packet
    std::vector<uint8_t> write_name{
        kCommandHciWriteLocalName,
        kCommandHciWriteLocalName + sizeof(kCommandHciWriteLocalName)};
    // With a name
    char new_name[] = "John Jacob Jingleheimer Schmidt ___________________0";
    size_t new_name_length = strlen(new_name);
    for (size_t i = 0; i < new_name_length; i++) {
      write_name.push_back(static_cast<uint8_t>(new_name[i]));
    }
    // And the packet number
    size_t i = new_name_length - 1;
    for (int digits = n; digits > 0; digits = digits / 10, i--) {
      write_name[i] = static_cast<uint8_t>('0' + digits % 10);
    }
    // And padding
    for (size_t i = 0; i < 248 - new_name_length; i++) {
      write_name.push_back(static_cast<uint8_t>(0));
    }

    hci->sendHciCommand(write_name);

    // Check the loopback of the HCI packet
    ASSERT_NO_FATAL_FAILURE(wait_for_event());

    std::vector<uint8_t> event;
    ASSERT_TRUE(event_queue.pop(event));

    size_t compare_length = (write_name.size() > static_cast<size_t>(0xff)
                                 ? static_cast<size_t>(0xff)
                                 : write_name.size());
    ASSERT_GT(event.size(), compare_length + kEventFirstPayloadByte - 1);

    ASSERT_EQ(kEventLoopbackCommand, event[kEventCodeByte]);
    ASSERT_EQ(compare_length, event[kEventLengthByte]);

    // Don't compare past the end of the event.
    if (compare_length + kEventFirstPayloadByte > event.size()) {
      compare_length = event.size() - kEventFirstPayloadByte;
      ALOGE("Only comparing %d bytes", static_cast<int>(compare_length));
    }

    if (n == num_packets - 1) {
      command_size = write_name.size();
    }

    for (size_t i = 0; i < compare_length; i++) {
      ASSERT_EQ(write_name[i], event[kEventFirstPayloadByte + i]);
    }
  }
  logger.setTotalBytes(command_size * num_packets * 2);
}

// Send a SCO data packet (in Loopback mode) and check the response.
void BluetoothAidlTest::sendAndCheckSco(int num_packets, size_t size,
                                        uint16_t handle) {
  ThroughputLogger logger = {__func__};
  for (int n = 0; n < num_packets; n++) {
    // Send a SCO packet
    std::vector<uint8_t> sco_packet;
    sco_packet.push_back(static_cast<uint8_t>(handle & 0xff));
    sco_packet.push_back(static_cast<uint8_t>((handle & 0x0f00) >> 8));
    sco_packet.push_back(static_cast<uint8_t>(size & 0xff));
    for (size_t i = 0; i < size; i++) {
      sco_packet.push_back(static_cast<uint8_t>(i + n));
    }
    hci->sendScoData(sco_packet);

    // Check the loopback of the SCO packet
    std::vector<uint8_t> sco_loopback;
    ASSERT_TRUE(
        sco_queue.tryPopWithTimeout(sco_loopback, kWaitForScoDataTimeout));

    ASSERT_EQ(sco_packet.size(), sco_loopback.size());
    size_t successful_bytes = 0;

    for (size_t i = 0; i < sco_packet.size(); i++) {
      if (sco_packet[i] == sco_loopback[i]) {
        successful_bytes = i;
      } else {
        ALOGE("Miscompare at %d (expected %x, got %x)", static_cast<int>(i),
              sco_packet[i], sco_loopback[i]);
        ALOGE("At %d (expected %x, got %x)", static_cast<int>(i + 1),
              sco_packet[i + 1], sco_loopback[i + 1]);
        break;
      }
    }
    ASSERT_EQ(sco_packet.size(), successful_bytes + 1);
  }
  logger.setTotalBytes(num_packets * size * 2);
}

// Send an ACL data packet (in Loopback mode) and check the response.
void BluetoothAidlTest::sendAndCheckAcl(int num_packets, size_t size,
                                        uint16_t handle) {
  ThroughputLogger logger = {__func__};
  for (int n = 0; n < num_packets; n++) {
    // Send an ACL packet
    std::vector<uint8_t> acl_packet;
    acl_packet.push_back(static_cast<uint8_t>(handle & 0xff));
    acl_packet.push_back(static_cast<uint8_t>((handle & 0x0f00) >> 8) |
                         kAclBroadcastPointToPoint |
                         kAclPacketBoundaryFirstAutoFlushable);
    acl_packet.push_back(static_cast<uint8_t>(size & 0xff));
    acl_packet.push_back(static_cast<uint8_t>((size & 0xff00) >> 8));
    for (size_t i = 0; i < size; i++) {
      acl_packet.push_back(static_cast<uint8_t>(i + n));
    }
    hci->sendAclData(acl_packet);

    std::vector<uint8_t> acl_loopback;
    // Check the loopback of the ACL packet
    ASSERT_TRUE(
        acl_queue.tryPopWithTimeout(acl_loopback, kWaitForAclDataTimeout));

    ASSERT_EQ(acl_packet.size(), acl_loopback.size());
    size_t successful_bytes = 0;

    for (size_t i = 0; i < acl_packet.size(); i++) {
      if (acl_packet[i] == acl_loopback[i]) {
        successful_bytes = i;
      } else {
        ALOGE("Miscompare at %d (expected %x, got %x)", static_cast<int>(i),
              acl_packet[i], acl_loopback[i]);
        ALOGE("At %d (expected %x, got %x)", static_cast<int>(i + 1),
              acl_packet[i + 1], acl_loopback[i + 1]);
        break;
      }
    }
    ASSERT_EQ(acl_packet.size(), successful_bytes + 1);
  }
  logger.setTotalBytes(num_packets * size * 2);
}

// Return the number of completed packets reported by the controller.
int BluetoothAidlTest::wait_for_completed_packets_event(uint16_t handle) {
  int packets_processed = 0;
  while (true) {
    // There should be at least one event.
    wait_for_event(packets_processed == 0);
    if (event_queue.empty()) {
      if (packets_processed == 0) {
        ALOGW("%s: waitForBluetoothCallback timed out.", __func__);
      }
      return packets_processed;
    }
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

// Send local loopback command and initialize SCO and ACL handles.
void BluetoothAidlTest::enterLoopbackMode() {
  std::vector<uint8_t> cmd{kCommandHciWriteLoopbackModeLocal,
                           kCommandHciWriteLoopbackModeLocal +
                               sizeof(kCommandHciWriteLoopbackModeLocal)};
  hci->sendHciCommand(cmd);

  // Receive connection complete events with data channels
  int connection_event_count = 0;
  bool command_complete_received = false;
  while (true) {
    wait_for_event(false);
    if (event_queue.empty()) {
      // Fail if there was no event received or no connections completed.
      ASSERT_TRUE(command_complete_received);
      ASSERT_LT(0, connection_event_count);
      return;
    }
    std::vector<uint8_t> event;
    ASSERT_TRUE(event_queue.pop(event));
    ASSERT_GT(event.size(),
              static_cast<size_t>(kEventCommandCompleteStatusByte));
    if (event[kEventCodeByte] == kEventConnectionComplete) {
      ASSERT_GT(event.size(),
                static_cast<size_t>(kEventConnectionCompleteType));
      ASSERT_EQ(event[kEventLengthByte], kEventConnectionCompleteParamLength);
      uint8_t connection_type = event[kEventConnectionCompleteType];

      ASSERT_TRUE(connection_type == kEventConnectionCompleteTypeSco ||
                  connection_type == kEventConnectionCompleteTypeAcl);

      // Save handles
      uint16_t handle = event[kEventConnectionCompleteHandleLsByte] |
                        event[kEventConnectionCompleteHandleLsByte + 1] << 8;
      if (connection_type == kEventConnectionCompleteTypeSco) {
        sco_connection_handles.push_back(handle);
      } else {
        acl_connection_handles.push_back(handle);
      }

      ALOGD("Connect complete type = %d handle = %d",
            event[kEventConnectionCompleteType], handle);
      connection_event_count++;
    } else {
      ASSERT_EQ(kEventCommandComplete, event[kEventCodeByte]);
      ASSERT_EQ(cmd[0], event[kEventCommandCompleteOpcodeLsByte]);
      ASSERT_EQ(cmd[1], event[kEventCommandCompleteOpcodeLsByte + 1]);
      ASSERT_EQ(kHciStatusSuccess, event[kEventCommandCompleteStatusByte]);
      command_complete_received = true;
    }
  }
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

  ASSERT_NO_FATAL_FAILURE(wait_for_event());

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

  ASSERT_NO_FATAL_FAILURE(wait_for_event());

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

// Enter loopback mode, but don't send any packets.
TEST_P(BluetoothAidlTest, WriteLoopbackMode) { enterLoopbackMode(); }

// Enter loopback mode and send a single command.
TEST_P(BluetoothAidlTest, LoopbackModeSingleCommand) {
  setBufferSizes();

  enterLoopbackMode();

  sendAndCheckHci(1);
}

// Enter loopback mode and send a single SCO packet.
TEST_P(BluetoothAidlTest, LoopbackModeSingleSco) {
  setBufferSizes();
  setSynchronousFlowControlEnable();

  enterLoopbackMode();

  if (!sco_connection_handles.empty()) {
    ASSERT_LT(0, max_sco_data_packet_length);
    sendAndCheckSco(1, max_sco_data_packet_length, sco_connection_handles[0]);
    int sco_packets_sent = 1;
    int completed_packets =
        wait_for_completed_packets_event(sco_connection_handles[0]);
    if (sco_packets_sent != completed_packets) {
      ALOGW("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            sco_packets_sent, completed_packets);
    }
  }
}

// Enter loopback mode and send a single ACL packet.
TEST_P(BluetoothAidlTest, LoopbackModeSingleAcl) {
  setBufferSizes();

  enterLoopbackMode();

  if (!acl_connection_handles.empty()) {
    ASSERT_LT(0, max_acl_data_packet_length);
    sendAndCheckAcl(1, max_acl_data_packet_length - 1,
                    acl_connection_handles[0]);
    int acl_packets_sent = 1;
    int completed_packets =
        wait_for_completed_packets_event(acl_connection_handles[0]);
    if (acl_packets_sent != completed_packets) {
      ALOGW("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            acl_packets_sent, completed_packets);
    }
  }
  ASSERT_GE(acl_cb_count, 1);
}

// Enter loopback mode and send command packets for bandwidth measurements.
TEST_P(BluetoothAidlTest, LoopbackModeCommandBandwidth) {
  setBufferSizes();

  enterLoopbackMode();

  sendAndCheckHci(kNumHciCommandsBandwidth);
}

// Enter loopback mode and send SCO packets for bandwidth measurements.
TEST_P(BluetoothAidlTest, LoopbackModeScoBandwidth) {
  setBufferSizes();
  setSynchronousFlowControlEnable();

  enterLoopbackMode();

  if (!sco_connection_handles.empty()) {
    ASSERT_LT(0, max_sco_data_packet_length);
    sendAndCheckSco(kNumScoPacketsBandwidth, max_sco_data_packet_length,
                    sco_connection_handles[0]);
    int sco_packets_sent = kNumScoPacketsBandwidth;
    int completed_packets =
        wait_for_completed_packets_event(sco_connection_handles[0]);
    if (sco_packets_sent != completed_packets) {
      ALOGW("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            sco_packets_sent, completed_packets);
    }
  }
}

// Enter loopback mode and send packets for ACL bandwidth measurements.
TEST_P(BluetoothAidlTest, LoopbackModeAclBandwidth) {
  setBufferSizes();

  enterLoopbackMode();

  if (!acl_connection_handles.empty()) {
    ASSERT_LT(0, max_acl_data_packet_length);
    sendAndCheckAcl(kNumAclPacketsBandwidth, max_acl_data_packet_length - 1,
                    acl_connection_handles[0]);
    int acl_packets_sent = kNumAclPacketsBandwidth;
    int completed_packets =
        wait_for_completed_packets_event(acl_connection_handles[0]);
    if (acl_packets_sent != completed_packets) {
      ALOGW("%s: packets_sent (%d) != completed_packets (%d)", __func__,
            acl_packets_sent, completed_packets);
    }
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

// Call initialize twice, second one should fail.
TEST_P(BluetoothAidlTest, CallInitializeTwice) {
  class SecondCb
      : public aidl::android::hardware::bluetooth::BnBluetoothHciCallbacks {
   public:
    ndk::ScopedAStatus initializationComplete(Status status) {
      EXPECT_EQ(status, Status::ALREADY_INITIALIZED);
      init_promise.set_value();
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus hciEventReceived(const std::vector<uint8_t>& /*event*/) {
      ADD_FAILURE();
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus aclDataReceived(const std::vector<uint8_t>& /*data*/) {
      ADD_FAILURE();
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus scoDataReceived(const std::vector<uint8_t>& /*data*/) {
      ADD_FAILURE();
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus isoDataReceived(const std::vector<uint8_t>& /*data*/) {
      ADD_FAILURE();
      return ScopedAStatus::ok();
    };
    std::promise<void> init_promise;
  };

  std::shared_ptr<SecondCb> second_cb = ndk::SharedRefBase::make<SecondCb>();
  ASSERT_NE(second_cb, nullptr);

  auto future = second_cb->init_promise.get_future();
  ASSERT_TRUE(hci->initialize(second_cb).isOk());
  auto status = future.wait_for(std::chrono::seconds(1));
  ASSERT_EQ(status, std::future_status::ready);
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
