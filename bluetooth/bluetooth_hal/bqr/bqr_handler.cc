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

#define LOG_TAG "bluetooth_hal.bqr"

#include "bluetooth_hal/bqr/bqr_handler.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "android-base/logging.h"
#include "bluetooth_hal/bqr/bqr_event.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v1_to_v3.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v4.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v5.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v6.h"
#include "bluetooth_hal/bqr/bqr_link_quality_event_v7.h"
#include "bluetooth_hal/bqr/bqr_root_inflammation_event.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace bqr {
namespace {

using ::bluetooth_hal::debug::DebugCentral;
using ::bluetooth_hal::hci::CommandOpCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciCommandCompleteEventMonitor;
using ::bluetooth_hal::hci::MonitorMode;

constexpr size_t kVendorCapabilityVersionOffset = 14;
const std::unordered_map<uint16_t, BqrVersion> kVersionToBqrMap = {
    {0x0001, BqrVersion::kV1ToV3}, {0x0101, BqrVersion::kV1ToV3},
    {0x0201, BqrVersion::kV4},     {0x0301, BqrVersion::kV5},
    {0x0401, BqrVersion::kV6},     {0x0501, BqrVersion::kV7},
};

}  // namespace

BqrHandler::BqrHandler()
    : local_supported_bqr_version_(BqrVersion::kNone),
      vendor_capability_monitor_(HciCommandCompleteEventMonitor(
          static_cast<uint16_t>(CommandOpCode::kGoogleVendorCapability))) {}

BqrHandler& BqrHandler::GetHandler() {
  static BqrHandler handler;
  return handler;
}

void BqrHandler::OnMonitorPacketCallback([[maybe_unused]] MonitorMode mode,
                                         const HalPacket& packet) {
  if (local_supported_bqr_version_ == BqrVersion::kNone) {
    if (packet.GetCommandCompleteEventResult() ==
            static_cast<uint8_t>(EventResultCode::kSuccess) &&
        packet.GetCommandOpcodeFromGeneratedEvent() ==
            static_cast<uint16_t>(CommandOpCode::kGoogleVendorCapability)) {
      HandleVendorCapabilityEvent(packet);
    }
    return;
  }

  BqrEvent bqr_event(packet);
  if (bqr_event.IsValid()) {
    auto bqr_event_type = bqr_event.GetBqrEventType();
    switch (bqr_event_type) {
      case BqrEventType::kRootInflammation:
        HandleRootInflammationEvent(bqr_event);
        break;
      case BqrEventType::kLinkQuality:
        HandleLinkQualityEvent(bqr_event);
        break;
      default:
        break;
    }
  }
}

void BqrHandler::HandleVendorCapabilityEvent(const HalPacket& packet) {
  if (packet.size() < kVendorCapabilityVersionOffset + sizeof(uint16_t)) {
    return;
  }
  uint16_t version =
      packet.AtUint16LittleEndian(kVendorCapabilityVersionOffset);
  auto it = kVersionToBqrMap.find(version);
  if (it != kVersionToBqrMap.end()) {
    local_supported_bqr_version_ = it->second;
    LOG(INFO) << "BQR supported version is "
              << static_cast<int>(local_supported_bqr_version_);
  } else {
    local_supported_bqr_version_ = BqrVersion::kNone;
    LOG(WARNING) << "Unknown BQR version from vendor capability: 0x" << std::hex
                 << version;
  }
}

void BqrHandler::HandleRootInflammationEvent(const BqrEvent& bqr_event) {
  BqrRootInflammationEvent root_inflammation(bqr_event);
  if (!root_inflammation.IsValid()) {
    return;
  }
  LOG(ERROR) << "Received a root inflammation event! " << bqr_event.ToString();
  DebugCentral::Get().HandleRootInflammationEvent(root_inflammation);
}

void BqrHandler::HandleLinkQualityEvent(const BqrEvent& bqr_event) {
  switch (local_supported_bqr_version_) {
    case BqrVersion::kV1ToV3: {
      BqrLinkQualityEventV1ToV3 link_quality_event(bqr_event);
      LOG(INFO) << link_quality_event.ToString();
    } break;
    case BqrVersion::kV4: {
      BqrLinkQualityEventV4 link_quality_event(bqr_event);
      LOG(INFO) << link_quality_event.ToString();
    } break;
    case BqrVersion::kV5: {
      BqrLinkQualityEventV5 link_quality_event(bqr_event);
      LOG(INFO) << link_quality_event.ToString();
    } break;
    case BqrVersion::kV6: {
      BqrLinkQualityEventV6 link_quality_event(bqr_event);
      LOG(INFO) << link_quality_event.ToString();
    } break;
    case BqrVersion::kV7: {
      BqrLinkQualityEventV7 link_quality_event(bqr_event);
      LOG(INFO) << link_quality_event.ToString();
    } break;
    default:
      break;
  }
}

void BqrHandler::OnBluetoothEnabled() {
  RegisterMonitor(bqr_event_monitor_, MonitorMode::kMonitor);
  RegisterMonitor(vendor_capability_monitor_, MonitorMode::kMonitor);
}

void BqrHandler::OnBluetoothDisabled() {
  local_supported_bqr_version_ = BqrVersion::kNone;
  UnregisterMonitor(bqr_event_monitor_);
  UnregisterMonitor(vendor_capability_monitor_);
}

}  //  namespace bqr
}  //  namespace bluetooth_hal
