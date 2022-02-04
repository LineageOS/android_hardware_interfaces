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

#include <android/hardware/bluetooth/audio/2.2/IBluetoothAudioProvider.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_2 {
namespace implementation {

using ::android::sp;
using ::android::hardware::bluetooth::audio::V2_2::IBluetoothAudioPort;

using BluetoothAudioStatus =
    ::android::hardware::bluetooth::audio::V2_0::Status;

class BluetoothAudioDeathRecipient;

class BluetoothAudioProvider : public IBluetoothAudioProvider {
 public:
  BluetoothAudioProvider();
  ~BluetoothAudioProvider() = default;

  virtual bool isValid(const V2_1::SessionType& sessionType) = 0;
  virtual bool isValid(const V2_0::SessionType& sessionType) = 0;

  Return<void> startSession(const sp<V2_0::IBluetoothAudioPort>& hostIf,
                            const V2_0::AudioConfiguration& audioConfig,
                            startSession_cb _hidl_cb) override;
  Return<void> startSession_2_1(const sp<V2_0::IBluetoothAudioPort>& hostIf,
                                const V2_1::AudioConfiguration& audioConfig,
                                startSession_cb _hidl_cb) override;
  Return<void> startSession_2_2(const sp<V2_2::IBluetoothAudioPort>& hostIf,
                                const AudioConfiguration& audioConfig,
                                startSession_cb _hidl_cb) override;
  Return<void> streamStarted(BluetoothAudioStatus status) override;
  Return<void> streamSuspended(BluetoothAudioStatus status) override;
  Return<void> endSession() override;
  Return<void> updateAudioConfiguration(
      const AudioConfiguration& audioConfig) override;

  Return<void> setLowLatencyModeAllowed(bool allowed) override;

 protected:
  sp<BluetoothAudioDeathRecipient> death_recipient_;

  V2_1::SessionType session_type_;
  AudioConfiguration audio_config_;
  sp<V2_2::IBluetoothAudioPort> stack_iface_;

  virtual Return<void> onSessionReady(startSession_cb _hidl_cb) = 0;
};

class BluetoothAudioDeathRecipient : public hidl_death_recipient {
 public:
  BluetoothAudioDeathRecipient(const sp<BluetoothAudioProvider> provider)
      : provider_(provider) {}

  virtual void serviceDied(
      uint64_t cookie,
      const wp<::android::hidl::base::V1_0::IBase>& who) override;

 private:
  sp<BluetoothAudioProvider> provider_;
};

}  // namespace implementation
}  // namespace V2_2
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
