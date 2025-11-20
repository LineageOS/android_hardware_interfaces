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
#include "bluetooth_hal/hci_monitor.h"

namespace bluetooth_hal {
namespace hci {

class HciRouterCallback {
 public:
  virtual ~HciRouterCallback() = default;

  /**
   * @brief Callback for the command complete and command status events.
   *
   * @param packet The HCI event packet.
   */
  virtual void OnCommandCallback(const HalPacket& packet) = 0;

  /**
   * @brief Callback for the packets other than command complete and command
   * status events
   *
   * @param packet The HCI event packet.
   * @return The monitor mode of the callback handling the packet.
   */
  virtual MonitorMode OnPacketCallback(const HalPacket& packet) = 0;

  /**
   * @brief Callback for the HAL state change from the HciRouter.
   *
   * @param previous_state The previous HAL state.
   * @param new_state The new HAL state.
   */
  virtual void OnHalStateChanged(const ::bluetooth_hal::HalState new_state,
                                 const ::bluetooth_hal::HalState old_state) = 0;
};

}  // namespace hci
}  // namespace bluetooth_hal
