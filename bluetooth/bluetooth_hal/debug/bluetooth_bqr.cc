/*
 * Copyright 2023 The Android Open Source Project
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

#define LOG_TAG "bthal.bqr"

#include "bluetooth_hal/debug/bluetooth_bqr.h"

#include "android-base/logging.h"
#include "android-base/stringprintf.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/util/files.h"
#include "bluetooth_hal/util/logging.h"

namespace bluetooth_hal {
namespace debug {

using ::android::base::StringPrintf;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::util::Logger;

static uint16_t kSupportedVersion = 256;
static constexpr int kVendorReportIdOffset = 4;

static constexpr int kVendorQualityEventIdOffset = 4;
static constexpr int kVendorQualityEventHRModeLength =
    1 /* vendor_specific_event */ + 1 /* parameter_total_length */ +
    1 /* sub_event */ + 1 /* quality_report_id */ +
    1 /* vendor_specific_quality_event_id */ + 2 /* conn_handle */ +
    132 /* HR mode counter statistics */;

// Bluetooth Bqr Energy Monitor Payload offset
static constexpr int kBqrEnergyMonitorPacketOffset = 4;

const std::string kBtActivitiesBqrEnergyLogPath =
    "/data/vendor/bluetooth/bt_activity_bqr_energy.txt";

void AddOctets(size_t bytes, uint64_t value, std::vector<uint8_t>& value_vec);

BtBqrEnergyRecoder bt_energy_instance;

BtBqrEnergyRecoder* BtBqrEnergyRecoder::GetInstacne() {
  return &bt_energy_instance;
}

std::string QualityReportIdToString(BqrQualityReportId quality_report_id) {
  switch (quality_report_id) {
    case BqrQualityReportId::kMonitorMode:
      return "Monitoring ";
    case BqrQualityReportId::kApproachLsto:
      return "Appro LSTO ";
    case BqrQualityReportId::kA2dpAudioChoppy:
      return "A2DP Choppy";
    case BqrQualityReportId::kScoVoiceChoppy:
      return "SCO Choppy ";
    case BqrQualityReportId::kLeAudioChoppy:
      return "LE Audio Choppy";
    default:
      return "Invalid    ";
  }
}

std::string PacketTypeToString(BqrPacketType packet_type) {
  switch (packet_type) {
    case BqrPacketType::kId:
      return "ID";
    case BqrPacketType::kNull:
      return "NULL";
    case BqrPacketType::kPoll:
      return "POLL";
    case BqrPacketType::kFhs:
      return "FHS";
    case BqrPacketType::kHv1:
      return "HV1";
    case BqrPacketType::kHv2:
      return "HV2";
    case BqrPacketType::kHv3:
      return "HV3";
    case BqrPacketType::kDv:
      return "DV";
    case BqrPacketType::kEv3:
      return "EV3";
    case BqrPacketType::kEv4:
      return "EV4";
    case BqrPacketType::kEv5:
      return "EV5";
    case BqrPacketType::k2Ev3:
      return "2EV3";
    case BqrPacketType::k2Ev5:
      return "2EV5";
    case BqrPacketType::k3Ev3:
      return "3EV3";
    case BqrPacketType::k3Ev5:
      return "3EV5";
    case BqrPacketType::kDm1:
      return "DM1";
    case BqrPacketType::kDh1:
      return "DH1";
    case BqrPacketType::kDm3:
      return "DM3";
    case BqrPacketType::kDh3:
      return "DH3";
    case BqrPacketType::kDm5:
      return "DM5";
    case BqrPacketType::kDh5:
      return "DH5";
    case BqrPacketType::kAux1:
      return "AUX1";
    case BqrPacketType::k2Dh1:
      return "2DH1";
    case BqrPacketType::k2Dh3:
      return "2DH3";
    case BqrPacketType::k2Dh5:
      return "2DH5";
    case BqrPacketType::k3Dh1:
      return "3DH1";
    case BqrPacketType::k3Dh3:
      return "3DH3";
    case BqrPacketType::k3Dh5:
      return "3DH5";
    case BqrPacketType::k4Dh1:
      return "4DH1";
    case BqrPacketType::k4Dh3:
      return "4DH3";
    case BqrPacketType::k4Dh5:
      return "4DH5";
    case BqrPacketType::k8Dh1:
      return "8DH1";
    case BqrPacketType::k8Dh3:
      return "8DH3";
    case BqrPacketType::k8Dh5:
      return "8DH5";
    case BqrPacketType::k4Ev3:
      return "4EV3";
    case BqrPacketType::k4Ev5:
      return "4EV5";
    case BqrPacketType::k8Ev3:
      return "8EV3";
    case BqrPacketType::k8Ev5:
      return "8EV5";
    case BqrPacketType::kIso:
      return "ISO";
    default:
      return "UnKnown ";
  }
}

