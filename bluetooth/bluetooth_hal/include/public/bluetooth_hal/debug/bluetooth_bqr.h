/*
 * Copyright 2021 The Android Open Source Project
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

#include <cstdint>
#include <fstream>

#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace debug {

/* Bluetooth Quality Report VSE sub event */
#define HCI_VSE_SUBCODE_BQR_SUB_EVT 0x58
/* The version supports ISO packets start from v1.01(257)*/
#define BQR_VERSION_V4 257
/* BQR v1.03(259)*/
#define BQR_VERSION_V5 259
/* BQR v1.04(260)*/
#define BQR_VERSION_V6 260
/* BQR v1.05(261)*/
#define BQR_VERSION_V7 261

/*BQR Advance RF stats event: Extension_info*/
#define BQR_RFSTATS_EXT_INFO_V6 1

// Report ID definition.
enum class BqrQualityReportId : uint8_t {
  kMonitorMode = 0x01,
  kApproachLsto = 0x02,
  kA2dpAudioChoppy = 0x03,
  kScoVoiceChoppy = 0x04,
  kRootInflammation = 0x05,
  kEnergyMonitoring = 0x06,
  kLeAudioChoppy = 0x07,
  kConnectFail = 0x08,
  kAdvanceRfStats = 0x09,
  kAdvanceRfStatsPeriodic = 0x0A,
  kControllerHealthMonitor = 0x0B,
  kControllerHealthMonitorPeriodic = 0x0C,
  kVendorSpecificQuality = 0x10,
  kLmpLlMessageTrace = 0x11,
  kBtSchedulingTrace = 0x12,
  kControllerDbgInfo = 0x13,
  kChreDbgInfo = 0x14,
  kVendorSpecificTrace = 0x20,
};

// Vendor Report ID definition.
enum class VendorReportId : uint8_t {
  kA2dpLatencyMeasurement = 0x00,
  kHrMode = 0x01,
};

// Packet Type definition.
enum class BqrPacketType : uint8_t {
  kId = 0x01,
  kNull,
  kPoll,
  kFhs,
  kHv1,
  kHv2,
  kHv3,
  kDv,
  kEv3,
  kEv4,
  kEv5,
  k2Ev3,
  k2Ev5,
  k3Ev3,
  k3Ev5,
  kDm1,
  kDh1,
  kDm3,
  kDh3,
  kDm5,
  kDh5,
  kAux1,
  k2Dh1,
  k2Dh3,
  k2Dh5,
  k3Dh1,
  k3Dh3,
  k3Dh5,
  k4Dh1 = 0x20,
  k4Dh3,
  k4Dh5,
  k8Dh1,
  k8Dh3,
  k8Dh5,
  k4Ev3,
  k4Ev5,
  k8Ev3,
  k8Ev5,
  kIso = 0x51,
};

