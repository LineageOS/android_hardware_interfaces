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
#include <memory>
#include <mutex>

#include "bluetooth_hal/bluetooth_hci_callback.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {

class BluetoothHci {
 public:
  static BluetoothHci& GetHci();
  static void StartHci() { GetHci(); }

  BluetoothHci();
  bool Initialize(const std::shared_ptr<BluetoothHciCallback>& cb);

  bool SendHciCommand(const ::bluetooth_hal::hci::HalPacket& packet);
  bool SendAclData(const ::bluetooth_hal::hci::HalPacket& packet);
  bool SendScoData(const ::bluetooth_hal::hci::HalPacket& packet);
  bool SendIsoData(const ::bluetooth_hal::hci::HalPacket& packet);
  bool Close();
  bool Dump(int fd);
  void HandleSignal(int signum);
  void HandleServiceDied();

 private:
  void DispatchPacketToStack(const ::bluetooth_hal::hci::HalPacket& packet);
  void HandleHalStateChanged(::bluetooth_hal::HalState new_state,
                             ::bluetooth_hal::HalState old_state);
  void SendDataToController(const ::bluetooth_hal::hci::HalPacket& packet);

  std::shared_ptr<BluetoothHciCallback> bluetooth_hci_callback_;
  bool is_initializing_;
  std::mutex callback_mutex_;

  static inline std::atomic<bool> is_sigterm_handled_{false};
};

}  // namespace bluetooth_hal
