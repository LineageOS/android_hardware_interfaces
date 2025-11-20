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

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/BnBluetoothChannelSoundingSession.h"
#include "aidl/android/hardware/bluetooth/ranging/ChannelSoudingRawData.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/Reason.h"
#include "aidl/android/hardware/bluetooth/ranging/ResultType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android/binder_auto_utils.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

class BluetoothChannelSoundingSession
    : public ::aidl::android::hardware::bluetooth::ranging::
          BnBluetoothChannelSoundingSession {
 public:
  BluetoothChannelSoundingSession(
      std::shared_ptr<::aidl::android::hardware::bluetooth::ranging::
                          IBluetoothChannelSoundingSessionCallback>
          callback,
      ::aidl::android::hardware::bluetooth::ranging::Reason reason);

  ::ndk::ScopedAStatus getVendorSpecificReplies(
      std::optional<std::vector<std::optional<
          ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData>>>*
          _aidl_return) override;
  ::ndk::ScopedAStatus getSupportedResultTypes(
      std::vector<::aidl::android::hardware::bluetooth::ranging::ResultType>*
          _aidl_return) override;
  ::ndk::ScopedAStatus isAbortedProcedureRequired(bool* _aidl_return) override;
  ::ndk::ScopedAStatus writeRawData(
      const ::aidl::android::hardware::bluetooth::ranging::
          ChannelSoudingRawData& in_rawData) override;
  ::ndk::ScopedAStatus close(
      ::aidl::android::hardware::bluetooth::ranging::Reason in_reason) override;

  void HandleVendorSpecificData(
      const std::optional<std::vector<std::optional<
          ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData>>>
          vendor_specific_data);
  bool ShouldEnableFakeNotification();
  bool ShouldEnableMode0ChannelMap();

 private:
  std::shared_ptr<::aidl::android::hardware::bluetooth::ranging::
                      IBluetoothChannelSoundingSessionCallback>
      callback_;
  bool uuid_matched_ = false;
  bool enable_fake_notification_ = false;
  bool enable_mode_0_channel_map_ = false;
};

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
