/*
 * Copyright 2023 The Android Open Source Project
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

#include "bluetooth_hal/extensions/sar/bluetooth_sar.h"

#include <array>
#include <cstdint>

#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/sar/bluetooth_sar_handler.h"

namespace bluetooth_hal {
namespace extensions {
namespace sar {

using ::ndk::ScopedAStatus;

ScopedAStatus BluetoothSar::setBluetoothTxPowerCap(int8_t cap) {
  bool status = bluetooth_sar_handler_.SetBluetoothTxPowerCap(cap);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothSar::setBluetoothTechBasedTxPowerCap(int8_t br_cap,
                                                            int8_t edr_cap,
                                                            int8_t ble_cap) {
  bool status = bluetooth_sar_handler_.SetBluetoothTechBasedTxPowerCap(
      br_cap, edr_cap, ble_cap);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothSar::setBluetoothModeBasedTxPowerCap(
    const std::array<uint8_t, 3>& chain_0_cap,
    const std::array<uint8_t, 3>& chain_1_cap,
    const std::array<uint8_t, 6>& beamforming_cap) {
  bool status = bluetooth_sar_handler_.SetBluetoothModeBasedTxPowerCap(
      chain_0_cap, chain_1_cap, beamforming_cap);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}
ScopedAStatus BluetoothSar::setBluetoothModeBasedTxPowerCapPlusHR(
    const std::array<uint8_t, 4>& chain_0_cap,
    const std::array<uint8_t, 4>& chain_1_cap,
    const std::array<uint8_t, 8>& beamforming_cap) {
  bool status = bluetooth_sar_handler_.SetBluetoothModeBasedTxPowerCapPlusHR(
      chain_0_cap, chain_1_cap, beamforming_cap);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothSar::setBluetoothAreaCode(
    const std::array<uint8_t, 3>& /*area_code*/) {
  return ScopedAStatus::ok();
}

}  // namespace sar
}  // namespace extensions
}  // namespace bluetooth_hal
