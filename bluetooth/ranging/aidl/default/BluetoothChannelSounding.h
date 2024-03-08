/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/android/hardware/bluetooth/ranging/BnBluetoothChannelSounding.h>

#include <vector>

namespace aidl::android::hardware::bluetooth::ranging::impl {

using ::aidl::android::hardware::bluetooth::ranging::
    BluetoothChannelSoundingParameters;
using ::aidl::android::hardware::bluetooth::ranging::BnBluetoothChannelSounding;
using ::aidl::android::hardware::bluetooth::ranging::CsSecurityLevel;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSession;
using ::aidl::android::hardware::bluetooth::ranging::
    IBluetoothChannelSoundingSessionCallback;
using ::aidl::android::hardware::bluetooth::ranging::SessionType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;

class BluetoothChannelSounding : public BnBluetoothChannelSounding {
 public:
  BluetoothChannelSounding();
  ~BluetoothChannelSounding();  // Add the destructor declaration
  ndk::ScopedAStatus getVendorSpecificData(
      std::optional<std::vector<std::optional<VendorSpecificData>>>*
          _aidl_return) override;
  ndk::ScopedAStatus getSupportedSessionTypes(
      std::optional<std::vector<SessionType>>* _aidl_return) override;
  ndk::ScopedAStatus getMaxSupportedCsSecurityLevel(
      CsSecurityLevel* _aidl_return) override;
  ndk::ScopedAStatus openSession(
      const BluetoothChannelSoundingParameters& in_params,
      const std::shared_ptr<IBluetoothChannelSoundingSessionCallback>&
          in_callback,
      std::shared_ptr<IBluetoothChannelSoundingSession>* _aidl_return) override;
};

}  // namespace aidl::android::hardware::bluetooth::ranging::impl
