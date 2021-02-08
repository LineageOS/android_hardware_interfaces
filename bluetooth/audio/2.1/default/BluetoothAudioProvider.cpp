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

#define LOG_TAG "BTAudioProviderStub"

#include "BluetoothAudioProvider.h"

#include <android-base/logging.h>

#include "BluetoothAudioSessionReport_2_1.h"
#include "BluetoothAudioSupportedCodecsDB_2_1.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_1 {
namespace implementation {

using ::android::bluetooth::audio::BluetoothAudioSessionReport_2_1;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::Void;

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

void BluetoothAudioDeathRecipient::serviceDied(
    uint64_t cookie __unused,
    const wp<::android::hidl::base::V1_0::IBase>& who __unused) {
  LOG(ERROR) << "BluetoothAudioDeathRecipient::" << __func__
             << " - BluetoothAudio Service died";
  provider_->endSession();
}

BluetoothAudioProvider::BluetoothAudioProvider()
    : death_recipient_(new BluetoothAudioDeathRecipient(this)),
      session_type_(SessionType::UNKNOWN),
      audio_config_({}) {}

Return<void> BluetoothAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const V2_0::AudioConfiguration& audioConfig, startSession_cb _hidl_cb) {
  AudioConfiguration audioConfig_2_1;

  if (audioConfig.getDiscriminator() ==
      V2_0::AudioConfiguration::hidl_discriminator::pcmConfig) {
    audioConfig_2_1.pcmConfig({
        .sampleRate =
            static_cast<SampleRate>(audioConfig.pcmConfig().sampleRate),
        .channelMode = audioConfig.pcmConfig().channelMode,
        .bitsPerSample = audioConfig.pcmConfig().bitsPerSample,
        .dataIntervalUs = 0});
  } else {
    audioConfig_2_1.codecConfig(audioConfig.codecConfig());
  }

  return startSession_2_1(hostIf, audioConfig_2_1, _hidl_cb);
}

Return<void> BluetoothAudioProvider::startSession_2_1(
    const sp<IBluetoothAudioPort>& hostIf,
    const AudioConfiguration& audioConfig, startSession_cb _hidl_cb) {
  if (hostIf == nullptr) {
    _hidl_cb(BluetoothAudioStatus::FAILURE, DataMQ::Descriptor());
    return Void();
  }

  /**
   * Initialize the audio platform if audioConfiguration is supported.
   * Save the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  audio_config_ = audioConfig;
  stack_iface_ = hostIf;
  stack_iface_->linkToDeath(death_recipient_, 0);

  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", AudioConfiguration=[" << toString(audio_config_) << "]";

  onSessionReady(_hidl_cb);
  return Void();
}

Return<void> BluetoothAudioProvider::streamStarted(
    BluetoothAudioStatus status) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", status=" << toString(status);

  /**
   * Streaming on control path has started,
   * HAL server should start the streaming on data path.
   */
  if (stack_iface_) {
    BluetoothAudioSessionReport_2_1::ReportControlStatus(session_type_, true,
                                                         status);
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", status=" << toString(status) << " has NO session";
  }

  return Void();
}

Return<void> BluetoothAudioProvider::streamSuspended(
    BluetoothAudioStatus status) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", status=" << toString(status);

  /**
   * Streaming on control path has suspend,
   * HAL server should suspend the streaming on data path.
   */
  if (stack_iface_) {
    BluetoothAudioSessionReport_2_1::ReportControlStatus(session_type_, false,
                                                         status);
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", status=" << toString(status) << " has NO session";
  }

  return Void();
}

Return<void> BluetoothAudioProvider::endSession() {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_);

  if (stack_iface_) {
    BluetoothAudioSessionReport_2_1::OnSessionEnded(session_type_);
    stack_iface_->unlinkToDeath(death_recipient_);
  } else {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO session";
  }

  /**
   * Clean up the audio platform as remote audio device is no
   * longer active
   */
  stack_iface_ = nullptr;
  audio_config_ = {};

  return Void();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
