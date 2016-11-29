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

#ifndef WIFI_LEGACY_WIFI_HAL_H_
#define WIFI_LEGACY_WIFI_HAL_H_

#include <functional>
#include <thread>
#include <vector>

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
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
struct LinkLayerStats {
  wifi_iface_stat iface;
  wifi_radio_stat radio;
  std::vector<uint32_t> radio_tx_time_per_levels;
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

// Full scan results contain IE info and are hence passed by reference, to
// preserve the variable length array member |ie_data|. Callee must not retain
// the pointer.
using on_gscan_full_result_callback =
    std::function<void(wifi_request_id, const wifi_scan_result*, uint32_t)>;
// These scan results don't contain any IE info, so no need to pass by
// reference.
using on_gscan_results_callback = std::function<void(
    wifi_request_id, const std::vector<wifi_cached_scan_results>&)>;

// Callback for ring buffer data.
using on_ring_buffer_data_callback =
    std::function<void(const std::string&,
                       const std::vector<uint8_t>&,
                       const wifi_ring_buffer_status&)>;

/**
 * Class that encapsulates all legacy HAL interactions.
 * This class manages the lifetime of the event loop thread used by legacy HAL.
 */
class WifiLegacyHal {
 public:
  WifiLegacyHal();
  // Names to use for the different types of iface.
  std::string getApIfaceName();
  std::string getNanIfaceName();
  std::string getP2pIfaceName();
  std::string getStaIfaceName();

  // Initialize the legacy HAL and start the event looper thread.
  wifi_error start();
  // Deinitialize the legacy HAL and stop the event looper thread.
  wifi_error stop(const std::function<void()>& on_complete_callback);
  // Wrappers for all the functions in the legacy HAL function table.
  std::pair<wifi_error, std::string> getDriverVersion();
  std::pair<wifi_error, std::string> getFirmwareVersion();
  std::pair<wifi_error, std::vector<uint8_t>> requestDriverMemoryDump();
  std::pair<wifi_error, std::vector<uint8_t>> requestFirmwareMemoryDump();
  std::pair<wifi_error, uint32_t> getSupportedFeatureSet();
  // APF functions.
  std::pair<wifi_error, PacketFilterCapabilities> getPacketFilterCapabilities();
  wifi_error setPacketFilter(const std::vector<uint8_t>& program);
  // Gscan functions.
  std::pair<wifi_error, wifi_gscan_capabilities> getGscanCapabilities();
  // These API's provides a simplified interface over the legacy Gscan API's:
  // a) All scan events from the legacy HAL API other than the
  //    |WIFI_SCAN_FAILED| are treated as notification of results.
  //    This method then retrieves the cached scan results from the legacy
  //    HAL API and triggers the externally provided |on_results_user_callback|
  //    on success.
  // b) |WIFI_SCAN_FAILED| scan event or failure to retrieve cached scan results
  //    triggers the externally provided |on_failure_user_callback|.
  // c) Full scan result event triggers the externally provided
  //    |on_full_result_user_callback|.
  wifi_error startGscan(
      wifi_request_id id,
      const wifi_scan_cmd_params& params,
      const std::function<void(wifi_request_id)>& on_failure_callback,
      const on_gscan_results_callback& on_results_callback,
      const on_gscan_full_result_callback& on_full_result_callback);
  wifi_error stopGscan(wifi_request_id id);
  std::pair<wifi_error, std::vector<uint32_t>> getValidFrequenciesForGscan(
      wifi_band band);
  // Link layer stats functions.
  wifi_error enableLinkLayerStats(bool debug);
  wifi_error disableLinkLayerStats();
  std::pair<wifi_error, LinkLayerStats> getLinkLayerStats();
  // Logger/debug functions.
  std::pair<wifi_error, uint32_t> getLoggerSupportedFeatureSet();
  wifi_error startPktFateMonitoring();
  std::pair<wifi_error, std::vector<wifi_tx_report>> getTxPktFates();
  std::pair<wifi_error, std::vector<wifi_rx_report>> getRxPktFates();
  std::pair<wifi_error, WakeReasonStats> getWakeReasonStats();
  wifi_error registerRingBufferCallbackHandler(
      const on_ring_buffer_data_callback& on_data_callback);
  std::pair<wifi_error, std::vector<wifi_ring_buffer_status>>
  getRingBuffersStatus();
  wifi_error startRingBufferLogging(const std::string& ring_name,
                                    uint32_t verbose_level,
                                    uint32_t max_interval_sec,
                                    uint32_t min_data_size);
  wifi_error getRingBufferData(const std::string& ring_name);

 private:
  // Retrieve the interface handle to be used for the "wlan" interface.
  wifi_error retrieveWlanInterfaceHandle();
  // Run the legacy HAL event loop thread.
  void runEventLoop();
  // Retrieve the cached gscan results to pass the results back to the external
  // callbacks.
  std::pair<wifi_error, std::vector<wifi_cached_scan_results>>
  getGscanCachedResults();
  void invalidate();

  // Event loop thread used by legacy HAL.
  std::thread event_loop_thread_;
  // Global function table of legacy HAL.
  wifi_hal_fn global_func_table_;
  // Opaque handle to be used for all global operations.
  wifi_handle global_handle_;
  // Opaque handle to be used for all wlan0 interface specific operations.
  wifi_interface_handle wlan_interface_handle_;
  // Flag to indicate if we have initiated the cleanup of legacy HAL.
  bool awaiting_event_loop_termination_;
};

}  // namespace legacy_hal
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_LEGACY_WIFI_HAL_H_
