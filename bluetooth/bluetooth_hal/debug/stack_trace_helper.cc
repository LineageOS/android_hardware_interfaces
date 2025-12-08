/*
 * Copyright 2021 The Android Open Source Project
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

#include <cstdint>
#include <string>

#include "android-base/logging.h"
#include "bluetooth_hal/bqr/bqr_types.h"

namespace bluetooth_hal {
namespace debug {

using ::bluetooth_hal::bqr::BqrErrorCode;

typedef struct {
  BqrErrorCode error_code;
  void (*func)(std::string);
} ErrorCodeMap;

void UartParsing(std::string msg) { LOG(FATAL) << msg; }

void UartIncompletePacket(std::string msg) { LOG(FATAL) << msg; }

void FirmwareChecksum(std::string msg) { LOG(FATAL) << msg; }

void FirmwareHardFault(std::string msg) { LOG(FATAL) << msg; }

void FirmwareMemManageFault(std::string msg) { LOG(FATAL) << msg; }

void FirmwareBusFault(std::string msg) { LOG(FATAL) << msg; }

void FirmwareFirmwareUsageFault(std::string msg) { LOG(FATAL) << msg; }

void FirmwareWatchdogTimeout(std::string msg) { LOG(FATAL) << msg; }

void FirmwareAssertionFailure(std::string msg) { LOG(FATAL) << msg; }

void FirmwareMiscellaneous(std::string msg) { LOG(FATAL) << msg; }

void FirmwareMiscellaneousMajorFault(std::string msg) { LOG(FATAL) << msg; }

void FirmwareMiscellaneousCriticalFault(std::string msg) { LOG(FATAL) << msg; }

void FirmwareThreadGenericError(std::string msg) { LOG(FATAL) << msg; }

void FirmwareThreadInvalidFrame(std::string msg) { LOG(FATAL) << msg; }

void FirmwareThreadInvalidParam(std::string msg) { LOG(FATAL) << msg; }

void FirmwareThreadUnsupportedFrame(std::string msg) { LOG(FATAL) << msg; }

void SocBigHammerFault(std::string msg) { LOG(FATAL) << msg; }

void HostRxThreadStuck(std::string msg) { LOG(FATAL) << msg; }

void HostHciCommandTimeout(std::string msg) { LOG(FATAL) << msg; }

void HostInvalidHciEvent(std::string msg) { LOG(FATAL) << msg; }

void HostUnimplementedPacketType(std::string msg) { LOG(FATAL) << msg; }

void HosHcitH4TxError(std::string msg) { LOG(FATAL) << msg; }

void HostOpenUserial(std::string msg) { LOG(FATAL) << msg; }

void HostPowerUpController(std::string msg) { LOG(FATAL) << msg; }

void HostResetBeforeFw(std::string msg) { LOG(FATAL) << msg; }

void HostChangeBaudrate(std::string msg) { LOG(FATAL) << msg; }

void HostDownloadFw(std::string msg) { LOG(FATAL) << msg; }

void HostResetAfterFw(std::string msg) { LOG(FATAL) << msg; }

void HostBdaddrFault(std::string msg) { LOG(FATAL) << msg; }

void HostCoexDeviceOpenError(std::string msg) { LOG(FATAL) << msg; }

void HostAccelatedBtInitFailed(std::string msg) { LOG(FATAL) << msg; }

void HostAccelatedBtShutdownFailed(std::string msg) { LOG(FATAL) << msg; }

void ChreArbitratorUnimplementedPacket(std::string msg) { LOG(FATAL) << msg; }

void ChreArbitratorInvalidPacketSize(std::string msg) { LOG(FATAL) << msg; }

ErrorCodeMap kErrorCodeMap[]{
    {BqrErrorCode::kUartParsing, UartParsing},
    {BqrErrorCode::kUartIncompletePacket, UartIncompletePacket},
    {BqrErrorCode::kFirmwareChecksum, FirmwareChecksum},
    {BqrErrorCode::kFirmwareHardFault, FirmwareHardFault},
    {BqrErrorCode::kFirmwareMemManageFault, FirmwareMemManageFault},
    {BqrErrorCode::kFirmwareBusFault, FirmwareBusFault},
    {BqrErrorCode::kFirmwareUsageFault, FirmwareFirmwareUsageFault},
    {BqrErrorCode::kFirmwareWatchdogTimeout, FirmwareWatchdogTimeout},
    {BqrErrorCode::kFirmwareAssertionFailure, FirmwareAssertionFailure},
    {BqrErrorCode::kFirmwareMiscellaneous, FirmwareMiscellaneous},
    {BqrErrorCode::kFirmwareMiscellaneousMajorFault,
     FirmwareMiscellaneousMajorFault},
    {BqrErrorCode::kFirmwareMiscellaneousCriticalFault,
     FirmwareMiscellaneousCriticalFault},
    {BqrErrorCode::kFirmwareThreadGenericError, FirmwareThreadGenericError},
    {BqrErrorCode::kFirmwareThreadInvalidFrame, FirmwareThreadInvalidFrame},
    {BqrErrorCode::kFirmwareThreadInvalidParam, FirmwareThreadInvalidParam},
    {BqrErrorCode::kFirmwareThreadUnsupportedFrame,
     FirmwareThreadUnsupportedFrame},
    {BqrErrorCode::kSocBigHammerFault, SocBigHammerFault},
    {BqrErrorCode::kHostRxThreadStuck, HostRxThreadStuck},
    {BqrErrorCode::kHostHciCommandTimeout, HostHciCommandTimeout},
    {BqrErrorCode::kHostInvalidHciEvent, HostInvalidHciEvent},
    {BqrErrorCode::kHostUnimplementedPacketType, HostUnimplementedPacketType},
    {BqrErrorCode::kHostHciH4TxError, HosHcitH4TxError},
    {BqrErrorCode::kHostOpenUserial, HostOpenUserial},
    {BqrErrorCode::kHostPowerUpController, HostPowerUpController},
    {BqrErrorCode::kHostChangeBaudrate, HostChangeBaudrate},
    {BqrErrorCode::kHostResetBeforeFw, HostResetBeforeFw},
    {BqrErrorCode::kHostDownloadFw, HostDownloadFw},
    {BqrErrorCode::kHostResetAfterFw, HostResetAfterFw},
    {BqrErrorCode::kHostBdaddrFault, HostBdaddrFault},
    {BqrErrorCode::kHostOpenCoexDeviceError, HostCoexDeviceOpenError},
    {BqrErrorCode::kHostAccelBtInitFailed, HostAccelatedBtInitFailed},
    {BqrErrorCode::kHostAccelBtShutdownFailed, HostAccelatedBtShutdownFailed},
    {BqrErrorCode::kChreArbitratorUnimplementedPacket,
     ChreArbitratorUnimplementedPacket},
    {BqrErrorCode::kChreArbitratorInvalidPacketSize,
     ChreArbitratorInvalidPacketSize}};

void LogFatal(BqrErrorCode error_code, std::string extra_info) {
  int size = (int)(sizeof(kErrorCodeMap) / sizeof(ErrorCodeMap));
  std::string msg =
      "Bluetooth HAL crash with error code 0, vendor error code " +
      std::to_string((uint8_t)error_code);
  if (!extra_info.empty()) {
    msg = msg + ", extra info: " + extra_info;
  }
  for (int index = 0; index < size; index++) {
    if (kErrorCodeMap[index].error_code == error_code) {
      (kErrorCodeMap[index].func)(msg);
      break;
    }
  }
  // For unknown error code, will trigger crash on here.
  LOG(FATAL) << msg;
}

}  // namespace debug
}  // namespace bluetooth_hal
