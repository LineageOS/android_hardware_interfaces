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

#include "wifi_legacy_hal.h"
#include "wifi_status_util.h"

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <wifi_system/hal_tool.h>
#include <wifi_system/interface_tool.h>

namespace {
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
}

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

const uint32_t WifiLegacyHal::kMaxVersionStringLength = 256;

WifiLegacyHal::WifiLegacyHal()
    : global_handle_(nullptr),
      wlan_interface_handle_(nullptr),
      awaiting_event_loop_termination_(false) {}

wifi_error WifiLegacyHal::start() {
  // Ensure that we're starting in a good state.
  CHECK(!global_handle_ && !wlan_interface_handle_ &&
        !awaiting_event_loop_termination_);

  android::wifi_system::HalTool hal_tool;
  android::wifi_system::InterfaceTool if_tool;
  if (!hal_tool.InitFunctionTable(&global_func_table_)) {
    LOG(ERROR) << "Failed to initialize legacy hal function table";
    return WIFI_ERROR_UNKNOWN;
  }
  if (!if_tool.SetWifiUpState(true)) {
    LOG(ERROR) << "Failed to set WiFi interface up";
    return WIFI_ERROR_UNKNOWN;
  }

  LOG(INFO) << "Starting legacy HAL";
  wifi_error status = global_func_table_.wifi_initialize(&global_handle_);
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
    global_handle_ = nullptr;
    wlan_interface_handle_ = nullptr;
    on_stop_complete_internal_callback = nullptr;
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
  return std::make_pair(status, buffer.data());
}

std::pair<wifi_error, std::string> WifiLegacyHal::getFirmwareVersion() {
  std::array<char, kMaxVersionStringLength> buffer;
  buffer.fill(0);
  wifi_error status = global_func_table_.wifi_get_firmware_version(
      wlan_interface_handle_, buffer.data(), buffer.size());
  return std::make_pair(status, buffer.data());
}

std::pair<wifi_error, std::vector<char>>
WifiLegacyHal::requestDriverMemoryDump() {
  std::vector<char> driver_dump;
  on_driver_memory_dump_internal_callback = [&driver_dump](char* buffer,
                                                           int buffer_size) {
    driver_dump.insert(driver_dump.end(), buffer, buffer + buffer_size);
  };
  wifi_error status = global_func_table_.wifi_get_driver_memory_dump(
      wlan_interface_handle_, {onDriverMemoryDump});
  on_driver_memory_dump_internal_callback = nullptr;
  return std::make_pair(status, std::move(driver_dump));
}

std::pair<wifi_error, std::vector<char>>
WifiLegacyHal::requestFirmwareMemoryDump() {
  std::vector<char> firmware_dump;
  on_firmware_memory_dump_internal_callback = [&firmware_dump](
      char* buffer, int buffer_size) {
    firmware_dump.insert(firmware_dump.end(), buffer, buffer + buffer_size);
  };
  wifi_error status = global_func_table_.wifi_get_firmware_memory_dump(
      wlan_interface_handle_, {onFirmwareMemoryDump});
  on_firmware_memory_dump_internal_callback = nullptr;
  return std::make_pair(status, std::move(firmware_dump));
}

wifi_error WifiLegacyHal::retrieveWlanInterfaceHandle() {
  const std::string& ifname_to_find = getStaIfaceName();
  wifi_interface_handle* iface_handles = nullptr;
  int num_iface_handles = 0;
  wifi_error status = global_func_table_.wifi_get_ifaces(
      global_handle_, &num_iface_handles, &iface_handles);
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to enumerate interface handles: "
               << legacyErrorToString(status);
    return status;
  }
  for (int i = 0; i < num_iface_handles; ++i) {
    std::array<char, IFNAMSIZ> current_ifname;
    current_ifname.fill(0);
    status = global_func_table_.wifi_get_iface_name(
        iface_handles[i], current_ifname.data(), current_ifname.size());
    if (status != WIFI_SUCCESS) {
      LOG(WARNING) << "Failed to get interface handle name: "
                   << legacyErrorToString(status);
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

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
