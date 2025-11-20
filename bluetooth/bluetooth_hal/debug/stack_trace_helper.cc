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

#include "android-base/logging.h"
#include "bluetooth_hal/debug/debug_central.h"

namespace bluetooth_hal {
namespace debug {

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
    {BqrErrorCode::UART_PARSING, UartParsing},
    {BqrErrorCode::UART_INCOMPLETE_PACKET, UartIncompletePacket},
    {BqrErrorCode::FIRMWARE_CHECKSUM, FirmwareChecksum},
    {BqrErrorCode::FIRMWARE_HARD_FAULT, FirmwareHardFault},
    {BqrErrorCode::FIRMWARE_MEM_MANAGE_FAULT, FirmwareMemManageFault},
    {BqrErrorCode::FIRMWARE_BUS_FAULT, FirmwareBusFault},
    {BqrErrorCode::FIRMWARE_USAGE_FAULT, FirmwareFirmwareUsageFault},
    {BqrErrorCode::FIRMWARE_WATCHDOG_TIMEOUT, FirmwareWatchdogTimeout},
    {BqrErrorCode::FIRMWARE_ASSERTION_FAILURE, FirmwareAssertionFailure},
    {BqrErrorCode::FIRMWARE_MISCELLANEOUS, FirmwareMiscellaneous},
    {BqrErrorCode::FIRMWARE_MISCELLANEOUS_MAJOR_FAULT,
     FirmwareMiscellaneousMajorFault},
    {BqrErrorCode::FIRMWARE_MISCELLANEOUS_CRITICAL_FAULT,
     FirmwareMiscellaneousCriticalFault},
    {BqrErrorCode::FIRMWARE_THREAD_GENERIC_ERROR, FirmwareThreadGenericError},
    {BqrErrorCode::FIRMWARE_THREAD_INVALID_FRAME, FirmwareThreadInvalidFrame},
    {BqrErrorCode::FIRMWARE_THREAD_INVALID_PARAM, FirmwareThreadInvalidParam},
    {BqrErrorCode::FIRMWARE_THREAD_UNSUPPORTED_FRAME,
     FirmwareThreadUnsupportedFrame},
    {BqrErrorCode::SOC_BIG_HAMMER_FAULT, SocBigHammerFault},
    {BqrErrorCode::HOST_RX_THREAD_STUCK, HostRxThreadStuck},
    {BqrErrorCode::HOST_HCI_COMMAND_TIMEOUT, HostHciCommandTimeout},
    {BqrErrorCode::HOST_INVALID_HCI_EVENT, HostInvalidHciEvent},
    {BqrErrorCode::HOST_UNIMPLEMENTED_PACKET_TYPE, HostUnimplementedPacketType},
    {BqrErrorCode::HOST_HCI_H4_TX_ERROR, HosHcitH4TxError},
    {BqrErrorCode::HOST_OPEN_USERIAL, HostOpenUserial},
    {BqrErrorCode::HOST_POWER_UP_CONTROLLER, HostPowerUpController},
    {BqrErrorCode::HOST_CHANGE_BAUDRATE, HostChangeBaudrate},
    {BqrErrorCode::HOST_RESET_BEFORE_FW, HostResetBeforeFw},
    {BqrErrorCode::HOST_DOWNLOAD_FW, HostDownloadFw},
    {BqrErrorCode::HOST_RESET_AFTER_FW, HostResetAfterFw},
    {BqrErrorCode::HOST_BDADDR_FAULT, HostBdaddrFault},
    {BqrErrorCode::HOST_OPEN_COEX_DEVICE_ERROR, HostCoexDeviceOpenError},
    {BqrErrorCode::HOST_ACCEL_BT_INIT_FAILED, HostAccelatedBtInitFailed},
    {BqrErrorCode::HOST_ACCEL_BT_SHUTDOWN_FAILED,
     HostAccelatedBtShutdownFailed},
    {BqrErrorCode::CHRE_ARBITRATOR_UNIMPLEMENTED_PACKET,
     ChreArbitratorUnimplementedPacket},
    {BqrErrorCode::CHRE_ARBITRATOR_INVALID_PACKET_SIZE,
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
