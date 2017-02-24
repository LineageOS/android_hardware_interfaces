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

#pragma once

#include <functional>

#include <hidl/HidlSupport.h>

#include "hci_internals.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace hci {

using ::android::hardware::hidl_vec;
using HciPacketReadyCallback = std::function<void(void)>;

class HciPacketizer {
 public:
  HciPacketizer(HciPacketReadyCallback packet_cb) : hci_packet_ready_cb_(packet_cb) {};
  void OnDataReady(int fd);
  const hidl_vec<uint8_t>& GetPacket() const;
  HciPacketType GetPacketType() const;

 protected:
  enum HciParserState { HCI_IDLE, HCI_TYPE_READY, HCI_PAYLOAD };
  HciParserState hci_parser_state_{HCI_IDLE};
  HciPacketType hci_packet_type_{HCI_PACKET_TYPE_UNKNOWN};
  uint8_t hci_packet_preamble_[HCI_PREAMBLE_SIZE_MAX];
  hidl_vec<uint8_t> hci_packet_;
  size_t hci_packet_bytes_remaining_;
  size_t hci_packet_bytes_read_;
  HciPacketReadyCallback hci_packet_ready_cb_;
};

}  // namespace hci
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