// Sub-event code = 0x58 [Quality_Report_Id = 0x01 ~ 0x04, and 0x07 ~ 0x08, Link
// Quality related event]
void ParseLinkQualityRelatedEvt(const HalPacket& packet) {
  if (kSupportedVersion < BQR_VERSION_V4) {
    // Only parse the event containing Vendor Specific parameters
    if (packet.size() < sizeof(BqrLinkQualityEventV3AndBackward)) {
      return;
    }

    const BqrLinkQualityEventV3AndBackward* p_bqr_link_quality_event =
        reinterpret_cast<const BqrLinkQualityEventV3AndBackward*>(
            packet.data());

    LOG(WARNING)
        << __func__ << ": Generic Parameters: "
        << QualityReportIdToString(static_cast<BqrQualityReportId>(
               p_bqr_link_quality_event->quality_report_id))
        << StringPrintf(", Handle: 0x%04x",
                        p_bqr_link_quality_event->connection_handle)
        << ", "
        << PacketTypeToString(static_cast<BqrPacketType>(
               p_bqr_link_quality_event->packet_types))
        << StringPrintf(", %s",
                        ((p_bqr_link_quality_event->connection_role == 0)
                             ? "Central"
                             : "Peripheral "))
        << ", PwLv: "
        << std::to_string(p_bqr_link_quality_event->tx_power_level)
        << ", RSSI: " << std::to_string(p_bqr_link_quality_event->rssi)
        << ", SNR: " << std::to_string(p_bqr_link_quality_event->snr)
        << ", UnusedCh: "
        << std::to_string(p_bqr_link_quality_event->unused_afh_channel_count)
        << ", UnidealCh: "
        << std::to_string(
               p_bqr_link_quality_event->afh_select_unideal_channel_count)
        << ", ReTx: "
        << std::to_string(p_bqr_link_quality_event->retransmission_count)
        << ", NoRX: " << std::to_string(p_bqr_link_quality_event->no_rx_count)
        << ", NAK: " << std::to_string(p_bqr_link_quality_event->nak_count)
        << ", FlowOff: "
        << std::to_string(p_bqr_link_quality_event->flow_off_count)
        << ", OverFlow: "
        << std::to_string(p_bqr_link_quality_event->buffer_overflow_bytes)
        << ", UndFlow: "
        << std::to_string(p_bqr_link_quality_event->buffer_underflow_bytes)
        << ".";

    LOG(WARNING)
        << __func__ << ": Vendor Parameters: "
        << QualityReportIdToString(static_cast<BqrQualityReportId>(
               p_bqr_link_quality_event->quality_report_id))
        << StringPrintf(", Handle: 0x%04x",
                        p_bqr_link_quality_event->connection_handle)
        << ", RSSI_C0: "
        << std::to_string(p_bqr_link_quality_event->rssi_for_core0)
        << ", RSSI_C1: "
        << std::to_string(p_bqr_link_quality_event->rssi_for_core1)
        << ", TxPw_C0: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core0)
        << ", ReTxPw_C0: "
        << std::to_string(p_bqr_link_quality_event->retx_power_for_core0)
        << ", TxPw_C1: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core1)
        << ", ReTxPw_C1: "
        << std::to_string(p_bqr_link_quality_event->retx_power_for_core1)
        << StringPrintf(
               ", BFTx: 0x%02x, BFReTx: 0x%02x, DivTx: 0x%02x, DivReTx: "
               "0x%02x.",
               p_bqr_link_quality_event->bf_state_for_tx,
               p_bqr_link_quality_event->bf_state_for_retx,
               p_bqr_link_quality_event->div_state_for_tx,
               p_bqr_link_quality_event->div_state_for_retx);
  } else if (kSupportedVersion < BQR_VERSION_V5) {
    // Only parse the event containing Vendor Specific parameters
    if (packet.size() < sizeof(BqrLinkQualityEventV4)) {
      return;
    }

    const BqrLinkQualityEventV4* p_bqr_link_quality_event =
        reinterpret_cast<const BqrLinkQualityEventV4*>(packet.data());

    LOG(WARNING)
        << __func__ << ": Generic Parameters: "
        << QualityReportIdToString(static_cast<BqrQualityReportId>(
               p_bqr_link_quality_event->quality_report_id))
        << StringPrintf(", Handle: 0x%04x",
                        p_bqr_link_quality_event->connection_handle)
        << ", "
        << PacketTypeToString(static_cast<BqrPacketType>(
               p_bqr_link_quality_event->packet_types))
        << StringPrintf(", %s",
                        ((p_bqr_link_quality_event->connection_role == 0)
                             ? "Central"
                             : "Peripheral "))
        << ", PwLv: "
        << std::to_string(p_bqr_link_quality_event->tx_power_level)
        << ", RSSI: " << std::to_string(p_bqr_link_quality_event->rssi)
        << ", SNR: " << std::to_string(p_bqr_link_quality_event->snr)
        << ", UnusedCh: "
        << std::to_string(p_bqr_link_quality_event->unused_afh_channel_count)
        << ", UnidealCh: "
        << std::to_string(
               p_bqr_link_quality_event->afh_select_unideal_channel_count)
        << ", ReTx: "
        << std::to_string(p_bqr_link_quality_event->retransmission_count)
        << ", NoRX: " << std::to_string(p_bqr_link_quality_event->no_rx_count)
        << ", NAK: " << std::to_string(p_bqr_link_quality_event->nak_count)
        << ", FlowOff: "
        << std::to_string(p_bqr_link_quality_event->flow_off_count)
        << ", OverFlow: "
        << std::to_string(p_bqr_link_quality_event->buffer_overflow_bytes)
        << ", UndFlow: "
        << std::to_string(p_bqr_link_quality_event->buffer_underflow_bytes)
        // for BQR v4 iso le audio
        << ", TxTotal: "
        << std::to_string(p_bqr_link_quality_event->tx_total_packets)
        << ", TxUnAcked: "
        << std::to_string(p_bqr_link_quality_event->tx_unacked_packets)
        << ", TxFlushed: "
        << std::to_string(p_bqr_link_quality_event->tx_flushed_packets)
        << ", TxLastSubEvent: "
        << std::to_string(p_bqr_link_quality_event->tx_last_subevent_packets)
        << ", CRCError: "
        << std::to_string(p_bqr_link_quality_event->crc_error_packets)
        << ", RxDuplicate: "
        << std::to_string(p_bqr_link_quality_event->rx_duplicate_packets)
        << ".";

    LOG(WARNING)
        << __func__ << ": Vendor Parameters: "
        << QualityReportIdToString(static_cast<BqrQualityReportId>(
               p_bqr_link_quality_event->quality_report_id))
        << StringPrintf(", Handle: 0x%04x",
                        p_bqr_link_quality_event->connection_handle)
        << ", RSSI_C0: "
        << std::to_string(p_bqr_link_quality_event->rssi_for_core0)
        << ", RSSI_C1: "
        << std::to_string(p_bqr_link_quality_event->rssi_for_core1)
        << ", TxPw_C0: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core0)
        << ", ReTxPw_C0: "
        << std::to_string(p_bqr_link_quality_event->retx_power_for_core0)
        << ", TxPw_C1: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core1)
        << ", ReTxPw_C1: "
        << std::to_string(p_bqr_link_quality_event->retx_power_for_core1)
        << StringPrintf(
               ", BFTx: 0x%02x, BFReTx: 0x%02x, DivTx: 0x%02x, DivReTx: 0x%02x",
               p_bqr_link_quality_event->bf_state_for_tx,
               p_bqr_link_quality_event->bf_state_for_retx,
               p_bqr_link_quality_event->div_state_for_tx,
               p_bqr_link_quality_event->div_state_for_retx)
        << StringPrintf(", Overall_link_quality: %u",
                        p_bqr_link_quality_event->overall_link_quality)
        << StringPrintf(", Tx_link_quality: %u",
                        p_bqr_link_quality_event->tx_link_quality)
        << StringPrintf(", Rx_link_quality: %u.",
                        p_bqr_link_quality_event->rx_link_quality);
  } else if (kSupportedVersion < BQR_VERSION_V6) {
    if (packet.size() < sizeof(BqrLinkQualityEventV5)) {
      return;
    }

    const BqrLinkQualityEventV5* p_bqr_link_quality_event =
        reinterpret_cast<const BqrLinkQualityEventV5*>(packet.data());

    LOG(WARNING)
        << __func__ << ": Generic Parameters: "
        << QualityReportIdToString(static_cast<BqrQualityReportId>(
               p_bqr_link_quality_event->quality_report_id))
        << StringPrintf(", Handle: 0x%04x",
                        p_bqr_link_quality_event->connection_handle)
        << ", "
        << PacketTypeToString(static_cast<BqrPacketType>(
               p_bqr_link_quality_event->packet_types))
        << StringPrintf(", %s",
                        ((p_bqr_link_quality_event->connection_role == 0)
                             ? "Central"
                             : "Peripheral "))
        << ", PwLv: "
        << std::to_string(p_bqr_link_quality_event->tx_power_level)
        << ", RSSI: " << std::to_string(p_bqr_link_quality_event->rssi)
        << ", SNR: " << std::to_string(p_bqr_link_quality_event->snr)
        << ", UnusedCh: "
        << std::to_string(p_bqr_link_quality_event->unused_afh_channel_count)
        << ", UnidealCh: "
        << std::to_string(
               p_bqr_link_quality_event->afh_select_unideal_channel_count)
        << ", ReTx: "
        << std::to_string(p_bqr_link_quality_event->retransmission_count)
        << ", NoRX: " << std::to_string(p_bqr_link_quality_event->no_rx_count)
        << ", NAK: " << std::to_string(p_bqr_link_quality_event->nak_count)
        << ", FlowOff: "
        << std::to_string(p_bqr_link_quality_event->flow_off_count)
        << ", OverFlow: "
        << std::to_string(p_bqr_link_quality_event->buffer_overflow_bytes)
        << ", UndFlow: "
        << std::to_string(p_bqr_link_quality_event->buffer_underflow_bytes)
        << ", failedCount: "
        << std::to_string(p_bqr_link_quality_event->call_failed_item_count)
        // for BQR v4 iso le audio
        << ", TxTotal: "
        << std::to_string(p_bqr_link_quality_event->tx_total_packets)
        << ", TxUnAcked: "
        << std::to_string(p_bqr_link_quality_event->tx_unacked_packets)
        << ", TxFlushed: "
        << std::to_string(p_bqr_link_quality_event->tx_flushed_packets)
        << ", TxLastSubEvent: "
        << std::to_string(p_bqr_link_quality_event->tx_last_subevent_packets)
        << ", CRCError: "
        << std::to_string(p_bqr_link_quality_event->crc_error_packets)
        << ", RxDuplicate: "
        << std::to_string(p_bqr_link_quality_event->rx_duplicate_packets)
        << ".";

    LOG(WARNING)
        << __func__ << ": Vendor Parameters: "
        << QualityReportIdToString(static_cast<BqrQualityReportId>(
               p_bqr_link_quality_event->quality_report_id))
        << StringPrintf(", Handle: 0x%04x",
                        p_bqr_link_quality_event->connection_handle)
        << ", RSSI_C0: "
        << std::to_string(p_bqr_link_quality_event->rssi_for_core0)
        << ", RSSI_C1: "
        << std::to_string(p_bqr_link_quality_event->rssi_for_core1)
        << ", TxPw_C0: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core0)
        << ", ReTxPw_C0: "
        << std::to_string(p_bqr_link_quality_event->retx_power_for_core0)
        << ", TxPw_C1: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core1)
        << ", ReTxPw_C1: "
        << std::to_string(p_bqr_link_quality_event->retx_power_for_core1)
        << StringPrintf(
               ", BFTx: 0x%02x, BFReTx: 0x%02x, DivTx: 0x%02x, DivReTx: 0x%02x",
               p_bqr_link_quality_event->bf_state_for_tx,
               p_bqr_link_quality_event->bf_state_for_retx,
               p_bqr_link_quality_event->div_state_for_tx,
               p_bqr_link_quality_event->div_state_for_retx)
        << StringPrintf(", Overall_link_quality: %u",
                        p_bqr_link_quality_event->overall_link_quality)
        << StringPrintf(", Tx_link_quality: %u",
                        p_bqr_link_quality_event->tx_link_quality)
        << StringPrintf(", Rx_link_quality: %u.",
                        p_bqr_link_quality_event->rx_link_quality);
  } else {  // BQRv6
    if (packet.size() < sizeof(BqrLinkQualityEventV6)) {
      return;
    }

    const BqrLinkQualityEventV6* p_bqr_link_quality_event =
        reinterpret_cast<const BqrLinkQualityEventV6*>(packet.data());

    LOG(WARNING)
        << __func__ << ": Generic Parameters: "
        << QualityReportIdToString(static_cast<BqrQualityReportId>(
               p_bqr_link_quality_event->quality_report_id))
        << StringPrintf(", Handle: 0x%04x",
                        p_bqr_link_quality_event->connection_handle)
        << ", "
        << PacketTypeToString(static_cast<BqrPacketType>(
               p_bqr_link_quality_event->packet_types))
        << StringPrintf(", %s",
                        ((p_bqr_link_quality_event->connection_role == 0)
                             ? "Central"
                             : "Peripheral "))
        << ", PwLv: "
        << std::to_string(p_bqr_link_quality_event->tx_power_level)
        << ", RSSI: " << std::to_string(p_bqr_link_quality_event->rssi)
        << ", SNR: " << std::to_string(p_bqr_link_quality_event->snr)
        << ", UnusedCh: "
        << std::to_string(p_bqr_link_quality_event->unused_afh_channel_count)
        << ", UnidealCh: "
        << std::to_string(
               p_bqr_link_quality_event->afh_select_unideal_channel_count)
        << ", ReTx: "
        << std::to_string(p_bqr_link_quality_event->retransmission_count)
        << ", NoRX: " << std::to_string(p_bqr_link_quality_event->no_rx_count)
        << ", NAK: " << std::to_string(p_bqr_link_quality_event->nak_count)
        << ", FlowOff: "
        << std::to_string(p_bqr_link_quality_event->flow_off_count)
        << ", OverFlow: "
        << std::to_string(p_bqr_link_quality_event->buffer_overflow_bytes)
        << ", UndFlow: "
        << std::to_string(p_bqr_link_quality_event->buffer_underflow_bytes)
        << ", failedCount: "
        << std::to_string(p_bqr_link_quality_event->call_failed_item_count)
        // for BQR v4 iso le audio
        << ", TxTotal: "
        << std::to_string(p_bqr_link_quality_event->tx_total_packets)
        << ", TxUnAcked: "
        << std::to_string(p_bqr_link_quality_event->tx_unacked_packets)
        << ", TxFlushed: "
        << std::to_string(p_bqr_link_quality_event->tx_flushed_packets)
        << ", TxLastSubEvent: "
        << std::to_string(p_bqr_link_quality_event->tx_last_subevent_packets)
        << ", CRCError: "
        << std::to_string(p_bqr_link_quality_event->crc_error_packets)
        << ", RxDuplicate: "
        << std::to_string(p_bqr_link_quality_event->rx_duplicate_packets)
        << ", RxUnreceived: "
        << std::to_string(p_bqr_link_quality_event->rx_unreceived_packets)
        << ", coex_info_mask: "
        << std::to_string(p_bqr_link_quality_event->coex_info_mask) << ".";

    LOG(WARNING)
        << __func__ << ": Vendor Parameters: "
        << QualityReportIdToString(static_cast<BqrQualityReportId>(
               p_bqr_link_quality_event->quality_report_id))
        << StringPrintf(", Handle: 0x%04x",
                        p_bqr_link_quality_event->connection_handle)
        << ", RSSI_C0: "
        << std::to_string(p_bqr_link_quality_event->rssi_for_core0)
        << ", RSSI_C1: "
        << std::to_string(p_bqr_link_quality_event->rssi_for_core1)
        << ", TxPw_C0: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core0)
        << ", ReTxPw_C0: "
        << std::to_string(p_bqr_link_quality_event->retx_power_for_core0)
        << ", TxPw_C1: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core1)
        << ", ReTxPw_C1: "
        << std::to_string(p_bqr_link_quality_event->retx_power_for_core1)
        << StringPrintf(
               ", BFTx: 0x%02x, BFReTx: 0x%02x, DivTx: 0x%02x, DivReTx: 0x%02x",
               p_bqr_link_quality_event->bf_state_for_tx,
               p_bqr_link_quality_event->bf_state_for_retx,
               p_bqr_link_quality_event->div_state_for_tx,
               p_bqr_link_quality_event->div_state_for_retx)
        << StringPrintf(", Overall_link_quality: %u",
                        p_bqr_link_quality_event->overall_link_quality)
        << StringPrintf(", Tx_link_quality: %u",
                        p_bqr_link_quality_event->tx_link_quality)
        << StringPrintf(", Rx_link_quality: %u",
                        p_bqr_link_quality_event->rx_link_quality)
        << ", TotalTx_pkts_c0: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core0)
        << ", TotalTx_pkts_c1: "
        << std::to_string(p_bqr_link_quality_event->tx_power_for_core1)
        << ", TotalTx_pkts_beamforming: "
        << std::to_string(p_bqr_link_quality_event->total_tx_pkts_beamforming)
        << ".";
  }
}

