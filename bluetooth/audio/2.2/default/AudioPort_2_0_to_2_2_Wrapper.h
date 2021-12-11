/*
 * Copyright 2021 The Android Open Source Project
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

#include <android/hardware/bluetooth/audio/2.2/types.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_2 {
namespace implementation {

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::audio::common::V5_0::SinkMetadata;
using ::android::hardware::audio::common::V5_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_2::IBluetoothAudioPort;

class AudioPort_2_0_to_2_2_Wrapper : public V2_2::IBluetoothAudioPort {
 public:
  AudioPort_2_0_to_2_2_Wrapper(const sp<V2_0::IBluetoothAudioPort>& port) {
    this->port = port;
  }

  Return<void> startStream() override { return port->startStream(); }
  Return<void> suspendStream() override { return port->suspendStream(); }
  Return<void> stopStream() override { return port->stopStream(); }
  Return<void> getPresentationPosition(
      getPresentationPosition_cb _hidl_cb) override {
    return port->getPresentationPosition(_hidl_cb);
  }
  Return<void> updateMetadata(const SourceMetadata& sourceMetadata) override {
    return port->updateMetadata(sourceMetadata);
  }
  Return<void> updateSinkMetadata(const SinkMetadata&) override {
    // DO NOTHING, 2.0 AudioPort doesn't support sink metadata updates
    return Void();
  }

  sp<V2_0::IBluetoothAudioPort> port;
};

}  // namespace implementation
}  // namespace V2_2
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android