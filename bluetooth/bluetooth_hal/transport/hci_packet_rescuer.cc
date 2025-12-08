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

#define LOG_TAG "bluetooth_hal.transport.hci_packet_rescuer"

#include "bluetooth_hal/transport/hci_packet_rescuer.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "android-base/logging.h"
#include "bluetooth_hal/debug/bluetooth_activities.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/vendor_packet_validator_interface.h"

namespace bluetooth_hal {
namespace transport {
namespace {

using ::bluetooth_hal::debug::BluetoothActivities;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciPacketType;

constexpr size_t kAclPacketRequiredLength = 3;
constexpr size_t kThreadPacketRequiredLength = 6;
constexpr size_t kNumberOfCompletedPacketNumHandlesOffset = 3;
constexpr size_t kCommandCompleteNumPacketsOffset = 3;
constexpr size_t kHciEventMinimumLength =
    HciConstants::kHciEventLengthOffset + 1;
constexpr uint8_t kBleMinimumEventSubCodeForRescue = 0x01;
constexpr uint8_t kBleMaximumEventSubCodeForRescue = 0x29;

static const std::unordered_map<EventCode, uint8_t> kEventCodeToItsParamLength =
    {
        {EventCode::kCommandStatus, 0x04},
        {EventCode::kConnectionComplete, 0x0B},
        {EventCode::kConnectionRequest, 0x0A},
        {EventCode::kDisconnectionComplete, 0x04},
        {EventCode::kReadRemoteVersionInformationComplete, 0x08},
        {EventCode::kQosSetupComplete, 0x15},
        {EventCode::kRoleChange, 0x08},
        {EventCode::kModeChange, 0x06},
        {EventCode::kLinkKeyRequest, 0x06},
        {EventCode::kMaxSlotsChange, 0x03},
        {EventCode::kReadRemoteExtendedFeaturesComplete, 0x0d},
        {EventCode::kSniffSubrating, 0x0b},
        {EventCode::kEncryptionKeyRefreshComplete, 0x03},
        {EventCode::kLinkSupervisionTimeoutChanged, 0x04},
        {EventCode::kEnhancedFlushComplete, 0x02},
};

}  // namespace

HciPacketRescuer::HciPacketRescuer() {
  if (vendor_packet_validator_) {
    return;
  }
  vendor_packet_validator_ = VendorPacketValidatorInterface::Create();
  if (!vendor_packet_validator_) {
    LOG(ERROR) << __func__ << ": Failed to create VendorPacketValidator.";
  }
}

bool HciPacketRescuer::VerifyEventCodeAndItsParamLength(
    std::span<const uint8_t> data, EventCode event_code) {
  const size_t length = data.size();

  if (HciConstants::kHciEventLengthOffset >= length ||
      data[HciConstants::kHciEventLengthOffset] !=
          length - kHciEventMinimumLength) {
    return false;
  }

  switch (event_code) {
    case EventCode::kBleMeta: {
      if (HciConstants::kHciBleEventSubCodeOffset >= length) {
        return false;
      }
      const uint8_t sub_event_code =
          data[HciConstants::kHciBleEventSubCodeOffset];
      return sub_event_code >= kBleMinimumEventSubCodeForRescue &&
             sub_event_code <= kBleMaximumEventSubCodeForRescue;
    }
    case EventCode::kVendorSpecific: {
      if (!vendor_packet_validator_) {
        return false;
      }
      return vendor_packet_validator_->IsValidVendorSpecificEvent(data);
    }
    case EventCode::kNumberOfCompletedPackets: {
      if (kNumberOfCompletedPacketNumHandlesOffset >= length) {
        return false;
      }
      const uint8_t num_handles =
          data[kNumberOfCompletedPacketNumHandlesOffset];
      return num_handles <=
             BluetoothActivities::Get().GetConnectionHandleCount();
    }
    case EventCode::kCommandComplete: {
      if (kCommandCompleteNumPacketsOffset >= length) {
        return false;
      }
      const uint8_t num_packets = data[kCommandCompleteNumPacketsOffset];
      return num_packets == 0x01;
    }
    case EventCode::kCommandStatus:
    case EventCode::kConnectionComplete:
    case EventCode::kConnectionRequest:
    case EventCode::kDisconnectionComplete:
    case EventCode::kReadRemoteVersionInformationComplete:
    case EventCode::kQosSetupComplete:
    case EventCode::kRoleChange:
    case EventCode::kModeChange:
    case EventCode::kLinkKeyRequest:
    case EventCode::kMaxSlotsChange:
    case EventCode::kReadRemoteExtendedFeaturesComplete:
    case EventCode::kSniffSubrating:
    case EventCode::kEncryptionKeyRefreshComplete:
    case EventCode::kLinkSupervisionTimeoutChanged:
    case EventCode::kEnhancedFlushComplete: {
      if (HciConstants::kHciEventLengthOffset >= length) {
        return false;
      }
      if (kEventCodeToItsParamLength.find(event_code) ==
          kEventCodeToItsParamLength.end()) {
        return false;
      }
      return data[HciConstants::kHciEventLengthOffset] ==
             kEventCodeToItsParamLength.at(event_code);
    }
    default:
      return false;
  }
}

bool HciPacketRescuer::IsProbablyValidAclPacket(std::span<const uint8_t> data) {
  // ACL Packet Rule: Check if handle connected.
  // byte 0   : ACL Packet Type (0x02).
  // byte 1, 2: Connection Handle.
  if (kAclPacketRequiredLength > data.size()) {
    return false;
  }
  uint16_t connect_handle = data[1] + ((data[2] << 8u) & 0x0F00);
  return BluetoothActivities::Get().IsConnected(connect_handle);
}

bool HciPacketRescuer::IsProbablyValidThreadPacket(
    std::span<const uint8_t> data) {
  // Thread Packet Rule: Check values in the below bytes.
  // byte 1, 2: Fixed value (0x00)
  // byte 5   : Value in range [0x80, 0x8f]
  if (kThreadPacketRequiredLength > data.size()) {
    return false;
  }
  return (data[1] == 0x00) && (data[2] == 0x00) && ((data[5] & 0xF0) == 0x80);
}

/**
 * @brief Checks if given packet data might be a valid HCI packet.
 *
 * This function validates the potential packet start by examining the packet
 * type indicator and performing type-specific checks. For some types (e.g.,
 * ACL), it performs semantic validation against the current system state, while
 * for others it performs syntactic checks on the packet's preamble.
 *
 * @param data A span representing the raw byte stream.
 * @return true if the offset points to a valid and recognized packet start,
 * otherwise false.
 */
bool HciPacketRescuer::IsValidHciPacket(std::span<const uint8_t> data) {
  const size_t length = data.size();
  if (length <= 0) {
    return false;
  }

  const HciPacketType hci_packet_type = static_cast<HciPacketType>(data[0]);
  switch (hci_packet_type) {
    case HciPacketType::kAclData: {
      return IsProbablyValidAclPacket(data);
    }
    case HciPacketType::kThreadData: {
      return IsProbablyValidThreadPacket(data);
    }
    case HciPacketType::kEvent: {
      if (length <= HciConstants::kHciEventCodeOffset) {
        return false;
      }
      const EventCode event_code =
          static_cast<EventCode>(data[HciConstants::kHciEventCodeOffset]);
      return VerifyEventCodeAndItsParamLength(data, event_code);
    }
    default:
      return false;
  }
}

size_t HciPacketRescuer::FindValidPacketOffset(std::span<const uint8_t> data) {
  const size_t length = data.size();
  for (size_t offset = 0; offset < length; ++offset) {
    if (IsValidHciPacket(data.subspan(offset))) {
      return offset;
    }
  }
  return length;
}

}  // namespace transport
}  // namespace bluetooth_hal