void updateControllerCapability(const HalPacket& packet) {
  if (packet.size() < 16) {
    return;
  }
  kSupportedVersion = (packet[15] << 8) + packet[14];
  LOG(INFO) << __func__ << ": Vendor capability supported version: "
            << static_cast<uint16_t>(kSupportedVersion) << ".";
}

void ParseVendorSpecificQualityEvt(const HalPacket& packet) {
  if (packet.size() <= kVendorQualityEventIdOffset) {
    LOG(ERROR) << __func__ << ": Invalid length of BQR vendor specific event!";
    return;
  }
  auto vendor_quality_event_id =
      static_cast<VendorReportId>(packet[kVendorQualityEventIdOffset]);
  switch (vendor_quality_event_id) {
    case VendorReportId::kHrMode:
      ParseHRModeStatisticLog(packet);
      break;
    default:
      LOG(ERROR) << __func__ << ": Invalid vendor specific quality id";
  }
}

void ParseHRModeStatisticLog(const HalPacket& packet) {
  if (packet.size() < kVendorQualityEventHRModeLength) {
    LOG(ERROR) << __func__
               << ": Invalid length of HR Mode statistic specific event!";
    return;
  }
  const BqrVendorSpecificEventHRMode* p_bqr_hr_evt =
      reinterpret_cast<const BqrVendorSpecificEventHRMode*>(packet.data());

  LOG(WARNING) << __func__
               << ": Vendor Specific quality event: HR Mode statistic"
               << StringPrintf(", Handle: 0x%04x", p_bqr_hr_evt->conn_handle)
               << StringPrintf(", rx_null_cnt: %d", p_bqr_hr_evt->rx_null_cnt)
               << StringPrintf(", rx_poll_cnt: %d", p_bqr_hr_evt->rx_poll_cnt)
               << StringPrintf(", rx_dm1_cnt: %d", p_bqr_hr_evt->rx_dm1_cnt)

               << StringPrintf(", tx_null_cnt: %d", p_bqr_hr_evt->tx_null_cnt)
               << StringPrintf(", tx_poll_cnt: %d", p_bqr_hr_evt->tx_poll_cnt)
               << StringPrintf(", tx_dm1_cnt: %d", p_bqr_hr_evt->tx_dm1_cnt)

               << StringPrintf(", rx_hr_2dh1: %d", p_bqr_hr_evt->rx_hr_2dh1)
               << StringPrintf(", rx_hr_4dh1: %d", p_bqr_hr_evt->rx_hr_4dh1)
               << StringPrintf(", rx_hr_8dh1: %d", p_bqr_hr_evt->rx_hr_8dh1)
               << StringPrintf(", rx_hr_2dh3: %d", p_bqr_hr_evt->rx_hr_2dh3)
               << StringPrintf(", rx_hr_4dh3: %d", p_bqr_hr_evt->rx_hr_4dh3)
               << StringPrintf(", rx_hr_8dh3: %d", p_bqr_hr_evt->rx_hr_8dh3)
               << StringPrintf(", rx_hr_2dh5: %d", p_bqr_hr_evt->rx_hr_2dh5)
               << StringPrintf(", rx_hr_4dh5: %d", p_bqr_hr_evt->rx_hr_4dh5)
               << StringPrintf(", rx_hr_8dh5: %d", p_bqr_hr_evt->rx_hr_8dh5)

               << StringPrintf(", tx_hr_2dh1: %d", p_bqr_hr_evt->tx_hr_2dh1)
               << StringPrintf(", tx_hr_4dh1: %d", p_bqr_hr_evt->tx_hr_4dh1)
               << StringPrintf(", tx_hr_8dh1: %d", p_bqr_hr_evt->tx_hr_8dh1)
               << StringPrintf(", tx_hr_2dh3: %d", p_bqr_hr_evt->tx_hr_2dh3)
               << StringPrintf(", tx_hr_4dh3: %d", p_bqr_hr_evt->tx_hr_4dh3)
               << StringPrintf(", tx_hr_8dh3: %d", p_bqr_hr_evt->tx_hr_8dh3)
               << StringPrintf(", tx_hr_2dh5: %d", p_bqr_hr_evt->tx_hr_2dh5)
               << StringPrintf(", tx_hr_4dh5: %d", p_bqr_hr_evt->tx_hr_4dh5)
               << StringPrintf(", tx_hr_8dh5: %d.", p_bqr_hr_evt->tx_hr_8dh5);
}