// BQR Link quality related event v3 and backward
typedef struct __attribute__((__packed__)) {
  // Vendor Specific Event
  uint8_t vendor_specific_event;
  // Parameter Total Length
  uint8_t parameter_total_length;
  // sub-event code
  uint8_t sub_event;

  // Common parameters of Link quality related event
  // Quality report ID.
  uint8_t quality_report_id;
  // Packet type of the connection.
  uint8_t packet_types;
  // Connection handle of the connection.
  uint16_t connection_handle;
  // Performing Role for the connection.
  uint8_t connection_role;
  // Current Transmit Power Level for the connection. This value is the same as
  // the controller's response to the HCI_Read_Transmit_Power_Level HCI command.
  int8_t tx_power_level;
  // Received Signal Strength Indication (RSSI) value for the connection. This
  // value is an absolute receiver signal strength value.
  int8_t rssi;
  // Signal-to-Noise Ratio (SNR) value for the connection. It is the average
  // SNR of all the channels used by the link currently.
  uint8_t snr;
  // Indicates the number of unused channels in AFH_channel_map.
  uint8_t unused_afh_channel_count;
  // Indicates the number of the channels which are interfered and quality is
  // bad but are still selected for AFH.
  uint8_t afh_select_unideal_channel_count;
  // Current Link Supervision Timeout Setting.
  // Unit: N * 0.3125 ms (1 Bluetooth Clock)
  uint16_t lsto;
  // Piconet Clock for the specified Connection_Handle. This value is the same
  // as the controller's response to HCI_Read_Clock HCI command with the
  // parameter "Which_Clock" of 0x01 (Piconet Clock).
  // Unit: N * 0.3125 ms (1 Bluetooth Clock)
  uint32_t connection_piconet_clock;
  // The count of retransmission.
  uint32_t retransmission_count;
  // The count of no RX.
  uint32_t no_rx_count;
  // The count of NAK (Negative Acknowledge).
  uint32_t nak_count;
  // Timestamp of last TX ACK.
  // Unit: N * 0.3125 ms (1 Bluetooth Clock)
  uint32_t last_tx_ack_timestamp;
  // The count of Flow-off (STOP).
  uint32_t flow_off_count;
  // Timestamp of last Flow-on (GO).
  // Unit: N * 0.3125 ms (1 Bluetooth Clock)
  uint32_t last_flow_on_timestamp;
  // Buffer overflow count (how many bytes of TX data are dropped) since the
  // last event.
  uint32_t buffer_overflow_bytes;
  // Buffer underflow count (in byte).
  uint32_t buffer_underflow_bytes;
} BqrLinkQualityEventGenericV3AndBackward;

// BQR Link quality related event v4
typedef struct __attribute__((__packed__))
    : BqrLinkQualityEventGenericV3AndBackward {
  // The number of packets that are sent out.
  uint32_t tx_total_packets;
  // The number of packets that don't receive an acknowledgment.
  uint32_t tx_unacked_packets;
  // The number of packets that are not sent out by its flush point.
  uint32_t tx_flushed_packets;
  // The number of packets that Link Layer transmits a CIS Data PDU in the last
  // subevent of a CIS event.
  uint32_t tx_last_subevent_packets;
  // The number of received packages with CRC error since the last event.
  uint32_t crc_error_packets;
  // The number of duplicate(retransmission) packages that are received since
  // the last event.
  uint32_t rx_duplicate_packets;

} BqrLinkQualityEventGenericV4;

// BQR Link quality related event v5
typedef struct __attribute__((__packed__))
    : BqrLinkQualityEventGenericV3AndBackward {
  // remote BD addr
  uint16_t remote_addr[3];
  // Cal_Failed_Item_Count
  uint8_t call_failed_item_count;
  // The number of packets that are sent out.
  uint32_t tx_total_packets;
  // The number of packets that don't receive an acknowledgment.
  uint32_t tx_unacked_packets;
  // The number of packets that are not sent out by its flush point.
  uint32_t tx_flushed_packets;
  // The number of packets that Link Layer transmits a CIS Data PDU in the last
  // subevent of a CIS event.
  uint32_t tx_last_subevent_packets;
  // The number of received packages with CRC error since the last event.
  uint32_t crc_error_packets;
  // The number of duplicate(retransmission) packages that are received since
  // the last event.
  uint32_t rx_duplicate_packets;

} BqrLinkQualityEventGenericV5;

// BQR Link quality related event v6
typedef struct __attribute__((__packed__)) : BqrLinkQualityEventGenericV5 {
  // The number of unreceived packets is the same as the parameter of LE Read
  // ISO Link Quality command.
  uint32_t rx_unreceived_packets;
  // Set to indicate coex activities are suspected to be involved when this
  // report is generated
  uint16_t coex_info_mask;

} BqrLinkQualityEventGenericV6;

