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

#include <array>

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <wifi_system/interface_tool.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace legacy_hal {
// Constants ported over from the legacy HAL calling code
// (com_android_server_wifi_WifiNative.cpp). This will all be thrown
// away when this shim layer is replaced by the real vendor
// implementation.
static constexpr uint32_t kMaxVersionStringLength = 256;
static constexpr uint32_t kMaxCachedGscanResults = 64;
static constexpr uint32_t kMaxGscanFrequenciesForBand = 64;
static constexpr uint32_t kLinkLayerStatsDataMpduSizeThreshold = 128;
static constexpr uint32_t kMaxWakeReasonStatsArraySize = 32;
static constexpr uint32_t kMaxRingBuffers = 10;

// Legacy HAL functions accept "C" style function pointers, so use global
// functions to pass to the legacy HAL function and store the corresponding
// std::function methods to be invoked.
// Callback to be invoked once |stop| is complete.
std::function<void(wifi_handle handle)> on_stop_complete_internal_callback;
void onStopComplete(wifi_handle handle) {
  if (on_stop_complete_internal_callback) {
    on_stop_complete_internal_callback(handle);
  }
}

// Callback to be invoked for driver dump.
std::function<void(char*, int)> on_driver_memory_dump_internal_callback;
void onDriverMemoryDump(char* buffer, int buffer_size) {
  if (on_driver_memory_dump_internal_callback) {
    on_driver_memory_dump_internal_callback(buffer, buffer_size);
  }
}

// Callback to be invoked for firmware dump.
std::function<void(char*, int)> on_firmware_memory_dump_internal_callback;
void onFirmwareMemoryDump(char* buffer, int buffer_size) {
  if (on_firmware_memory_dump_internal_callback) {
    on_firmware_memory_dump_internal_callback(buffer, buffer_size);
  }
}

// Callback to be invoked for Gscan events.
std::function<void(wifi_request_id, wifi_scan_event)>
    on_gscan_event_internal_callback;
void onGscanEvent(wifi_request_id id, wifi_scan_event event) {
  if (on_gscan_event_internal_callback) {
    on_gscan_event_internal_callback(id, event);
  }
}

// Callback to be invoked for Gscan full results.
std::function<void(wifi_request_id, wifi_scan_result*, uint32_t)>
    on_gscan_full_result_internal_callback;
void onGscanFullResult(wifi_request_id id,
                       wifi_scan_result* result,
                       uint32_t buckets_scanned) {
  if (on_gscan_full_result_internal_callback) {
    on_gscan_full_result_internal_callback(id, result, buckets_scanned);
  }
}

// Callback to be invoked for link layer stats results.
std::function<void((wifi_request_id, wifi_iface_stat*, int, wifi_radio_stat*))>
    on_link_layer_stats_result_internal_callback;
void onLinkLayerStatsDataResult(wifi_request_id id,
                                wifi_iface_stat* iface_stat,
                                int num_radios,
                                wifi_radio_stat* radio_stat) {
  if (on_link_layer_stats_result_internal_callback) {
    on_link_layer_stats_result_internal_callback(
        id, iface_stat, num_radios, radio_stat);
  }
}

// Callback to be invoked for ring buffer data indication.
std::function<void(char*, char*, int, wifi_ring_buffer_status*)>
    on_ring_buffer_data_internal_callback;
void onRingBufferData(char* ring_name,
                      char* buffer,
                      int buffer_size,
                      wifi_ring_buffer_status* status) {
  if (on_ring_buffer_data_internal_callback) {
    on_ring_buffer_data_internal_callback(
        ring_name, buffer, buffer_size, status);
  }
}

// End of the free-standing "C" style callbacks.

WifiLegacyHal::WifiLegacyHal()
    : global_handle_(nullptr),
      wlan_interface_handle_(nullptr),
      awaiting_event_loop_termination_(false) {}

wifi_error WifiLegacyHal::start() {
  // Ensure that we're starting in a good state.
  CHECK(!global_handle_ && !wlan_interface_handle_ &&
        !awaiting_event_loop_termination_);

  android::wifi_system::InterfaceTool if_tool;
  // TODO: Add back the HAL Tool if we need to. All we need from the HAL tool
  // for now is this function call which we can directly call.
  wifi_error status = init_wifi_vendor_hal_func_table(&global_func_table_);
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to initialize legacy hal function table";
    return WIFI_ERROR_UNKNOWN;
  }
  if (!if_tool.SetWifiUpState(true)) {
    LOG(ERROR) << "Failed to set WiFi interface up";
    return WIFI_ERROR_UNKNOWN;
  }

  LOG(INFO) << "Starting legacy HAL";
  status = global_func_table_.wifi_initialize(&global_handle_);
  if (status != WIFI_SUCCESS || !global_handle_) {
    LOG(ERROR) << "Failed to retrieve global handle";
    return status;
  }
  event_loop_thread_ = std::thread(&WifiLegacyHal::runEventLoop, this);
  status = retrieveWlanInterfaceHandle();
  if (status != WIFI_SUCCESS || !wlan_interface_handle_) {
    LOG(ERROR) << "Failed to retrieve wlan interface handle";
    return status;
  }
  LOG(VERBOSE) << "Legacy HAL start complete";
  return WIFI_SUCCESS;
}

