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

#include <memory>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_callback.h"

namespace bluetooth_hal {
namespace hci {

class HciRouter {
 public:
  virtual ~HciRouter() = default;

  /**
   * @brief Get the singleton instance of HciRouter.
   *
   * @return The singleton instance of HciRouter.
   *
   */
  static HciRouter& GetRouter();

  /**
   * @brief Initialize the HciRouter instance with callbacks from the Bluetooth
   * stack. It also initializes the layers below the HciRouter.
   *
   * @return true if success, otherwise false.
   *
   */
  virtual bool Initialize(
      const std::shared_ptr<HciRouterCallback>& callback) = 0;

  /**
   * @brief Cleanup callbacks and de-initialize the lower layers.
   */
  virtual void Cleanup() = 0;

  /**
   * @brief Send data to the Bluetooth chip. Used for all types of data. The
   * corresponding command complete/status event will be returned to the main
   * HCI aidl client if the sent data is a command.
   *
   * @param packet The packet instance that was requested to be sent.
   *
   * @return true if the packet was sent successfully, otherwise false.
   *
   */
  virtual bool Send(const HalPacket& packet) = 0;

  /**
   * @brief Send a HCI command to the Bluetooth chip. The corresponding command
   * complete/status event will be returned to the HciRouterCallback instead
   * of the main HCI aidl client.
   *
   * @param packet The HCI command that was requested to be sent.
   * @param callback The callback for the corresponding event for the command.
   *
   * @return true if the command was sent successfully, otherwise false.
   *
   */
  virtual bool SendCommand(const HalPacket& packet,
                           const HalPacketCallback& callback) = 0;

  /**
   * @brief Send a HCI command to the Bluetooth chip. The command sent over this
   * method skips the HCI flow control in the Bluetooth HAL. Used for some
   * speicall vendor commands which do not return a command complete/status
   * event.
   *
   * @param packet The HCI command that was requested to be sent.
   *
   * @return true if the command was sent successfully, otherwise false.
   *
   */
  virtual bool SendCommandNoAck(const HalPacket& packet) = 0;

  /**
   * @brief Get the current state of the HciRouter state machine.
   *
   * @return The current state of the state machine.
   *
   */
  virtual ::bluetooth_hal::HalState GetHalState() = 0;

  /**
   * @brief Update the current state of the HciRouter state machine.
   *
   * @param state The state to which the HCI router would change.
   *
   */
  virtual void UpdateHalState(::bluetooth_hal::HalState state) = 0;

  /**
   * @brief Send the packet to the stack through HCI Router.
   *
   * All packets that are not from the controller and need to be sent to the
   * stack must first be processed by the HCI router. This function ensures that
   * the HCI router eventually routes them to the stack.
   *
   * @param packet The packet that would be sent to the stack.
   *
   */
  virtual void SendPacketToStack(const HalPacket& packet) = 0;
};

}  // namespace hci
}  // namespace bluetooth_hal