// BQR Link quality related event for vendor specific parameters
typedef struct __attribute__((__packed__))
    : BqrLinkQualityEventGenericV3AndBackward {
  // Vendor Specific parameters
  uint32_t req_cnt;
  uint32_t grant_cnt;
  uint32_t defer_cnt;
  uint32_t dur_invalid_req;
  uint32_t dur_acl_req;
  uint32_t dur_sco_req;
  uint32_t dur_esco_req;
  uint32_t dur_a2dp_req;
  uint32_t dur_sniff_req;
  uint32_t dur_pagescan_req;
  uint32_t dur_inqscan_req;
  uint32_t dur_page_req;
  uint32_t dur_inq_req;
  uint32_t dur_crit10_req;
  uint32_t dur_crit11_req;
  uint32_t dur_rssi_req;
  uint32_t dur_inqscan_sco_req;
  uint32_t dur_pagescan_sco_req;
  uint32_t dur_tpoll_req;
  uint32_t dur_ant_req;
  uint32_t dur_crit17_req;
  uint32_t dur_crit18_req;
  uint32_t dur_crit19_req;
  uint32_t dur_ulpadv_req;
  uint32_t dur_ulpscan_req;
  uint32_t dur_ulpinit_req;
  uint32_t dur_ulpconn_req;
  uint32_t dur_ulplmp_req;
  uint32_t dur_escoretran_req;
  uint32_t dur_tbfcscan_req;
  uint32_t dur_tbfcbcn_req;
  uint32_t dur_mac154_req;
  uint32_t dur_pred_req;
  uint32_t dur_misc_req;

  int8_t rssi_for_core0;
  int8_t rssi_for_core1;
  int8_t tx_power_for_core0;
  int8_t retx_power_for_core0;
  int8_t tx_power_for_core1;
  int8_t retx_power_for_core1;

  uint8_t bf_state_for_tx;
  uint8_t bf_state_for_retx;
  uint8_t div_state_for_tx;
  uint8_t div_state_for_retx;
} BqrLinkQualityEventV3AndBackward;

// BQR Link quality related event for vendor specific parameters
typedef struct __attribute__((__packed__)) : BqrLinkQualityEventGenericV4 {
  // Vendor Specific parameters
  uint32_t req_cnt;
  uint32_t grant_cnt;
  uint32_t defer_cnt;
  uint32_t dur_acl_req;
  uint32_t dur_sco_req;
  uint32_t dur_esco_req;
  uint32_t dur_a2dp_req;
  uint32_t dur_sniff_req;
  uint32_t dur_pagescan_req;
  uint32_t dur_inqscan_req;
  uint32_t dur_page_req;
  uint32_t dur_inq_req;
  uint32_t dur_rssi_req;
  uint32_t dur_ulpadv_req;
  uint32_t dur_ulpscan_req;
  uint32_t dur_ulpinit_req;
  uint32_t dur_ulpconn_req;
  uint32_t dur_pred_req;
  uint32_t dur_misc_req;

  int8_t rssi_for_core0;
  int8_t rssi_for_core1;
  int8_t tx_power_for_core0;
  int8_t retx_power_for_core0;
  int8_t tx_power_for_core1;
  int8_t retx_power_for_core1;

  uint8_t bf_state_for_tx;
  uint8_t bf_state_for_retx;
  uint8_t div_state_for_tx;
  uint8_t div_state_for_retx;

  uint32_t total_tx_pkts;
  uint32_t total_tx_pkts_core0;
  uint32_t total_tx_pkts_core1;
  uint32_t total_tx_pkts_epa;
  uint32_t total_tx_pkts_beamforming;

  // A2DP offload EWP info
  uint16_t a2dp_offload_ewp_types;
  // The total length for payload length is 6 bytes
  uint16_t i2s_payload_len[3];
  // The total length for current sequence number is 12 bytes
  uint16_t curSeqN[6];
  uint32_t drop_pkt_cnt;
  uint8_t exceed_mtu_cnt;
  uint8_t out_of_seq_cnt;
  uint8_t drop_pkt_time_evnt_cnt;
  uint8_t unused;

  uint32_t i2s_in_pkt_rxd;
  uint32_t i2s_in_err_sync_fail;
  uint32_t i2s_in_err_mem_fail;
  uint32_t i2s_in_err_len_fail;
  uint32_t i2s_out_pkt_txd;

  uint8_t overall_link_quality;
  uint8_t tx_link_quality;
  uint8_t rx_link_quality;

  uint32_t ema_tx_acked_pkt_cnt;
  uint32_t ema_tx_unacked_pkt_cnt;
  uint32_t ema_rx_good_pkt_cnt;
  uint32_t ema_rx_bad_pkt_cnt;
} BqrLinkQualityEventV4;

