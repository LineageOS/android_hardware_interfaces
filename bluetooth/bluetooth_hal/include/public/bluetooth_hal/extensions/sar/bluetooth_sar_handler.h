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

#include <array>
#include <cstddef>
#include <cstdint>

#include "bluetooth_hal/debug/debug_client.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace extensions {
namespace sar {

// OGC 0x03 | OCF 0x0269
constexpr uint16_t kHciVscSetPowerCapOpcode = 0xfe69;
constexpr uint8_t kHciVscSetPowerCapSubOpCode = 0x01;
constexpr uint8_t kHciVscSetPowerCapSubOpCodeHighResolution = 0x05;
constexpr uint8_t kHciVscSetPowerCapSubOpCodeHRMode = 0x08;
constexpr uint8_t kHciVscSetPowerCapSubOpCodeLENonConnectionMode = 0x0F;
constexpr uint8_t kHciVscSetPowerCapPlusHRCommandVersion = 1;
constexpr uint8_t kHciVscSetPowerCapChain0PowerLimitSize = 3;
constexpr uint8_t kHciVscSetPowerCapChain1PowerLimitSize = 3;
constexpr uint8_t kHciVscSetPowerCapBeamformingPowerLimitSize = 6;
constexpr uint8_t kHciVscSetPowerCapChain0PowerLimitSizePlusHR = 4;
constexpr uint8_t kHciVscSetPowerCapChain1PowerLimitSizePlusHR = 4;
constexpr uint8_t kHciVscSetPowerCapBeamformingPowerLimitSizePlusHR = 8;
constexpr size_t kHciVscSetPowerCapCmdLength =
    1 /* Packet type */ +
    ::bluetooth_hal::hci::HciConstants::kHciCommandPreambleSize +
    1 /* Sub Opcode size*/
    + kHciVscSetPowerCapChain0PowerLimitSize +
    kHciVscSetPowerCapChain1PowerLimitSize +
    kHciVscSetPowerCapBeamformingPowerLimitSize;
constexpr size_t kHciVscSetPowerCapCmdLengthPlusHR =
    1 /* Packet type */ +
    ::bluetooth_hal::hci::HciConstants::kHciCommandPreambleSize +
    1 /* Sub Opcode size*/ + 1 /* Command version size */
    + kHciVscSetPowerCapChain0PowerLimitSizePlusHR +
    kHciVscSetPowerCapChain1PowerLimitSizePlusHR +
    kHciVscSetPowerCapBeamformingPowerLimitSizePlusHR;
constexpr uint8_t kHciVscPowerCapScale = 4;

class BluetoothSarHandler : public ::bluetooth_hal::hci::HciRouterClient,
                            public ::bluetooth_hal::debug::DebugClient {
 public:
  BluetoothSarHandler();
  bool SetBluetoothTxPowerCap(int8_t cap);
  bool SetBluetoothTechBasedTxPowerCap(int8_t br_cap, int8_t edr_cap,
                                       int8_t ble_cap);
  bool SetBluetoothModeBasedTxPowerCap(
      const std::array<uint8_t, 3>& chain_0_cap,
      const std::array<uint8_t, 3>& chain_1_cap,
      const std::array<uint8_t, 6>& beamforming_cap);
  bool SetBluetoothModeBasedTxPowerCapPlusHR(
      const std::array<uint8_t, 4>& chain_0_cap,
      const std::array<uint8_t, 4>& chain_1_cap,
      const std::array<uint8_t, 8>& beamforming_cap);
  bool SetBluetoothAreaCode(int32_t area_code);

 protected:
  ::bluetooth_hal::hci::HalPacket BuildCommandHRMode(
      const std::array<uint8_t, 4>& chain_0_cap,
      const std::array<uint8_t, 4>& chain_1_cap,
      const std::array<uint8_t, 8>& beamforming_cap, bool high_resolution_cap,
      bool is_ble_non_connection_enabled);
  ::bluetooth_hal::hci::HalPacket BuildCommand(
      const std::array<uint8_t, 3>& chain_0_cap,
      const std::array<uint8_t, 3>& chain_1_cap,
      const std::array<uint8_t, 6>& beamforming_cap, bool high_resolution_cap);
  ::bluetooth_hal::hci::HalPacket BuildCommand(uint8_t br_cap, uint8_t edr_cap,
                                               uint8_t ble_cap,
                                               bool high_resolution_cap);

  void OnBluetoothChipReady() override {};
  void OnBluetoothChipClosed() override {};
  void OnBluetoothEnabled() override;
  void OnBluetoothDisabled() override;
  void OnCommandCallback(
      const ::bluetooth_hal::hci::HalPacket& packet) override;
  void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const ::bluetooth_hal::hci::HalPacket& packet) override;

 private:
  bool high_resolution_cap_ = false;
  bool is_ble_non_connection_enabled_ = false;
};

}  // namespace sar
}  // namespace extensions
}  // namespace bluetooth_hal
