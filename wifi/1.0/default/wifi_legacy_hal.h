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

#include <hardware_legacy/wifi_hal.h>

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

/**
 * Class that encapsulates all legacy HAL interactions.
 * This class manages the lifetime of the event loop thread used by legacy HAL.
 */
class WifiLegacyHal {
 public:
  WifiLegacyHal();
  // Initialize the legacy HAL and start the event looper thread.
  wifi_error start();
  // Deinitialize the legacy HAL and stop the event looper thread.
  wifi_error stop(const std::function<void()>& on_complete_callback);
  // Wrappers for all the functions in the legacy HAL function table.
  std::pair<wifi_error, std::string> getWlanDriverVersion();
  std::pair<wifi_error, std::string> getWlanFirmwareVersion();
  std::pair<wifi_error, std::vector<char>> requestWlanDriverMemoryDump();
  std::pair<wifi_error, std::vector<char>> requestWlanFirmwareMemoryDump();

 private:
  static const uint32_t kMaxVersionStringLength;

  // Retrieve the interface handle to be used for the "wlan" interface.
  wifi_error retrieveWlanInterfaceHandle();
  // Run the legacy HAL event loop thread.
  void runEventLoop();

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

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_LEGACY_WIFI_HAL_H_