// BQR Link quality related event for vendor specific parameters
typedef struct __attribute__((__packed__)) : BqrLinkQualityEventGenericV5 {
  // Vendor Specific parameters
  uint32_t req_cnt;
  uint32_t grant_cnt;
  uint32_t defer_cnt;

  uint32_t dur_acl_req;
  uint32_t dur_sco_esco_req;
  uint32_t dur_a2dp_req;
  uint32_t dur_sniff_req;
  uint32_t dur_pagescan_inqscan__req;
  uint32_t dur_page_inq_req;
  uint32_t dur_rssi_req;
  uint32_t dur_ulpadv_req;
  uint32_t dur_ulpscan_ulpinit_req;
  uint32_t dur_ulpconn_req;
  uint32_t dur_pred_req;
  uint32_t dur_misc_req;

  int8_t rssi_for_core0;
  int8_t rssi_for_core1;
  int8_t tx_power_for_core0;
  int8_t retx_power_for_core0;
  int8_t tx_power_for_core1;
  int8_t retx_power_for_core1;

  uint8_t bf_state_for_tx;
  uint8_t bf_state_for_retx;
  uint8_t div_state_for_tx;
  uint8_t div_state_for_retx;

  uint32_t total_tx_pkts;
  uint32_t total_tx_pkts_core0;
  uint32_t total_tx_pkts_core1;
  uint32_t total_tx_pkts_epa;
  uint32_t total_tx_pkts_beamforming;

  // A2DP offload EWP info
  uint16_t a2dp_offload_ewp_types;
  // The total length for payload length is 6 bytes
  uint16_t i2s_payload_len[3];
  // The total length for current sequence number is 12 bytes
  uint16_t curSeqN[6];
  uint32_t drop_pkt_cnt;
  uint8_t exceed_mtu_cnt;
  uint8_t out_of_seq_cnt;
  uint8_t drop_pkt_time_evnt_cnt;
  uint8_t unused;

  uint32_t i2s_in_pkt_rxd;
  uint32_t i2s_in_err_sync_fail;
  uint32_t i2s_in_err_mem_fail;
  uint32_t i2s_in_err_len_fail;
  uint32_t i2s_out_pkt_txd;

  uint8_t overall_link_quality;
  uint8_t tx_link_quality;
  uint8_t rx_link_quality;

  uint32_t ema_tx_acked_pkt_cnt;
  uint32_t ema_tx_unacked_pkt_cnt;
  uint32_t ema_rx_good_pkt_cnt;
  uint32_t ema_rx_bad_pkt_cnt;

  uint8_t cis_sdulist_queue_length;
  uint8_t cis_sdulist_overflow_cnt;
  uint8_t rx_mrc_status;
} BqrLinkQualityEventV5;

