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
#include <span>
#include <utility>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/transport/hci_packet_rescuer.h"

namespace bluetooth_hal {
namespace transport {

class HciPacketizer {
 public:
  explicit HciPacketizer(
      ::bluetooth_hal::hci::HalPacketCallback on_packet_ready)
      : on_packet_ready_(std::move(on_packet_ready)) {}

  /**
   * @brief Processes incoming data to construct an HCI packet.
   *
   * This function reads data from the input `data` span, appends it to the
   * internal packet buffer, and attempts to assemble a complete HCI packet.
   * If a complete packet is formed, the `packet_ready_callback_` (if set)
   * is invoked with the complete packet. The function continues to process
   * the input data until it's exhausted, potentially forming multiple
   * packets.
   *
   * @param data A const span referencing the incoming data buffer. The data
   * within this span is treated as read-only and is not modified.
   *
   * @return The number of bytes consumed from the `data` span. This might be
   * less than `data.size()` if the function constructs a complete packet before
   * the end of the input data, subsequent calls with the remaining data in
   * `data` should be made to process it fully. Returns 0 if the packetizer
   * cannot format packets based on the bluetooth spec.
   *
   */
  size_t ProcessData(std::span<const uint8_t> data);

 private:
  enum class State : uint8_t {
    kHciHeader,
    kHciPreamble,
    kHciPayload,
  };

  State state_{State::kHciHeader};

  ::bluetooth_hal::hci::HalPacket packet_;

  size_t payload_length_{0};
  size_t total_bytes_read_{0};

  ::bluetooth_hal::hci::HalPacketCallback on_packet_ready_;
  ::bluetooth_hal::transport::HciPacketRescuer hci_packet_rescuer_;
};

}  // namespace transport
}  // namespace bluetooth_hal
