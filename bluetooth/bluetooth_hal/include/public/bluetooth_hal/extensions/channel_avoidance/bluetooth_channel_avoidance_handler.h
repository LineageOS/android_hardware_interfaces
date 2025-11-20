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

#include <array>
#include <atomic>
#include <cstdint>
#include <future>
#include <mutex>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace extensions {
namespace channel_avoidance {

class BluetoothChannelAvoidanceHandler
    : public ::bluetooth_hal::hci::HciRouterClient {
 public:
  BluetoothChannelAvoidanceHandler() = default;

  bool SetBluetoothChannelStatus(const std::array<uint8_t, 10>& channel_map);

 protected:
  void OnBluetoothChipReady() override {};
  void OnBluetoothChipClosed() override {};
  void OnBluetoothEnabled() override {};
  void OnBluetoothDisabled() override {};
  void OnCommandCallback(
      const ::bluetooth_hal::hci::HalPacket& packet) override;
  void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const bluetooth_hal::hci::HalPacket& packet) override;

  ::bluetooth_hal::hci::HalPacket BuildSetChannelAvoidanceCommand(
      const std::array<uint8_t, 10>& channel_map);

 private:
  std::mutex command_mtx_;
  std::promise<void> command_promise_;
  std::atomic<bool> command_success_{false};
};

}  // namespace channel_avoidance
}  // namespace extensions
}  // namespace bluetooth_hal
