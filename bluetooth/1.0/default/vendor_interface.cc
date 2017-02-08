//
// Copyright 2016 The Android Open Source Project
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

#include "vendor_interface.h"

#include <assert.h>

#define LOG_TAG "android.hardware.bluetooth@1.0-impl"
#include <android-base/logging.h>
#include <cutils/properties.h>
#include <utils/Log.h>

#include <dlfcn.h>
#include <fcntl.h>

#include "bluetooth_address.h"

static const char* VENDOR_LIBRARY_NAME = "libbt-vendor.so";
static const char* VENDOR_LIBRARY_SYMBOL_NAME =
    "BLUETOOTH_VENDOR_LIB_INTERFACE";

static const int INVALID_FD = -1;

namespace {

using android::hardware::bluetooth::V1_0::implementation::VendorInterface;
using android::hardware::hidl_vec;

struct {
  tINT_CMD_CBACK cb;
  uint16_t opcode;
} internal_command;

// True when LPM is not enabled yet or wake is not asserted.
bool lpm_wake_deasserted;
uint32_t lpm_timeout_ms;
bool recent_activity_flag;

VendorInterface* g_vendor_interface = nullptr;

const size_t preamble_size_for_type[] = {
    0, HCI_COMMAND_PREAMBLE_SIZE, HCI_ACL_PREAMBLE_SIZE, HCI_SCO_PREAMBLE_SIZE,
    HCI_EVENT_PREAMBLE_SIZE};
const size_t packet_length_offset_for_type[] = {
    0, HCI_LENGTH_OFFSET_CMD, HCI_LENGTH_OFFSET_ACL, HCI_LENGTH_OFFSET_SCO,
    HCI_LENGTH_OFFSET_EVT};

size_t HciGetPacketLengthForType(HciPacketType type, const uint8_t* preamble) {
  size_t offset = packet_length_offset_for_type[type];
  if (type != HCI_PACKET_TYPE_ACL_DATA) return preamble[offset];
  return (((preamble[offset + 1]) << 8) | preamble[offset]);
}

HC_BT_HDR* WrapPacketAndCopy(uint16_t event, const hidl_vec<uint8_t>& data) {
  size_t packet_size = data.size() + sizeof(HC_BT_HDR);
  HC_BT_HDR* packet = reinterpret_cast<HC_BT_HDR*>(new uint8_t[packet_size]);
  packet->offset = 0;
  packet->len = data.size();
  packet->layer_specific = 0;
  packet->event = event;
  // TODO(eisenbach): Avoid copy here; if BT_HDR->data can be ensured to
  // be the only way the data is accessed, a pointer could be passed here...
  memcpy(packet->data, data.data(), data.size());
  return packet;
}

size_t write_safely(int fd, const uint8_t* data, size_t length) {
  size_t transmitted_length = 0;
  while (length > 0) {
    ssize_t ret =
        TEMP_FAILURE_RETRY(write(fd, data + transmitted_length, length));

    if (ret == -1) {
      if (errno == EAGAIN) continue;
      ALOGE("%s error writing to UART (%s)", __func__, strerror(errno));
      break;

    } else if (ret == 0) {
      // Nothing written :(
      ALOGE("%s zero bytes written - something went wrong...", __func__);
      break;
    }

    transmitted_length += ret;
    length -= ret;
  }

  return transmitted_length;
}

bool internal_command_event_match(const hidl_vec<uint8_t>& packet) {
  uint8_t event_code = packet[0];
  if (event_code != HCI_COMMAND_COMPLETE_EVENT) {
    ALOGE("%s: Unhandled event type %02X", __func__, event_code);
    return false;
  }

  size_t opcode_offset = HCI_EVENT_PREAMBLE_SIZE + 1;  // Skip num packets.

  uint16_t opcode = packet[opcode_offset] | (packet[opcode_offset + 1] << 8);

  ALOGV("%s internal_command.opcode = %04X opcode = %04x", __func__,
        internal_command.opcode, opcode);
  return opcode == internal_command.opcode;
}

uint8_t transmit_cb(uint16_t opcode, void* buffer, tINT_CMD_CBACK callback) {
  ALOGV("%s opcode: 0x%04x, ptr: %p, cb: %p", __func__, opcode, buffer,
        callback);
  internal_command.cb = callback;
  internal_command.opcode = opcode;
  uint8_t type = HCI_PACKET_TYPE_COMMAND;
  HC_BT_HDR* bt_hdr = reinterpret_cast<HC_BT_HDR*>(buffer);
  VendorInterface::get()->Send(type, bt_hdr->data, bt_hdr->len);
  delete[] reinterpret_cast<uint8_t*>(buffer);
  return true;
}

void firmware_config_cb(bt_vendor_op_result_t result) {
  ALOGV("%s result: %d", __func__, result);
  VendorInterface::get()->OnFirmwareConfigured(result);
}

void sco_config_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
}