wifi_error WifiLegacyHal::stop(
    const std::function<void()>& on_stop_complete_user_callback) {
  LOG(INFO) << "Stopping legacy HAL";
  on_stop_complete_internal_callback = [&](wifi_handle handle) {
    CHECK_EQ(global_handle_, handle) << "Handle mismatch";
    on_stop_complete_user_callback();
    // Invalidate all the internal pointers now that the HAL is
    // stopped.
    invalidate();
  };
  awaiting_event_loop_termination_ = true;
  global_func_table_.wifi_cleanup(global_handle_, onStopComplete);
  LOG(VERBOSE) << "Legacy HAL stop initiated";
  return WIFI_SUCCESS;
}

std::string WifiLegacyHal::getApIfaceName() {
  // Fake name. This interface does not exist in legacy HAL
  // API's.
  return "ap0";
}

std::string WifiLegacyHal::getNanIfaceName() {
  // Fake name. This interface does not exist in legacy HAL
  // API's.
  return "nan0";
}

std::string WifiLegacyHal::getP2pIfaceName() {
  std::array<char, PROPERTY_VALUE_MAX> buffer;
  property_get("wifi.direct.interface", buffer.data(), "p2p0");
  return buffer.data();
}

std::string WifiLegacyHal::getStaIfaceName() {
  std::array<char, PROPERTY_VALUE_MAX> buffer;
  property_get("wifi.interface", buffer.data(), "wlan0");
  return buffer.data();
}

std::pair<wifi_error, std::string> WifiLegacyHal::getDriverVersion() {
  std::array<char, kMaxVersionStringLength> buffer;
  buffer.fill(0);
  wifi_error status = global_func_table_.wifi_get_driver_version(
      wlan_interface_handle_, buffer.data(), buffer.size());
  return {status, buffer.data()};
}

std::pair<wifi_error, std::string> WifiLegacyHal::getFirmwareVersion() {
  std::array<char, kMaxVersionStringLength> buffer;
  buffer.fill(0);
  wifi_error status = global_func_table_.wifi_get_firmware_version(
      wlan_interface_handle_, buffer.data(), buffer.size());
  return {status, buffer.data()};
}

std::pair<wifi_error, std::vector<uint8_t>>
WifiLegacyHal::requestDriverMemoryDump() {
  std::vector<uint8_t> driver_dump;
  on_driver_memory_dump_internal_callback = [&driver_dump](char* buffer,
                                                           int buffer_size) {
    driver_dump.insert(driver_dump.end(),
                       reinterpret_cast<uint8_t*>(buffer),
                       reinterpret_cast<uint8_t*>(buffer) + buffer_size);
  };
  wifi_error status = global_func_table_.wifi_get_driver_memory_dump(
      wlan_interface_handle_, {onDriverMemoryDump});
  on_driver_memory_dump_internal_callback = nullptr;
  return {status, std::move(driver_dump)};
}

std::pair<wifi_error, std::vector<uint8_t>>
WifiLegacyHal::requestFirmwareMemoryDump() {
  std::vector<uint8_t> firmware_dump;
  on_firmware_memory_dump_internal_callback = [&firmware_dump](
      char* buffer, int buffer_size) {
    firmware_dump.insert(firmware_dump.end(),
                         reinterpret_cast<uint8_t*>(buffer),
                         reinterpret_cast<uint8_t*>(buffer) + buffer_size);
  };
  wifi_error status = global_func_table_.wifi_get_firmware_memory_dump(
      wlan_interface_handle_, {onFirmwareMemoryDump});
  on_firmware_memory_dump_internal_callback = nullptr;
  return {status, std::move(firmware_dump)};
}

