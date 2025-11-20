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

#include <condition_variable>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_handler_callback.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {

class BluetoothCccHandler : public ::bluetooth_hal::hci::HciRouterClient {
 public:
  BluetoothCccHandler();

  static BluetoothCccHandler& GetHandler();

  bool RegisterForLmpEvents(
      const std::shared_ptr<BluetoothCccHandlerCallback>& callback);

  bool UnregisterLmpEvents(
      const ::bluetooth_hal::hci::BluetoothAddress& address);

 protected:
  void OnCommandCallback(
      const ::bluetooth_hal::hci::HalPacket& packet) override;
  void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const ::bluetooth_hal::hci::HalPacket& packet) override;
  void OnBluetoothChipReady() override {};
  void OnBluetoothChipClosed() override {};
  void OnBluetoothEnabled() override;
  void OnBluetoothDisabled() override;

 private:
  uint64_t GetSystemTime(uint8_t current_toggle_count, uint16_t offse);

  std::deque<std::shared_ptr<BluetoothCccHandlerCallback>>
      pending_callbacks_deque_;
  std::list<std::shared_ptr<BluetoothCccHandlerCallback>> monitor_callbacks_;
  std::mutex mutex_;
  std::condition_variable pending_callbacks_cv_;
  uint8_t previous_toggle_count_;
};

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
