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

#include "hci_packetizer.h"

#define LOG_TAG "android.hardware.bluetooth.hci-packetizer"
#include "log/log.h"

namespace android::hardware::bluetooth::hci {

namespace {

const size_t header_size_for_type[] = {0,
                                       kCommandHeaderSize,
                                       kAclHeaderSize,
                                       kScoHeaderSize,
                                       kEventHeaderSize,
                                       kIsoHeaderSize};
const size_t packet_length_offset_for_type[] = {0,
                                                kCommandLengthOffset,
                                                kAclLengthOffset,
                                                kScoLengthOffset,
                                                kEventLengthOffset,
                                                kIsoLengthOffset};

size_t HciGetPacketLengthForType(PacketType type,
                                 const std::vector<uint8_t>& header) {
  size_t offset = packet_length_offset_for_type[static_cast<uint8_t>(type)];
  if (type != PacketType::ACL_DATA && type != PacketType::ISO_DATA) {
    return header[offset];
  }
  return (((header[offset + 1]) << 8) | header[offset]);
}

}  // namespace

const std::vector<uint8_t>& HciPacketizer::GetPacket() const { return packet_; }

bool HciPacketizer::OnDataReady(PacketType packet_type,
                                const std::vector<uint8_t>& buffer,
                                size_t* offset) {
  bool packet_completed = false;
  size_t bytes_available = buffer.size() - *offset;

  switch (state_) {
    case HCI_HEADER: {
      size_t header_size =
          header_size_for_type[static_cast<size_t>(packet_type)];
      if (bytes_remaining_ == 0) {
        bytes_remaining_ = header_size;
        packet_.clear();
      }

      size_t bytes_to_copy = std::min(bytes_remaining_, bytes_available);
      packet_.insert(packet_.end(), buffer.begin() + *offset,
                     buffer.begin() + *offset + bytes_to_copy);
      bytes_remaining_ -= bytes_to_copy;
      bytes_available -= bytes_to_copy;
      *offset += bytes_to_copy;

      if (bytes_remaining_ == 0) {
        bytes_remaining_ = HciGetPacketLengthForType(packet_type, packet_);
        if (bytes_remaining_ > 0) {
          state_ = HCI_PAYLOAD;
          if (bytes_available > 0) {
            packet_completed = OnDataReady(packet_type, buffer, offset);
          }
        } else {
          packet_completed = true;
        }
      }
      break;
    }

    case HCI_PAYLOAD: {
      size_t bytes_to_copy = std::min(bytes_remaining_, bytes_available);
      packet_.insert(packet_.end(), buffer.begin() + *offset,
                     buffer.begin() + *offset + bytes_to_copy);
      bytes_remaining_ -= bytes_to_copy;
      *offset += bytes_to_copy;
      if (bytes_remaining_ == 0) {
        state_ = HCI_HEADER;
        packet_completed = true;
      }
      break;
    }
  }

  return packet_completed;
}

}  // namespace android::hardware::bluetooth::hci