std::pair<wifi_error, uint32_t> WifiLegacyHal::getSupportedFeatureSet() {
  feature_set set;
  static_assert(sizeof(set) == sizeof(uint32_t),
                "Some features can not be represented in output");
  wifi_error status = global_func_table_.wifi_get_supported_feature_set(
      wlan_interface_handle_, &set);
  return {status, static_cast<uint32_t>(set)};
}

std::pair<wifi_error, PacketFilterCapabilities>
WifiLegacyHal::getPacketFilterCapabilities() {
  PacketFilterCapabilities caps;
  wifi_error status = global_func_table_.wifi_get_packet_filter_capabilities(
      wlan_interface_handle_, &caps.version, &caps.max_len);
  return {status, caps};
}

wifi_error WifiLegacyHal::setPacketFilter(const std::vector<uint8_t>& program) {
  return global_func_table_.wifi_set_packet_filter(
      wlan_interface_handle_, program.data(), program.size());
}

std::pair<wifi_error, wifi_gscan_capabilities>
WifiLegacyHal::getGscanCapabilities() {
  wifi_gscan_capabilities caps;
  wifi_error status = global_func_table_.wifi_get_gscan_capabilities(
      wlan_interface_handle_, &caps);
  return {status, caps};
}

wifi_error WifiLegacyHal::startGscan(
    wifi_request_id id,
    const wifi_scan_cmd_params& params,
    const std::function<void(wifi_request_id)>& on_failure_user_callback,
    const on_gscan_results_callback& on_results_user_callback,
    const on_gscan_full_result_callback& on_full_result_user_callback) {
  // If there is already an ongoing background scan, reject new scan requests.
  if (on_gscan_event_internal_callback ||
      on_gscan_full_result_internal_callback) {
    return WIFI_ERROR_NOT_AVAILABLE;
  }

  // This callback will be used to either trigger |on_results_user_callback| or
  // |on_failure_user_callback|.
  on_gscan_event_internal_callback =
      [on_failure_user_callback, on_results_user_callback, this](
          wifi_request_id id, wifi_scan_event event) {
        switch (event) {
          case WIFI_SCAN_RESULTS_AVAILABLE:
          case WIFI_SCAN_THRESHOLD_NUM_SCANS:
          case WIFI_SCAN_THRESHOLD_PERCENT: {
            wifi_error status;
            std::vector<wifi_cached_scan_results> cached_scan_results;
            std::tie(status, cached_scan_results) = getGscanCachedResults();
            if (status == WIFI_SUCCESS) {
              on_results_user_callback(id, cached_scan_results);
              return;
            }
          }
          // Fall through if failed. Failure to retrieve cached scan results
          // should trigger a background scan failure.
          case WIFI_SCAN_FAILED:
            on_failure_user_callback(id);
            on_gscan_event_internal_callback = nullptr;
            on_gscan_full_result_internal_callback = nullptr;
            return;
        }
        LOG(FATAL) << "Unexpected gscan event received: " << event;
      };

  on_gscan_full_result_internal_callback = [on_full_result_user_callback](
      wifi_request_id id, wifi_scan_result* result, uint32_t buckets_scanned) {
    if (result) {
      on_full_result_user_callback(id, result, buckets_scanned);
    }
  };

  wifi_scan_result_handler handler = {onGscanFullResult, onGscanEvent};
  wifi_error status = global_func_table_.wifi_start_gscan(
      id, wlan_interface_handle_, params, handler);
  if (status != WIFI_SUCCESS) {
    on_gscan_event_internal_callback = nullptr;
    on_gscan_full_result_internal_callback = nullptr;
  }
  return status;
}

wifi_error WifiLegacyHal::stopGscan(wifi_request_id id) {
  // If there is no an ongoing background scan, reject stop requests.
  // TODO(b/32337212): This needs to be handled by the HIDL object because we
  // need to return the NOT_STARTED error code.
  if (!on_gscan_event_internal_callback &&
      !on_gscan_full_result_internal_callback) {
    return WIFI_ERROR_NOT_AVAILABLE;
  }
  wifi_error status =
      global_func_table_.wifi_stop_gscan(id, wlan_interface_handle_);
  // If the request Id is wrong, don't stop the ongoing background scan. Any
  // other error should be treated as the end of background scan.
  if (status != WIFI_ERROR_INVALID_REQUEST_ID) {
    on_gscan_event_internal_callback = nullptr;
    on_gscan_full_result_internal_callback = nullptr;
  }
  return status;
}