// BQR Link quality related event for vendor specific parameters
typedef struct __attribute__((__packed__)) : BqrLinkQualityEventGenericV6 {
  // Vendor Specific parameters
  uint32_t req_cnt;
  uint32_t grant_cnt;
  uint32_t defer_cnt;

  uint32_t dur_acl_req;
  uint32_t dur_sco_esco_req;
  uint32_t dur_a2dp_req;
  uint32_t dur_sniff_req;
  uint32_t dur_pagescan_inqscan__req;
  uint32_t dur_page_inq_req;
  uint32_t dur_rssi_req;
  uint32_t dur_ulpadv_req;
  uint32_t dur_ulpscan_ulpinit_req;
  uint32_t dur_ulpconn_req;
  uint32_t dur_pred_req;
  uint32_t dur_misc_req;

  int8_t rssi_for_core0;
  int8_t rssi_for_core1;
  int8_t tx_power_for_core0;
  int8_t retx_power_for_core0;
  int8_t tx_power_for_core1;
  int8_t retx_power_for_core1;

  uint8_t bf_state_for_tx;
  uint8_t bf_state_for_retx;
  uint8_t div_state_for_tx;
  uint8_t div_state_for_retx;

  uint32_t total_tx_pkts;
  uint32_t total_tx_pkts_core0;
  uint32_t total_tx_pkts_core1;
  uint32_t total_tx_pkts_epa;
  uint32_t total_tx_pkts_beamforming;

  // A2DP offload EWP info
  uint16_t a2dp_offload_ewp_types;
  // The total length for payload length is 6 bytes
  uint16_t i2s_payload_len[3];
  // The total length for current sequence number is 12 bytes
  uint16_t curSeqN[6];
  uint32_t drop_pkt_cnt;
  uint8_t exceed_mtu_cnt;
  uint8_t out_of_seq_cnt;
  uint8_t drop_pkt_time_evnt_cnt;
  uint8_t unused;

  uint32_t i2s_in_pkt_rxd;
  uint32_t i2s_in_err_sync_fail;
  uint32_t i2s_in_err_mem_fail;
  uint32_t i2s_in_err_len_fail;
  uint32_t i2s_out_pkt_txd;

  uint8_t overall_link_quality;
  uint8_t tx_link_quality;
  uint8_t rx_link_quality;

  uint32_t ema_tx_acked_pkt_cnt;
  uint32_t ema_tx_unacked_pkt_cnt;
  uint32_t ema_rx_good_pkt_cnt;
  uint32_t ema_rx_bad_pkt_cnt;

  uint8_t cis_sdulist_queue_length;
  uint8_t cis_sdulist_overflow_cnt;
  uint8_t rx_mrc_status;
} BqrLinkQualityEventV6;

// Vendor BQR Vendor Specific Trace Event - A2DP Latency Measurement
typedef struct __attribute__((__packed__)) {
  // Unit: BT Slot
  // Host packet reception time offset. This is the time offset from
  // Base_Timestamp logged in this event
  uint16_t packet_entry_time_offset;
  // The first packet transmission by the controller. The delay measured between
  // Packet_Entry_Time_Offset and First_Packet_Transmit_Delay.
  uint16_t first_packet_transmit_delay;
  // Time until the remote device responds with the first ACK or NACK. This is
  // the delay measured between Packet_Entry_Time_Offset and
  // First_Packet_Ack_Delay.
  uint16_t first_packet_ack_delay;
  // Time until the packet was either successfully transmitted or flushed by the
  // controller. Delay measure between Packet_Entry_Time_Offset and
  // Final_Packet_Transmit_Delay.
  uint16_t final_packet_transmit_delay;
} A2DPLatency;

typedef struct __attribute__((__packed__)) {
  // Vendor Specific Event
  uint8_t vendor_specific_event;
  // Parameter Total Length
  uint8_t parameter_total_length;
  // Sub-event code
  uint8_t sub_event;

  // Quality report ID.
  uint8_t quality_report_id;

  // Vendor Specific Parameters
  uint8_t vendor_report_id;
  // Connection handle of the connection for which the latency report is sent
  uint16_t conn_handle;
  // Number of Packets for which the latency is reported in this event
  uint8_t num_packets_logged;
  // Timestamp of the first packet logged in this event. Unit: BT CLK
  uint32_t base_timestamp;
  A2DPLatency a2dp_latencies[];
} BqrVsteA2dpLatencyMeasurement;

typedef struct __attribute__((__packed__)) {
  // Vendor Specific Event
  uint8_t vendor_specific_event;
  // Parameter Total Length
  uint8_t parameter_total_length;
  // Sub-event code
  uint8_t sub_event;

  // Quality report ID.
  uint8_t quality_report_id;

  // Vendor Specific quality event id
  uint8_t vendor_specific_quality_event_id;
  // Connection handle of the connection for which the latency report is sent
  uint16_t conn_handle;

} BqrVendorSpecificEventGeneric;

