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

#pragma once

#include <memory>
#include <vector>

#include "hci_internals.h"
#include "hci_packetizer.h"

namespace android::hardware::bluetooth::hci {

using PacketReadCallback = std::function<void(const std::vector<uint8_t>&)>;
using DisconnectCallback = std::function<void(void)>;

class H4Protocol {
 public:
  H4Protocol(int fd, PacketReadCallback cmd_cb, PacketReadCallback acl_cb,
             PacketReadCallback sco_cb, PacketReadCallback event_cb,
             PacketReadCallback iso_cb, DisconnectCallback disconnect_cb);

  size_t Send(PacketType type, const uint8_t* data, size_t length);
  size_t Send(PacketType type, const std::vector<uint8_t>& data);

  void OnDataReady();

 protected:
  size_t OnPacketReady(const std::vector<uint8_t>& packet);
  void SendDataToPacketizer(uint8_t* buffer, size_t length);

 private:
  int uart_fd_;
  bool disconnected_{false};

  PacketReadCallback cmd_cb_;
  PacketReadCallback acl_cb_;
  PacketReadCallback sco_cb_;
  PacketReadCallback event_cb_;
  PacketReadCallback iso_cb_;
  DisconnectCallback disconnect_cb_;

  PacketType hci_packet_type_{PacketType::UNKNOWN};
  HciPacketizer hci_packetizer_;

  /**
   * Question : Why read in single chunk rather than multiple reads?
   * Answer: Using multiple reads does not work with some BT USB dongles.
   * Reading in single shot gives expected response.
   * ACL max length is 2 bytes, so using 64K as the buffer length.
   */
  static constexpr size_t kMaxPacketLength = 64 * 1024;
};

}  // namespace android::hardware::bluetooth::hci