std::pair<wifi_error, std::vector<uint32_t>>
WifiLegacyHal::getValidFrequenciesForGscan(wifi_band band) {
  static_assert(sizeof(uint32_t) >= sizeof(wifi_channel),
                "Wifi Channel cannot be represented in output");
  std::vector<uint32_t> freqs;
  freqs.resize(kMaxGscanFrequenciesForBand);
  int32_t num_freqs = 0;
  wifi_error status = global_func_table_.wifi_get_valid_channels(
      wlan_interface_handle_,
      band,
      freqs.size(),
      reinterpret_cast<wifi_channel*>(freqs.data()),
      &num_freqs);
  CHECK(num_freqs >= 0 &&
        static_cast<uint32_t>(num_freqs) <= kMaxGscanFrequenciesForBand);
  freqs.resize(num_freqs);
  return {status, std::move(freqs)};
}

wifi_error WifiLegacyHal::enableLinkLayerStats(bool debug) {
  wifi_link_layer_params params;
  params.mpdu_size_threshold = kLinkLayerStatsDataMpduSizeThreshold;
  params.aggressive_statistics_gathering = debug;
  return global_func_table_.wifi_set_link_stats(wlan_interface_handle_, params);
}

wifi_error WifiLegacyHal::disableLinkLayerStats() {
  // TODO: Do we care about these responses?
  uint32_t clear_mask_rsp;
  uint8_t stop_rsp;
  return global_func_table_.wifi_clear_link_stats(
      wlan_interface_handle_, 0xFFFFFFFF, &clear_mask_rsp, 1, &stop_rsp);
}

std::pair<wifi_error, LinkLayerStats> WifiLegacyHal::getLinkLayerStats() {
  LinkLayerStats link_stats{};
  LinkLayerStats* link_stats_ptr = &link_stats;

  on_link_layer_stats_result_internal_callback = [&link_stats_ptr](
      wifi_request_id /* id */,
      wifi_iface_stat* iface_stats_ptr,
      int num_radios,
      wifi_radio_stat* radio_stats_ptr) {
    if (iface_stats_ptr != nullptr) {
      link_stats_ptr->iface = *iface_stats_ptr;
      link_stats_ptr->iface.num_peers = 0;
    } else {
      LOG(ERROR) << "Invalid iface stats in link layer stats";
    }
    if (num_radios == 1 && radio_stats_ptr != nullptr) {
      link_stats_ptr->radio = *radio_stats_ptr;
      // Copy over the tx level array to the separate vector.
      if (radio_stats_ptr->num_tx_levels > 0 &&
          radio_stats_ptr->tx_time_per_levels != nullptr) {
        link_stats_ptr->radio_tx_time_per_levels.assign(
            radio_stats_ptr->tx_time_per_levels,
            radio_stats_ptr->tx_time_per_levels +
                radio_stats_ptr->num_tx_levels);
      }
      link_stats_ptr->radio.num_tx_levels = 0;
      link_stats_ptr->radio.tx_time_per_levels = nullptr;
    } else {
      LOG(ERROR) << "Invalid radio stats in link layer stats";
    }
  };

  wifi_error status = global_func_table_.wifi_get_link_stats(
      0, wlan_interface_handle_, {onLinkLayerStatsDataResult});
  on_link_layer_stats_result_internal_callback = nullptr;
  return {status, link_stats};
}

std::pair<wifi_error, uint32_t> WifiLegacyHal::getLoggerSupportedFeatureSet() {
  uint32_t supported_features;
  wifi_error status = global_func_table_.wifi_get_logger_supported_feature_set(
      wlan_interface_handle_, &supported_features);
  return {status, supported_features};
}

wifi_error WifiLegacyHal::startPktFateMonitoring() {
  return global_func_table_.wifi_start_pkt_fate_monitoring(
      wlan_interface_handle_);
}

std::pair<wifi_error, std::vector<wifi_tx_report>>
WifiLegacyHal::getTxPktFates() {
  std::vector<wifi_tx_report> tx_pkt_fates;
  tx_pkt_fates.resize(MAX_FATE_LOG_LEN);
  size_t num_fates = 0;
  wifi_error status =
      global_func_table_.wifi_get_tx_pkt_fates(wlan_interface_handle_,
                                               tx_pkt_fates.data(),
                                               tx_pkt_fates.size(),
                                               &num_fates);
  CHECK(num_fates <= MAX_FATE_LOG_LEN);
  tx_pkt_fates.resize(num_fates);
  return {status, std::move(tx_pkt_fates)};
}