void low_power_mode_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
}

void sco_audiostate_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
}

void* buffer_alloc_cb(int size) {
  void* p = new uint8_t[size];
  ALOGV("%s pts: %p, size: %d", __func__, p, size);
  return p;
}

void buffer_free_cb(void* buffer) {
  ALOGV("%s ptr: %p", __func__, buffer);
  delete[] reinterpret_cast<uint8_t*>(buffer);
}

void epilog_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
}

void a2dp_offload_cb(bt_vendor_op_result_t result, bt_vendor_opcode_t op,
                     uint8_t av_handle) {
  ALOGD("%s result: %d, op: %d, handle: %d", __func__, result, op, av_handle);
}

const bt_vendor_callbacks_t lib_callbacks = {
    sizeof(lib_callbacks), firmware_config_cb, sco_config_cb,
    low_power_mode_cb,     sco_audiostate_cb,  buffer_alloc_cb,
    buffer_free_cb,        transmit_cb,        epilog_cb,
    a2dp_offload_cb};

}  // namespace

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

class FirmwareStartupTimer {
 public:
  FirmwareStartupTimer() : start_time_(std::chrono::steady_clock::now()) {}

  ~FirmwareStartupTimer() {
    std::chrono::duration<double> duration =
        std::chrono::steady_clock::now() - start_time_;
    double s = duration.count();
    if (s == 0) return;
    ALOGI("Firmware configured in %.3fs", s);
  }

 private:
  std::chrono::steady_clock::time_point start_time_;
};

bool VendorInterface::Initialize(
    InitializeCompleteCallback initialize_complete_cb,
    PacketReadCallback packet_read_cb) {
  assert(!g_vendor_interface);
  g_vendor_interface = new VendorInterface();
  return g_vendor_interface->Open(initialize_complete_cb, packet_read_cb);
}

void VendorInterface::Shutdown() {
  CHECK(g_vendor_interface);
  g_vendor_interface->Close();
  delete g_vendor_interface;
  g_vendor_interface = nullptr;
}

VendorInterface* VendorInterface::get() { return g_vendor_interface; }

