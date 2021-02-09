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

#define LOG_TAG "BTAudioProviderA2dpOffload"

#include "A2dpOffloadAudioProvider.h"

#include <android-base/logging.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>

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
using ::android::hardware::bluetooth::audio::V2_0::AudioConfiguration;

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

A2dpOffloadAudioProvider::A2dpOffloadAudioProvider()
    : BluetoothAudioProvider() {
  session_type_ = SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH;
}

bool A2dpOffloadAudioProvider::isValid(const V2_0::SessionType& sessionType) {
  return isValid(static_cast<SessionType>(sessionType));
}

bool A2dpOffloadAudioProvider::isValid(const SessionType& sessionType) {
  return (sessionType == session_type_);
}

Return<void> A2dpOffloadAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const AudioConfiguration& audioConfig, startSession_cb _hidl_cb) {
  /**
   * Initialize the audio platform if audioConfiguration is supported.
   * Save the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  if (audioConfig.getDiscriminator() !=
      AudioConfiguration::hidl_discriminator::codecConfig) {
    LOG(WARNING) << __func__
                 << " - Invalid Audio Configuration=" << toString(audioConfig);
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  } else if (!android::bluetooth::audio::IsOffloadCodecConfigurationValid(
                 session_type_, audioConfig.codecConfig())) {
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  }

  return BluetoothAudioProvider::startSession(hostIf, audioConfig, _hidl_cb);
}

Return<void> A2dpOffloadAudioProvider::onSessionReady(
    startSession_cb _hidl_cb) {
  BluetoothAudioSessionReport_2_1::OnSessionStarted(session_type_, stack_iface_,
                                                    nullptr, audio_config_);
  _hidl_cb(BluetoothAudioStatus::SUCCESS, DataMQ::Descriptor());
  return Void();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
