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

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding.h"

#include <memory>
#include <optional>
#include <vector>

#include "Eigen/Dense"
#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/CsSecurityLevel.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSession.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/SessionType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android-base/logging.h"
#include "android-base/properties.h"
#include "android/binder_auto_utils.h"
#include "android/binder_interface_utils.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

using ::aidl::android::hardware::bluetooth::ranging::
    BluetoothChannelSoundingParameters;
using ::aidl::android::hardware::bluetooth::ranging::CsSecurityLevel;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSession;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSessionCallback;
using ::aidl::android::hardware::bluetooth::ranging::SessionType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;

using ::android::base::GetProperty;
using ::bluetooth_hal::Property;

using ::ndk::ScopedAStatus;
using ::ndk::SharedRefBase;

ScopedAStatus BluetoothChannelSounding::getVendorSpecificData(
    std::optional<std::vector<std::optional<VendorSpecificData>>>*
        _aidl_return) {
  bool status =
      bluetooth_channel_sounding_handler_.GetVendorSpecificData(_aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothChannelSounding::getSupportedSessionTypes(
    std::optional<std::vector<SessionType>>* _aidl_return) {
  std::vector<SessionType> supported_session_types = {
      SessionType::SOFTWARE_STACK_DATA_PARSING};
  bool status = bluetooth_channel_sounding_handler_.GetSupportedSessionTypes(
      _aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothChannelSounding::getMaxSupportedCsSecurityLevel(
    CsSecurityLevel* _aidl_return) {
  bool status =
      bluetooth_channel_sounding_handler_.GetMaxSupportedCsSecurityLevel(
          _aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothChannelSounding::openSession(
    const BluetoothChannelSoundingParameters& in_params,
    const std::shared_ptr<IBluetoothChannelSoundingSessionCallback>&
        in_callback,
    std::shared_ptr<IBluetoothChannelSoundingSession>* _aidl_return) {
  LOG(INFO) << __func__;

  if (in_callback.get() == nullptr) {
    return ScopedAStatus::fromExceptionCodeWithMessage(
        EX_ILLEGAL_ARGUMENT, "Invalid nullptr callback");
  }

  bool status = bluetooth_channel_sounding_handler_.OpenSession(
      in_params, in_callback, _aidl_return);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
