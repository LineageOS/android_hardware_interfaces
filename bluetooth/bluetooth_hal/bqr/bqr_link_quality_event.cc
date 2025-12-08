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

#include "bluetooth_hal/bqr/bqr_link_quality_event.h"

#include <cstdint>
#include <iomanip>
#include <sstream>

#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::hci::HalPacket;

constexpr size_t kLinkQualityEventMinSize =
    static_cast<size_t>(LinkQualityOffset::kEnd);

}  // namespace

BqrLinkQualityEventBase::BqrLinkQualityEventBase(const HalPacket& packet)
    : BqrEvent(packet),
      is_valid_(BqrEvent::IsValid() &&
                GetBqrEventType() == BqrEventType::kLinkQuality &&
                size() >= kLinkQualityEventMinSize),
      packet_types_(0),
      connection_handle_(0),
      connection_role_(0),
      tx_power_level_(0),
      rssi_(0),
      snr_(0),
      unused_afh_channel_count_(0),
      afh_select_unideal_channel_count_(0),
      lsto_(0),
      connection_piconet_clock_(0),
      retransmission_count_(0),
      no_rx_count_(0),
      nak_count_(0),
      last_tx_ack_timestamp_(0),
      flow_off_count_(0),
      last_flow_on_timestamp_(0),
      buffer_overflow_bytes_(0),
      buffer_underflow_bytes_(0) {
  ParseData();
}

void BqrLinkQualityEventBase::ParseData() {
  if (is_valid_) {
    packet_types_ = At(LinkQualityOffset::kPacketTypes);
    connection_handle_ =
        AtUint16LittleEndian(LinkQualityOffset::kConnectionHandle);
    connection_role_ = At(LinkQualityOffset::kConnectionRole);
    tx_power_level_ = static_cast<int8_t>(At(LinkQualityOffset::kTxPowerLevel));
    rssi_ = static_cast<int8_t>(At(LinkQualityOffset::kRssi));
    snr_ = At(LinkQualityOffset::kSnr);
    unused_afh_channel_count_ = At(LinkQualityOffset::kUnusedAfhChannelCount);
    afh_select_unideal_channel_count_ =
        At(LinkQualityOffset::kAfhSelectUnidealChannelCount);
    lsto_ = AtUint16LittleEndian(LinkQualityOffset::kLsto);
    connection_piconet_clock_ =
        AtUint32LittleEndian(LinkQualityOffset::kConnectionPiconetClock);
    retransmission_count_ =
        AtUint32LittleEndian(LinkQualityOffset::kRetransmissionCount);
    no_rx_count_ = AtUint32LittleEndian(LinkQualityOffset::kNoRxCount);
    nak_count_ = AtUint32LittleEndian(LinkQualityOffset::kNakCount);
    last_tx_ack_timestamp_ =
        AtUint32LittleEndian(LinkQualityOffset::kLastTxAckTimestamp);
    flow_off_count_ = AtUint32LittleEndian(LinkQualityOffset::kFlowOffCount);
    last_flow_on_timestamp_ =
        AtUint32LittleEndian(LinkQualityOffset::kLastFlowOnTimestamp);
    buffer_overflow_bytes_ =
        AtUint32LittleEndian(LinkQualityOffset::kBufferOverflowBytes);
    buffer_underflow_bytes_ =
        AtUint32LittleEndian(LinkQualityOffset::kBufferUnderflowBytes);
  }
}
bool BqrLinkQualityEventBase::IsValid() const { return is_valid_; }

uint8_t BqrLinkQualityEventBase::GetPacketTypes() const {
  return packet_types_;
}

uint16_t BqrLinkQualityEventBase::GetConnectionHandle() const {
  return connection_handle_;
}

uint8_t BqrLinkQualityEventBase::GetConnectionRole() const {
  return connection_role_;
}

int8_t BqrLinkQualityEventBase::GetTxPowerLevel() const {
  return tx_power_level_;
}

int8_t BqrLinkQualityEventBase::GetRssi() const { return rssi_; }

uint8_t BqrLinkQualityEventBase::GetSnr() const { return snr_; }

uint8_t BqrLinkQualityEventBase::GetUnusedAfhChannelCount() const {
  return unused_afh_channel_count_;
}

uint8_t BqrLinkQualityEventBase::GetAfhSelectUnidealChannelCount() const {
  return afh_select_unideal_channel_count_;
}

uint16_t BqrLinkQualityEventBase::GetLsto() const { return lsto_; }

uint32_t BqrLinkQualityEventBase::GetConnectionPiconetClock() const {
  return connection_piconet_clock_;
}

uint32_t BqrLinkQualityEventBase::GetRetransmissionCount() const {
  return retransmission_count_;
}

uint32_t BqrLinkQualityEventBase::GetNoRxCount() const { return no_rx_count_; }

uint32_t BqrLinkQualityEventBase::GetNakCount() const { return nak_count_; }

uint32_t BqrLinkQualityEventBase::GetLastTxAckTimestamp() const {
  return last_tx_ack_timestamp_;
}

uint32_t BqrLinkQualityEventBase::GetFlowOffCount() const {
  return flow_off_count_;
}

uint32_t BqrLinkQualityEventBase::GetLastFlowOnTimestamp() const {
  return last_flow_on_timestamp_;
}

uint32_t BqrLinkQualityEventBase::GetBufferOverflowBytes() const {
  return buffer_overflow_bytes_;
}

uint32_t BqrLinkQualityEventBase::GetBufferUnderflowBytes() const {
  return buffer_underflow_bytes_;
}

BqrVersion BqrLinkQualityEventBase::GetVersion() const { return version_; }

std::string BqrLinkQualityEventBase::ToString() const {
  if (!is_valid_) {
    return "BqrLinkQualityEvent(Invalid)";
  }
  return "BqrLinkQualityEvent: " + ToBqrString();
}

std::string BqrLinkQualityEventBase::ToBqrString() const {
  std::stringstream ss;
  ss << BqrEvent::ToBqrString() << ", Handle: 0x" << std::hex << std::setw(4)
     << std::setfill('0') << connection_handle_ << ", "
     << BqrPacketTypeToString(packet_types_)
     << (connection_role_ ? ", Peripheral" : ", Central")
     << ", PwLv: " << std::dec << static_cast<int>(tx_power_level_)
     << ", RSSI: " << std::dec << static_cast<int>(rssi_)
     << ", SNR: " << std::dec << static_cast<int>(snr_)
     << ", UnusedCh: " << std::dec
     << static_cast<int>(unused_afh_channel_count_)
     << ", UnidealCh: " << std::dec
     << static_cast<int>(afh_select_unideal_channel_count_)
     << ", LSTO: " << std::dec << lsto_
     << ", Connection Piconet Clock: " << std::dec << connection_piconet_clock_
     << ", ReTx: " << std::dec << retransmission_count_
     << ", NoRx: " << std::dec << no_rx_count_ << ", NAK: " << std::dec
     << nak_count_ << ", LastTX: " << std::dec << last_tx_ack_timestamp_
     << ", FlowOff: " << std::dec << flow_off_count_
     << ", LastFlowOn: " << std::dec << last_flow_on_timestamp_
     << ", Overflow: " << std::dec << buffer_overflow_bytes_
     << ", Underflow: " << std::dec << buffer_underflow_bytes_;
  return ss.str();
}

}  // namespace bqr
}  // namespace bluetooth_hal
