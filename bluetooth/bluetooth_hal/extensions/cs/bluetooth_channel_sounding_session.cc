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

#define LOG_TAG "bluetooth_hal.extensions.cs"

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/ChannelSoudingRawData.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/RangingResult.h"
#include "aidl/android/hardware/bluetooth/ranging/Reason.h"
#include "aidl/android/hardware/bluetooth/ranging/ResultType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android-base/logging.h"
#include "android-base/properties.h"
#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator_interface.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_util.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

using ::aidl::android::hardware::bluetooth::ranging::ChannelSoudingRawData;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSessionCallback;
using ::aidl::android::hardware::bluetooth::ranging::RangingResult;
using ::aidl::android::hardware::bluetooth::ranging::Reason;
using ::aidl::android::hardware::bluetooth::ranging::ResultType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;

using ::android::base::GetBoolProperty;
using ::android::base::GetUintProperty;
using ::bluetooth_hal::Property;

using ::ndk::ScopedAStatus;

BluetoothChannelSoundingSession::BluetoothChannelSoundingSession(
    std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback,
    Reason /* reason */)
    : distance_estimator_(ChannelSoundingDistanceEstimatorInterface::Create()) {
  callback_ = callback;
}

ScopedAStatus BluetoothChannelSoundingSession::getVendorSpecificReplies(
    std::optional<std::vector<std::optional<VendorSpecificData>>>*
        _aidl_return) {
  LOG(INFO) << __func__;

  if (!uuid_matched_) {
    LOG(INFO) << ": UUID doesn't matched, ignore.";
    return ScopedAStatus::ok();
  }

  *_aidl_return =
      std::make_optional<std::vector<std::optional<VendorSpecificData>>>();
  VendorSpecificData capability;
  capability.characteristicUuid = kUuidSpecialRangingSettingCapability;
  capability.opaqueValue = {kDataTypeReply, 0x00, 0x00, 0x00, 0x00};
  (*_aidl_return)->push_back(capability);

  uint8_t enable_inline_pct =
      enable_fake_notification_ ? kCommandValueEnable : kCommandValueIgnore;

  // Event mask used by `Set event mask for connection` command. Set all event
  // bits to 0 — responder should ignore this if unsupported or inline PCT is
  // not enabled.
  constexpr uint32_t kEventMask = 0x00000000;

  uint8_t enable_mode_0_channel_map =
      enable_mode_0_channel_map_ ? kCommandValueEnable : kCommandValueIgnore;

  VendorSpecificData command;
  command.characteristicUuid = kUuidSpecialRangingSettingCommand;
  command.opaqueValue = {kDataTypeReply,
                         enable_inline_pct,
                         static_cast<uint8_t>((kEventMask >> 24) & 0xFF),
                         static_cast<uint8_t>((kEventMask >> 16) & 0xFF),
                         static_cast<uint8_t>((kEventMask >> 8) & 0xFF),
                         static_cast<uint8_t>((kEventMask) & 0xFF),
                         enable_mode_0_channel_map};
  (*_aidl_return)->push_back(command);

  for (auto& data : _aidl_return->value()) {
    LOG(INFO) << "uuid:" << ToHex(data->characteristicUuid)
              << ", data:" << ToHex(data->opaqueValue);
  }

  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothChannelSoundingSession::getSupportedResultTypes(
    std::vector<ResultType>* _aidl_return) {
  std::vector<ResultType> supported_result_types = {ResultType::RESULT_METERS};
  *_aidl_return = supported_result_types;
  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothChannelSoundingSession::isAbortedProcedureRequired(
    bool* _aidl_return) {
  *_aidl_return = false;
  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothChannelSoundingSession::writeRawData(
    const ChannelSoudingRawData& in_rawData) {
  if (in_rawData.stepChannels.empty()) {
    LOG(WARNING) << __func__ << " in_rawData.stepChannels is empty, skip";
    return ScopedAStatus::ok();
  }

  RangingResult ranging_result;
  distance_estimator_->ResetVariables();
  ranging_result.resultMeters =
      distance_estimator_->EstimateDistance(in_rawData);
  ranging_result.confidenceLevel =
      distance_estimator_->GetConfidenceLevel() * 100;
  callback_->onResult(ranging_result);
  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothChannelSoundingSession::close(Reason in_reason) {
  callback_->onClose(in_reason);
  return ScopedAStatus::ok();
}

void BluetoothChannelSoundingSession::HandleVendorSpecificData(
    const std::optional<std::vector<std::optional<VendorSpecificData>>>
        vendor_specific_data) {
  uuid_matched_ = IsUuidMatched(vendor_specific_data);
  if (!uuid_matched_) {
    return;
  }

  auto uuid0 = vendor_specific_data.value()[0];
  uint8_t vendor_specific_data_byte_1 =
      GetUintProperty(Property::kChannelSoundingVendorSpecificFirstDataByte,
                      uuid0.value().opaqueValue[1]);
  LOG(INFO) << __func__
            << ": vendor_specific_data_byte_1: " << vendor_specific_data_byte_1;

  if ((vendor_specific_data_byte_1 &
       static_cast<uint8_t>(CsFeature::kInlinePct)) != 0) {
    LOG(INFO) << __func__ << ": Support 1-side PCT.";
    enable_fake_notification_ = true;
  } else {
    LOG(INFO) << __func__ << ": Do not support Inline PCT.";
    enable_fake_notification_ = false;
  }
  if ((vendor_specific_data_byte_1 &
       static_cast<uint8_t>(CsFeature::kMode0ChannelMap)) != 0) {
    LOG(INFO) << __func__ << ": Support mode 0 Channel Map.";
    enable_mode_0_channel_map_ = true;
  } else {
    LOG(INFO) << __func__ << ": Do not support mode 0 Channel Map.";
    enable_mode_0_channel_map_ = false;
  }
}

bool BluetoothChannelSoundingSession::ShouldEnableFakeNotification() {
  return enable_fake_notification_;
}

bool BluetoothChannelSoundingSession::ShouldEnableMode0ChannelMap() {
  return enable_mode_0_channel_map_;
}

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
