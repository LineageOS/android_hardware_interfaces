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

#include "bluetooth_hal/bqr/bqr_link_quality_event_v4.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "bluetooth_hal/bqr/bqr_link_quality_event_v1_to_v3.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kLinkQualityEventV4MinSize =
    static_cast<size_t>(LinkQualityOffsetV4::kEnd);

}  // namespace

BqrLinkQualityEventV4::BqrLinkQualityEventV4(const HalPacket& packet)
    : BqrLinkQualityEventV1ToV3(packet),
      tx_total_packets_(0),
      tx_unacked_packets_(0),
      tx_flushed_packets_(0),
      tx_last_subevent_packets_(0),
      crc_error_packets_(0),
      rx_duplicate_packets_(0) {
  is_valid_ = BqrLinkQualityEventV1ToV3::IsValid() &&
              size() >= kLinkQualityEventV4MinSize;
  ParseData();
}

void BqrLinkQualityEventV4::ParseData() {
  if (is_valid_) {
    version_ = BqrVersion::kV4;
    tx_total_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV4::kTxTotalPackets);
    tx_unacked_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV4::kTxUnackedPackets);
    tx_flushed_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV4::kTxFlushedPackets);
    tx_last_subevent_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV4::kTxLastSubeventPackets);
    crc_error_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV4::kCrcErrorPackets);
    rx_duplicate_packets_ =
        AtUint32LittleEndian(LinkQualityOffsetV4::kRxDuplicatePackets);
  }
}

bool BqrLinkQualityEventV4::IsValid() const { return is_valid_; }

uint32_t BqrLinkQualityEventV4::GetTxTotalPackets() const {
  return tx_total_packets_;
}

uint32_t BqrLinkQualityEventV4::GetTxUnackedPackets() const {
  return tx_unacked_packets_;
}

uint32_t BqrLinkQualityEventV4::GetTxFlushedPackets() const {
  return tx_flushed_packets_;
}

uint32_t BqrLinkQualityEventV4::GetTxLastSubeventPackets() const {
  return tx_last_subevent_packets_;
}

uint32_t BqrLinkQualityEventV4::GetCrcErrorPackets() const {
  return crc_error_packets_;
}

uint32_t BqrLinkQualityEventV4::GetRxDuplicatePackets() const {
  return rx_duplicate_packets_;
}

std::string BqrLinkQualityEventV4::ToString() const {
  if (!is_valid_) {
    return "BqrLinkQualityEventV4(Invalid)";
  }
  return "BqrLinkQualityEventV4: " + ToBqrString();
}

std::string BqrLinkQualityEventV4::ToBqrString() const {
  std::stringstream ss;
  ss << BqrLinkQualityEventV1ToV3::ToBqrString() << ", TxTotal: " << std::dec
     << tx_total_packets_ << ", TxUnAcked: " << std::dec << tx_unacked_packets_
     << ", TxFlushed: " << std::dec << tx_flushed_packets_
     << ", TxLastSubEvent: " << std::dec << tx_last_subevent_packets_
     << ", CRCError: " << std::dec << crc_error_packets_
     << ", RxDuplicate: " << std::dec << rx_duplicate_packets_;
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
