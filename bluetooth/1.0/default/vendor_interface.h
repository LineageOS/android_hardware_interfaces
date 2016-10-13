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

#pragma once

#include <hidl/HidlSupport.h>

#include "async_fd_watcher.h"
#include "bt_vendor_lib.h"
#include "hci_internals.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_vec;
using PacketReadCallback =
    std::function<void(HciPacketType, const hidl_vec<uint8_t> &)>;

class VendorInterface {
 public:
  static bool Initialize(PacketReadCallback packet_read_cb);
  static void Shutdown();
  static VendorInterface* get();

  size_t Send(const uint8_t *data, size_t length);

  void OnFirmwareConfigured(uint8_t result);

  // Actually send the data.
  size_t SendPrivate(const uint8_t *data, size_t length);

 private:
  VendorInterface() { queued_data_.resize(0); }
  virtual ~VendorInterface() = default;

  bool Open(PacketReadCallback packet_read_cb);
  void Close();

  void OnDataReady(int fd);

  // Queue data from Send() until the interface is ready.
  hidl_vec<uint8_t> queued_data_;

  void *lib_handle_;
  bt_vendor_interface_t *lib_interface_;
  AsyncFdWatcher fd_watcher_;
  int uart_fd_;
  PacketReadCallback packet_read_cb_;
  bool firmware_configured_;

  enum HciParserState {
    HCI_IDLE,
    HCI_TYPE_READY,
    HCI_PAYLOAD
  };
  HciParserState hci_parser_state_{HCI_IDLE};
  HciPacketType hci_packet_type_{HCI_PACKET_TYPE_UNKNOWN};
  hidl_vec<uint8_t> hci_packet_;
  size_t hci_packet_bytes_remaining_;
  size_t hci_packet_bytes_read_;
};

} // namespace implementation
} // namespace V1_0
} // namespace bluetooth
} // namespace hardware
} // namespace android
