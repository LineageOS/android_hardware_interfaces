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

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

constexpr std::array<uint8_t, 16> kUuidSpecialRangingSettingCapability = {
    0x00, 0x00, 0x8f, 0x01, 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0x80, 0x5f, 0x9c, 0x35, 0xf1};

constexpr std::array<uint8_t, 16> kUuidSpecialRangingSettingCommand = {
    0x00, 0x00, 0x8f, 0x02, 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0x80, 0x5f, 0x9c, 0x35, 0xf1};

constexpr uint8_t kMinNumUuid = 0x02;
constexpr uint8_t kDataTypeData = 0x00;
constexpr uint8_t kDataTypeReply = 0x01;
constexpr uint8_t kCommandValueDisable = 0x00;
constexpr uint8_t kCommandValueEnable = 0x01;
constexpr uint8_t kCommandValueIgnore = 0x02;

constexpr uint8_t kCommandCompleteSubOpcodeOffset =
    ::bluetooth_hal::hci::HciConstants::kHciCommandCompleteResultOffset + 1;

constexpr uint16_t kHciVscSpecialRangingSettingOpcode = 0xff0b;

constexpr uint8_t kHciVscReadLocalCapabilityParamLength = 0x01;
constexpr uint8_t kHciVscReadLocalCapabilitySubOpCode = 0x01;
constexpr uint8_t kCommandCompleteReadLocalCapabilityOffset =
    kCommandCompleteSubOpcodeOffset + 1;
constexpr uint8_t kCommandCompleteReadLocalCapabilityValueLength = 4;

constexpr uint8_t kHciVscEnableInlinePctParamLength = 0x02;
constexpr uint8_t kHciVscEnableInlinePctSubOpCode = 0x02;

constexpr uint8_t kHciVscEnableCsSubeventReportParamLength = 0x04;
constexpr uint8_t kHciVscEnableCsSubeventReportSubOpCode = 0x03;

constexpr uint8_t kHciVscEnableMode0ChannelMapSubOpCode = 0x04;
constexpr uint8_t kHciVscEnableMode0ChannelMapParamLength = 0x04;

constexpr uint8_t kLeCsProcedureEnableCompleteCode = 0x30;
constexpr uint8_t kLeCsSubEventResultCode = 0x31;

// Used for RAS notification.
constexpr uint16_t kFlagFirstAutomaticallyFlushablePacket = 0x2000;
constexpr uint8_t kFakeRasDataLen = 0x18;
constexpr uint8_t kGattNotification = 0x1b;

constexpr uint16_t kInitialProcedureCounter = 0xffff;

enum class CsFeature : uint8_t {
  kInlinePct = 0x01,
  kMode0ChannelMap = 0x02,
  kPreferredConnectionInterval = 0x04,
  kPreferredSniffInterval = 0x08,
};

std::string ToHex(const std::span<const uint8_t> data);

bool IsUuidMatched(
    const std::optional<std::vector<std::optional<
        ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData>>>
        vendor_specific_data);

::bluetooth_hal::hci::HalPacket BuildReadLocalCapabilityCommand();

::bluetooth_hal::hci::HalPacket BuildEnableInlinePctCommand(uint8_t enable);

::bluetooth_hal::hci::HalPacket BuildEnableCsSubeventReportCommand(
    uint16_t connection_handle, uint8_t enable);

::bluetooth_hal::hci::HalPacket BuildEnableMode0ChannelMapCommand(
    uint16_t connection_handle, uint8_t enable);

::bluetooth_hal::hci::HalPacket BuildRasNotification(
    const ::aidl::android::hardware::bluetooth::ranging::
        BluetoothChannelSoundingParameters& parameters,
    int procedure_counter);

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
