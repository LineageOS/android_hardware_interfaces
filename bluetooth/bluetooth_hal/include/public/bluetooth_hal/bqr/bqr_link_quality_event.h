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

#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {

enum class LinkQualityOffset : uint8_t {
  // After H4 type(1) + event code(1) + length(1) + sub event(1) + report id(1)
  kPacketTypes = 5,                    // 1 byte
  kConnectionHandle = 6,               // 2 bytes
  kConnectionRole = 8,                 // 1 byte
  kTxPowerLevel = 9,                   // 1 byte
  kRssi = 10,                          // 1 byte
  kSnr = 11,                           // 1 byte
  kUnusedAfhChannelCount = 12,         // 1 byte
  kAfhSelectUnidealChannelCount = 13,  // 1 byte
  kLsto = 14,                          // 2 byte
  kConnectionPiconetClock = 16,        // 4 bytes
  kRetransmissionCount = 20,           // 4 bytes
  kNoRxCount = 24,                     // 4 bytes
  kNakCount = 28,                      // 4 bytes
  kLastTxAckTimestamp = 32,            // 4 bytes
  kFlowOffCount = 36,                  // 4 bytes
  kLastFlowOnTimestamp = 40,           // 4 bytes
  kBufferOverflowBytes = 44,           // 4 bytes
  kBufferUnderflowBytes = 48,          // 4 bytes
  kEnd = 52,
};

// Base class for all BQR Link Quality events.
// It provides methods to access common parameters across different versions.
class BqrLinkQualityEventBase : public BqrEvent {
 public:
  explicit BqrLinkQualityEventBase(
      const ::bluetooth_hal::hci::HalPacket& packet);
  virtual ~BqrLinkQualityEventBase() = default;

  // Checks if the BQR Link Quality Event is valid.
  // Overrides the base BqrEvent::IsValid to include Link Quality specific
  // checks.
  bool IsValid() const override;

  // Returns the supported version of the BQR event.
  virtual BqrVersion GetVersion() const;

  // Retrieves the Packet Type of the connection.
  uint8_t GetPacketTypes() const;
  // Retrieves the Connection Handle of the connection.
  uint16_t GetConnectionHandle() const;
  // Retrieves the Performing Role for the connection.
  uint8_t GetConnectionRole() const;
  // Retrieves the Current Transmit Power Level for the connection.
  int8_t GetTxPowerLevel() const;
  // Retrieves the Received Signal Strength Indication (RSSI) value for the
  // connection.
  int8_t GetRssi() const;
  // Retrieves the Signal-to-Noise Ratio (SNR) value for the connection.
  uint8_t GetSnr() const;
  // Retrieves the number of unused channels in AFH_channel_map.
  uint8_t GetUnusedAfhChannelCount() const;
  // Retrieves the number of channels which are interfered and quality is bad
  // but are still selected for AFH.
  uint8_t GetAfhSelectUnidealChannelCount() const;
  // Retrieves the Current Link Supervision Timeout Setting in 0.3125 ms units.
  uint16_t GetLsto() const;
  // Retrieves the Piconet Clock for the specified Connection_Handle in 0.3125
  // ms units.
  uint32_t GetConnectionPiconetClock() const;
  // Retrieves the count of retransmissions.
  uint32_t GetRetransmissionCount() const;
  // Retrieves the count of no RX.
  uint32_t GetNoRxCount() const;
  // Retrieves the count of NAK (Negative Acknowledge).
  uint32_t GetNakCount() const;
  // Retrieves the timestamp of the last TX ACK in 0.3125 ms units.
  uint32_t GetLastTxAckTimestamp() const;
  // Retrieves the count of Flow-off (STOP).
  uint32_t GetFlowOffCount() const;
  // Retrieves the timestamp of the last Flow-on (GO) in 0.3125 ms units.
  uint32_t GetLastFlowOnTimestamp() const;
  // Retrieves the buffer overflow count (how many bytes of TX data are dropped)
  // since the last event.
  uint32_t GetBufferOverflowBytes() const;
  // Retrieves the buffer underflow count (in bytes).
  uint32_t GetBufferUnderflowBytes() const;

  // Returns a string representation of the event.
  std::string ToString() const;

 protected:
  void ParseData();
  std::string ToBqrString() const;

  bool is_valid_;
  BqrVersion version_;
  uint8_t packet_types_;
  uint16_t connection_handle_;
  uint8_t connection_role_;
  int8_t tx_power_level_;
  int8_t rssi_;
  uint8_t snr_;
  uint8_t unused_afh_channel_count_;
  uint8_t afh_select_unideal_channel_count_;
  uint16_t lsto_;
  uint32_t connection_piconet_clock_;
  uint32_t retransmission_count_;
  uint32_t no_rx_count_;
  uint32_t nak_count_;
  uint32_t last_tx_ack_timestamp_;
  uint32_t flow_off_count_;
  uint32_t last_flow_on_timestamp_;
  uint32_t buffer_overflow_bytes_;
  uint32_t buffer_underflow_bytes_;
};

}  // namespace bqr
}  // namespace bluetooth_hal