void ParseVendorSpecificTraceEvt(const HalPacket& packet) {
  if (packet.size() <= kVendorReportIdOffset) {
    LOG(ERROR) << __func__ << ": Invalid length of BQR vendor specific event!";
    return;
  }

  auto vendor_report_id =
      static_cast<VendorReportId>(packet[kVendorReportIdOffset]);
  switch (vendor_report_id) {
    case VendorReportId::kA2dpLatencyMeasurement:
      ParseA2DPLatencyMeasurement(packet);
      break;
    default:
      LOG(ERROR) << __func__ << ": Invalid vendor report id.";
  }
}

void ParseA2DPLatencyMeasurement(const HalPacket& packet) {
  const BqrVsteA2dpLatencyMeasurement* p_bqr_a2dp_latency_evt =
      reinterpret_cast<const BqrVsteA2dpLatencyMeasurement*>(packet.data());

  LOG(WARNING) << __func__
               << ": Vendor Specific Trace Event: A2DP Latency Measurement"
               << StringPrintf(", Handle: 0x%04x",
                               p_bqr_a2dp_latency_evt->conn_handle)
               << StringPrintf(", Num_Packets_Logged: %d, Base_Timestamp: %d.",
                               p_bqr_a2dp_latency_evt->num_packets_logged,
                               p_bqr_a2dp_latency_evt->base_timestamp);

  if (p_bqr_a2dp_latency_evt->num_packets_logged == 0) return;

  const A2DPLatency* a2dp_latencies =
      &(p_bqr_a2dp_latency_evt->a2dp_latencies[0]);
  uint8_t num_packets = p_bqr_a2dp_latency_evt->num_packets_logged;
  double first_transmit;
  double first_ack;
  double final_transmit;
  double first_transmit_avg = 0;
  double first_ack_avg = 0;
  double final_transmit_avg = 0;

  for (uint8_t i = 0; i < num_packets; ++i) {
    // 1 bt slot = 0.625 ms
    first_transmit =
        (double)a2dp_latencies[i].first_packet_transmit_delay * 0.625;
    first_ack = (double)a2dp_latencies[i].first_packet_ack_delay * 0.625;
    final_transmit =
        (double)a2dp_latencies[i].final_packet_transmit_delay * 0.625;

    LOG(WARNING) << __func__
                 << StringPrintf(
                        ": Packet[%d/%d], Packet_Entry_Time_Offset: %3.3f,"
                        "First_Packet_Transmit_Delay: %3.3f, "
                        "First_Packet_Ack_Delay: %3.3f,"
                        "Final_Packet_Transmit_Delay: %3.3f.",
                        i + 1, num_packets,
                        (double)a2dp_latencies[i].packet_entry_time_offset *
                            0.625,
                        first_transmit, first_ack, final_transmit);

    first_transmit_avg += first_transmit;
    first_ack_avg += first_ack;
    final_transmit_avg += final_transmit;
  }

  first_transmit_avg /= num_packets;
  first_ack_avg /= num_packets;
  final_transmit_avg /= num_packets;
  LOG(WARNING)
      << __func__
      << StringPrintf(
             ": Average, Packet_num: %d, First_Packet_Transmit_Delay_avg: "
             "%3.3f, First_Packet_Ack_Delay_avg: %3.3f, "
             "Final_Packet_Transmit_Delay_avg: %3.3f.",
             num_packets, first_transmit_avg, first_ack_avg,
             final_transmit_avg);
}

