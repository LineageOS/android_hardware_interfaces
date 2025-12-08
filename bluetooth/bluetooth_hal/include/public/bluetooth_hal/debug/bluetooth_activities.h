/*
 * Copyright 2025 The Android Open Source Project
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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace debug {

class BluetoothActivities {
 public:
  virtual ~BluetoothActivities() = default;

  /**
   * @brief Start the activities monitoring process.
   *
   * This function should be called before any other functions in this class are
   * used.
   */
  static void Start();

  /**
   * @brief Get the singleton instance of BluetoothActivities.
   *
   * @return The singleton instance of BluetoothActivities.
   */
  static BluetoothActivities& Get();

  /**
   * @brief Stop the activities monitoring process, and clear the connections.
   *
   * After calling this function, all of the current connections and the
   * connection history will be cleared. The instance will be reset.
   */
  static void Stop();

  /**
   * @brief Checks if there are any connected Bluetooth devices.
   *
   * @return true if there is at least one connected device, false otherwise.
   */
  virtual bool HasConnectedDevice() const = 0;

  /**
   * @brief Checks if a specific Bluetooth device is connected.
   *
   * @param connection_handle The handle of the Bluetooth connection to check.
   * @return true if the device with the given connection handle is connected,
   * false otherwise.
   */
  virtual bool IsConnected(uint16_t connection_handle) const = 0;

  /**
   * @brief Gets the number of connected devices.
   *
   * @return The number of connected devices.
   */
  virtual size_t GetConnectionHandleCount() const = 0;

  virtual void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const ::bluetooth_hal::hci::HalPacket& packet) = 0;

  virtual void OnBluetoothChipClosed() = 0;

 protected:
  BluetoothActivities() = default;

 private:
  static inline std::unique_ptr<BluetoothActivities> instance_;
  static inline std::mutex mutex_;
};

}  // namespace debug
}  // namespace bluetooth_hal
