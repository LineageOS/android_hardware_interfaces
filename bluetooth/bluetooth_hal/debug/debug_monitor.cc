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

#define LOG_TAG "bluetooth_hal.debug_monitor"

#include "bluetooth_hal/debug/debug_monitor.h"

#include <cstdint>

#include "android-base/logging.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace debug {
namespace {
using ::bluetooth_hal::hci::CommandOpCode;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::GoogleEventSubCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciCommandMonitor;
using ::bluetooth_hal::hci::HciEventMonitor;
using ::bluetooth_hal::hci::HciRouterClient;
using ::bluetooth_hal::hci::MonitorMode;

constexpr uint8_t kGoogleSubEventOffset = 3;
}  // namespace

DebugMonitor::DebugMonitor()
    : debug_info_command_monitor_(HciCommandMonitor(
          static_cast<uint16_t>(CommandOpCode::kGoogleDebugInfo))),
      debug_info_event_monitor_(HciEventMonitor(
          static_cast<uint8_t>(EventCode::kVendorSpecific),
          static_cast<uint8_t>(GoogleEventSubCode::kControllerDebugInfo),
          kGoogleSubEventOffset)) {
  RegisterMonitor(debug_info_command_monitor_, MonitorMode::kMonitor);
  RegisterMonitor(debug_info_event_monitor_, MonitorMode::kIntercept);
}

void DebugMonitor::OnMonitorPacketCallback([[maybe_unused]] MonitorMode mode,
                                           const HalPacket& packet) {
  if (packet.GetCommandOpcode() ==
      static_cast<uint16_t>(CommandOpCode::kGoogleDebugInfo)) {
    LOG(ERROR) << "Debug Info command detected!";
    DebugCentral::Get().HandleDebugInfoCommand();
    return;
  }
  if (packet.IsVendorEvent() && packet.size() > kGoogleSubEventOffset &&
      packet.At(kGoogleSubEventOffset) ==
          static_cast<uint8_t>(GoogleEventSubCode::kControllerDebugInfo)) {
    DebugCentral::Get().HandleDebugInfoEvent(packet);
  }
}

bool DebugMonitor::IsBluetoothEnabled() {
  return HciRouterClient::IsBluetoothEnabled();
}

void DebugMonitor::OnBluetoothEnabled() {
  DebugCentral::Get().ResetCoredumpGenerator();
}

void DebugMonitor::OnBluetoothDisabled() {
  DebugCentral::Get().ResetCoredumpGenerator();
}

}  //  namespace debug
}  //  namespace bluetooth_hal
