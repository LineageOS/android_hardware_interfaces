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

#include <atomic>
#include <future>
#include <mutex>
#include <vector>

#include "aidl/android/hardware/bluetooth/finder/Eid.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace extensions {
namespace finder {

class BluetoothFinderHandler : public ::bluetooth_hal::hci::HciRouterClient {
 public:
  enum class State {
    kIdle,
    kReset,
    kSendingKeys,
    kStartingPof,
    kStarted,
  };

  bool SendEids(
      const std::vector<::aidl::android::hardware::bluetooth::finder::Eid>&
          keys);

  bool SetPoweredOffFinderMode(bool enable);
  bool GetPoweredOffFinderMode(bool* return_value);

  bool IsPoweredOffFinderEnabled() const;
  bool StartPoweredOffFinderMode();

  static BluetoothFinderHandler& GetHandler();

 protected:
  BluetoothFinderHandler() = default;

  void OnBluetoothChipReady() override {};
  void OnBluetoothChipClosed() override {};
  void OnBluetoothEnabled() override {};
  void OnBluetoothDisabled() override {};
  void OnCommandCallback(const bluetooth_hal::hci::HalPacket& packet) override;
  void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const bluetooth_hal::hci::HalPacket& packet) override;

  ::bluetooth_hal::hci::HalPacket BuildFinderResetCommand();
  ::bluetooth_hal::hci::HalPacket BuildPrecomputedKeyCommand(
      const std::vector<::aidl::android::hardware::bluetooth::finder::Eid>&
          keys,
      uint_t cur_key_idx);
  ::bluetooth_hal::hci::HalPacket BuildStartPoweredOffFinderModeCommand(
      int32_t cur_key_idx);

  bool SendKeys();
  bool StartPoweredOffFinderModeInternal();

  void HandleNextStep(State next_state);
  bool SendCommandAndWait(const ::bluetooth_hal::hci::HalPacket& packet);

  std::vector<::aidl::android::hardware::bluetooth::finder::Eid> keys_;
  bool is_pof_enabled_{false};
  std::atomic<State> state_{State::kIdle};
  size_t current_key_index_{0};

  std::mutex finder_mtx_;

  // For synchronizing command sending.
  std::promise<void> command_promise_;
  bool command_success_{false};
};

}  // namespace finder
}  // namespace extensions
}  // namespace bluetooth_hal
