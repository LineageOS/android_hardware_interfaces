/*
 * Copyright 2024 The Android Open Source Project
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

#include <map>
#include <mutex>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_client_callback.h"

namespace bluetooth_hal {
namespace hci {

class HciRouterClient : public HciRouterClientCallback {
 public:
  HciRouterClient();
  ~HciRouterClient();

  /**
   * @brief Called when the controller responds to a command.
   *
   * @param packet The HAL packet containing the response.
   *
   * @note Subclasses **must** implement this method to use the `send_command`
   * function.
   *
   */
  virtual void OnCommandCallback(const HalPacket& packet) override = 0;

  /**
   * @brief Called when the router client receives an HCI packet.
   *
   * @param packet The HAL packet containing the HCI event.
   *
   * @return A `MonitorMode` value indicating whether the packet should be
   * processed by other clients.
   *
   * @note The default implementation allows each client to register HCI
   * monitors to monitor/intercept HCI event. If a client does not require this
   * functionality, it can directly override this method with its specific
   * implementation.
   *
   */
  MonitorMode OnPacketCallback(const HalPacket& packet) override;

  /**
   * @brief Called when the HAL state changes.
   *
   * @param old_state The old HAL state.
   * @param new_state The new HAL state.
   *
   * @note It is **not recommended** to implement this method. The
   * `HciRouterClientAgent` class handles all HAL state change logic. Instead,
   * subclasses can use the following methods to determine the HAL state:
   *        - `OnBluetoothChipReady()`
   *        - `OnBluetoothChipClosed()`
   *        - `OnBluetoothEnabled()`
   *        - `OnBluetoothDisabled()`
   *        - `IsBluetoothEnabled()`
   *        - `IsBluetoothChipReady()`
   *
   */
  void OnHalStateChanged(
      [[maybe_unused]] ::bluetooth_hal::HalState new_state,
      [[maybe_unused]] ::bluetooth_hal::HalState old_state) override {};

 protected:
  /**
   * @brief Callback invoked when a received HCI packet matches a registered
   * monitor.
   *
   * This method is invoked by the `HciRouterClient` class when an incoming HCI
   * packet is received that matches the monitor mode registered by the
   * `RegisterMonitor` method.
   *
   * @param mode The monitor mode that was triggered.
   * @param packet The received HCI packet.
   *
   */
  virtual void OnMonitorPacketCallback(MonitorMode mode,
                                       const HalPacket& packet) = 0;

  /**
   * @brief Called when the Bluetooth chip is ready.
   *
   * This method is invoked by the `HciRouterClient` class when the HAL state
   * changes to `HalState::kBtChipReady`.
   *
   */
  virtual void OnBluetoothChipReady() = 0;

  /**
   * @brief Called when the Bluetooth chip is closed.
   *
   * This method is invoked by the `HciRouterClient` class when the HAL state
   * changes to the state < `HalState::kBtChipReady`.
   *
   */
  virtual void OnBluetoothChipClosed() = 0;

  /**
   * @brief Called when Bluetooth is enabled.
   *
   * This method is invoked by the `HciRouterClient` class when the HAL state
   * changes to `HalState::kRunning`.
   *
   */
  virtual void OnBluetoothEnabled() = 0;

  /**
   * @brief Called when Bluetooth is disabled.
   *
   * This method is invoked by the `HciRouterClient` class when the HAL state
   * changes to a state < `HalState::kRunning`.
   *
   */
  virtual void OnBluetoothDisabled() = 0;

  /**
   * @brief Returns whether Bluetooth is enabled.
   *
   * @return `true` if Bluetooth is enabled, `false` otherwise.
   *
   */
  bool IsBluetoothEnabled();

  /**
   * @brief Returns whether the Bluetooth chip is ready.
   *
   * @return `true` if the Bluetooth chip is ready, `false` otherwise.
   *
   */
  bool IsBluetoothChipReady();

  /**
   * @brief Registers a monitor to receive HCI events.
   *
   * @param monitor The monitor to register.
   * @param mode The monitor mode to register.
   *
   * @return `true` if the monitor was registered successfully, `false`
   * otherwise.
   *
   */
  bool RegisterMonitor(const HciMonitor& monitor, MonitorMode mode);

  /**
   * @brief Unregisters a monitor.
   *
   * @param monitor The monitor to unregister.
   *
   * @return `true` if the monitor was unregistered successfully, `false`
   * otherwise.
   *
   */
  bool UnregisterMonitor(const HciMonitor& monitor);

  /**
   * @brief Sends a command to the HCI router.
   *
   * This method should only be used to send HCI commands.
   *
   * @param packet The HAL packet containing the command.
   *
   * @return `true` if the command was sent successfully, `false` otherwise.
   *
   */
  bool SendCommand(const HalPacket& packet);

  /**
   * @brief Sends data to the HCI router.
   *
   * This method can be used to send various types of packets to the HAL,
   * excluding HCI commands and events.
   *
   * @param packet The HAL packet containing the data.
   *
   * @return `true` if the data was sent successfully, `false` otherwise.
   *
   */
  bool SendData(const HalPacket& packet);

 private:
  std::map<HciMonitor, MonitorMode> monitors_;
  std::recursive_mutex mutex_;
};

}  // namespace hci
}  // namespace bluetooth_hal