// Sub-event code = 0x58 [Quality_Report_Id = 0x09~0x0A, Advance RF Stats event]
void ParseAdvanceRFStatsEvt(const HalPacket& packet) {
  if (packet.size() < sizeof(BqrAdvanceRFStatsEvent)) {
    LOG(WARNING) << __func__ << ": Packet size() error.";
    return;
  }

  const BqrAdvanceRFStatsEvent* p_bqr_rf_stats_event =
      reinterpret_cast<const BqrAdvanceRFStatsEvent*>(packet.data());

  if (p_bqr_rf_stats_event->ext_info == BQR_RFSTATS_EXT_INFO_V6) {
    LOG(WARNING) << __func__ << ": Advance RF Stats: Time Period:"
                 << std::to_string(p_bqr_rf_stats_event->tm_period) << " ms"
                 << ", Extension id: "
                 << std::to_string(p_bqr_rf_stats_event->ext_info)
                 << ", TW_Pw_iPA_BF: "
                 << std::to_string(p_bqr_rf_stats_event->tx_pw_ipa_bf)
                 << ", TW_Pw_ePA_BF: "
                 << std::to_string(p_bqr_rf_stats_event->tx_pw_epa_bf)
                 << ", TW_Pw_iPA_Div: "
                 << std::to_string(p_bqr_rf_stats_event->tx_pw_ipa_div)
                 << ", TW_Pw_ePA_Div: "
                 << std::to_string(p_bqr_rf_stats_event->tx_pw_epa_div)
                 << ", RSSI_Chain_>-50: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_50)
                 << ", RSSI_Chain_-50_-55: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_50_55)
                 << ", RSSI_Chain_-55_-60: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_55_60)
                 << ", RSSI_Chain_-60_-65: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_60_65)
                 << ", RSSI_Chain_-65_-70: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_65_70)
                 << ", RSSI_Chain_-70_-75: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_70_75)
                 << ", RSSI_Chain_-75_-80: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_75_80)
                 << ", RSSI_Chain_-80_-85: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_80_85)
                 << ", RSSI_Chain_-85_-90: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_85_90)
                 << ", RSSI_Chain_<-90: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_ch_90)
                 << ", RSSI_Delta_<2: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_delta_2_down)
                 << ", RSSI_Delta_2_5: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_delta_2_5)
                 << ", RSSI_Delta_5_8: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_delta_5_8)
                 << ", RSSI_Delta_8_11: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_delta_8_11)
                 << ", RSSI_Delta_>11: "
                 << std::to_string(p_bqr_rf_stats_event->rssi_delta_11_up)
                 << ".";
  } else {
    LOG(WARNING) << __func__
                 << ": Advance RF Stats: Invalid Extension Info ID.";
  }
}

inline uint8_t stream_to_int8(const HalPacket& event, uint8_t* offset_ptr) {
  uint8_t offset = *offset_ptr;
  uint8_t number = 0;
  number = ((uint8_t)(event[offset]));
  *offset_ptr += 1;
  return number;
}

inline uint16_t stream_to_int16(const HalPacket& event, uint8_t* offset_ptr) {
  uint8_t offset = *offset_ptr;
  uint16_t number = 0;
  number = ((uint16_t)(event[offset])) + ((uint16_t)(event[offset + 1] << 8));
  *offset_ptr += 2;
  return number;
}

