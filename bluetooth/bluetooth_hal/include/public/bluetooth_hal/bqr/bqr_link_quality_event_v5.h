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

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v1_to_v3.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {

enum class LinkQualityOffsetV5 : uint8_t {
  kRemoteAddr = static_cast<uint8_t>(LinkQualityOffset::kEnd),  // 6 bytes
  kCallFailedItemCount = 58,                                    // 1 bytes
  // V4-like parameters, but at new offsets unique to V5 structure
  kTxTotalPackets = 59,         // 4 byte
  kTxUnackedPackets = 63,       // 4 bytes
  kTxFlushedPackets = 67,       // 4 bytes
  kTxLastSubeventPackets = 71,  // 4 bytes
  kCrcErrorPackets = 75,        // 4 bytes
  kRxDuplicatePackets = 79,     // 4 bytes
  kEnd = 83,
};

class BqrLinkQualityEventV5 : public BqrLinkQualityEventV1ToV3 {
 public:
  explicit BqrLinkQualityEventV5(const ::bluetooth_hal::hci::HalPacket& packet);
  ~BqrLinkQualityEventV5() = default;

  // Checks if the BQR Link Quality Event V5 is valid.
  bool IsValid() const override;

  // Retrieves the remote Bluetooth address.
  ::bluetooth_hal::hci::BluetoothAddress GetRemoteAddress() const;
  // Retrieves the count of calibration failed items.
  uint8_t GetCallFailedItemCount() const;

  // V4-like parameters, but at offsets specific to V5
  uint32_t GetTxTotalPackets() const;
  uint32_t GetTxUnackedPackets() const;
  uint32_t GetTxFlushedPackets() const;
  uint32_t GetTxLastSubeventPackets() const;
  uint32_t GetCrcErrorPackets() const;
  uint32_t GetRxDuplicatePackets() const;

  // Returns a string representation of the event.
  std::string ToString() const;

 protected:
  void ParseData();
  std::string ToBqrString() const;

  ::bluetooth_hal::hci::BluetoothAddress remote_addr_;
  uint8_t call_failed_item_count_;
  uint32_t v5_tx_total_packets_;
  uint32_t v5_tx_unacked_packets_;
  uint32_t v5_tx_flushed_packets_;
  uint32_t v5_tx_last_subevent_packets_;
  uint32_t v5_crc_error_packets_;
  uint32_t v5_rx_duplicate_packets_;
};

}  // namespace bqr
}  // namespace bluetooth_hal
