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

// Full scan results contain IE info and are hence passed by reference, to
// preserve the variable length array member |ie_data|. Callee must not retain
// the pointer.
using on_gscan_full_result_callback =
    std::function<void(wifi_request_id, const wifi_scan_result*, uint32_t)>;
// These scan results don't contain any IE info, so no need to pass by
// reference.
using on_gscan_results_callback = std::function<void(
    wifi_request_id, const std::vector<wifi_cached_scan_results>&)>;

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
