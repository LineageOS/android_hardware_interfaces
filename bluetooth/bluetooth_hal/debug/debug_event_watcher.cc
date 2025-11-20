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

#define LOG_TAG "bthal.debug.watcher"

#include "bluetooth_hal/debug/debug_event_watcher.h"

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"

namespace bluetooth_hal {
namespace debug {

using ::bluetooth_hal::hci::CommandOpCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::MonitorMode;

DebugEventWatcher::DebugEventWatcher()
    : bqr_event_monitor_(),
      google_vendor_capability_event_monitor_(
          static_cast<uint16_t>(CommandOpCode::kGoogleVendorCapability)) {
  RegisterMonitor(bqr_event_monitor_, MonitorMode::kMonitor);
  RegisterMonitor(google_vendor_capability_event_monitor_,
                  MonitorMode::kMonitor);
}

DebugEventWatcher::~DebugEventWatcher() {
  UnregisterMonitor(bqr_event_monitor_);
  UnregisterMonitor(google_vendor_capability_event_monitor_);
}

void DebugEventWatcher::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode, const HalPacket& packet) {
  LOG(INFO) << __func__ << ": " << packet.ToString();
};

}  // namespace debug
}  // namespace bluetooth_hal
