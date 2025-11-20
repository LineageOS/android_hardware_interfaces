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

#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/BnBluetoothChannelSounding.h"
#include "aidl/android/hardware/bluetooth/ranging/CsSecurityLevel.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSession.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/SessionType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_handler.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

class BluetoothChannelSounding : public ::aidl::android::hardware::bluetooth::
                                     ranging::BnBluetoothChannelSounding {
 public:
  BluetoothChannelSounding() = default;
  ~BluetoothChannelSounding() = default;

  ::ndk::ScopedAStatus getVendorSpecificData(
      std::optional<std::vector<std::optional<
          ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData>>>*
          _aidl_return) override;
  ::ndk::ScopedAStatus getSupportedSessionTypes(
      std::optional<std::vector<
          ::aidl::android::hardware::bluetooth::ranging::SessionType>>*
          _aidl_return) override;
  ::ndk::ScopedAStatus getMaxSupportedCsSecurityLevel(
      ::aidl::android::hardware::bluetooth::ranging::CsSecurityLevel*
          _aidl_return) override;
  ::ndk::ScopedAStatus openSession(
      const ::aidl::android::hardware::bluetooth::ranging::
          BluetoothChannelSoundingParameters& in_params,
      const std::shared_ptr<::aidl::android::hardware::bluetooth::ranging::
                                IBluetoothChannelSoundingSessionCallback>&
          in_callback,
      std::shared_ptr<::aidl::android::hardware::bluetooth::ranging::
                          IBluetoothChannelSoundingSession>* _aidl_return)
      override;

 private:
  BluetoothChannelSoundingHandler bluetooth_channel_sounding_handler_;
};

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
