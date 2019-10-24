/*
 * Copyright 2020 The Android Open Source Project
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

#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

#include "BluetoothAudioProvider.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_1 {
namespace implementation {

using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

class HearingAidAudioProvider : public BluetoothAudioProvider {
 public:
  HearingAidAudioProvider();

  bool isValid(const SessionType& sessionType) override;
  bool isValid(const V2_0::SessionType& sessionType) override;

  Return<void> startSession(const sp<IBluetoothAudioPort>& hostIf,
                            const V2_0::AudioConfiguration& audioConfig,
                            startSession_cb _hidl_cb) override;

 private:
  // audio data queue for software encoding
  std::unique_ptr<DataMQ> mDataMQ;

  Return<void> onSessionReady(startSession_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
