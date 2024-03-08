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

#include <aidl/android/hardware/bluetooth/ranging/BnBluetoothChannelSoundingSession.h>
#include <aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h>

#include <vector>

namespace aidl::android::hardware::bluetooth::ranging::impl {

using ::aidl::android::hardware::bluetooth::ranging::ChannelSoudingRawData;
using ::aidl::android::hardware::bluetooth::ranging::Reason;
using ::aidl::android::hardware::bluetooth::ranging::ResultType;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;

class BluetoothChannelSoundingSession
    : public BnBluetoothChannelSoundingSession {
 public:
  BluetoothChannelSoundingSession(
      std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback,
      Reason reason);

  ndk::ScopedAStatus getVendorSpecificReplies(
      std::optional<std::vector<std::optional<VendorSpecificData>>>*
          _aidl_return) override;
  ndk::ScopedAStatus getSupportedResultTypes(
      std::vector<ResultType>* _aidl_return) override;
  ndk::ScopedAStatus isAbortedProcedureRequired(bool* _aidl_return) override;
  ndk::ScopedAStatus writeRawData(
      const ChannelSoudingRawData& in_rawData) override;
  ndk::ScopedAStatus close(Reason in_reason) override;

 private:
  std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback_;
};

}  // namespace aidl::android::hardware::bluetooth::ranging::impl
