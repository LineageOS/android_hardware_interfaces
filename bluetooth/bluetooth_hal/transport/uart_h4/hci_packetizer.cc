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

#define LOG_TAG "bthal.hci_packetizer"

#include "bluetooth_hal/transport/uart_h4/hci_packetizer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ios>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace transport {
namespace {

using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciPacketType;

constexpr size_t kLogByteLimit = 6;

std::string GenerateUnimplementedPacketLog(HciPacketType packet_type,
                                           std::span<const uint8_t> buffer) {
  std::ostringstream oss;
  oss << "Host Received Unimplemented Packet Type: " << std::hex
      << std::uppercase << static_cast<uint8_t>(packet_type)
      << ", bytes_read: " << buffer.size() << ", packet:";

  const unsigned long max_bytes_to_print =
      std::min(buffer.size(), kLogByteLimit);
  for (size_t i = 0; i < max_bytes_to_print; ++i) {
    oss << " " << std::hex << std::uppercase << static_cast<int>(buffer[i]);
  }
  return oss.str();
}

bool IsValidHciPacketType(HciPacketType hci_packet_type) {
  switch (hci_packet_type) {
    case HciPacketType::kCommand:
    case HciPacketType::kAclData:
    case HciPacketType::kScoData:
    case HciPacketType::kIsoData:
    case HciPacketType::kEvent:
    case HciPacketType::kThreadData:
      return true;
    default:
      return false;
  }
}

size_t GetPayloadLength(std::span<const uint8_t> packet) {
  if (packet.empty()) {
    return 0;
  }

  HciPacketType packet_type = static_cast<HciPacketType>(packet[0]);
  if (packet.size() < 1 + HciConstants::GetPreambleSize(packet_type)) {
    return 0;
  }

  const size_t offset = HciConstants::GetPacketLengthOffset(packet_type);

  switch (packet_type) {
    case HciPacketType::kAclData:
    case HciPacketType::kThreadData:
      return (static_cast<size_t>(packet[offset + 1]) << 8) | packet[offset];
    case HciPacketType::kIsoData:
      return ((static_cast<size_t>(packet[offset + 1]) & 0x3F) << 8) |
             packet[offset];
    default:
      return packet[offset];
  }
}

}  // namespace

size_t HciPacketizer::ProcessData(std::span<const uint8_t> data) {
  if (!data.size()) {
    return 0;
  }

  size_t cur_bytes_read = 0;
  const size_t len = data.size();

  switch (state_) {
    case State::kHciHeader: {
      const auto hci_packet_type = static_cast<HciPacketType>(data[0]);
      packet_.clear();

      if (!IsValidHciPacketType(hci_packet_type)) {
        const std::string err_msg =
            GenerateUnimplementedPacketLog(hci_packet_type, data);
      } else {
        packet_.push_back(data[0]);

        state_ = State::kHciPreamble;
        cur_bytes_read = 1;
      }
      break;
    }

    case State::kHciPreamble: {
      const size_t preamble_size =
          HciConstants::GetPreambleSize(packet_.GetType());
      const size_t to_read = std::min(len, preamble_size - total_bytes_read_);

      packet_.insert(packet_.end(), data.begin(), data.begin() + to_read);

      total_bytes_read_ += to_read;
      cur_bytes_read = to_read;

      if (total_bytes_read_ == preamble_size) {
        state_ = State::kHciPayload;
        payload_length_ = GetPayloadLength(std::span(packet_));
        total_bytes_read_ = 0;
      }
      break;
    }

    case State::kHciPayload: {
      const size_t to_read = std::min(len, payload_length_ - total_bytes_read_);

      packet_.insert(packet_.end(), data.begin(), data.begin() + to_read);

      total_bytes_read_ += to_read;
      cur_bytes_read = to_read;

      if (total_bytes_read_ == payload_length_) {
        on_packet_ready_(packet_);

        state_ = State::kHciHeader;
        payload_length_ = 0;
        total_bytes_read_ = 0;
      }
      break;
    }
  }

  return cur_bytes_read;
}

}  // namespace transport
}  // namespace bluetooth_hal