typedef struct __attribute__((__packed__)) : BqrVendorSpecificEventGeneric {
  // Some simple statistic for Rx
  uint32_t rx_null_cnt;
  uint32_t rx_poll_cnt;
  uint32_t rx_dm1_cnt;

  // Some simple statistic for Tx
  uint32_t tx_null_cnt;
  uint32_t tx_poll_cnt;
  uint32_t tx_dm1_cnt;

  // Packet type statistic for Rx
  uint32_t rx_hr_2dh1;
  uint32_t rx_hr_4dh1;
  uint32_t rx_hr_8dh1;
  uint32_t rx_hr_2dh3;
  uint32_t rx_hr_4dh3;
  uint32_t rx_hr_8dh3;
  uint32_t rx_hr_2dh5;
  uint32_t rx_hr_4dh5;
  uint32_t rx_hr_8dh5;

  // Packet type statistic for Tx
  uint32_t tx_hr_2dh1;
  uint32_t tx_hr_4dh1;
  uint32_t tx_hr_8dh1;
  uint32_t tx_hr_2dh3;
  uint32_t tx_hr_4dh3;
  uint32_t tx_hr_8dh3;
  uint32_t tx_hr_2dh5;
  uint32_t tx_hr_4dh5;
  uint32_t tx_hr_8dh5;
} BqrVendorSpecificEventHRMode;

// BQR Link Advance RF stats event
typedef struct __attribute__((__packed__)) {
  // Vendor Specific Event
  uint8_t event;
  // Parameter Total Length
  uint8_t sub_code;
  // sub-event code
  uint8_t event_type;

  // Quality_Report_Id = 0x09 or 0x0a
  uint8_t report_id;
  // Extension for Further usage = 0x01 for BQRv6
  uint8_t ext_info;

  // time period (ms)
  uint32_t tm_period;

  // Stats for TX Power
  uint32_t tx_pw_ipa_bf;
  uint32_t tx_pw_epa_bf;
  uint32_t tx_pw_ipa_div;
  uint32_t tx_pw_epa_div;

  // Stats for RSSI_chain
  uint32_t rssi_ch_50;
  uint32_t rssi_ch_50_55;
  uint32_t rssi_ch_55_60;
  uint32_t rssi_ch_60_65;
  uint32_t rssi_ch_65_70;
  uint32_t rssi_ch_70_75;
  uint32_t rssi_ch_75_80;
  uint32_t rssi_ch_80_85;
  uint32_t rssi_ch_85_90;
  uint32_t rssi_ch_90;

  // Stats for RSSI_delta
  uint32_t rssi_delta_2_down;
  uint32_t rssi_delta_2_5;
  uint32_t rssi_delta_5_8;
  uint32_t rssi_delta_8_11;
  uint32_t rssi_delta_11_up;
} BqrAdvanceRFStatsEvent;