inline uint32_t stream_to_int32(const HalPacket& event, uint8_t* offset_ptr) {
  uint8_t offset = *offset_ptr;
  uint32_t number = 0;
  number = ((uint16_t)(event[offset])) + ((uint16_t)(event[offset + 1] << 8)) +
           ((uint16_t)(event[offset + 2] << 16)) +
           ((uint16_t)(event[offset + 3] << 24));
  *offset_ptr += 4;
  return number;
}

void BtBqrEnergyRecoder::StartLogging() {
  LOG(INFO) << __func__;
  if (HalConfigLoader::GetLoader().IsEnergyControllerLoggingSupported()) {
    bt_activities_bqr_energy_log_path_ =
        std::move(kBtActivitiesBqrEnergyLogPath);
    open_new_energy_log_file();
  }
}

void BtBqrEnergyRecoder::StopLogging() {
  LOG(INFO) << __func__;
  if (HalConfigLoader::GetLoader().IsEnergyControllerLoggingSupported()) {
    LOG(DEBUG) << __func__ << ": Closing bqr energy log data at "
               << bt_activities_bqr_energy_log_path_ << ".";
    os::CloseLogFileStream(bqr_energy_activity_ostream_);
  }
}

void BtBqrEnergyRecoder::open_new_energy_log_file() {
  LOG(INFO) << __func__;
  os::CloseLogFileStream(bqr_energy_activity_ostream_);
  os::CreateLogFile(bt_activities_bqr_energy_log_path_,
                    bqr_energy_activity_ostream_);
  bqr_energy_activity_ostream_
      << "TimeStamp" << ", Batt_Per" << ", Avg_Cur_Pwr" << ", BEr_Tx_Plv"
      << ", Le_Tx_Plv" << ", Idle_Tm" << ", Act_Tm" << ", Act_Cnt"
      << ", BEr_Tx_Tm" << ", BEr_Tx_Cnt" << ", BEr_Rx_Tm" << ", BEr_Rx_Cnt"
      << ", Le_Tx_Tm" << ", Le_Tx_Cnt" << ", Le_Rx_Tm" << ", Le_Rx_Cnt"
      << std::endl;

  if (!bqr_energy_activity_ostream_.flush()) {
    LOG(ERROR) << __func__ << ": Failed to flush, error: \"" << strerror(errno)
               << "\".";
  }
}

void BtBqrEnergyRecoder::update_bqr_energy_report(bt_energy_sector_t& stat) {
  if (!HalConfigLoader::GetLoader().IsEnergyControllerLoggingSupported()) {
    return;
  }

  LOG(INFO) << __func__
            << ": Avg_Cur_Pwr: " << stat.entries.average_current_consumption
            << ", BEr_Tx_Plv: " << stat.entries.br_edr_tx_average_power_level
            << " dBm"
            << ", Le_Tx_Plv: " << stat.entries.le_tx_average_power_level << ".";

  packet_counter_++;
  if (packet_counter_ > kMaxPacketsPerFile_) {
    LOG(INFO) << __func__
              << ": Exceed kMaxPacketsPerFile_, open another new log file.";

    open_new_energy_log_file();
    packet_counter_ = 0;
  }
  bqr_energy_activity_ostream_
      << stat.timestamp << ", " << batt_level_.c_str() << ", "
      << stat.entries.average_current_consumption << ", "
      << std::to_string(stat.entries.br_edr_tx_average_power_level) << ", "
      << std::to_string(stat.entries.le_tx_average_power_level) << ", "
      << std::to_string(stat.entries.idle_total_time) << ", "
      << std::to_string(stat.entries.active_total_time) << ", "
      << std::to_string(stat.entries.active_state_enter_count) << ", "
      << std::to_string(stat.entries.br_edr_tx_total_time) << ", "
      << std::to_string(stat.entries.br_edr_tx_state_enter_count) << ", "
      << std::to_string(stat.entries.br_edr_rx_total_time) << ", "
      << std::to_string(stat.entries.br_edr_rx_state_enter_count) << ", "
      << std::to_string(stat.entries.le_tx_total_time) << ", "
      << std::to_string(stat.entries.le_tx_state_enter_count) << ", "
      << std::to_string(stat.entries.le_rx_total_time) << ", "
      << std::to_string(stat.entries.le_rx_state_enter_count) << std::endl;

  if (!bqr_energy_activity_ostream_.flush()) {
    LOG(ERROR) << __func__ << ": Failed to flush, error: \"" << strerror(errno)
               << "\".";
  }
}

void BtBqrEnergyRecoder::update_bqr_energy_report(bt_energy_sectorv6_t& stat) {
  if (!HalConfigLoader::GetLoader().IsEnergyControllerLoggingSupported()) {
    return;
  }
  LOG(INFO) << __func__
            << ": Avg_Cur_Pwr: " << stat.entries.average_current_consumption
            << ", BEr_Tx_Plv: " << stat.entries.br_edr_tx_average_power_level
            << " dBm"
            << ", Le_Tx_Plv: " << stat.entries.le_tx_average_power_level << ".";

  packet_counter_++;
  if (packet_counter_ > kMaxPacketsPerFile_) {
    LOG(INFO) << __func__
              << ": Exceed kMaxPacketsPerFile_, open another new log file.";
    open_new_energy_log_file();
    packet_counter_ = 0;
  }
  bqr_energy_activity_ostream_
      << stat.timestamp << ", " << batt_level_.c_str() << ", "
      << stat.entries.average_current_consumption << ", "
      << std::to_string(stat.entries.br_edr_tx_average_power_level) << ", "
      << std::to_string(stat.entries.le_tx_average_power_level) << ", "
      << std::to_string(stat.entries.idle_total_time) << ", "
      << std::to_string(stat.entries.active_total_time) << ", "
      << std::to_string(stat.entries.active_state_enter_count) << ", "
      << std::to_string(stat.entries.br_edr_tx_total_time) << ", "
      << std::to_string(stat.entries.br_edr_tx_state_enter_count) << ", "
      << std::to_string(stat.entries.br_edr_rx_total_time) << ", "
      << std::to_string(stat.entries.br_edr_rx_state_enter_count) << ", "
      << std::to_string(stat.entries.le_tx_total_time) << ", "
      << std::to_string(stat.entries.le_tx_state_enter_count) << ", "
      << std::to_string(stat.entries.le_rx_total_time) << ", "
      << std::to_string(stat.entries.le_rx_state_enter_count) << ", "
      << std::to_string(stat.entries.report_time_duration) << ", "
      << std::to_string(stat.entries.rx_active_one_chain_time) << ", "
      << std::to_string(stat.entries.rx_active_two_chain_time) << ", "
      << std::to_string(stat.entries.tx_ipa_active_one_chain_time) << ", "
      << std::to_string(stat.entries.tx_ipa_active_two_chain_time) << ", "
      << std::to_string(stat.entries.tx_xpa_active_one_chain_time) << ", "
      << std::to_string(stat.entries.tx_xpa_active_two_chain_time) << std::endl;

  if (!bqr_energy_activity_ostream_.flush()) {
    LOG(ERROR) << __func__ << ": Failed to flush, error: \"" << strerror(errno)
               << "\".";
  }
}

