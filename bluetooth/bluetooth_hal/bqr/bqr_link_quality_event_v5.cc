/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bluetooth_hal/bqr/bqr_link_quality_event_v5.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v1_to_v3.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kLinkQualityEventV5MinSize =
    static_cast<size_t>(LinkQualityOffsetV5::kEnd);

}  // namespace

BqrLinkQualityEventV5::BqrLinkQualityEventV5(const HalPacket& packet)
    : BqrLinkQualityEventV1ToV3(packet),
      remote_addr_(),
      call_failed_item_count_(0),
      v5_tx_total_packets_(0),
      v5_tx_unacked_packets_(0),
      v5_tx_flushed_packets_(0),
      v5_tx_last_subevent_packets_(0),
      v5_crc_error_packets_(0),
      v5_rx_duplicate_packets_(0) {
  is_valid_ = BqrLinkQualityEventV1ToV3::IsValid() &&
              size() >= kLinkQualityEventV5MinSize;
  ParseData();
}

void BqrLinkQualityEventV5::ParseData() {
  if (is_valid_) {
    version_ = BqrVersion::kV5;
    // Read each of the 6 bytes directly from the packet, little-endian.
    std::reverse_copy(
        begin() + static_cast<uint8_t>(LinkQualityOffsetV5::kRemoteAddr),
        begin() + static_cast<uint8_t>(LinkQualityOffsetV5::kRemoteAddr) +
            remote_addr_.size(),
        remote_addr_.begin());
    call_failed_item_count_ = At(LinkQualityOffsetV5::kCallFailedItemCount);
    v5_tx_total_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV5::kTxTotalPackets);
    v5_tx_unacked_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV5::kTxUnackedPackets);
    v5_tx_flushed_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV5::kTxFlushedPackets);
    v5_tx_last_subevent_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV5::kTxLastSubeventPackets);
    v5_crc_error_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV5::kCrcErrorPackets);
    v5_rx_duplicate_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV5::kRxDuplicatePackets);
  }
}

bool BqrLinkQualityEventV5::IsValid() const { return is_valid_; }

BluetoothAddress BqrLinkQualityEventV5::GetRemoteAddress() const {
  return remote_addr_;
}

uint8_t BqrLinkQualityEventV5::GetCallFailedItemCount() const {
  return call_failed_item_count_;
}

// Implement V4-like getters for V5, using V5-specific offsets
uint32_t BqrLinkQualityEventV5::GetTxTotalPackets() const {
  return v5_tx_total_packets_;
}

uint32_t BqrLinkQualityEventV5::GetTxUnackedPackets() const {
  return v5_tx_unacked_packets_;
}

uint32_t BqrLinkQualityEventV5::GetTxFlushedPackets() const {
  return v5_tx_flushed_packets_;
}

uint32_t BqrLinkQualityEventV5::GetTxLastSubeventPackets() const {
  return v5_tx_last_subevent_packets_;
}

uint32_t BqrLinkQualityEventV5::GetCrcErrorPackets() const {
  return v5_crc_error_packets_;
}

uint32_t BqrLinkQualityEventV5::GetRxDuplicatePackets() const {
  return v5_rx_duplicate_packets_;
}

std::string BqrLinkQualityEventV5::ToString() const {
  if (!is_valid_) {
    return "BqrLinkQualityEventV5(Invalid)";
  }
  return "BqrLinkQualityEventV5: " + ToBqrString();
}

std::string BqrLinkQualityEventV5::ToBqrString() const {
  std::stringstream ss;
  ss << BqrLinkQualityEventV1ToV3::ToBqrString()
     << ", Address: " << remote_addr_.ToString()
     << ", FailedCount: " << std::dec
     << static_cast<int>(call_failed_item_count_) << ", TxTotal: " << std::dec
     << v5_tx_total_packets_ << ", TxUnAcked: " << std::dec
     << v5_tx_unacked_packets_ << ", TxFlushed: " << std::dec
     << v5_tx_flushed_packets_ << ", TxLastSubEvent: " << std::dec
     << v5_tx_last_subevent_packets_ << ", CRCError: " << std::dec
     << v5_crc_error_packets_ << ", RxDuplicate: " << std::dec
     << v5_rx_duplicate_packets_;
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
