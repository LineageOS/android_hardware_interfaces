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

#include <cstdint>

namespace bluetooth_hal {
namespace debug {

enum class AnchorType : uint8_t {
  kNone = 0,

  // HciProxy
  kStartHci,

  // BluetoothHci
  kInitialize,
  kClose,
  kServiceDied,
  kSendHciCommand,
  kSendAclData,
  kSendScoData,
  kSendIsoData,
  kCallbackHciEvent,
  kCallbackAclData,
  kCallbackScoData,
  kCallbackIsoData,

  // HciRouter
  kRouterInitialize,
  kTxTask,
  kRxTask,

  // Thread
  kThreadAcceptClient,
  kThreadDaemonClosed,
  kThreadSocketFileDeleted,
  kThreadClientError,
  kThreadClientConnect,
  kThreadHardwareReset,

  // H4 UART
  kUserialOpen,
  kUserialClose,
  kUserialTtyOpen,

  // PowerManager
  kPowerControl,
  kLowPowerMode,

  // WakelockWatchdog
  kWatchdog,
};

enum class CoredumpErrorCode : uint8_t {
  // Error codes for controller errors.
  kForceCollectCoredump,
  kControllerHwError,
  kControllerRootInflammed,
  kControllerDebugDumpWithoutRootInflammed,
  kControllerDebugInfo,
  kControllerUnimplementedPacketType,

  // Vendor specific error code for external implementations.
  kVendor = 0xFF,
};

}  // namespace debug
}  // namespace bluetooth_hal