bool VendorInterface::Open(InitializeCompleteCallback initialize_complete_cb,
                           PacketReadCallback packet_read_cb) {
  initialize_complete_cb_ = initialize_complete_cb;
  packet_read_cb_ = packet_read_cb;

  // Initialize vendor interface

  lib_handle_ = dlopen(VENDOR_LIBRARY_NAME, RTLD_NOW);
  if (!lib_handle_) {
    ALOGE("%s unable to open %s (%s)", __func__, VENDOR_LIBRARY_NAME,
          dlerror());
    return false;
  }

  lib_interface_ = reinterpret_cast<bt_vendor_interface_t*>(
      dlsym(lib_handle_, VENDOR_LIBRARY_SYMBOL_NAME));
  if (!lib_interface_) {
    ALOGE("%s unable to find symbol %s in %s (%s)", __func__,
          VENDOR_LIBRARY_SYMBOL_NAME, VENDOR_LIBRARY_NAME, dlerror());
    return false;
  }

  // Get the local BD address

  uint8_t local_bda[BluetoothAddress::kBytes];
  CHECK(BluetoothAddress::get_local_address(local_bda));
  int status = lib_interface_->init(&lib_callbacks, (unsigned char*)local_bda);
  if (status) {
    ALOGE("%s unable to initialize vendor library: %d", __func__, status);
    return false;
  }

  ALOGD("%s vendor library loaded", __func__);

  // Power cycle chip

  int power_state = BT_VND_PWR_OFF;
  lib_interface_->op(BT_VND_OP_POWER_CTRL, &power_state);
  power_state = BT_VND_PWR_ON;
  lib_interface_->op(BT_VND_OP_POWER_CTRL, &power_state);

  // Get the UART socket

  int fd_list[CH_MAX] = {0};
  int fd_count = lib_interface_->op(BT_VND_OP_USERIAL_OPEN, &fd_list);

  if (fd_count != 1) {
    ALOGE("%s fd count %d != 1; we can't handle this currently...", __func__,
          fd_count);
    return false;
  }

  uart_fd_ = fd_list[0];
  if (uart_fd_ == INVALID_FD) {
    ALOGE("%s unable to determine UART fd", __func__);
    return false;
  }

  ALOGI("%s UART fd: %d", __func__, uart_fd_);

  fd_watcher_.WatchFdForNonBlockingReads(uart_fd_,
                                         [this](int fd) { OnDataReady(fd); });

  // Initially, the power management is off.
  lpm_wake_deasserted = true;

  // Start configuring the firmware
  firmware_startup_timer_ = new FirmwareStartupTimer();
  lib_interface_->op(BT_VND_OP_FW_CFG, nullptr);

  return true;
}

void VendorInterface::Close() {
  fd_watcher_.StopWatchingFileDescriptor();

  if (lib_interface_ != nullptr) {
    bt_vendor_lpm_mode_t mode = BT_VND_LPM_DISABLE;
    lib_interface_->op(BT_VND_OP_LPM_SET_MODE, &mode);

    lib_interface_->op(BT_VND_OP_USERIAL_CLOSE, nullptr);
    uart_fd_ = INVALID_FD;
    int power_state = BT_VND_PWR_OFF;
    lib_interface_->op(BT_VND_OP_POWER_CTRL, &power_state);
  }

  if (lib_handle_ != nullptr) {
    dlclose(lib_handle_);
    lib_handle_ = nullptr;
  }

  if (firmware_startup_timer_ != nullptr) {
    delete firmware_startup_timer_;
    firmware_startup_timer_ = nullptr;
  }
}

size_t VendorInterface::Send(uint8_t type, const uint8_t* data, size_t length) {
  if (uart_fd_ == INVALID_FD) return 0;

  recent_activity_flag = true;

  if (lpm_wake_deasserted == true) {
    // Restart the timer.
    fd_watcher_.ConfigureTimeout(std::chrono::milliseconds(lpm_timeout_ms),
                                 [this]() { OnTimeout(); });
    // Assert wake.
    lpm_wake_deasserted = false;
    bt_vendor_lpm_wake_state_t wakeState = BT_VND_LPM_WAKE_ASSERT;
    lib_interface_->op(BT_VND_OP_LPM_WAKE_SET_STATE, &wakeState);
    ALOGV("%s: Sent wake before (%02x)", __func__, data[0] | (data[1] << 8));
  }

  int rv = write_safely(uart_fd_, &type, sizeof(type));
  if (rv == sizeof(type))
    rv = write_safely(uart_fd_, data, length);

  return rv;
}

void VendorInterface::OnFirmwareConfigured(uint8_t result) {
  ALOGD("%s result: %d", __func__, result);

  if (firmware_startup_timer_ != nullptr) {
    delete firmware_startup_timer_;
    firmware_startup_timer_ = nullptr;
  }

  if (initialize_complete_cb_ != nullptr) {
    initialize_complete_cb_(result == 0);
    initialize_complete_cb_ = nullptr;
  }

  lib_interface_->op(BT_VND_OP_GET_LPM_IDLE_TIMEOUT, &lpm_timeout_ms);
  ALOGI("%s: lpm_timeout_ms %d", __func__, lpm_timeout_ms);

  bt_vendor_lpm_mode_t mode = BT_VND_LPM_ENABLE;
  lib_interface_->op(BT_VND_OP_LPM_SET_MODE, &mode);

  ALOGD("%s Calling StartLowPowerWatchdog()", __func__);
  fd_watcher_.ConfigureTimeout(std::chrono::milliseconds(lpm_timeout_ms),
                               [this]() { OnTimeout(); });
}

