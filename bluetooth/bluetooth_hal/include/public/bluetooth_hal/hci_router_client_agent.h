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

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_client_callback.h"

namespace bluetooth_hal {
namespace hci {

class HciRouterClientAgent {
 public:
  virtual ~HciRouterClientAgent() = default;

  /**
   * @brief Get the singleton instance of HciRouterClientAgent.
   *
   * @return The singleton instance of HciRouterClientAgent.
   *
   */
  static HciRouterClientAgent& GetAgent();

  /**
   * @brief Register HciRouterClientCallback for the agent to service.
   *
   * @param callback The callback for the HciRouter client.
   * @return true if the callback is registered, otherwise false.
   *
   */
  virtual bool RegisterClient(HciRouterClientCallback* callback) = 0;

  /**
   * @brief Unregister HciRouterClientCallback from the agent.
   *
   * @param callback The callback for the HciRouter client.
   * @return true if the callback is unregistered, otherwise false.
   *
   */
  virtual bool UnregisterClient(HciRouterClientCallback* callback) = 0;

  /**
   * @brief Called when the router receives an HCI packet. The router agent will
   * try to dispatch the packet to the interested clients.
   *
   * @param packet The HAL packet containing the HCI event.
   *
   * @return A `MonitorMode` value indicating whether the packet should be
   * processed by other clients.
   *
   */
  virtual MonitorMode DispatchPacketToClients(const HalPacket& packet) = 0;

  /**
   * @brief Called when the HAL state changes.
   *
   * @param old_state The old HAL state.
   * @param new_state The new HAL state.
   *
   * @note The `HciRouterClientAgent` uses below method to inform
   * HciRouterClients for HAL state changes.
   *        - `OnBluetoothChipReady()`
   *        - `OnBluetoothChipClosed()`
   *        - `OnBluetoothEnabled()`
   *        - `OnBluetoothDisabled()`
   *
   */
  virtual void NotifyHalStateChange(HalState new_state, HalState old_state) = 0;

  /**
   * @brief Returns whether Bluetooth is enabled.
   *
   * @return `true` if Bluetooth is enabled, `false` otherwise.
   *
   */
  virtual bool IsBluetoothEnabled() = 0;

  /**
   * @brief Returns whether the Bluetooth chip is ready.
   *
   * @return `true` if the Bluetooth chip is ready, `false` otherwise.
   *
   */
  virtual bool IsBluetoothChipReady() = 0;
};

}  // namespace hci
}  // namespace bluetooth_hal
