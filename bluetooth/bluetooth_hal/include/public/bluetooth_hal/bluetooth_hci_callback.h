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

#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {

enum class BluetoothHciStatus : uint8_t {
  kSuccess = 0,
  kAlreadyInitialized,
  kHardwareInitializeError,
};

class BluetoothHciCallback {
 public:
  virtual ~BluetoothHciCallback() = default;

  virtual void InitializationComplete(BluetoothHciStatus status) = 0;
  virtual void HciEventReceived(
      const ::bluetooth_hal::hci::HalPacket& packet) = 0;
  virtual void AclDataReceived(
      const ::bluetooth_hal::hci::HalPacket& packet) = 0;
  virtual void ScoDataReceived(
      const ::bluetooth_hal::hci::HalPacket& packet) = 0;
  virtual void IsoDataReceived(
      const ::bluetooth_hal::hci::HalPacket& packet) = 0;
};

}  // namespace bluetooth_hal
