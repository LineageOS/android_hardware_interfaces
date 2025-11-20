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

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/transport/uart_h4/hci_packetizer.h"
#include "bluetooth_hal/util/fd_watcher.h"

namespace bluetooth_hal {
namespace transport {

class DataProcessor {
 public:
  DataProcessor(int fd, ::bluetooth_hal::hci::HalPacketCallback on_packet_ready)
      : fd_(fd), hci_packetizer_(std::move(on_packet_ready)) {}

  ~DataProcessor();

  /**
   * @brief Starts the data processing pipeline.
   *
   * This method initiates the listening thread of the FileDescriptor, starting
   * the data monitoring process.
   *
   */
  void StartProcessing();

  /**
   * @brief Sends a packet over a specified file descriptor.
   *
   * @param packet The packet to send.
   ＊
   * @return The total number of bytes written.
   *
   */
  size_t Send(std::span<const uint8_t> packet);

  /**
   * @brief Receives data from a file descriptor.
   *
   * This function reads data from a file descriptor, processes it into an HCI
   * packet, and then invokes a callback to pass the packets to the upper layer.
   *
   * @param fd The file descriptor to receive data from.
   *
   */
  void Recv(int fd);

 private:
  void ParseHciPacket(std::span<const uint8_t> buffer);

  int fd_;

  HciPacketizer hci_packetizer_;
  ::bluetooth_hal::util::FdWatcher fd_watcher_;
};

}  // namespace transport
}  // namespace bluetooth_hal