// BQR Controller Health Status Event
typedef struct __attribute__((__packed__)) {
  // Vendor Specific Event
  uint8_t event;
  // Parameter Total Length
  uint8_t sub_code;
  // sub-event code
  uint8_t event_type;

  // Quality_Report_Id = 0x0B
  uint8_t report_id;
  // Total count of packets sent from Host to Controller over HCI transport.
  // This field is employed for the purpose of debugging HCI (e.g., UART)
  // issues. Behavior: the counters reset when the controller received HCI
  // reset.
  uint32_t packet_count_host_to_controller;

  // Total count of HCI + Spinel Event packets sent to Host.
  // This field is employed for the purpose of debugging HCI (e.g., UART)
  // issues. Behavior: the counters reset when the controller received HCI
  // reset.
  uint32_t packet_count_controller_to_host;

  // Length of the last HCI packet received from Host UART.
  // Note: HCI Packet Length max 2 octet (Include HCI, ACL, SCO, ISO)
  uint16_t last_packet_length_controller_to_host;

  // Length of the last HCI packet sent to Host UART.
  // Note: HCI Packet Length max 2 octet (Include HCI, ACL, SCO, ISO)
  uint16_t last_packet_length_host_to_controller;

  // The aggregate tally of BT_Wake pin assertions by the Host entity.
  // This field serves as a diagnostic tool for debugging power-related issues.
  // Behavior: the counters reset when the controller received HCI reset.
  uint32_t total_bt_wake_count;

  // Aggregate calculation of Host_Wake pin assertions initiated by the
  // Controller. This field serves as a diagnostic tool for debugging
  // power-related issues. Behavior: the counters reset when the controller
  // received HCI reset.
  uint32_t total_host_wake_count;

  // Last Timestamp when Host Asserted BT_Wake Pin.This field is implemented for
  // the purpose of debugging Power issues.
  uint32_t last_bt_wake_timestamp;

  // The most recent timestamp when the controller asserted the Host_Wake pin.
  // This field is used for debugging power issues.
  uint32_t last_host_wake_timestamp;

  // Timestamp indicating the completion of the most recent HCI Reset.
  // This field is utilized for the express purpose of facilitating the
  // resolution of timing-related issues. It should serve as the initial
  // recording point against which all other items are referenced.
  uint32_t reset_timestamp;

  // The present time when this event is generated.
  // This field is utilized for the purpose of troubleshooting timing
  // discrepancies. It should serve as the trigger recording point that all
  // other elements reference.
  uint32_t current_timestamp;

  // Flag to denote that this health status event is generated by the
  // controller as an early warning of watch dog expiration.
  // The current timestamp serves to indicate the time of occurrence.
  uint32_t is_watchdog_timer_about_to_expire;

  // Bit 0 - Reserved
  // Bit 1 - WL 2G Radioactive: Set to indicate WLAN 2G Radio is active.
  // Bit 2 - WL 2G Connected: Set to indicate WLAN 2G Radio is active and
  // connected. Bit 3 - WL 5G/6G Radioactive: Set to indicate WLAN 5G/6G Radio
  // is active. Bit 4 - Thread Radioactive status Bit 5-15 - Reserved
  uint16_t coex_status_mask;

  // Total link count of BR/EDR/LE in Active state.
  uint8_t total_links_br_edr_le_active;

  // Total link count of BR/EDR in Sniff/Idle state(a.k.a link layer power
  // saving mode).
  uint8_t total_links_br_edr_sniff;

  // Total link count of ISO for le audio part.
  uint8_t total_links_cis;

  // Indicator to check if the SCO link is currently activated.
  uint8_t is_sco_active;
} BqrControllerHealthMonitorEvent;

// Get a string representation of the Quality Report ID.
//
// @param quality_report_id The quality report ID to convert.
// @return a string representation of the Quality Report ID.
std::string QualityReportIdToString(uint8_t quality_report_id);

// Parse the Link Quality related event.
//
// @param packet A pointer to the Vendor Specific Event packet.
void ParseLinkQualityRelatedEvt(const ::bluetooth_hal::hci::HalPacket& packet);

// Update the controller capability
//
// @param packet A pointer to the LE Get Vendor Capabilities packet.
void updateControllerCapability(const ::bluetooth_hal::hci::HalPacket& packet);

// Parse the vendor specific trace event.
//
// @param packet A pointer to the Vendor Specific Event packet.
void ParseVendorSpecificTraceEvt(const ::bluetooth_hal::hci::HalPacket& packet);

// Parse the vendor specific trace event: A2DP latency measurement.
//
// @param packet A pointer to the Vendor Specific Event A2DP latency measurement
// packet.
void ParseA2DPLatencyMeasurement(const ::bluetooth_hal::hci::HalPacket& packet);

// Parse the vendor specific quality event.
//
// @param packet A pointer to the Vendor Specific Event packet.
void ParseVendorSpecificQualityEvt(
    const ::bluetooth_hal::hci::HalPacket& packet);

// Parse the vendor specific quality event: HR Mode statistic.
//
// @param packet A pointer to the Vendor Specific Event HR Mode statistic.
// packet.
void ParseHRModeStatisticLog(const ::bluetooth_hal::hci::HalPacket& packet);

// Parse the Advance RF Stats event.
//
// @param packet A pointer to the Advance RF Stats event packet.
void ParseAdvanceRFStatsEvt(const ::bluetooth_hal::hci::HalPacket& packet);

// Parse the Controller Health Monitor event.
//
// @param packet A pointer to the Controller Health Monitor event packet.
void ParseControllerHealthMonitorEvt(
    const ::bluetooth_hal::hci::HalPacket& health_monitor_event);

