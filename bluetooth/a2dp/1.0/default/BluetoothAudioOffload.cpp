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

#include "BluetoothAudioOffload.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace a2dp {
namespace V1_0 {
namespace implementation {

IBluetoothAudioOffload* HIDL_FETCH_IBluetoothAudioOffload(
    const char* /* name */) {
  return new BluetoothAudioOffload();
}

// Methods from
// ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload follow.
Return<::android::hardware::bluetooth::a2dp::V1_0::Status>
BluetoothAudioOffload::startSession(
    const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>&
        hostIf __unused,
    const ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration&
        codecConfig __unused) {
  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioHost interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  return ::android::hardware::bluetooth::a2dp::V1_0::Status::FAILURE;
}

Return<void> BluetoothAudioOffload::streamStarted(
    ::android::hardware::bluetooth::a2dp::V1_0::Status status __unused) {
  /**
   * Streaming on control path has started,
   * HAL server should start the streaming on data path.
   */
  return Void();
}

Return<void> BluetoothAudioOffload::streamSuspended(
    ::android::hardware::bluetooth::a2dp::V1_0::Status status __unused) {
  /**
   * Streaming on control path has suspend,
   * HAL server should suspend the streaming on data path.
   */
  return Void();
}

Return<void> BluetoothAudioOffload::endSession() {
  /**
   * Cleanup the audio platform as remote A2DP Sink device is no
   * longer active
   */
  return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace a2dp
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
