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

#pragma once

#include <cstdint>
#include <string>

#include "bluetooth_hal/bqr/bqr_link_quality_event_v1_to_v3.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {

enum class LinkQualityOffsetV4 : uint8_t {
  kTxTotalPackets = static_cast<uint8_t>(LinkQualityOffsetV1ToV3::kEnd),
  kTxUnackedPackets = 56,       // 4 bytes
  kTxFlushedPackets = 60,       // 4 bytes
  kTxLastSubeventPackets = 64,  // 4 bytes
  kCrcErrorPackets = 68,        // 4 bytes
  kRxDuplicatePackets = 72,     // 4 bytes
  kEnd = 76,
};

class BqrLinkQualityEventV4 : public BqrLinkQualityEventV1ToV3 {
 public:
  explicit BqrLinkQualityEventV4(const ::bluetooth_hal::hci::HalPacket& packet);
  ~BqrLinkQualityEventV4() = default;

  // Checks if the BQR Link Quality Event V4 is valid.
  bool IsValid() const override;

  // Retrieves the number of packets that are sent out.
  uint32_t GetTxTotalPackets() const;
  // Retrieves the number of packets that don't receive an acknowledgment.
  uint32_t GetTxUnackedPackets() const;
  // Retrieves the number of packets that are not sent out by its flush point.
  uint32_t GetTxFlushedPackets() const;
  // Retrieves the number of packets that Link Layer transmits a CIS Data PDU in
  // the last subevent of a CIS event.
  uint32_t GetTxLastSubeventPackets() const;
  // Retrieves the number of received packages with CRC error since the last
  // event.
  uint32_t GetCrcErrorPackets() const;
  // Retrieves the number of duplicate (retransmission) packages that are
  // received since the last event.
  uint32_t GetRxDuplicatePackets() const;

  // Returns a string representation of the event.
  std::string ToString() const;

 protected:
  void ParseData();
  std::string ToBqrString() const;

  uint32_t tx_total_packets_;
  uint32_t tx_unacked_packets_;
  uint32_t tx_flushed_packets_;
  uint32_t tx_last_subevent_packets_;
  uint32_t crc_error_packets_;
  uint32_t rx_duplicate_packets_;
};

}  // namespace bqr
}  // namespace bluetooth_hal