// BQR Energy Monitoring Event Packet
typedef struct __attribute__((__packed__)) {
  // Average Current Consumption
  uint16_t average_current_consumption;
  // Idle Total Time
  uint32_t idle_total_time;
  // Idle State Enter Count
  uint32_t idle_state_enter_count;
  // Active Total Time
  uint32_t active_total_time;
  // Active State Enter Count
  uint32_t active_state_enter_count;
  // BR_EDR Tx Total Time
  uint32_t br_edr_tx_total_time;
  // BR_EDR Tx State Enter Count
  uint32_t br_edr_tx_state_enter_count;
  // BR_EDR_Tx_Average_Power_Level
  int8_t br_edr_tx_average_power_level;
  // BR_EDR_Rx_Total_Time
  uint32_t br_edr_rx_total_time;
  // BR_EDR_Rx_State_Enter_Count
  uint32_t br_edr_rx_state_enter_count;
  // LE_Tx_Total_Time
  uint32_t le_tx_total_time;
  // LE_Tx_State_Enter_Count
  uint32_t le_tx_state_enter_count;
  // LE_Tx_Average_Power_Level
  int8_t le_tx_average_power_level;
  // LE_Rx_Total_Time
  uint32_t le_rx_total_time;
  // LE_Rx_State_Enter_Count
  uint32_t le_rx_state_enter_count;
} BqrEnergyMonitoringEvent;

// BQR Energy Monitoring Event Packet
typedef struct __attribute__((__packed__)) : BqrEnergyMonitoringEvent {
  // Report Time Duration (Total Time: ms)
  uint32_t report_time_duration;
  // RX Active One Chain Time (ms)
  uint32_t rx_active_one_chain_time;
  // RX Active Two Chain Time (ms)
  uint32_t rx_active_two_chain_time;
  // TX iPA Active One Chain Time (ms)
  uint32_t tx_ipa_active_one_chain_time;
  // TX iPA  Active Two Chain Time (ms)
  uint32_t tx_ipa_active_two_chain_time;
  // TX xPA Active One Chain Time (ms)
  uint32_t tx_xpa_active_one_chain_time;
  // TX xPA  Active Two Chain Time (ms)
  uint32_t tx_xpa_active_two_chain_time;
} BqrEnergyMonitoringEventV6;

struct bt_energy_sector_t {
  std::string timestamp;
  BqrEnergyMonitoringEvent entries;
};

struct bt_energy_sectorv6_t {
  std::string timestamp;
  BqrEnergyMonitoringEventV6 entries;
};

class BtBqrEnergyRecoder {
 public:
  static BtBqrEnergyRecoder* GetInstacne();
  void ParseBqrEnergyMonitorEvt(
      const ::bluetooth_hal::hci::HalPacket& energy_event);
  void StartLogging();
  void StopLogging();

 private:
  uint16_t kMaxPacketsPerFile_ = 7200;
  uint16_t packet_counter_ = 0;
  std::string batt_level_{};
  std::string bt_activities_bqr_energy_log_path_;
  std::ofstream bqr_energy_activity_ostream_;
  void open_new_energy_log_file();
  void update_bqr_energy_report(bt_energy_sector_t& stat);
  void update_bqr_energy_report(bt_energy_sectorv6_t& stat);
};

enum class BqrCmdScenario { ENABLE_BQR_BT_OFF = 0, DISABLE_BQR };

enum class BqrReportAction : uint8_t {
  kAdd = 0x00,
  kDelete = 0x01,
  kClear = 0x02,
  kQuery = 0x03,
};

typedef struct {
  BqrReportAction report_action;
  uint32_t quality_event_mask;
  uint16_t minimum_report_interval_ms;
  uint32_t vnd_quality_mask;
  uint32_t vnd_trace_mask;
  uint32_t report_interval_multiple;
} BqrV6CmdConfiguration;

constexpr uint16_t kBqrHciCmdOpCode = 0xfd5e;
constexpr uint8_t kBqrV6CmddSize = 0x13;

std::vector<uint8_t> GetBqrV6Cmd(BqrCmdScenario type);

}  // namespace debug
}  // namespace bluetooth_hal
