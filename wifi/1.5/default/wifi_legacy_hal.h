/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef WIFI_LEGACY_HAL_H_
#define WIFI_LEGACY_HAL_H_

#include <condition_variable>
#include <functional>
#include <map>
#include <thread>
#include <vector>

#include <hardware_legacy/wifi_hal.h>
#include <wifi_system/interface_tool.h>

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {
// This is in a separate namespace to prevent typename conflicts between
// the legacy HAL types and the HIDL interface types.
namespace legacy_hal {
// Import all the types defined inside the legacy HAL header files into this
// namespace.
using ::frame_info;
using ::frame_type;
using ::FRAME_TYPE_80211_MGMT;
using ::FRAME_TYPE_ETHERNET_II;
using ::FRAME_TYPE_UNKNOWN;
using ::fw_roaming_state_t;
using ::mac_addr;
using ::NAN_CHANNEL_24G_BAND;
using ::NAN_CHANNEL_5G_BAND_HIGH;
using ::NAN_CHANNEL_5G_BAND_LOW;
using ::NAN_DISABLE_RANGE_REPORT;
using ::NAN_DO_NOT_USE_SRF;
using ::NAN_DP_CHANNEL_NOT_REQUESTED;
using ::NAN_DP_CONFIG_NO_SECURITY;
using ::NAN_DP_CONFIG_SECURITY;
using ::NAN_DP_END;
using ::NAN_DP_FORCE_CHANNEL_SETUP;
using ::NAN_DP_INITIATOR_RESPONSE;
using ::NAN_DP_INTERFACE_CREATE;
using ::NAN_DP_INTERFACE_DELETE;
using ::NAN_DP_REQUEST_ACCEPT;
using ::NAN_DP_REQUEST_CHANNEL_SETUP;
using ::NAN_DP_REQUEST_REJECT;
using ::NAN_DP_RESPONDER_RESPONSE;
using ::NAN_GET_CAPABILITIES;
using ::NAN_MATCH_ALG_MATCH_CONTINUOUS;
using ::NAN_MATCH_ALG_MATCH_NEVER;
using ::NAN_MATCH_ALG_MATCH_ONCE;
using ::NAN_PUBLISH_TYPE_SOLICITED;
using ::NAN_PUBLISH_TYPE_UNSOLICITED;
using ::NAN_PUBLISH_TYPE_UNSOLICITED_SOLICITED;
using ::NAN_RANGING_AUTO_RESPONSE_DISABLE;
using ::NAN_RANGING_AUTO_RESPONSE_ENABLE;
using ::NAN_RANGING_DISABLE;
using ::NAN_RANGING_ENABLE;
using ::NAN_RESPONSE_BEACON_SDF_PAYLOAD;
using ::NAN_RESPONSE_CONFIG;
using ::NAN_RESPONSE_DISABLED;
using ::NAN_RESPONSE_ENABLED;
using ::NAN_RESPONSE_ERROR;
using ::NAN_RESPONSE_PUBLISH;
using ::NAN_RESPONSE_PUBLISH_CANCEL;
using ::NAN_RESPONSE_STATS;
using ::NAN_RESPONSE_SUBSCRIBE;
using ::NAN_RESPONSE_SUBSCRIBE_CANCEL;
using ::NAN_RESPONSE_TCA;
using ::NAN_RESPONSE_TRANSMIT_FOLLOWUP;
using ::NAN_SECURITY_KEY_INPUT_PASSPHRASE;
using ::NAN_SECURITY_KEY_INPUT_PMK;
using ::NAN_SERVICE_ACCEPT_POLICY_ALL;
using ::NAN_SERVICE_ACCEPT_POLICY_NONE;
using ::NAN_SRF_ATTR_BLOOM_FILTER;
using ::NAN_SRF_ATTR_PARTIAL_MAC_ADDR;
using ::NAN_SRF_INCLUDE_DO_NOT_RESPOND;
using ::NAN_SRF_INCLUDE_RESPOND;
using ::NAN_SSI_NOT_REQUIRED_IN_MATCH_IND;
using ::NAN_SSI_REQUIRED_IN_MATCH_IND;
using ::NAN_STATUS_ALREADY_ENABLED;
using ::NAN_STATUS_FOLLOWUP_QUEUE_FULL;
using ::NAN_STATUS_INTERNAL_FAILURE;
using ::NAN_STATUS_INVALID_NDP_ID;
using ::NAN_STATUS_INVALID_PARAM;
using ::NAN_STATUS_INVALID_PUBLISH_SUBSCRIBE_ID;
using ::NAN_STATUS_INVALID_REQUESTOR_INSTANCE_ID;
using ::NAN_STATUS_NAN_NOT_ALLOWED;
using ::NAN_STATUS_NO_OTA_ACK;
using ::NAN_STATUS_NO_RESOURCE_AVAILABLE;
using ::NAN_STATUS_PROTOCOL_FAILURE;
using ::NAN_STATUS_SUCCESS;
using ::NAN_STATUS_UNSUPPORTED_CONCURRENCY_NAN_DISABLED;
using ::NAN_SUBSCRIBE_TYPE_ACTIVE;
using ::NAN_SUBSCRIBE_TYPE_PASSIVE;
using ::NAN_TRANSMIT_IN_DW;
using ::NAN_TRANSMIT_IN_FAW;
using ::NAN_TX_PRIORITY_HIGH;
using ::NAN_TX_PRIORITY_NORMAL;
using ::NAN_TX_TYPE_BROADCAST;
using ::NAN_TX_TYPE_UNICAST;
using ::NAN_USE_SRF;
using ::NanBeaconSdfPayloadInd;
using ::NanCapabilities;
using ::NanChannelInfo;
using ::NanConfigRequest;
using ::NanDataPathChannelCfg;
using ::NanDataPathConfirmInd;
using ::NanDataPathEndInd;
using ::NanDataPathIndicationResponse;
using ::NanDataPathInitiatorRequest;
using ::NanDataPathRequestInd;
using ::NanDataPathScheduleUpdateInd;
using ::NanDisabledInd;
using ::NanDiscEngEventInd;
using ::NanEnableRequest;
using ::NanFollowupInd;
using ::NanMatchAlg;
using ::NanMatchExpiredInd;
using ::NanMatchInd;
using ::NanPublishCancelRequest;
using ::NanPublishRequest;
using ::NanPublishTerminatedInd;
using ::NanPublishType;
using ::NanRangeReportInd;
using ::NanRangeRequestInd;
using ::NanResponseMsg;
using ::NanSRFType;
using ::NanStatusType;
using ::NanSubscribeCancelRequest;
using ::NanSubscribeRequest;
using ::NanSubscribeTerminatedInd;
using ::NanSubscribeType;
using ::NanTransmitFollowupInd;
using ::NanTransmitFollowupRequest;
using ::NanTxType;
using ::ROAMING_DISABLE;
using ::ROAMING_ENABLE;
using ::RTT_PEER_AP;
using ::RTT_PEER_NAN;
using ::RTT_PEER_P2P_CLIENT;
using ::RTT_PEER_P2P_GO;
using ::RTT_PEER_STA;
using ::rtt_peer_type;
using ::RTT_STATUS_ABORTED;
using ::RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL;
using ::RTT_STATUS_FAIL_BUSY_TRY_LATER;
using ::RTT_STATUS_FAIL_FTM_PARAM_OVERRIDE;
using ::RTT_STATUS_FAIL_INVALID_TS;
using ::RTT_STATUS_FAIL_NO_CAPABILITY;
using ::RTT_STATUS_FAIL_NO_RSP;
using ::RTT_STATUS_FAIL_NOT_SCHEDULED_YET;
using ::RTT_STATUS_FAIL_PROTOCOL;
using ::RTT_STATUS_FAIL_REJECTED;
using ::RTT_STATUS_FAIL_SCHEDULE;
using ::RTT_STATUS_FAIL_TM_TIMEOUT;
using ::RTT_STATUS_FAILURE;
using ::RTT_STATUS_INVALID_REQ;
using ::RTT_STATUS_NAN_RANGING_CONCURRENCY_NOT_SUPPORTED;
using ::RTT_STATUS_NAN_RANGING_PROTOCOL_FAILURE;
using ::RTT_STATUS_NO_WIFI;
using ::RTT_STATUS_SUCCESS;
using ::RTT_TYPE_1_SIDED;
using ::RTT_TYPE_2_SIDED;
using ::RX_PKT_FATE_DRV_DROP_FILTER;
using ::RX_PKT_FATE_DRV_DROP_INVALID;
using ::RX_PKT_FATE_DRV_DROP_NOBUFS;
using ::RX_PKT_FATE_DRV_DROP_OTHER;
using ::RX_PKT_FATE_DRV_QUEUED;
using ::RX_PKT_FATE_FW_DROP_FILTER;
using ::RX_PKT_FATE_FW_DROP_INVALID;
using ::RX_PKT_FATE_FW_DROP_NOBUFS;
using ::RX_PKT_FATE_FW_DROP_OTHER;
using ::RX_PKT_FATE_FW_QUEUED;
using ::RX_PKT_FATE_SUCCESS;
using ::ssid_t;
using ::transaction_id;
using ::TX_PKT_FATE_ACKED;
using ::TX_PKT_FATE_DRV_DROP_INVALID;
using ::TX_PKT_FATE_DRV_DROP_NOBUFS;
using ::TX_PKT_FATE_DRV_DROP_OTHER;
using ::TX_PKT_FATE_DRV_QUEUED;
using ::TX_PKT_FATE_FW_DROP_INVALID;
using ::TX_PKT_FATE_FW_DROP_NOBUFS;
using ::TX_PKT_FATE_FW_DROP_OTHER;
using ::TX_PKT_FATE_FW_QUEUED;
using ::TX_PKT_FATE_SENT;
using ::WIFI_AC_BE;
using ::WIFI_AC_BK;
using ::WIFI_AC_VI;
using ::WIFI_AC_VO;
using ::wifi_band;
using ::WIFI_BAND_A;
using ::WIFI_BAND_A_DFS;
using ::WIFI_BAND_A_WITH_DFS;
using ::WIFI_BAND_ABG;
using ::WIFI_BAND_ABG_WITH_DFS;
using ::WIFI_BAND_BG;
using ::WIFI_BAND_UNSPECIFIED;
using ::wifi_cached_scan_results;
using ::WIFI_CHAN_WIDTH_10;
using ::WIFI_CHAN_WIDTH_160;
using ::WIFI_CHAN_WIDTH_20;
using ::WIFI_CHAN_WIDTH_40;
using ::WIFI_CHAN_WIDTH_5;
using ::WIFI_CHAN_WIDTH_80;
using ::WIFI_CHAN_WIDTH_80P80;
using ::WIFI_CHAN_WIDTH_INVALID;
using ::wifi_channel_info;
using ::wifi_channel_stat;
using ::wifi_channel_width;
using ::wifi_coex_restriction;
using ::wifi_coex_unsafe_channel;
using ::WIFI_DUAL_STA_NON_TRANSIENT_UNBIASED;
using ::WIFI_DUAL_STA_TRANSIENT_PREFER_PRIMARY;
using ::wifi_error;
using ::WIFI_ERROR_BUSY;
using ::WIFI_ERROR_INVALID_ARGS;
using ::WIFI_ERROR_INVALID_REQUEST_ID;
using ::WIFI_ERROR_NONE;
using ::WIFI_ERROR_NOT_AVAILABLE;
using ::WIFI_ERROR_NOT_SUPPORTED;
using ::WIFI_ERROR_OUT_OF_MEMORY;
using ::WIFI_ERROR_TIMED_OUT;
using ::WIFI_ERROR_TOO_MANY_REQUESTS;
using ::WIFI_ERROR_UNINITIALIZED;
using ::WIFI_ERROR_UNKNOWN;
using ::wifi_gscan_capabilities;
using ::wifi_hal_fn;
using ::wifi_information_element;
using ::WIFI_INTERFACE_IBSS;
using ::WIFI_INTERFACE_MESH;
using ::wifi_interface_mode;
using ::WIFI_INTERFACE_NAN;
using ::WIFI_INTERFACE_P2P_CLIENT;
using ::WIFI_INTERFACE_P2P_GO;
using ::WIFI_INTERFACE_SOFTAP;
using ::WIFI_INTERFACE_STA;
using ::WIFI_INTERFACE_TDLS;
using ::wifi_interface_type;
using ::WIFI_INTERFACE_TYPE_AP;
using ::WIFI_INTERFACE_TYPE_NAN;
using ::WIFI_INTERFACE_TYPE_P2P;
using ::WIFI_INTERFACE_TYPE_STA;
using ::WIFI_INTERFACE_UNKNOWN;
using ::wifi_latency_mode;
using ::WIFI_LATENCY_MODE_LOW;
using ::WIFI_LATENCY_MODE_NORMAL;
using ::wifi_lci_information;
using ::wifi_lcr_information;
using ::WIFI_LOGGER_CONNECT_EVENT_SUPPORTED;
using ::WIFI_LOGGER_DRIVER_DUMP_SUPPORTED;
using ::WIFI_LOGGER_MEMORY_DUMP_SUPPORTED;
using ::WIFI_LOGGER_PACKET_FATE_SUPPORTED;
using ::WIFI_LOGGER_POWER_EVENT_SUPPORTED;
using ::WIFI_LOGGER_WAKE_LOCK_SUPPORTED;
using ::WIFI_MOTION_EXPECTED;
using ::WIFI_MOTION_NOT_EXPECTED;
using ::wifi_motion_pattern;
using ::WIFI_MOTION_UNKNOWN;
using ::wifi_multi_sta_use_case;
using ::wifi_power_scenario;
using ::WIFI_POWER_SCENARIO_ON_BODY_CELL_OFF;
using ::WIFI_POWER_SCENARIO_ON_BODY_CELL_ON;
using ::WIFI_POWER_SCENARIO_ON_HEAD_CELL_OFF;
using ::WIFI_POWER_SCENARIO_ON_HEAD_CELL_ON;
using ::WIFI_POWER_SCENARIO_VOICE_CALL;
using ::wifi_rate;
using ::wifi_request_id;
using ::wifi_ring_buffer_status;
using ::wifi_roaming_capabilities;
using ::wifi_roaming_config;
using ::wifi_rtt_bw;
using ::WIFI_RTT_BW_10;
using ::WIFI_RTT_BW_160;
using ::WIFI_RTT_BW_20;
using ::WIFI_RTT_BW_40;
using ::WIFI_RTT_BW_5;
using ::WIFI_RTT_BW_80;
using ::wifi_rtt_capabilities;
using ::wifi_rtt_config;
using ::wifi_rtt_preamble;
using ::WIFI_RTT_PREAMBLE_HE;
using ::WIFI_RTT_PREAMBLE_HT;
using ::WIFI_RTT_PREAMBLE_LEGACY;
using ::WIFI_RTT_PREAMBLE_VHT;
using ::wifi_rtt_responder;
using ::wifi_rtt_result;
using ::wifi_rtt_status;
using ::wifi_rtt_type;
using ::wifi_rx_packet_fate;
using ::wifi_rx_report;
using ::wifi_scan_bucket_spec;
using ::wifi_scan_cmd_params;
using ::WIFI_SCAN_FLAG_INTERRUPTED;
using ::wifi_scan_result;
using ::WIFI_SUCCESS;
using ::wifi_tx_packet_fate;
using ::wifi_tx_report;
using ::wifi_usable_channel;
using ::WIFI_USABLE_CHANNEL_FILTER_CELLULAR_COEXISTENCE;
using ::WIFI_USABLE_CHANNEL_FILTER_CONCURRENCY;
using ::WLAN_MAC_2_4_BAND;
using ::WLAN_MAC_5_0_BAND;
using ::WLAN_MAC_60_0_BAND;
using ::WLAN_MAC_6_0_BAND;

// APF capabilities supported by the iface.
struct PacketFilterCapabilities {
    uint32_t version;
    uint32_t max_len;
};

// WARNING: We don't care about the variable sized members of either
// |wifi_iface_stat|, |wifi_radio_stat| structures. So, using the pragma
// to escape the compiler warnings regarding this.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-variable-sized-type-not-at-end"
// The |wifi_radio_stat.tx_time_per_levels| stats is provided as a pointer in
// |wifi_radio_stat| structure in the legacy HAL API. Separate that out
// into a separate return element to avoid passing pointers around.
struct LinkLayerRadioStats {
    wifi_radio_stat stats;
    std::vector<uint32_t> tx_time_per_levels;
    std::vector<wifi_channel_stat> channel_stats;
};

struct WifiPeerInfo {
    wifi_peer_info peer_info;
    std::vector<wifi_rate_stat> rate_stats;
};

struct LinkLayerStats {
    wifi_iface_stat iface;
    std::vector<LinkLayerRadioStats> radios;
    std::vector<WifiPeerInfo> peers;
};
#pragma GCC diagnostic pop

// The |WLAN_DRIVER_WAKE_REASON_CNT.cmd_event_wake_cnt| and
// |WLAN_DRIVER_WAKE_REASON_CNT.driver_fw_local_wake_cnt| stats is provided
// as a pointer in |WLAN_DRIVER_WAKE_REASON_CNT| structure in the legacy HAL
// API. Separate that out into a separate return elements to avoid passing
// pointers around.
struct WakeReasonStats {
    WLAN_DRIVER_WAKE_REASON_CNT wake_reason_cnt;
    std::vector<uint32_t> cmd_event_wake_cnt;
    std::vector<uint32_t> driver_fw_local_wake_cnt;
};

// NAN response and event callbacks struct.
struct NanCallbackHandlers {
    // NotifyResponse invoked to notify the status of the Request.
    std::function<void(transaction_id, const NanResponseMsg&)>
        on_notify_response;
    // Various event callbacks.
    std::function<void(const NanPublishTerminatedInd&)>
        on_event_publish_terminated;
    std::function<void(const NanMatchInd&)> on_event_match;
    std::function<void(const NanMatchExpiredInd&)> on_event_match_expired;
    std::function<void(const NanSubscribeTerminatedInd&)>
        on_event_subscribe_terminated;
    std::function<void(const NanFollowupInd&)> on_event_followup;
    std::function<void(const NanDiscEngEventInd&)> on_event_disc_eng_event;
    std::function<void(const NanDisabledInd&)> on_event_disabled;
    std::function<void(const NanTCAInd&)> on_event_tca;
    std::function<void(const NanBeaconSdfPayloadInd&)>
        on_event_beacon_sdf_payload;
    std::function<void(const NanDataPathRequestInd&)>
        on_event_data_path_request;
    std::function<void(const NanDataPathConfirmInd&)>
        on_event_data_path_confirm;
    std::function<void(const NanDataPathEndInd&)> on_event_data_path_end;
    std::function<void(const NanTransmitFollowupInd&)>
        on_event_transmit_follow_up;
    std::function<void(const NanRangeRequestInd&)> on_event_range_request;
    std::function<void(const NanRangeReportInd&)> on_event_range_report;
    std::function<void(const NanDataPathScheduleUpdateInd&)>
        on_event_schedule_update;
};

// Full scan results contain IE info and are hence passed by reference, to
// preserve the variable length array member |ie_data|. Callee must not retain
// the pointer.
using on_gscan_full_result_callback =
    std::function<void(wifi_request_id, const wifi_scan_result*, uint32_t)>;
// These scan results don't contain any IE info, so no need to pass by
// reference.
using on_gscan_results_callback = std::function<void(
    wifi_request_id, const std::vector<wifi_cached_scan_results>&)>;

// Invoked when the rssi value breaches the thresholds set.
using on_rssi_threshold_breached_callback =
    std::function<void(wifi_request_id, std::array<uint8_t, 6>, int8_t)>;

// Callback for RTT range request results.
// Rtt results contain IE info and are hence passed by reference, to
// preserve the |LCI| and |LCR| pointers. Callee must not retain
// the pointer.
using on_rtt_results_callback = std::function<void(
    wifi_request_id, const std::vector<const wifi_rtt_result*>&)>;

// Callback for ring buffer data.
using on_ring_buffer_data_callback =
    std::function<void(const std::string&, const std::vector<uint8_t>&,
                       const wifi_ring_buffer_status&)>;

// Callback for alerts.
using on_error_alert_callback =
    std::function<void(int32_t, const std::vector<uint8_t>&)>;

// Callback for subsystem restart
using on_subsystem_restart_callback = std::function<void(const std::string&)>;

// Struct for the mac info from the legacy HAL. This is a cleaner version
// of the |wifi_mac_info| & |wifi_iface_info|.
typedef struct {
    std::string name;
    wifi_channel channel;
} WifiIfaceInfo;

typedef struct {
    uint32_t wlan_mac_id;
    /* BIT MASK of BIT(WLAN_MAC*) as represented by wlan_mac_band */
    uint32_t mac_band;
    /* Represents the connected Wi-Fi interfaces associated with each MAC */
    std::vector<WifiIfaceInfo> iface_infos;
} WifiMacInfo;

// Callback for radio mode change
using on_radio_mode_change_callback =
    std::function<void(const std::vector<WifiMacInfo>&)>;

// TWT response and event callbacks struct.
struct TwtCallbackHandlers {
    // Callback for TWT setup response
    std::function<void(const TwtSetupResponse&)> on_setup_response;
    // Callback for TWT teardown completion
    std::function<void(const TwtTeardownCompletion&)> on_teardown_completion;
    // Callback for TWT info frame received event
    std::function<void(const TwtInfoFrameReceived&)> on_info_frame_received;
    // Callback for TWT notification from the device
    std::function<void(const TwtDeviceNotify&)> on_device_notify;
};

/**
 * Class that encapsulates all legacy HAL interactions.
 * This class manages the lifetime of the event loop thread used by legacy HAL.
 *
 * Note: There will only be a single instance of this class created in the Wifi
 * object and will be valid for the lifetime of the process.
 */
class WifiLegacyHal {
   public:
    WifiLegacyHal(const std::weak_ptr<wifi_system::InterfaceTool> iface_tool,
                  const wifi_hal_fn& fn, bool is_primary);
    virtual ~WifiLegacyHal() = default;

    // Initialize the legacy HAL function table.
    virtual wifi_error initialize();
    // Start the legacy HAL and the event looper thread.
    virtual wifi_error start();
    // Deinitialize the legacy HAL and wait for the event loop thread to exit
    // using a predefined timeout.
    virtual wifi_error stop(std::unique_lock<std::recursive_mutex>* lock,
                            const std::function<void()>& on_complete_callback);
    virtual wifi_error waitForDriverReady();
    // Checks if legacy HAL has successfully started
    bool isStarted();
    // Wrappers for all the functions in the legacy HAL function table.
    virtual std::pair<wifi_error, std::string> getDriverVersion(
        const std::string& iface_name);
    virtual std::pair<wifi_error, std::string> getFirmwareVersion(
        const std::string& iface_name);
    std::pair<wifi_error, std::vector<uint8_t>> requestDriverMemoryDump(
        const std::string& iface_name);
    std::pair<wifi_error, std::vector<uint8_t>> requestFirmwareMemoryDump(
        const std::string& iface_name);
    std::pair<wifi_error, uint64_t> getSupportedFeatureSet(
        const std::string& iface_name);
    // APF functions.
    std::pair<wifi_error, PacketFilterCapabilities> getPacketFilterCapabilities(
        const std::string& iface_name);
    wifi_error setPacketFilter(const std::string& iface_name,
                               const std::vector<uint8_t>& program);
    std::pair<wifi_error, std::vector<uint8_t>> readApfPacketFilterData(
        const std::string& iface_name);
    // Gscan functions.
    std::pair<wifi_error, wifi_gscan_capabilities> getGscanCapabilities(
        const std::string& iface_name);
    // These API's provides a simplified interface over the legacy Gscan API's:
    // a) All scan events from the legacy HAL API other than the
    //    |WIFI_SCAN_FAILED| are treated as notification of results.
    //    This method then retrieves the cached scan results from the legacy
    //    HAL API and triggers the externally provided
    //    |on_results_user_callback| on success.
    // b) |WIFI_SCAN_FAILED| scan event or failure to retrieve cached scan
    // results
    //    triggers the externally provided |on_failure_user_callback|.
    // c) Full scan result event triggers the externally provided
    //    |on_full_result_user_callback|.
    wifi_error startGscan(
        const std::string& iface_name, wifi_request_id id,
        const wifi_scan_cmd_params& params,
        const std::function<void(wifi_request_id)>& on_failure_callback,
        const on_gscan_results_callback& on_results_callback,
        const on_gscan_full_result_callback& on_full_result_callback);
    wifi_error stopGscan(const std::string& iface_name, wifi_request_id id);
    std::pair<wifi_error, std::vector<uint32_t>> getValidFrequenciesForBand(
        const std::string& iface_name, wifi_band band);
    virtual wifi_error setDfsFlag(const std::string& iface_name, bool dfs_on);
    // Link layer stats functions.
    wifi_error enableLinkLayerStats(const std::string& iface_name, bool debug);
    wifi_error disableLinkLayerStats(const std::string& iface_name);
    std::pair<wifi_error, LinkLayerStats> getLinkLayerStats(
        const std::string& iface_name);
    // RSSI monitor functions.
    wifi_error startRssiMonitoring(const std::string& iface_name,
                                   wifi_request_id id, int8_t max_rssi,
                                   int8_t min_rssi,
                                   const on_rssi_threshold_breached_callback&
                                       on_threshold_breached_callback);
    wifi_error stopRssiMonitoring(const std::string& iface_name,
                                  wifi_request_id id);
    std::pair<wifi_error, wifi_roaming_capabilities> getRoamingCapabilities(
        const std::string& iface_name);
    wifi_error configureRoaming(const std::string& iface_name,
                                const wifi_roaming_config& config);
    wifi_error enableFirmwareRoaming(const std::string& iface_name,
                                     fw_roaming_state_t state);
    wifi_error configureNdOffload(const std::string& iface_name, bool enable);
    wifi_error startSendingOffloadedPacket(
        const std::string& iface_name, uint32_t cmd_id, uint16_t ether_type,
        const std::vector<uint8_t>& ip_packet_data,
        const std::array<uint8_t, 6>& src_address,
        const std::array<uint8_t, 6>& dst_address, uint32_t period_in_ms);
    wifi_error stopSendingOffloadedPacket(const std::string& iface_name,
                                          uint32_t cmd_id);
    virtual wifi_error selectTxPowerScenario(const std::string& iface_name,
                                             wifi_power_scenario scenario);
    virtual wifi_error resetTxPowerScenario(const std::string& iface_name);
    wifi_error setLatencyMode(const std::string& iface_name,
                              wifi_latency_mode mode);
    wifi_error setThermalMitigationMode(wifi_thermal_mode mode,
                                        uint32_t completion_window);
    wifi_error setDscpToAccessCategoryMapping(uint32_t start, uint32_t end,
                                              uint32_t access_category);
    wifi_error resetDscpToAccessCategoryMapping();
    // Logger/debug functions.
    std::pair<wifi_error, uint32_t> getLoggerSupportedFeatureSet(
        const std::string& iface_name);
    wifi_error startPktFateMonitoring(const std::string& iface_name);
    std::pair<wifi_error, std::vector<wifi_tx_report>> getTxPktFates(
        const std::string& iface_name);
    std::pair<wifi_error, std::vector<wifi_rx_report>> getRxPktFates(
        const std::string& iface_name);
    std::pair<wifi_error, WakeReasonStats> getWakeReasonStats(
        const std::string& iface_name);
    wifi_error registerRingBufferCallbackHandler(
        const std::string& iface_name,
        const on_ring_buffer_data_callback& on_data_callback);
    wifi_error deregisterRingBufferCallbackHandler(
        const std::string& iface_name);
    wifi_error registerSubsystemRestartCallbackHandler(
        const on_subsystem_restart_callback& on_restart_callback);
    std::pair<wifi_error, std::vector<wifi_ring_buffer_status>>
    getRingBuffersStatus(const std::string& iface_name);
    wifi_error startRingBufferLogging(const std::string& iface_name,
                                      const std::string& ring_name,
                                      uint32_t verbose_level,
                                      uint32_t max_interval_sec,
                                      uint32_t min_data_size);
    wifi_error getRingBufferData(const std::string& iface_name,
                                 const std::string& ring_name);
    wifi_error registerErrorAlertCallbackHandler(
        const std::string& iface_name,
        const on_error_alert_callback& on_alert_callback);
    wifi_error deregisterErrorAlertCallbackHandler(
        const std::string& iface_name);
    // Radio mode functions.
    virtual wifi_error registerRadioModeChangeCallbackHandler(
        const std::string& iface_name,
        const on_radio_mode_change_callback& on_user_change_callback);
    // RTT functions.
    wifi_error startRttRangeRequest(
        const std::string& iface_name, wifi_request_id id,
        const std::vector<wifi_rtt_config>& rtt_configs,
        const on_rtt_results_callback& on_results_callback);
    wifi_error cancelRttRangeRequest(
        const std::string& iface_name, wifi_request_id id,
        const std::vector<std::array<uint8_t, 6>>& mac_addrs);
    std::pair<wifi_error, wifi_rtt_capabilities> getRttCapabilities(
        const std::string& iface_name);
    std::pair<wifi_error, wifi_rtt_responder> getRttResponderInfo(
        const std::string& iface_name);
    wifi_error enableRttResponder(const std::string& iface_name,
                                  wifi_request_id id,
                                  const wifi_channel_info& channel_hint,
                                  uint32_t max_duration_secs,
                                  const wifi_rtt_responder& info);
    wifi_error disableRttResponder(const std::string& iface_name,
                                   wifi_request_id id);
    wifi_error setRttLci(const std::string& iface_name, wifi_request_id id,
                         const wifi_lci_information& info);
    wifi_error setRttLcr(const std::string& iface_name, wifi_request_id id,
                         const wifi_lcr_information& info);
    // NAN functions.
    virtual wifi_error nanRegisterCallbackHandlers(
        const std::string& iface_name, const NanCallbackHandlers& callbacks);
    wifi_error nanEnableRequest(const std::string& iface_name,
                                transaction_id id, const NanEnableRequest& msg);
    virtual wifi_error nanDisableRequest(const std::string& iface_name,
                                         transaction_id id);
    wifi_error nanPublishRequest(const std::string& iface_name,
                                 transaction_id id,
                                 const NanPublishRequest& msg);
    wifi_error nanPublishCancelRequest(const std::string& iface_name,
                                       transaction_id id,
                                       const NanPublishCancelRequest& msg);
    wifi_error nanSubscribeRequest(const std::string& iface_name,
                                   transaction_id id,
                                   const NanSubscribeRequest& msg);
    wifi_error nanSubscribeCancelRequest(const std::string& iface_name,
                                         transaction_id id,
                                         const NanSubscribeCancelRequest& msg);
    wifi_error nanTransmitFollowupRequest(
        const std::string& iface_name, transaction_id id,
        const NanTransmitFollowupRequest& msg);
    wifi_error nanStatsRequest(const std::string& iface_name, transaction_id id,
                               const NanStatsRequest& msg);
    wifi_error nanConfigRequest(const std::string& iface_name,
                                transaction_id id, const NanConfigRequest& msg);
    wifi_error nanTcaRequest(const std::string& iface_name, transaction_id id,
                             const NanTCARequest& msg);
    wifi_error nanBeaconSdfPayloadRequest(
        const std::string& iface_name, transaction_id id,
        const NanBeaconSdfPayloadRequest& msg);
    std::pair<wifi_error, NanVersion> nanGetVersion();
    wifi_error nanGetCapabilities(const std::string& iface_name,
                                  transaction_id id);
    wifi_error nanDataInterfaceCreate(const std::string& iface_name,
                                      transaction_id id,
                                      const std::string& data_iface_name);
    virtual wifi_error nanDataInterfaceDelete(
        const std::string& iface_name, transaction_id id,
        const std::string& data_iface_name);
    wifi_error nanDataRequestInitiator(const std::string& iface_name,
                                       transaction_id id,
                                       const NanDataPathInitiatorRequest& msg);
    wifi_error nanDataIndicationResponse(
        const std::string& iface_name, transaction_id id,
        const NanDataPathIndicationResponse& msg);
    wifi_error nanDataEnd(const std::string& iface_name, transaction_id id,
                          uint32_t ndpInstanceId);
    // AP functions.
    wifi_error setCountryCode(const std::string& iface_name,
                              std::array<int8_t, 2> code);

    // interface functions.
    virtual wifi_error createVirtualInterface(const std::string& ifname,
                                              wifi_interface_type iftype);
    virtual wifi_error deleteVirtualInterface(const std::string& ifname);
    wifi_error getSupportedIfaceName(uint32_t iface_type, std::string& ifname);

    // STA + STA functions
    virtual wifi_error multiStaSetPrimaryConnection(const std::string& ifname);
    virtual wifi_error multiStaSetUseCase(wifi_multi_sta_use_case use_case);

    // Coex functions.
    virtual wifi_error setCoexUnsafeChannels(
        std::vector<wifi_coex_unsafe_channel> unsafe_channels,
        uint32_t restrictions);

    wifi_error setVoipMode(const std::string& iface_name, wifi_voip_mode mode);

    wifi_error twtRegisterHandler(const std::string& iface_name,
                                  const TwtCallbackHandlers& handler);

    std::pair<wifi_error, TwtCapabilitySet> twtGetCapability(
        const std::string& iface_name);

    wifi_error twtSetupRequest(const std::string& iface_name,
                               const TwtSetupRequest& msg);

    wifi_error twtTearDownRequest(const std::string& iface_name,
                                  const TwtTeardownRequest& msg);

    wifi_error twtInfoFrameRequest(const std::string& iface_name,
                                   const TwtInfoFrameRequest& msg);

    std::pair<wifi_error, TwtStats> twtGetStats(const std::string& iface_name,
                                                uint8_t configId);

    wifi_error twtClearStats(const std::string& iface_name, uint8_t configId);

    wifi_error setDtimConfig(const std::string& iface_name,
                             uint32_t multiplier);

    // Retrieve the list of usable channels in the requested bands
    // for the requested modes
    std::pair<wifi_error, std::vector<wifi_usable_channel>> getUsableChannels(
        uint32_t band_mask, uint32_t iface_mode_mask, uint32_t filter_mask);

    wifi_error triggerSubsystemRestart();

   private:
    // Retrieve interface handles for all the available interfaces.
    wifi_error retrieveIfaceHandles();
    wifi_interface_handle getIfaceHandle(const std::string& iface_name);
    // Run the legacy HAL event loop thread.
    void runEventLoop();
    // Retrieve the cached gscan results to pass the results back to the
    // external callbacks.
    std::pair<wifi_error, std::vector<wifi_cached_scan_results>>
    getGscanCachedResults(const std::string& iface_name);
    void invalidate();
    // Handles wifi (error) status of Virtual interface create/delete
    wifi_error handleVirtualInterfaceCreateOrDeleteStatus(
        const std::string& ifname, wifi_error status);

    // Global function table of legacy HAL.
    wifi_hal_fn global_func_table_;
    // Opaque handle to be used for all global operations.
    wifi_handle global_handle_;
    // Map of interface name to handle that is to be used for all interface
    // specific operations.
    std::map<std::string, wifi_interface_handle> iface_name_to_handle_;
    // Flag to indicate if we have initiated the cleanup of legacy HAL.
    std::atomic<bool> awaiting_event_loop_termination_;
    std::condition_variable_any stop_wait_cv_;
    // Flag to indicate if the legacy HAL has been started.
    bool is_started_;
    std::weak_ptr<wifi_system::InterfaceTool> iface_tool_;
    // flag to indicate if this HAL is for the primary chip. This is used
    // in order to avoid some hard-coded behavior used with older HALs,
    // such as bring wlan0 interface up/down on start/stop HAL.
    // it may be removed once vendor HALs are updated.
    bool is_primary_;
};

}  // namespace legacy_hal
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_LEGACY_HAL_H_
