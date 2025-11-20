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

#define LOG_TAG "bthal.extensions.cs"

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
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_algorithm.h"
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

// Feature.
constexpr uint8_t kOneSidePct = 0x01;
constexpr uint8_t kMode0ChannelMap = 0x02;

static std::unique_ptr<ChannelSoundingAlgorithm> channel_sounding_algorithm =
    nullptr;

BluetoothChannelSoundingSession::BluetoothChannelSoundingSession(
    std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback,
    Reason /* reason */) {
  callback_ = callback;
  if (channel_sounding_algorithm == nullptr) {
    channel_sounding_algorithm = std::make_unique<ChannelSoundingAlgorithm>();
  }
}

ScopedAStatus BluetoothChannelSoundingSession::getVendorSpecificReplies(
    std::optional<std::vector<std::optional<VendorSpecificData>>>*
        _aidl_return) {
  LOG(INFO) << __func__;

  if (!uuid_matched_) {
    LOG(INFO) << "UUID doesn't matched, ignore";
    return ScopedAStatus::ok();
  }

  *_aidl_return =
      std::make_optional<std::vector<std::optional<VendorSpecificData>>>();
  VendorSpecificData capability;
  capability.characteristicUuid = kUuidSpecialRangingSettingCapability;
  capability.opaqueValue = {kDataTypeReply, 0x00, 0x00, 0x00, 0x00};
  (*_aidl_return)->push_back(capability);

  uint8_t enable_one_side_pct =
      enable_fake_notification_ ? kCommandValueEnable : kCommandValueIgnore;
  uint8_t enable_cs_subevent_report =
      enable_fake_notification_ ? kCommandValueDisable : kCommandValueIgnore;
  uint8_t enable_mode_0_channel_map =
      enable_mode_0_channel_map_ ? kCommandValueEnable : kCommandValueIgnore;

  VendorSpecificData command;
  command.characteristicUuid = kUuidSpecialRangingSettingCommand;
  command.opaqueValue = {kDataTypeReply, enable_one_side_pct,
                         enable_cs_subevent_report, enable_mode_0_channel_map};
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
  channel_sounding_algorithm->reset_variables();
  ranging_result.resultMeters =
      channel_sounding_algorithm->estimate_distance(in_rawData);
  ranging_result.confidenceLevel =
      channel_sounding_algorithm->get_confidence_level() * 100;
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
            << " vendor_specific_data_byte_1: " << vendor_specific_data_byte_1;

  if ((vendor_specific_data_byte_1 & kOneSidePct) != 0) {
    LOG(INFO) << __func__ << " support 1-side PCT";
    enable_fake_notification_ = true;
  } else {
    LOG(INFO) << __func__ << " do not support 1-side PCT";
    enable_fake_notification_ = false;
  }
  if ((vendor_specific_data_byte_1 & kMode0ChannelMap) != 0) {
    LOG(INFO) << __func__ << " support mode 0 Channel Map";
    enable_mode_0_channel_map_ = true;
  } else {
    LOG(INFO) << __func__ << " do not support mode 0 Channel Map";
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
