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
#include "bluetooth_hal/hci_router_callback.h"

namespace bluetooth_hal {
namespace hci {

class HciRouterClientCallback : public HciRouterCallback {
 public:
  virtual ~HciRouterClientCallback() = default;

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
  virtual MonitorMode OnPacketCallback(const HalPacket& packet) override = 0;

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
  virtual void OnHalStateChanged(HalState new_state,
                                 HalState old_state) override = 0;

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
};

}  // namespace hci
}  // namespace bluetooth_hal
