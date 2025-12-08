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

#define LOG_TAG "bluetooth_hal.extensions.sar"

#include "bluetooth_hal/extensions/sar/bluetooth_sar_handler.h"

#include <array>
#include <cstdint>
#include <sstream>
#include <string>

#include "android-base/logging.h"
#include "android-base/stringprintf.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace sar {

using ::android::base::StringPrintf;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::MonitorMode;

std::string logPowerLimit(uint8_t br_cap, uint8_t edr_cap, uint8_t ble_cap) {
  return StringPrintf("<br=%d, edr=%d, ble=%d>", br_cap, edr_cap, ble_cap);
}

std::string logPowerLimit(uint8_t br_cap, uint8_t edr_cap, uint8_t ble_cap,
                          uint8_t hr_cap) {
  return StringPrintf("<br=%d, edr=%d, ble=%d, hr=%d>", br_cap, edr_cap,
                      ble_cap, hr_cap);
}

uint8_t scopeCap(uint8_t cap, bool high_resolution_cap) {
  if (cap > 80) {
    LOG(WARNING) << __func__ << ": cap " << +cap
                 << " is greater than 80, set to 80";
    cap = 80;
  } else if (cap < 0) {
    LOG(WARNING) << __func__ << ": cap " << +cap
                 << " is smaller than 0, set to 0";
    cap = 0;
  }

  if (high_resolution_cap) {
    return cap;
  } else {
    return cap / kHciVscPowerCapScale;
  }
}

BluetoothSarHandler::BluetoothSarHandler() {
  SetClientLogTag("Bluetooth SAR Handler");
}

HalPacket BluetoothSarHandler::BuildCommandHRMode(
    const std::array<uint8_t, 4>& chain_0_cap,
    const std::array<uint8_t, 4>& chain_1_cap,
    const std::array<uint8_t, 8>& beamforming_cap, bool high_resolution_cap,
    bool is_ble_non_connection_enabled) {
  HalPacket command;
  command.resize(kHciVscSetPowerCapCmdLengthPlusHR);
  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = kHciVscSetPowerCapOpcode & 0xff;
  command[2] = (kHciVscSetPowerCapOpcode >> 8u) & 0xff;
  command[3] = 1 /* sub op */ + 1 /* command version */ +
               kHciVscSetPowerCapChain0PowerLimitSizePlusHR +
               kHciVscSetPowerCapChain1PowerLimitSizePlusHR +
               kHciVscSetPowerCapBeamformingPowerLimitSizePlusHR;
  command[4] = is_ble_non_connection_enabled
                   ? kHciVscSetPowerCapSubOpCodeLENonConnectionMode
                   : kHciVscSetPowerCapSubOpCodeHRMode;
  command[5] = kHciVscSetPowerCapPlusHRCommandVersion;
  for (int i = 0; i < kHciVscSetPowerCapChain0PowerLimitSizePlusHR; i++) {
    command[6 + i] = scopeCap(chain_0_cap[i], high_resolution_cap);
  }
  for (int i = 0; i < kHciVscSetPowerCapChain1PowerLimitSizePlusHR; i++) {
    command[6 + kHciVscSetPowerCapChain0PowerLimitSizePlusHR + i] =
        scopeCap(chain_1_cap[i], high_resolution_cap);
  }
  for (int i = 0; i < kHciVscSetPowerCapBeamformingPowerLimitSizePlusHR; i++) {
    command[6 + (2 * kHciVscSetPowerCapChain0PowerLimitSizePlusHR) + i] =
        scopeCap(beamforming_cap[i], high_resolution_cap);
  }
  return command;
}

HalPacket BluetoothSarHandler::BuildCommand(
    const std::array<uint8_t, 3>& chain_0_cap,
    const std::array<uint8_t, 3>& chain_1_cap,
    const std::array<uint8_t, 6>& beamforming_cap, bool high_resolution_cap) {
  HalPacket command;
  command.resize(kHciVscSetPowerCapCmdLength);
  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = kHciVscSetPowerCapOpcode & 0xff;
  command[2] = (kHciVscSetPowerCapOpcode >> 8u) & 0xff;
  command[3] = 1 /* sub op */ + kHciVscSetPowerCapChain0PowerLimitSize +
               kHciVscSetPowerCapChain1PowerLimitSize +
               kHciVscSetPowerCapBeamformingPowerLimitSize;
  if (high_resolution_cap) {
    command[4] = kHciVscSetPowerCapSubOpCodeHighResolution;
  } else {
    command[4] = kHciVscSetPowerCapSubOpCode;
  }
  for (int i = 0; i < kHciVscSetPowerCapChain0PowerLimitSize; i++) {
    command[5 + i] = scopeCap(chain_0_cap[i], high_resolution_cap);
  }
  for (int i = 0; i < kHciVscSetPowerCapChain1PowerLimitSize; i++) {
    command[5 + kHciVscSetPowerCapChain0PowerLimitSize + i] =
        scopeCap(chain_1_cap[i], high_resolution_cap);
  }
  for (int i = 0; i < kHciVscSetPowerCapBeamformingPowerLimitSize; i++) {
    command[5 + (2 * kHciVscSetPowerCapChain0PowerLimitSize) + i] =
        scopeCap(beamforming_cap[i], high_resolution_cap);
  }
  return command;
}

HalPacket BluetoothSarHandler::BuildCommand(uint8_t br_cap, uint8_t edr_cap,
                                            uint8_t ble_cap,
                                            bool high_resolution_cap) {
  const std::array<uint8_t, 3> chain_cap({br_cap, edr_cap, ble_cap});
  const std::array<uint8_t, 6> beamforming_cap(
      {br_cap, edr_cap, ble_cap, br_cap, edr_cap, ble_cap});
  return BuildCommand(chain_cap, chain_cap, beamforming_cap,
                      high_resolution_cap);
}

