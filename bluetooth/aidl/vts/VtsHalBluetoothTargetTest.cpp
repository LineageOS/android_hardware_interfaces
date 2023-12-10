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

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/BnBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHci.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/Status.h>
#include <android-base/properties.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

// TODO: Remove custom logging defines from PDL packets.
#undef LOG_INFO
#undef LOG_DEBUG
#undef LOG_TAG
#define LOG_TAG "VtsHalBluetooth"
#include "hci/hci_packets.h"
#include "packet/raw_builder.h"

using aidl::android::hardware::bluetooth::IBluetoothHci;
using aidl::android::hardware::bluetooth::IBluetoothHciCallbacks;
using aidl::android::hardware::bluetooth::Status;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

using ::bluetooth::hci::CommandBuilder;
using ::bluetooth::hci::CommandCompleteView;
using ::bluetooth::hci::CommandView;
using ::bluetooth::hci::ErrorCode;
using ::bluetooth::hci::EventView;
using ::bluetooth::hci::LeReadLocalSupportedFeaturesBuilder;
using ::bluetooth::hci::LeReadLocalSupportedFeaturesCompleteView;
using ::bluetooth::hci::LeReadNumberOfSupportedAdvertisingSetsBuilder;
using ::bluetooth::hci::LeReadNumberOfSupportedAdvertisingSetsCompleteView;
using ::bluetooth::hci::LeReadResolvingListSizeBuilder;
using ::bluetooth::hci::LeReadResolvingListSizeCompleteView;
using ::bluetooth::hci::LLFeaturesBits;
using ::bluetooth::hci::OpCode;
using ::bluetooth::hci::OpCodeText;
using ::bluetooth::hci::PacketView;
using ::bluetooth::hci::ReadLocalVersionInformationBuilder;
using ::bluetooth::hci::ReadLocalVersionInformationCompleteView;

static constexpr uint8_t kMinLeAdvSetForBt5 = 16;
static constexpr uint8_t kMinLeAdvSetForBt5FoTv = 10;
static constexpr uint8_t kMinLeResolvingListForBt5 = 8;

static constexpr size_t kNumHciCommandsBandwidth = 100;
static constexpr size_t kNumScoPacketsBandwidth = 100;
static constexpr size_t kNumAclPacketsBandwidth = 100;
static constexpr std::chrono::milliseconds kWaitForInitTimeout(2000);
static constexpr std::chrono::milliseconds kWaitForHciEventTimeout(2000);
static constexpr std::chrono::milliseconds kWaitForScoDataTimeout(1000);
static constexpr std::chrono::milliseconds kWaitForAclDataTimeout(1000);
static constexpr std::chrono::milliseconds kInterfaceCloseDelayMs(200);

// To discard Qualcomm ACL debugging
static constexpr uint16_t kAclHandleQcaDebugMessage = 0xedc;

static int get_vsr_api_level() {
  int vendor_api_level =
      ::android::base::GetIntProperty("ro.vendor.api_level", -1);
  if (vendor_api_level != -1) {
    return vendor_api_level;
  }

  // Android S and older devices do not define ro.vendor.api_level
  vendor_api_level = ::android::base::GetIntProperty("ro.board.api_level", -1);
  if (vendor_api_level == -1) {
    vendor_api_level =
        ::android::base::GetIntProperty("ro.board.first_api_level", -1);
  }

  int product_api_level =
      ::android::base::GetIntProperty("ro.product.first_api_level", -1);
  if (product_api_level == -1) {
    product_api_level =
        ::android::base::GetIntProperty("ro.build.version.sdk", -1);
    EXPECT_NE(product_api_level, -1) << "Could not find ro.build.version.sdk";
  }

  // VSR API level is the minimum of vendor_api_level and product_api_level.
  if (vendor_api_level == -1 || vendor_api_level > product_api_level) {
    return product_api_level;
  }
  return vendor_api_level;
}

static bool isTv() {
  return testing::deviceSupportsFeature("android.software.leanback") ||
         testing::deviceSupportsFeature("android.hardware.type.television");
}