void BtBqrEnergyRecoder::ParseBqrEnergyMonitorEvt(
    const HalPacket& energy_event) {
  if (kSupportedVersion < BQR_VERSION_V6) {
    if (energy_event.size() < sizeof(BqrEnergyMonitoringEvent)) {
      return;
    }

    bt_energy_sector_t bqr_energy_sector;
    BqrEnergyMonitoringEvent bqr_energy_packet;
    uint8_t offset = kBqrEnergyMonitorPacketOffset;

    std::string engery_timestamp = Logger::GetLogFormatTimestamp();

    // Parse Energy Monitor Event
    bqr_energy_packet.average_current_consumption =
        stream_to_int16(energy_event, &offset);
    bqr_energy_packet.idle_total_time = stream_to_int32(energy_event, &offset);
    bqr_energy_packet.idle_state_enter_count =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.active_total_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.active_state_enter_count =
        stream_to_int32(energy_event, &offset);

    bqr_energy_packet.br_edr_tx_total_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.br_edr_tx_state_enter_count =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.br_edr_tx_average_power_level =
        stream_to_int8(energy_event, &offset);
    bqr_energy_packet.br_edr_rx_total_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.br_edr_rx_state_enter_count =
        stream_to_int32(energy_event, &offset);

    bqr_energy_packet.le_tx_total_time = stream_to_int32(energy_event, &offset);
    bqr_energy_packet.le_tx_state_enter_count =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.le_tx_average_power_level =
        stream_to_int8(energy_event, &offset);
    bqr_energy_packet.le_rx_total_time = stream_to_int32(energy_event, &offset);
    bqr_energy_packet.le_rx_state_enter_count =
        stream_to_int32(energy_event, &offset);

    // Update bqr_energy_sector
    bqr_energy_sector.timestamp = engery_timestamp;
    bqr_energy_sector.entries = bqr_energy_packet;
    LOG(INFO) << __func__ << ": Batt_Per: " << batt_level_ << ", Avg_Cur_Pwr: "
              << bqr_energy_packet.average_current_consumption << " mA"
              << ", BEr_Tx_Plv: "
              << bqr_energy_packet.br_edr_tx_average_power_level << " dBm"
              << ", Le_Tx_Plv: " << bqr_energy_packet.le_tx_average_power_level
              << " dBm"
              << ", Idle_Tm: " << bqr_energy_packet.idle_total_time << " ms"
              << ", Act_Tm: " << bqr_energy_packet.active_total_time << " ms"
              << ", BEr_Tx_Tm: " << bqr_energy_packet.br_edr_tx_total_time
              << " ms"
              << ", BEr_Rx_Tm: " << bqr_energy_packet.br_edr_rx_total_time
              << " ms"
              << ", Le_Tx_Tm: " << bqr_energy_packet.le_tx_total_time << " ms"
              << ", Le_Rx_Tm: " << bqr_energy_packet.le_rx_total_time << " ms.";

    update_bqr_energy_report(bqr_energy_sector);
  } else {  // BQRv6
    if (energy_event.size() < sizeof(BqrEnergyMonitoringEventV6)) {
      return;
    }

    bt_energy_sectorv6_t bqr_energy_sector;
    BqrEnergyMonitoringEventV6 bqr_energy_packet;
    uint8_t offset = kBqrEnergyMonitorPacketOffset;

    std::string engery_timestamp = Logger::GetLogFormatTimestamp();

    bqr_energy_packet.average_current_consumption =
        stream_to_int16(energy_event, &offset);
    bqr_energy_packet.idle_total_time = stream_to_int32(energy_event, &offset);
    bqr_energy_packet.idle_state_enter_count =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.active_total_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.active_state_enter_count =
        stream_to_int32(energy_event, &offset);

    bqr_energy_packet.br_edr_tx_total_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.br_edr_tx_state_enter_count =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.br_edr_tx_average_power_level =
        stream_to_int8(energy_event, &offset);
    bqr_energy_packet.br_edr_rx_total_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.br_edr_rx_state_enter_count =
        stream_to_int32(energy_event, &offset);

    bqr_energy_packet.le_tx_total_time = stream_to_int32(energy_event, &offset);
    bqr_energy_packet.le_tx_state_enter_count =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.le_tx_average_power_level =
        stream_to_int8(energy_event, &offset);
    bqr_energy_packet.le_rx_total_time = stream_to_int32(energy_event, &offset);
    bqr_energy_packet.le_rx_state_enter_count =
        stream_to_int32(energy_event, &offset);

    bqr_energy_packet.report_time_duration =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.rx_active_one_chain_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.rx_active_two_chain_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.tx_ipa_active_one_chain_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.tx_ipa_active_two_chain_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.tx_xpa_active_one_chain_time =
        stream_to_int32(energy_event, &offset);
    bqr_energy_packet.tx_xpa_active_two_chain_time =
        stream_to_int32(energy_event, &offset);

    // Update bqr_energy_sector
    bqr_energy_sector.timestamp = engery_timestamp;
    bqr_energy_sector.entries = bqr_energy_packet;

    LOG(INFO)
        << __func__ << ": Batt_Per: " << batt_level_
        << ", Avg_Cur_Pwr: " << bqr_energy_packet.average_current_consumption
        << " mA"
        << ", BEr_Tx_Plv: " << bqr_energy_packet.br_edr_tx_average_power_level
        << " dBm"
        << ", Le_Tx_Plv: " << bqr_energy_packet.le_tx_average_power_level
        << " dBm"
        << ", Idle_Tm: " << bqr_energy_packet.idle_total_time << " ms"
        << ", Act_Tm: " << bqr_energy_packet.active_total_time << " ms"
        << ", BEr_Tx_Tm: " << bqr_energy_packet.br_edr_tx_total_time << " ms"
        << ", BEr_Rx_Tm: " << bqr_energy_packet.br_edr_rx_total_time << " ms"
        << ", Le_Tx_Tm: " << bqr_energy_packet.le_tx_total_time << " ms"
        << ", Le_Rx_Tm: " << bqr_energy_packet.le_rx_total_time << " ms"
        << ", total_Tm: " << bqr_energy_packet.report_time_duration << " ms"
        << ", Rx_1Ch_Tm: " << bqr_energy_packet.rx_active_one_chain_time
        << " ms"
        << ", Rx_2Ch_Tm: " << bqr_energy_packet.rx_active_two_chain_time
        << " ms"
        << ", Tx_iPA_1Ch_Tm: " << bqr_energy_packet.tx_ipa_active_one_chain_time
        << " ms"
        << ", Tx_iPA_2Ch_Tm: " << bqr_energy_packet.tx_ipa_active_two_chain_time
        << " ms"
        << ", Tx_ePA_1Ch_Tm: " << bqr_energy_packet.tx_xpa_active_one_chain_time
        << " ms"
        << ", Tx_ePA_2Ch_Tm: " << bqr_energy_packet.tx_xpa_active_two_chain_time
        << " ms.";

    update_bqr_energy_report(bqr_energy_sector);
  }
}