bool BluetoothSarHandler::SetBluetoothTxPowerCap(int8_t cap) {
  if (!IsBluetoothEnabled()) {
    LOG(WARNING) << __func__ << ": Unable to set power cap " << +cap;
    return false;
  }
  LOG(INFO) << __func__ << ": " << " Cap=" << +cap;
  HalPacket set_power_cap = BuildCommand(cap, cap, cap, high_resolution_cap_);
  return SendCommand(set_power_cap);
}

bool BluetoothSarHandler::SetBluetoothTechBasedTxPowerCap(int8_t br_cap,
                                                          int8_t edr_cap,
                                                          int8_t ble_cap) {
  if (!IsBluetoothEnabled()) {
    CLIENT_LOG(WARNING) << __func__ << ": Unable to set power cap "
                        << logPowerLimit(br_cap, edr_cap, ble_cap);
    return false;
  }
  CLIENT_LOG(INFO) << __func__ << ": "
                   << logPowerLimit(br_cap, edr_cap, ble_cap);
  HalPacket set_power_cap =
      BuildCommand(br_cap, edr_cap, ble_cap, high_resolution_cap_);
  return SendCommand(set_power_cap);
}

bool BluetoothSarHandler::SetBluetoothModeBasedTxPowerCap(
    const std::array<uint8_t, 3>& chain_0_cap,
    const std::array<uint8_t, 3>& chain_1_cap,
    const std::array<uint8_t, 6>& beamforming_cap) {
  std::stringstream tx_power_cap_ss;
  tx_power_cap_ss << "Chain 0 Power Cap:"
                  << logPowerLimit(chain_0_cap[0], chain_0_cap[1],
                                   chain_0_cap[2])
                  << ", Chain 1 Power Cap:"
                  << logPowerLimit(chain_1_cap[0], chain_1_cap[1],
                                   chain_1_cap[2])
                  << ", Beamforming Power Cap Chain 0: "
                  << logPowerLimit(beamforming_cap[0], beamforming_cap[1],
                                   beamforming_cap[2])
                  << ", Chain 1:"
                  << logPowerLimit(beamforming_cap[3], beamforming_cap[4],
                                   beamforming_cap[5]);

  if (!IsBluetoothEnabled()) {
    CLIENT_LOG(WARNING) << __func__ << ": Unable to set power cap - "
                        << tx_power_cap_ss.str();
    return false;
  }
  CLIENT_LOG(INFO) << __func__ << ": " << tx_power_cap_ss.str();

  HalPacket set_power_cap = BuildCommand(chain_0_cap, chain_1_cap,
                                         beamforming_cap, high_resolution_cap_);
  return SendCommand(set_power_cap);
}
bool BluetoothSarHandler::SetBluetoothModeBasedTxPowerCapPlusHR(
    const std::array<uint8_t, 4>& chain_0_cap,
    const std::array<uint8_t, 4>& chain_1_cap,
    const std::array<uint8_t, 8>& beamforming_cap) {
  std::stringstream tx_power_cap_ss;
  tx_power_cap_ss << "Chain 0 Power Cap:"
                  << logPowerLimit(chain_0_cap[0], chain_0_cap[1],
                                   chain_0_cap[2], chain_0_cap[3])
                  << ", Chain 1 Power Cap:"
                  << logPowerLimit(chain_1_cap[0], chain_1_cap[1],
                                   chain_1_cap[2], chain_1_cap[3])
                  << ", Beamforming Power Cap Chain 0: "
                  << logPowerLimit(beamforming_cap[0], beamforming_cap[1],
                                   beamforming_cap[2], beamforming_cap[3])
                  << ", Chain 1:"
                  << logPowerLimit(beamforming_cap[4], beamforming_cap[5],
                                   beamforming_cap[6], beamforming_cap[7]);

  if (!IsBluetoothEnabled()) {
    CLIENT_LOG(WARNING) << __func__ << ": Unable to set power cap "
                        << tx_power_cap_ss.str();
    return false;
  }
  CLIENT_LOG(INFO) << __func__ << ": " << tx_power_cap_ss.str();

  HalPacket set_power_cap =
      BuildCommandHRMode(chain_0_cap, chain_1_cap, beamforming_cap,
                         high_resolution_cap_, is_ble_non_connection_enabled_);
  return SendCommand(set_power_cap);
}

bool BluetoothSarHandler::SetBluetoothAreaCode(int32_t /*area_code*/) {
  return true;
}

void BluetoothSarHandler::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode,
    [[maybe_unused]] const HalPacket& packet) {
  // Unused method
}

void BluetoothSarHandler::OnCommandCallback(const HalPacket& event) {
  bool success = (event.GetCommandCompleteEventResult() ==
                  static_cast<uint8_t>(EventResultCode::kSuccess));
  CLIENT_LOG(INFO) << __func__ << ": Recv VSE <" << event.ToString() << "> "
                   << (success ? "[Success]" : "[Failed]");
}

void BluetoothSarHandler::OnBluetoothEnabled() {
  LOG(DEBUG) << __func__;
  high_resolution_cap_ =
      HalConfigLoader::GetLoader().IsSarBackoffHighResolutionEnabled();
  is_ble_non_connection_enabled_ =
      HalConfigLoader::GetLoader().IsBleNonConnectionSarEnabled();
}

void BluetoothSarHandler::OnBluetoothDisabled() { LOG(DEBUG) << __func__; }

}  // namespace sar
}  // namespace extensions
}  // namespace bluetooth_hal