std::pair<wifi_error, std::vector<wifi_rx_report>>
WifiLegacyHal::getRxPktFates() {
  std::vector<wifi_rx_report> rx_pkt_fates;
  rx_pkt_fates.resize(MAX_FATE_LOG_LEN);
  size_t num_fates = 0;
  wifi_error status =
      global_func_table_.wifi_get_rx_pkt_fates(wlan_interface_handle_,
                                               rx_pkt_fates.data(),
                                               rx_pkt_fates.size(),
                                               &num_fates);
  CHECK(num_fates <= MAX_FATE_LOG_LEN);
  rx_pkt_fates.resize(num_fates);
  return {status, std::move(rx_pkt_fates)};
}

std::pair<wifi_error, WakeReasonStats> WifiLegacyHal::getWakeReasonStats() {
  WakeReasonStats stats;
  stats.cmd_event_wake_cnt.resize(kMaxWakeReasonStatsArraySize);
  stats.driver_fw_local_wake_cnt.resize(kMaxWakeReasonStatsArraySize);

  // This legacy struct needs separate memory to store the variable sized wake
  // reason types.
  stats.wake_reason_cnt.cmd_event_wake_cnt =
      reinterpret_cast<int32_t*>(stats.cmd_event_wake_cnt.data());
  stats.wake_reason_cnt.cmd_event_wake_cnt_sz = stats.cmd_event_wake_cnt.size();
  stats.wake_reason_cnt.cmd_event_wake_cnt_used = 0;
  stats.wake_reason_cnt.driver_fw_local_wake_cnt =
      reinterpret_cast<int32_t*>(stats.driver_fw_local_wake_cnt.data());
  stats.wake_reason_cnt.driver_fw_local_wake_cnt_sz =
      stats.driver_fw_local_wake_cnt.size();
  stats.wake_reason_cnt.driver_fw_local_wake_cnt_used = 0;

  wifi_error status = global_func_table_.wifi_get_wake_reason_stats(
      wlan_interface_handle_, &stats.wake_reason_cnt);

  CHECK(stats.wake_reason_cnt.cmd_event_wake_cnt_used >= 0 &&
        static_cast<uint32_t>(stats.wake_reason_cnt.cmd_event_wake_cnt_used) <=
            kMaxWakeReasonStatsArraySize);
  stats.cmd_event_wake_cnt.resize(
      stats.wake_reason_cnt.cmd_event_wake_cnt_used);
  stats.wake_reason_cnt.cmd_event_wake_cnt = nullptr;

  CHECK(stats.wake_reason_cnt.driver_fw_local_wake_cnt_used >= 0 &&
        static_cast<uint32_t>(
            stats.wake_reason_cnt.driver_fw_local_wake_cnt_used) <=
            kMaxWakeReasonStatsArraySize);
  stats.driver_fw_local_wake_cnt.resize(
      stats.wake_reason_cnt.driver_fw_local_wake_cnt_used);
  stats.wake_reason_cnt.driver_fw_local_wake_cnt = nullptr;

  return {status, stats};
}

wifi_error WifiLegacyHal::registerRingBufferCallbackHandler(
    const on_ring_buffer_data_callback& on_user_data_callback) {
  if (on_ring_buffer_data_internal_callback) {
    return WIFI_ERROR_NOT_AVAILABLE;
  }
  on_ring_buffer_data_internal_callback = [on_user_data_callback](
      char* ring_name,
      char* buffer,
      int buffer_size,
      wifi_ring_buffer_status* status) {
    if (status && buffer) {
      std::vector<uint8_t> buffer_vector(
          reinterpret_cast<uint8_t*>(buffer),
          reinterpret_cast<uint8_t*>(buffer) + buffer_size);
      on_user_data_callback(ring_name, buffer_vector, *status);
    }
  };
  return global_func_table_.wifi_set_log_handler(
      0, wlan_interface_handle_, {onRingBufferData});
}

std::pair<wifi_error, std::vector<wifi_ring_buffer_status>>
WifiLegacyHal::getRingBuffersStatus() {
  std::vector<wifi_ring_buffer_status> ring_buffers_status;
  ring_buffers_status.resize(kMaxRingBuffers);
  uint32_t num_rings = 0;
  wifi_error status = global_func_table_.wifi_get_ring_buffers_status(
      wlan_interface_handle_, &num_rings, ring_buffers_status.data());
  CHECK(num_rings <= kMaxRingBuffers);
  ring_buffers_status.resize(num_rings);
  return {status, std::move(ring_buffers_status)};
}