class ThroughputLogger {
 public:
  explicit ThroughputLogger(std::string task)
      : total_bytes_(0),
        task_(std::move(task)),
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
  void SetUp() override {
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
                                   bluetooth_hci_death_recipient, nullptr));

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

  void TearDown() override {
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
  void wait_for_command_complete_event(OpCode opCode,
                                       std::vector<uint8_t>& complete_event);
  // Wait until a command complete is received.
  // Command complete will be consumed after this method
  void wait_and_validate_command_complete_event(OpCode opCode);
  int wait_for_completed_packets_event(uint16_t handle);
  void send_and_wait_for_cmd_complete(std::unique_ptr<CommandBuilder> cmd,
                                      std::vector<uint8_t>& cmd_complete);
  void reassemble_sco_loopback_pkt(std::vector<uint8_t>& scoPackets,
    size_t size);

  // A simple test implementation of BluetoothHciCallbacks.
  class BluetoothHciCallbacks
      : public aidl::android::hardware::bluetooth::BnBluetoothHciCallbacks {
    BluetoothAidlTest& parent_;

   public:
    explicit BluetoothHciCallbacks(BluetoothAidlTest& parent)
        : parent_(parent){};

    ~BluetoothHciCallbacks() override = default;

    ndk::ScopedAStatus initializationComplete(Status status) override {
      parent_.initialized_promise.set_value(status == Status::SUCCESS);
      ALOGV("%s (status = %d)", __func__, static_cast<int>(status));
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus hciEventReceived(
        const std::vector<uint8_t>& event) override {
      parent_.event_cb_count++;
      parent_.event_queue.push(event);
      ALOGI("Event received (length = %d)", static_cast<int>(event.size()));
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus aclDataReceived(
        const std::vector<uint8_t>& data) override {
      parent_.acl_cb_count++;
      parent_.acl_queue.push(data);
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus scoDataReceived(
        const std::vector<uint8_t>& data) override {
      parent_.sco_cb_count++;
      parent_.sco_queue.push(data);
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus isoDataReceived(
        const std::vector<uint8_t>& data) override {
      parent_.iso_cb_count++;
      parent_.iso_queue.push(data);
      return ScopedAStatus::ok();
    };
  };

  template <class T>
  class WaitQueue {
   public:
    WaitQueue() = default;
    ;

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

    void pop() {
      std::lock_guard<std::mutex> lock(m_);
      if (q_.empty()) {
        return;
      }
      q_.pop();
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
  AIBinder_DeathRecipient* bluetooth_hci_death_recipient{};
  WaitQueue<std::vector<uint8_t>> event_queue;
  WaitQueue<std::vector<uint8_t>> acl_queue;
  WaitQueue<std::vector<uint8_t>> sco_queue;
  WaitQueue<std::vector<uint8_t>> iso_queue;

  std::promise<bool> initialized_promise;
  int event_cb_count{};
  int sco_cb_count{};
  int acl_cb_count{};
  int iso_cb_count{};

  int max_acl_data_packet_length{};
  int max_sco_data_packet_length{};
  int max_acl_data_packets{};
  int max_sco_data_packets{};

  std::vector<uint16_t> sco_connection_handles;
  std::vector<uint16_t> acl_connection_handles;
};

// Discard NO-OPs from the event queue.
void BluetoothAidlTest::handle_no_ops() {
  while (!event_queue.empty()) {
    std::vector<uint8_t> event;
    event_queue.front(event);
    auto complete_view = ::bluetooth::hci::CommandCompleteView::Create(
        ::bluetooth::hci::EventView::Create(::bluetooth::hci::PacketView<true>(
            std::make_shared<std::vector<uint8_t>>(event))));
    auto status_view = ::bluetooth::hci::CommandCompleteView::Create(
        ::bluetooth::hci::EventView::Create(::bluetooth::hci::PacketView<true>(
            std::make_shared<std::vector<uint8_t>>(event))));
    bool is_complete_no_op =
        complete_view.IsValid() &&
        complete_view.GetCommandOpCode() == ::bluetooth::hci::OpCode::NONE;
    bool is_status_no_op =
        status_view.IsValid() &&
        status_view.GetCommandOpCode() == ::bluetooth::hci::OpCode::NONE;
    if (is_complete_no_op || is_status_no_op) {
      event_queue.pop();
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
    auto acl_view =
        ::bluetooth::hci::AclView::Create(::bluetooth::hci::PacketView<true>(
            std::make_shared<std::vector<uint8_t>>(acl_packet)));
    EXPECT_TRUE(acl_view.IsValid());
    if (acl_view.GetHandle() == kAclHandleQcaDebugMessage) {
      acl_queue.pop();
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

void BluetoothAidlTest::wait_for_command_complete_event(
    OpCode opCode, std::vector<uint8_t>& complete_event) {
  ASSERT_NO_FATAL_FAILURE(wait_for_event());
  ASSERT_FALSE(event_queue.empty());
  ASSERT_TRUE(event_queue.pop(complete_event));
  auto complete_view = ::bluetooth::hci::CommandCompleteView::Create(
      ::bluetooth::hci::EventView::Create(::bluetooth::hci::PacketView<true>(
          std::make_shared<std::vector<uint8_t>>(complete_event))));
  ASSERT_TRUE(complete_view.IsValid());
  ASSERT_EQ(complete_view.GetCommandOpCode(), opCode);
  ASSERT_EQ(complete_view.GetPayload()[0],
            static_cast<uint8_t>(::bluetooth::hci::ErrorCode::SUCCESS));
}

void BluetoothAidlTest::wait_and_validate_command_complete_event(
    ::bluetooth::hci::OpCode opCode) {
  std::vector<uint8_t> complete_event;
  ASSERT_NO_FATAL_FAILURE(
      wait_for_command_complete_event(opCode, complete_event));
}

// Send the command to read the controller's buffer sizes.
void BluetoothAidlTest::setBufferSizes() {
  std::vector<uint8_t> cmd;
  ::bluetooth::packet::BitInserter bi{cmd};
  ::bluetooth::hci::ReadBufferSizeBuilder::Create()->Serialize(bi);
  hci->sendHciCommand(cmd);

  ASSERT_NO_FATAL_FAILURE(wait_for_event());
  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));
  auto complete_view = ::bluetooth::hci::ReadBufferSizeCompleteView::Create(
      ::bluetooth::hci::CommandCompleteView::Create(
          ::bluetooth::hci::EventView::Create(
              ::bluetooth::hci::PacketView<true>(
                  std::make_shared<std::vector<uint8_t>>(event)))));

  ASSERT_TRUE(complete_view.IsValid());
  ASSERT_EQ(complete_view.GetStatus(), ::bluetooth::hci::ErrorCode::SUCCESS);
  max_acl_data_packet_length = complete_view.GetAclDataPacketLength();
  max_sco_data_packet_length = complete_view.GetSynchronousDataPacketLength();
  max_acl_data_packets = complete_view.GetTotalNumAclDataPackets();
  max_sco_data_packets = complete_view.GetTotalNumSynchronousDataPackets();

  ALOGD("%s: ACL max %d num %d SCO max %d num %d", __func__,
        static_cast<int>(max_acl_data_packet_length),
        static_cast<int>(max_acl_data_packets),
        static_cast<int>(max_sco_data_packet_length),
        static_cast<int>(max_sco_data_packets));
}

// Enable flow control packets for SCO
void BluetoothAidlTest::setSynchronousFlowControlEnable() {
  std::vector<uint8_t> cmd;
  ::bluetooth::packet::BitInserter bi{cmd};
  ::bluetooth::hci::WriteSynchronousFlowControlEnableBuilder::Create(
      ::bluetooth::hci::Enable::ENABLED)
      ->Serialize(bi);
  hci->sendHciCommand(cmd);

  wait_and_validate_command_complete_event(
      ::bluetooth::hci::OpCode::WRITE_SYNCHRONOUS_FLOW_CONTROL_ENABLE);
}

// Send an HCI command (in Loopback mode) and check the response.
void BluetoothAidlTest::sendAndCheckHci(int num_packets) {
  ThroughputLogger logger{__func__};
  size_t command_size = 0;
  char new_name[] = "John Jacob Jingleheimer Schmidt ___________________";
  size_t new_name_length = strlen(new_name);
  for (int n = 0; n < num_packets; n++) {
    // The name to set is new_name
    std::array<uint8_t, 248> name_array{};
    for (size_t i = 0; i < new_name_length; i++) {
      name_array[i] = new_name[i];
    }
    // And the packet number
    char number[11] = "0000000000";
    snprintf(number, sizeof(number), "%010d", static_cast<int>(n));
    for (size_t i = new_name_length; i < new_name_length + sizeof(number) - 1;
         i++) {
      name_array[new_name_length + i] = number[i];
    }
    std::vector<uint8_t> write_name;
    ::bluetooth::packet::BitInserter bi{write_name};
    ::bluetooth::hci::WriteLocalNameBuilder::Create(name_array)->Serialize(bi);
    hci->sendHciCommand(write_name);

    // Check the loopback of the HCI packet
    ASSERT_NO_FATAL_FAILURE(wait_for_event());

    std::vector<uint8_t> event;
    ASSERT_TRUE(event_queue.pop(event));
    auto event_view = ::bluetooth::hci::LoopbackCommandView::Create(
        ::bluetooth::hci::EventView::Create(::bluetooth::hci::PacketView<true>(
            std::make_shared<std::vector<uint8_t>>(event))));
    ASSERT_TRUE(event_view.IsValid());
    std::vector<uint8_t> looped_back_command{event_view.GetPayload().begin(),
                                             event_view.GetPayload().end()};
    ASSERT_EQ(looped_back_command, write_name);

    if (n == num_packets - 1) {
      command_size = write_name.size();
    }
  }
  logger.setTotalBytes(command_size * num_packets * 2);
}

// Send a SCO data packet (in Loopback mode) and check the response.
void BluetoothAidlTest::sendAndCheckSco(int num_packets, size_t size,
                                        uint16_t handle) {
  ThroughputLogger logger{__func__};
  for (int n = 0; n < num_packets; n++) {
    // Send a SCO packet
    std::vector<uint8_t> sco_packet;
    std::vector<uint8_t> payload;
    for (size_t i = 0; i < size; i++) {
      payload.push_back(static_cast<uint8_t>(i + n));
    }
    ::bluetooth::packet::BitInserter bi{sco_packet};
    ::bluetooth::hci::ScoBuilder::Create(
        handle, ::bluetooth::hci::PacketStatusFlag::CORRECTLY_RECEIVED, payload)
        ->Serialize(bi);
    hci->sendScoData(sco_packet);

    // Check the loopback of the SCO packet
    std::vector<uint8_t> sco_loopback;
    ASSERT_TRUE(
        sco_queue.tryPopWithTimeout(sco_loopback, kWaitForScoDataTimeout));

    if (sco_loopback.size() < size) {
      // The packets may have been split for USB. Reassemble before checking.
      reassemble_sco_loopback_pkt(sco_loopback, size);
    }

    ASSERT_EQ(sco_packet, sco_loopback);
  }
  logger.setTotalBytes(num_packets * size * 2);
}

// Send an ACL data packet (in Loopback mode) and check the response.
void BluetoothAidlTest::sendAndCheckAcl(int num_packets, size_t size,
                                        uint16_t handle) {
  ThroughputLogger logger{__func__};
  for (int n = 0; n < num_packets; n++) {
    // Send an ACL packet with counting data
    auto payload = std::make_unique<::bluetooth::packet::RawBuilder>();
    for (size_t i = 0; i < size; i++) {
      payload->AddOctets1(static_cast<uint8_t>(i + n));
    }
    std::vector<uint8_t> acl_packet;
    ::bluetooth::packet::BitInserter bi{acl_packet};
    ::bluetooth::hci::AclBuilder::Create(
        handle,
        ::bluetooth::hci::PacketBoundaryFlag::FIRST_AUTOMATICALLY_FLUSHABLE,
        ::bluetooth::hci::BroadcastFlag::POINT_TO_POINT, std::move(payload))
        ->Serialize(bi);
    hci->sendAclData(acl_packet);

    std::vector<uint8_t> acl_loopback;
    // Check the loopback of the ACL packet
    ASSERT_TRUE(
        acl_queue.tryPopWithTimeout(acl_loopback, kWaitForAclDataTimeout));

    ASSERT_EQ(acl_packet, acl_loopback);
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
    auto event_view = ::bluetooth::hci::NumberOfCompletedPacketsView::Create(
        ::bluetooth::hci::EventView::Create(::bluetooth::hci::PacketView<true>(
            std::make_shared<std::vector<uint8_t>>(event))));
    if (!event_view.IsValid()) {
      ADD_FAILURE();
      return packets_processed;
    }
    auto completed_packets = event_view.GetCompletedPackets();
    for (const auto& entry : completed_packets) {
      EXPECT_EQ(handle, entry.connection_handle_);
      packets_processed += entry.host_num_of_completed_packets_;
    }
  }
  return packets_processed;
}

// Send local loopback command and initialize SCO and ACL handles.
void BluetoothAidlTest::enterLoopbackMode() {
  std::vector<uint8_t> cmd;
  ::bluetooth::packet::BitInserter bi{cmd};
  ::bluetooth::hci::WriteLoopbackModeBuilder::Create(
      bluetooth::hci::LoopbackMode::ENABLE_LOCAL)
      ->Serialize(bi);
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
    auto event_view =
        ::bluetooth::hci::EventView::Create(::bluetooth::hci::PacketView<true>(
            std::make_shared<std::vector<uint8_t>>(event)));
    ASSERT_TRUE(event_view.IsValid());

    if (event_view.GetEventCode() ==
        ::bluetooth::hci::EventCode::CONNECTION_COMPLETE) {
      auto complete_view =
          ::bluetooth::hci::ConnectionCompleteView::Create(event_view);
      ASSERT_TRUE(complete_view.IsValid());
      switch (complete_view.GetLinkType()) {
        case ::bluetooth::hci::LinkType::ACL:
          acl_connection_handles.push_back(complete_view.GetConnectionHandle());
          break;
        case ::bluetooth::hci::LinkType::SCO:
          sco_connection_handles.push_back(complete_view.GetConnectionHandle());
          break;
        default:
          ASSERT_EQ(complete_view.GetLinkType(),
                    ::bluetooth::hci::LinkType::ACL);
      }
      connection_event_count++;
    } else {
      auto command_complete_view =
          ::bluetooth::hci::WriteLoopbackModeCompleteView::Create(
              ::bluetooth::hci::CommandCompleteView::Create(event_view));
      ASSERT_TRUE(command_complete_view.IsValid());
      ASSERT_EQ(::bluetooth::hci::ErrorCode::SUCCESS,
                command_complete_view.GetStatus());
      command_complete_received = true;
    }
  }
}

void BluetoothAidlTest::send_and_wait_for_cmd_complete(
    std::unique_ptr<CommandBuilder> cmd, std::vector<uint8_t>& cmd_complete) {
  std::vector<uint8_t> cmd_bytes = cmd->SerializeToBytes();
  hci->sendHciCommand(cmd_bytes);

  auto view = CommandView::Create(
      PacketView<true>(std::make_shared<std::vector<uint8_t>>(cmd_bytes)));
  ASSERT_TRUE(view.IsValid());
  ALOGI("Waiting for %s[0x%x]", OpCodeText(view.GetOpCode()).c_str(),
        static_cast<int>(view.GetOpCode()));
  ASSERT_NO_FATAL_FAILURE(
      wait_for_command_complete_event(view.GetOpCode(), cmd_complete));
}

// Handle the loopback packet.
void BluetoothAidlTest::reassemble_sco_loopback_pkt(std::vector<uint8_t>& scoPackets,
        size_t size) {
    std::vector<uint8_t> sco_packet_whole;
    sco_packet_whole.assign(scoPackets.begin(), scoPackets.end());
    while (size + 3 > sco_packet_whole.size()) {
      std::vector<uint8_t> sco_packets;
      ASSERT_TRUE(
      sco_queue.tryPopWithTimeout(sco_packets, kWaitForScoDataTimeout));
      sco_packet_whole.insert(sco_packet_whole.end(), sco_packets.begin() + 3,
          sco_packets.end());
    }
    scoPackets.assign(sco_packet_whole.begin(), sco_packet_whole.end());
    scoPackets[2] = size;
}

// Empty test: Initialize()/Close() are called in SetUp()/TearDown().
TEST_P(BluetoothAidlTest, InitializeAndClose) {}

// Send an HCI Reset with sendHciCommand and wait for a command complete event.
TEST_P(BluetoothAidlTest, HciReset) {
  std::vector<uint8_t> reset;
  ::bluetooth::packet::BitInserter bi{reset};
  ::bluetooth::hci::ResetBuilder::Create()->Serialize(bi);
  hci->sendHciCommand(reset);

  wait_and_validate_command_complete_event(::bluetooth::hci::OpCode::RESET);
}

// Read and check the HCI version of the controller.
TEST_P(BluetoothAidlTest, HciVersionTest) {
  std::vector<uint8_t> cmd;
  ::bluetooth::packet::BitInserter bi{cmd};
  ::bluetooth::hci::ReadLocalVersionInformationBuilder::Create()->Serialize(bi);
  hci->sendHciCommand(cmd);

  ASSERT_NO_FATAL_FAILURE(wait_for_event());

  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));
  auto complete_view =
      ::bluetooth::hci::ReadLocalVersionInformationCompleteView::Create(
          ::bluetooth::hci::CommandCompleteView::Create(
              ::bluetooth::hci::EventView::Create(
                  ::bluetooth::hci::PacketView<true>(
                      std::make_shared<std::vector<uint8_t>>(event)))));
  ASSERT_TRUE(complete_view.IsValid());
  ASSERT_EQ(::bluetooth::hci::ErrorCode::SUCCESS, complete_view.GetStatus());
  auto version = complete_view.GetLocalVersionInformation();
  ASSERT_LE(::bluetooth::hci::HciVersion::V_3_0, version.hci_version_);
  ASSERT_LE(::bluetooth::hci::LmpVersion::V_3_0, version.lmp_version_);
}

// Send an unknown HCI command and wait for the error message.
TEST_P(BluetoothAidlTest, HciUnknownCommand) {
  std::vector<uint8_t> cmd;
  ::bluetooth::packet::BitInserter bi{cmd};
  ::bluetooth::hci::CommandBuilder::Create(
      static_cast<::bluetooth::hci::OpCode>(0x3cff),
      std::make_unique<::bluetooth::packet::RawBuilder>())
      ->Serialize(bi);
  hci->sendHciCommand(cmd);

  ASSERT_NO_FATAL_FAILURE(wait_for_event());

  std::vector<uint8_t> event;
  ASSERT_TRUE(event_queue.pop(event));
  auto event_view =
      ::bluetooth::hci::EventView::Create(::bluetooth::hci::PacketView<true>(
          std::make_shared<std::vector<uint8_t>>(event)));
  ASSERT_TRUE(event_view.IsValid());

  switch (event_view.GetEventCode()) {
    case ::bluetooth::hci::EventCode::COMMAND_COMPLETE: {
      auto command_complete =
          ::bluetooth::hci::CommandCompleteView::Create(event_view);
      ASSERT_TRUE(command_complete.IsValid());
      ASSERT_EQ(command_complete.GetPayload()[0],
                static_cast<uint8_t>(
                    ::bluetooth::hci::ErrorCode::UNKNOWN_HCI_COMMAND));
    } break;
    case ::bluetooth::hci::EventCode::COMMAND_STATUS: {
      auto command_status =
          ::bluetooth::hci::CommandStatusView::Create(event_view);
      ASSERT_TRUE(command_status.IsValid());
      ASSERT_EQ(command_status.GetStatus(),
                ::bluetooth::hci::ErrorCode::UNKNOWN_HCI_COMMAND);
    } break;
    default:
      ADD_FAILURE();
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
  std::vector<uint8_t> cmd;
  ::bluetooth::packet::BitInserter bi{cmd};
  uint64_t full_mask = UINT64_MAX;
  ::bluetooth::hci::SetEventMaskBuilder::Create(full_mask)->Serialize(bi);
  hci->sendHciCommand(cmd);
  wait_and_validate_command_complete_event(
      ::bluetooth::hci::OpCode::SET_EVENT_MASK);
}

// Set all bits in the LE event mask
TEST_P(BluetoothAidlTest, SetLeEventMask) {
  std::vector<uint8_t> cmd;
  ::bluetooth::packet::BitInserter bi{cmd};
  uint64_t full_mask = UINT64_MAX;
  ::bluetooth::hci::LeSetEventMaskBuilder::Create(full_mask)->Serialize(bi);
  hci->sendHciCommand(cmd);
  wait_and_validate_command_complete_event(
      ::bluetooth::hci::OpCode::LE_SET_EVENT_MASK);
}

// Call initialize twice, second one should fail.
TEST_P(BluetoothAidlTest, CallInitializeTwice) {
  class SecondCb
      : public aidl::android::hardware::bluetooth::BnBluetoothHciCallbacks {
   public:
    ndk::ScopedAStatus initializationComplete(Status status) override {
      EXPECT_EQ(status, Status::ALREADY_INITIALIZED);
      init_promise.set_value();
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus hciEventReceived(
        const std::vector<uint8_t>& /*event*/) override {
      ADD_FAILURE();
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus aclDataReceived(
        const std::vector<uint8_t>& /*data*/) override {
      ADD_FAILURE();
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus scoDataReceived(
        const std::vector<uint8_t>& /*data*/) override {
      ADD_FAILURE();
      return ScopedAStatus::ok();
    };

    ndk::ScopedAStatus isoDataReceived(
        const std::vector<uint8_t>& /*data*/) override {
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

TEST_P(BluetoothAidlTest, Vsr_Bluetooth5Requirements) {
  std::vector<uint8_t> version_event;
  send_and_wait_for_cmd_complete(ReadLocalVersionInformationBuilder::Create(),
                                 version_event);
  auto version_view = ReadLocalVersionInformationCompleteView::Create(
      CommandCompleteView::Create(EventView::Create(PacketView<true>(
          std::make_shared<std::vector<uint8_t>>(version_event)))));
  ASSERT_TRUE(version_view.IsValid());
  ASSERT_EQ(::bluetooth::hci::ErrorCode::SUCCESS, version_view.GetStatus());
  auto version = version_view.GetLocalVersionInformation();
  if (version.hci_version_ < ::bluetooth::hci::HciVersion::V_5_0) {
    // This test does not apply to controllers below 5.0
    return;
  };
  // When HCI version is 5.0, LMP version must also be at least 5.0
  ASSERT_GE(static_cast<int>(version.lmp_version_),
            static_cast<int>(version.hci_version_));

  std::vector<uint8_t> le_features_event;
  send_and_wait_for_cmd_complete(LeReadLocalSupportedFeaturesBuilder::Create(),
                                 le_features_event);
  auto le_features_view = LeReadLocalSupportedFeaturesCompleteView::Create(
      CommandCompleteView::Create(EventView::Create(PacketView<true>(
          std::make_shared<std::vector<uint8_t>>(le_features_event)))));
  ASSERT_TRUE(le_features_view.IsValid());
  ASSERT_EQ(::bluetooth::hci::ErrorCode::SUCCESS, le_features_view.GetStatus());
  auto le_features = le_features_view.GetLeFeatures();
  ASSERT_TRUE(le_features & static_cast<uint64_t>(LLFeaturesBits::LL_PRIVACY));
  ASSERT_TRUE(le_features & static_cast<uint64_t>(LLFeaturesBits::LE_2M_PHY));
  ASSERT_TRUE(le_features &
              static_cast<uint64_t>(LLFeaturesBits::LE_CODED_PHY));
  ASSERT_TRUE(le_features &
              static_cast<uint64_t>(LLFeaturesBits::LE_EXTENDED_ADVERTISING));

  std::vector<uint8_t> num_adv_set_event;
  send_and_wait_for_cmd_complete(
      LeReadNumberOfSupportedAdvertisingSetsBuilder::Create(),
      num_adv_set_event);
  auto num_adv_set_view =
      LeReadNumberOfSupportedAdvertisingSetsCompleteView::Create(
          CommandCompleteView::Create(EventView::Create(PacketView<true>(
              std::make_shared<std::vector<uint8_t>>(num_adv_set_event)))));
  ASSERT_TRUE(num_adv_set_view.IsValid());
  ASSERT_EQ(::bluetooth::hci::ErrorCode::SUCCESS, num_adv_set_view.GetStatus());
  auto num_adv_set = num_adv_set_view.GetNumberSupportedAdvertisingSets();

  if (isTv() && get_vsr_api_level() == __ANDROID_API_U__) {
    ASSERT_GE(num_adv_set, kMinLeAdvSetForBt5FoTv);
  } else {
    ASSERT_GE(num_adv_set, kMinLeAdvSetForBt5);
  }

  std::vector<uint8_t> num_resolving_list_event;
  send_and_wait_for_cmd_complete(LeReadResolvingListSizeBuilder::Create(),
                                 num_resolving_list_event);
  auto num_resolving_list_view = LeReadResolvingListSizeCompleteView::Create(
      CommandCompleteView::Create(EventView::Create(PacketView<true>(
          std::make_shared<std::vector<uint8_t>>(num_resolving_list_event)))));
  ASSERT_TRUE(num_resolving_list_view.IsValid());
  ASSERT_EQ(::bluetooth::hci::ErrorCode::SUCCESS,
            num_resolving_list_view.GetStatus());
  auto num_resolving_list = num_resolving_list_view.GetResolvingListSize();
  ASSERT_GE(num_resolving_list, kMinLeResolvingListForBt5);
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