// Sub-event code = 0x58 [Quality_Report_Id = 0x0B ~ 0x0C Controller Health
// Monitoring Event]
void ParseControllerHealthMonitorEvt(const HalPacket& health_monitor_event) {
  // Health monitor only supported in bqr_v7 and make sure the size is as
  // expected.
  if (kSupportedVersion < BQR_VERSION_V7) {
    LOG(INFO) << __func__ << ": Error: Vdr BQR supp ver(" << kSupportedVersion
              << ") not as expect ver(" << BQR_VERSION_V7 << ")!!";
    return;
  }
  if (health_monitor_event.size() < sizeof(BqrControllerHealthMonitorEvent)) {
    LOG(INFO) << __func__ << ": Error: received evt size("
              << health_monitor_event.size() << ") not as expected size("
              << sizeof(BqrControllerHealthMonitorEvent) << ")!!";
    return;
  }
  static constexpr int kBqrHealthMonitorPacketOffset = 4;
  BqrControllerHealthMonitorEvent bqr_health_monitor;
  uint8_t offset = kBqrHealthMonitorPacketOffset;
  // Parse Controller Health Monitor Event
  bqr_health_monitor.packet_count_host_to_controller =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.packet_count_controller_to_host =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.last_packet_length_controller_to_host =
      stream_to_int16(health_monitor_event, &offset);
  bqr_health_monitor.last_packet_length_host_to_controller =
      stream_to_int16(health_monitor_event, &offset);
  bqr_health_monitor.total_bt_wake_count =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.total_host_wake_count =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.last_bt_wake_timestamp =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.last_host_wake_timestamp =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.reset_timestamp =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.current_timestamp =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.is_watchdog_timer_about_to_expire =
      stream_to_int32(health_monitor_event, &offset);
  bqr_health_monitor.coex_status_mask =
      stream_to_int16(health_monitor_event, &offset);
  bqr_health_monitor.total_links_br_edr_le_active =
      stream_to_int8(health_monitor_event, &offset);
  bqr_health_monitor.total_links_br_edr_sniff =
      stream_to_int8(health_monitor_event, &offset);
  bqr_health_monitor.total_links_cis =
      stream_to_int8(health_monitor_event, &offset);
  bqr_health_monitor.is_sco_active =
      stream_to_int8(health_monitor_event, &offset);

  LOG(INFO) << __func__ << ": pk_ct_to_ctrl: "
            << bqr_health_monitor.packet_count_host_to_controller
            << ", pk_ct_to_host: "
            << bqr_health_monitor.packet_count_controller_to_host
            << ", last_pk_len_to_host: "
            << bqr_health_monitor.last_packet_length_controller_to_host
            << ", last_pk_len_to_ctrl: "
            << bqr_health_monitor.last_packet_length_host_to_controller
            << ", bt_wake_cnt: " << bqr_health_monitor.total_bt_wake_count
            << ", host_wake_cnt: " << bqr_health_monitor.total_host_wake_count
            << ", reset_ts: " << bqr_health_monitor.reset_timestamp
            << ", cur_ts: " << bqr_health_monitor.current_timestamp
            << ", last_bt_wake_ts: "
            << bqr_health_monitor.last_bt_wake_timestamp
            << ", last_host_wake_ts: "
            << bqr_health_monitor.last_host_wake_timestamp << ", watchdog_exp: "
            << bqr_health_monitor.is_watchdog_timer_about_to_expire
            << ", coex_mask: " << bqr_health_monitor.coex_status_mask
            << ", links_br_edr_le: "
            << bqr_health_monitor.total_links_br_edr_le_active
            << ", links_br_edr_sniff: "
            << bqr_health_monitor.total_links_br_edr_sniff
            << ", links_cis: " << bqr_health_monitor.total_links_cis
            << ", sco_active: " << bqr_health_monitor.is_sco_active << ".";
}

std::vector<uint8_t> GetBqrV6Cmd(BqrCmdScenario type) {
  BqrV6CmdConfiguration bqr_config = {};
  switch (type) {
    case BqrCmdScenario::ENABLE_BQR_BT_OFF:
      bqr_config.report_action = BqrReportAction::kAdd;
      // Enable Root inflammation event
      bqr_config.quality_event_mask = 0x10;
      bqr_config.minimum_report_interval_ms = 0x00;
      bqr_config.vnd_quality_mask = 0x00;
      bqr_config.vnd_trace_mask = 0x00;
      bqr_config.report_interval_multiple = 0x00;
      break;
    case BqrCmdScenario::DISABLE_BQR:
    default:
      bqr_config.report_action = BqrReportAction::kClear;
      bqr_config.quality_event_mask = 0x00;
      bqr_config.minimum_report_interval_ms = 0x00;
      bqr_config.vnd_quality_mask = 0x00;
      bqr_config.vnd_trace_mask = 0x00;
      bqr_config.report_interval_multiple = 0x00;
  }

  std::vector<uint8_t> bqr_byte_vec = {};

  AddOctets(1, static_cast<uint64_t>(bqr_config.report_action), bqr_byte_vec);
  AddOctets(4, bqr_config.quality_event_mask, bqr_byte_vec);
  AddOctets(2, bqr_config.minimum_report_interval_ms, bqr_byte_vec);
  AddOctets(4, bqr_config.vnd_quality_mask, bqr_byte_vec);
  AddOctets(4, bqr_config.vnd_trace_mask, bqr_byte_vec);
  AddOctets(4, bqr_config.report_interval_multiple, bqr_byte_vec);

  return bqr_byte_vec;
}

void AddOctets(size_t bytes, uint64_t value, std::vector<uint8_t>& value_vec) {
  for (size_t i = 0; i < bytes; ++i) {
    value_vec.push_back(value & 0xff);
    value >>= 8;
  }
}

}  // namespace debug
}  // namespace bluetooth_hal