wifi_error WifiLegacyHal::startRingBufferLogging(const std::string& ring_name,
                                                 uint32_t verbose_level,
                                                 uint32_t max_interval_sec,
                                                 uint32_t min_data_size) {
  std::vector<char> ring_name_internal(ring_name.begin(), ring_name.end());
  return global_func_table_.wifi_start_logging(wlan_interface_handle_,
                                               verbose_level,
                                               0,
                                               max_interval_sec,
                                               min_data_size,
                                               ring_name_internal.data());
}

wifi_error WifiLegacyHal::getRingBufferData(const std::string& ring_name) {
  std::vector<char> ring_name_internal(ring_name.begin(), ring_name.end());
  return global_func_table_.wifi_get_ring_data(wlan_interface_handle_,
                                               ring_name_internal.data());
}

wifi_error WifiLegacyHal::retrieveWlanInterfaceHandle() {
  const std::string& ifname_to_find = getStaIfaceName();
  wifi_interface_handle* iface_handles = nullptr;
  int num_iface_handles = 0;
  wifi_error status = global_func_table_.wifi_get_ifaces(
      global_handle_, &num_iface_handles, &iface_handles);
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to enumerate interface handles";
    return status;
  }
  for (int i = 0; i < num_iface_handles; ++i) {
    std::array<char, IFNAMSIZ> current_ifname;
    current_ifname.fill(0);
    status = global_func_table_.wifi_get_iface_name(
        iface_handles[i], current_ifname.data(), current_ifname.size());
    if (status != WIFI_SUCCESS) {
      LOG(WARNING) << "Failed to get interface handle name";
      continue;
    }
    if (ifname_to_find == current_ifname.data()) {
      wlan_interface_handle_ = iface_handles[i];
      return WIFI_SUCCESS;
    }
  }
  return WIFI_ERROR_UNKNOWN;
}

void WifiLegacyHal::runEventLoop() {
  LOG(VERBOSE) << "Starting legacy HAL event loop";
  global_func_table_.wifi_event_loop(global_handle_);
  if (!awaiting_event_loop_termination_) {
    LOG(FATAL) << "Legacy HAL event loop terminated, but HAL was not stopping";
  }
  LOG(VERBOSE) << "Legacy HAL event loop terminated";
  awaiting_event_loop_termination_ = false;
  android::wifi_system::InterfaceTool if_tool;
  if_tool.SetWifiUpState(false);
}

std::pair<wifi_error, std::vector<wifi_cached_scan_results>>
WifiLegacyHal::getGscanCachedResults() {
  std::vector<wifi_cached_scan_results> cached_scan_results;
  cached_scan_results.resize(kMaxCachedGscanResults);
  int32_t num_results = 0;
  wifi_error status = global_func_table_.wifi_get_cached_gscan_results(
      wlan_interface_handle_,
      true /* always flush */,
      cached_scan_results.size(),
      cached_scan_results.data(),
      &num_results);
  CHECK(num_results >= 0 &&
        static_cast<uint32_t>(num_results) <= kMaxCachedGscanResults);
  cached_scan_results.resize(num_results);
  // Check for invalid IE lengths in these cached scan results and correct it.
  for (auto& cached_scan_result : cached_scan_results) {
    int num_scan_results = cached_scan_result.num_results;
    for (int i = 0; i < num_scan_results; i++) {
      auto& scan_result = cached_scan_result.results[i];
      if (scan_result.ie_length > 0) {
        LOG(ERROR) << "Cached scan result has non-zero IE length "
                   << scan_result.ie_length;
        scan_result.ie_length = 0;
      }
    }
  }
  return {status, std::move(cached_scan_results)};
}

void WifiLegacyHal::invalidate() {
  global_handle_ = nullptr;
  wlan_interface_handle_ = nullptr;
  on_stop_complete_internal_callback = nullptr;
  on_driver_memory_dump_internal_callback = nullptr;
  on_firmware_memory_dump_internal_callback = nullptr;
  on_gscan_event_internal_callback = nullptr;
  on_gscan_full_result_internal_callback = nullptr;
  on_link_layer_stats_result_internal_callback = nullptr;
  on_ring_buffer_data_internal_callback = nullptr;
}

}  // namespace legacy_hal
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
