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

#define LOG_TAG "bluetooth_hal.extensions.cs"

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_util.h"

#include <cstdint>
#include <iomanip>
#include <ios>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

using ::aidl::android::hardware::bluetooth::ranging::
    BluetoothChannelSoundingParameters;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;

using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;

std::string ToHex(const std::span<const uint8_t> data) {
  std::stringstream ss;
  ss << std::hex << std::uppercase;
  for (const uint8_t byte : data) {
    ss << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }
  return ss.str();
}

bool IsUuidMatched(
    const std::optional<std::vector<std::optional<VendorSpecificData>>>
        vendor_specific_data) {
  if (!vendor_specific_data.has_value()) {
    LOG(WARNING) << __func__ << ": No value.";
    return false;
  }

  const auto& data = *vendor_specific_data;

  if (data.size() < kMinNumUuid) {
    LOG(WARNING) << __func__ << ": Invalid size.";
    return false;
  }

  const auto uuid0 = data[0];
  if (!uuid0.has_value() ||
      uuid0->characteristicUuid != kUuidSpecialRangingSettingCapability) {
    LOG(WARNING)
        << __func__
        << ": uuid0 doesn't match kUuidSpecialRangingSettingCapability.";
    return false;
  }

  if (uuid0->opaqueValue.size() < 5) {
    LOG(WARNING) << __func__
                 << ": Invalid data for kUuidSpecialRangingSettingCapability.";
    return false;
  }

  const auto uuid1 = data[1];
  if (!uuid1.has_value() ||
      uuid1->characteristicUuid != kUuidSpecialRangingSettingCommand) {
    LOG(WARNING) << __func__
                 << ": uuid0 doesn't match kUuidSpecialRangingSettingCommand.";
    return false;
  }

  return true;
}

HalPacket BuildReadLocalCapabilityCommand() {
  HalPacket command;
  command.resize(1 + 3 + kHciVscReadLocalCapabilityParamLength);
  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = kHciVscSpecialRangingSettingOpcode & 0xff;
  command[2] = (kHciVscSpecialRangingSettingOpcode >> 8u) & 0xff;
  command[3] = kHciVscReadLocalCapabilityParamLength;
  command[4] = kHciVscReadLocalCapabilitySubOpCode;

  return command;
}

HalPacket BuildEnableInlinePctCommand(uint8_t enable) {
  HalPacket command;
  command.resize(1 + 3 + kHciVscEnableInlinePctParamLength);
  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = kHciVscSpecialRangingSettingOpcode & 0xff;
  command[2] = (kHciVscSpecialRangingSettingOpcode >> 8u) & 0xff;
  command[3] = kHciVscEnableInlinePctParamLength;
  command[4] = kHciVscEnableInlinePctSubOpCode;
  command[5] = enable;

  return command;
}

HalPacket BuildEnableCsSubeventReportCommand(uint16_t connection_handle,
                                             uint8_t enable) {
  HalPacket command;
  command.resize(1 + 3 + kHciVscEnableCsSubeventReportParamLength);
  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = kHciVscSpecialRangingSettingOpcode & 0xff;
  command[2] = (kHciVscSpecialRangingSettingOpcode >> 8u) & 0xff;
  command[3] = kHciVscEnableCsSubeventReportParamLength;
  command[4] = kHciVscEnableCsSubeventReportSubOpCode;
  command[5] = connection_handle & 0xff;
  command[6] = (connection_handle >> 8u) & 0xff;
  command[7] = enable;

  return command;
}

HalPacket BuildEnableMode0ChannelMapCommand(uint16_t connection_handle,
                                            uint8_t enable) {
  HalPacket command;
  command.resize(1 + 3 + kHciVscEnableMode0ChannelMapParamLength);
  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = kHciVscSpecialRangingSettingOpcode & 0xff;
  command[2] = (kHciVscSpecialRangingSettingOpcode >> 8u) & 0xff;
  command[3] = kHciVscEnableMode0ChannelMapParamLength;
  command[4] = kHciVscEnableMode0ChannelMapSubOpCode;
  command[5] = connection_handle & 0xff;
  command[6] = (connection_handle >> 8u) & 0xff;
  command[7] = enable;

  return command;
}

HalPacket BuildRasNotification(
    const BluetoothChannelSoundingParameters& parameters,
    int procedure_counter) {
  uint16_t connection_handle = parameters.aclHandle;
  connection_handle |= kFlagFirstAutomaticallyFlushablePacket;

  HalPacket packet;
  packet.resize(1 + kFakeRasDataLen);

  uint16_t acl_data_len = kFakeRasDataLen - 4;
  uint16_t l2cap_data_len = acl_data_len - 4;
  uint16_t cid_att = 0x0004;
  uint16_t start_acl_conn_event = 0x0053;
  uint16_t frequency_compensation = 0x0000;

  packet[0] = static_cast<uint8_t>(HciPacketType::kAclData);
  packet[1] = connection_handle & 0xff;
  packet[2] = (connection_handle >> 8u) & 0xff;
  packet[3] = acl_data_len & 0xff;
  packet[4] = (acl_data_len >> 8u) & 0xff;
  packet[5] = l2cap_data_len & 0xff;
  packet[6] = (l2cap_data_len >> 8u) & 0xff;
  packet[7] = cid_att & 0xff;
  packet[8] = (cid_att >> 8u) & 0xff;
  packet[9] = kGattNotification;
  packet[10] = parameters.realTimeProcedureDataAttHandle & 0xff;
  packet[11] = (parameters.realTimeProcedureDataAttHandle >> 8u) & 0xff;
  // RAS fragment data.
  packet[12] = 0x03;  // segmentation_header, first and last fragment.
  packet[13] = procedure_counter & 0xff;  // ranging counter.
  packet[14] = ((procedure_counter >> 8u) & 0x0f) +
               0x10;  // ranging_counter and configuration_id.
  packet[15] = 0xe0;  // selected_tx_power.
  packet[16] = 0x01;  // antenna_paths_mask, PctFormat.
  packet[17] = start_acl_conn_event & 0xff;
  packet[18] = (start_acl_conn_event >> 8u) & 0xff;
  packet[19] = frequency_compensation & 0xff;
  packet[20] = (frequency_compensation >> 8u) & 0xff;
  packet[21] = 0x00;  // RangingDoneStatus, RangingDoneStatus.
  packet[22] = 0x00;  // RangingAbortReason, SubeventAbortReason.
  packet[23] = 0xe7;  // reference_power_level.
  packet[24] = 0x00;  // num_steps_reported.

  return packet;
}

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
