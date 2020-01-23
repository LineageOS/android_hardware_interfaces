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

#include <wifi_system/interface_tool.h>

// HACK: The include inside the namespace below also transitively includes a
// bunch of libc headers into the namespace, which leads to functions like
// socketpair being defined in
// android::hardware::wifi::V1_1::implementation::legacy_hal. Include this one
// particular header as a hacky workaround until that's fixed.
#include <sys/socket.h>

namespace android {
namespace hardware {
namespace wifi {
namespace V1_4 {
namespace implementation {
// This is in a separate namespace to prevent typename conflicts between
// the legacy HAL types and the HIDL interface types.
namespace legacy_hal {
// Wrap all the types defined inside the legacy HAL header files inside this
// namespace.
#include <hardware_legacy/wifi_hal.h>

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

struct LinkLayerStats {
    wifi_iface_stat iface;
    std::vector<LinkLayerRadioStats> radios;
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

/**
 * Class that encapsulates all legacy HAL interactions.
 * This class manages the lifetime of the event loop thread used by legacy HAL.
 *
 * Note: There will only be a single instance of this class created in the Wifi
 * object and will be valid for the lifetime of the process.
 */
class WifiLegacyHal {
   public:
    WifiLegacyHal(const std::weak_ptr<wifi_system::InterfaceTool> iface_tool);
    virtual ~WifiLegacyHal() = default;

    // Initialize the legacy HAL function table.
    virtual wifi_error initialize();
    // Start the legacy HAL and the event looper thread.
    virtual wifi_error start();
    // Deinitialize the legacy HAL and wait for the event loop thread to exit
    // using a predefined timeout.
    virtual wifi_error stop(std::unique_lock<std::recursive_mutex>* lock,
                            const std::function<void()>& on_complete_callback);
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
    std::pair<wifi_error, uint32_t> getSupportedFeatureSet(
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
    wifi_error setScanningMacOui(const std::string& iface_name,
                                 const std::array<uint8_t, 3>& oui);
    virtual wifi_error selectTxPowerScenario(const std::string& iface_name,
                                             wifi_power_scenario scenario);
    virtual wifi_error resetTxPowerScenario(const std::string& iface_name);
    wifi_error setLatencyMode(const std::string& iface_name,
                              wifi_latency_mode mode);
    wifi_error setThermalMitigationMode(wifi_thermal_mode mode,
                                        uint32_t completion_window);
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
};

}  // namespace legacy_hal
}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_LEGACY_HAL_H_
