/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_BLUETOOTH_A2DP_V1_0_BLUETOOTHAUDIOOFFLOAD_H
#define ANDROID_HARDWARE_BLUETOOTH_A2DP_V1_0_BLUETOOTHAUDIOOFFLOAD_H

#include <android/hardware/bluetooth/a2dp/1.0/IBluetoothAudioOffload.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace a2dp {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct BluetoothAudioOffload : public IBluetoothAudioOffload {
  BluetoothAudioOffload() {}
  // Methods from
  // ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload follow.
  Return<::android::hardware::bluetooth::a2dp::V1_0::Status> startSession(
      const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>&
          hostIf,
      const ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration&
          codecConfig) override;
  Return<void> streamStarted(
      ::android::hardware::bluetooth::a2dp::V1_0::Status status) override;
  Return<void> streamSuspended(
      ::android::hardware::bluetooth::a2dp::V1_0::Status status) override;
  Return<void> endSession() override;
};

extern "C" IBluetoothAudioOffload* HIDL_FETCH_IBluetoothAudioOffload(
    const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace a2dp
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BLUETOOTH_A2DP_V1_0_BLUETOOTHAUDIOOFFLOAD_H