void VendorInterface::OnTimeout() {
  ALOGV("%s", __func__);
  if (recent_activity_flag == false) {
    lpm_wake_deasserted = true;
    bt_vendor_lpm_wake_state_t wakeState = BT_VND_LPM_WAKE_DEASSERT;
    lib_interface_->op(BT_VND_OP_LPM_WAKE_SET_STATE, &wakeState);
    fd_watcher_.ConfigureTimeout(std::chrono::seconds(0), []() {
      ALOGE("Zero timeout! Should never happen.");
    });
  }
  recent_activity_flag = false;
}

void VendorInterface::OnDataReady(int fd) {
  switch (hci_parser_state_) {
    case HCI_IDLE: {
      uint8_t buffer[1] = {0};
      size_t bytes_read = TEMP_FAILURE_RETRY(read(fd, buffer, 1));
      CHECK(bytes_read == 1);
      hci_packet_type_ = static_cast<HciPacketType>(buffer[0]);
      // TODO(eisenbach): Check for workaround(s)
      CHECK(hci_packet_type_ >= HCI_PACKET_TYPE_ACL_DATA &&
            hci_packet_type_ <= HCI_PACKET_TYPE_EVENT)
          << "buffer[0] = " << static_cast<unsigned int>(buffer[0]);
      hci_parser_state_ = HCI_TYPE_READY;
      hci_packet_bytes_remaining_ = preamble_size_for_type[hci_packet_type_];
      hci_packet_bytes_read_ = 0;
      break;
    }

    case HCI_TYPE_READY: {
      size_t bytes_read = TEMP_FAILURE_RETRY(
          read(fd, hci_packet_preamble_ + hci_packet_bytes_read_,
               hci_packet_bytes_remaining_));
      CHECK(bytes_read > 0);
      hci_packet_bytes_remaining_ -= bytes_read;
      hci_packet_bytes_read_ += bytes_read;
      if (hci_packet_bytes_remaining_ == 0) {
        size_t packet_length =
            HciGetPacketLengthForType(hci_packet_type_, hci_packet_preamble_);
        hci_packet_.resize(preamble_size_for_type[hci_packet_type_] +
                           packet_length);
        memcpy(hci_packet_.data(), hci_packet_preamble_,
               preamble_size_for_type[hci_packet_type_]);
        hci_packet_bytes_remaining_ = packet_length;
        hci_parser_state_ = HCI_PAYLOAD;
        hci_packet_bytes_read_ = 0;
      }
      break;
    }

    case HCI_PAYLOAD: {
      size_t bytes_read = TEMP_FAILURE_RETRY(
          read(fd,
               hci_packet_.data() + preamble_size_for_type[hci_packet_type_] +
                   hci_packet_bytes_read_,
               hci_packet_bytes_remaining_));
      hci_packet_bytes_remaining_ -= bytes_read;
      hci_packet_bytes_read_ += bytes_read;
      if (hci_packet_bytes_remaining_ == 0) {
        if (internal_command.cb != nullptr &&
            hci_packet_type_ == HCI_PACKET_TYPE_EVENT &&
            internal_command_event_match(hci_packet_)) {
          HC_BT_HDR* bt_hdr =
              WrapPacketAndCopy(HCI_PACKET_TYPE_EVENT, hci_packet_);

          // The callbacks can send new commands, so don't zero after calling.
          tINT_CMD_CBACK saved_cb = internal_command.cb;
          internal_command.cb = nullptr;
          saved_cb(bt_hdr);
        } else {
          packet_read_cb_(hci_packet_type_, hci_packet_);
        }
        hci_parser_state_ = HCI_IDLE;
      }
      break;
    }
  }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
