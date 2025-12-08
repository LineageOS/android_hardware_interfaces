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

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace config {
namespace constants {

inline constexpr ::bluetooth_hal::transport::TransportType
    kDefaultBtTransportType =
        ::bluetooth_hal::transport::TransportType::kUartH4;
inline constexpr std::string_view kHalConfigFile =
    "/vendor/etc/bluetooth/hal_config.json";
inline constexpr int kDefaultBtRegOnDelay = 100;
inline constexpr std::string_view kDefaultBtUartDevicePort = "/dev/ttySAC16";
inline constexpr int kDefaultVendorTransportCrashIntervalSec = 3600;

inline constexpr std::string_view kLpmEnableProcNode =
    "/proc/bluetooth/sleep/lpm";
inline constexpr std::string_view kLpmWakingProcNode =
    "/proc/bluetooth/sleep/btwrite";
inline constexpr std::string_view kLpmWakelockCtrlProcNode =
    "/proc/bluetooth/sleep/wakelock_ctrl";
inline constexpr std::string_view kRfkillFolderPrefix =
    "/sys/class/rfkill/rfkill";
inline constexpr std::string_view kRfkillTypeBluetooth = "bluetooth";

inline constexpr std::string_view kFirmwareConfigFile =
    "/vendor/etc/bluetooth/firmware_config.json";
inline constexpr int kDefaultLoadMiniDrvDelayMs = 50;
inline constexpr int kDefaultLaunchRamDelayMs = 250;

inline constexpr uint16_t kDefaultHciVscLaunchRamOpcode = 0xfc4e;
inline constexpr size_t kDefaultFixedChunkSize = 200;

}  // namespace constants
}  // namespace config
}  // namespace bluetooth_hal
