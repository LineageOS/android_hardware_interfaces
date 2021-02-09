/*
 * Copyright 2018 The Android Open Source Project
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

#define LOG_TAG "BTAudioProviderSession_2_1"

#include "BluetoothAudioSession_2_1.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>

namespace android {
namespace bluetooth {
namespace audio {
using SessionType_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::SessionType;
using SessionType_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::SessionType;

namespace {
bool is_2_0_session_type(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type) {
  if (session_type == SessionType_2_1::A2DP_SOFTWARE_ENCODING_DATAPATH ||
      session_type == SessionType_2_1::A2DP_HARDWARE_OFFLOAD_DATAPATH ||
      session_type == SessionType_2_1::HEARING_AID_SOFTWARE_ENCODING_DATAPATH) {
    return true;
  } else {
    return false;
  }
}
}  // namespace

BluetoothAudioSession_2_1::BluetoothAudioSession_2_1(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type)
    : audio_session(BluetoothAudioSessionInstance::GetSessionInstance(
          static_cast<SessionType_2_0>(session_type))) {
  if (is_2_0_session_type(session_type)) {
    session_type_2_1_ = (SessionType_2_1::UNKNOWN);
  } else {
    session_type_2_1_ = (session_type);
  }
}

std::shared_ptr<BluetoothAudioSession>
BluetoothAudioSession_2_1::GetAudioSession() {
  return audio_session;
}

// The report function is used to report that the Bluetooth stack has started
// this session without any failure, and will invoke session_changed_cb_ to
// notify those registered bluetooth_audio outputs
void BluetoothAudioSession_2_1::OnSessionStarted(
    const sp<IBluetoothAudioPort> stack_iface, const DataMQ::Descriptor* dataMQ,
    const ::android::hardware::bluetooth::audio::V2_1::AudioConfiguration&
        audio_config) {
  if (session_type_2_1_ == SessionType_2_1::UNKNOWN) {
    ::android::hardware::bluetooth::audio::V2_0::AudioConfiguration config;
    if (audio_config.getDiscriminator() ==
        ::android::hardware::bluetooth::audio::V2_1::AudioConfiguration::
            hidl_discriminator::codecConfig) {
      config.codecConfig(audio_config.codecConfig());
    } else {
      auto& tmpPcm = audio_config.pcmConfig();
      config.pcmConfig(
          ::android::hardware::bluetooth::audio::V2_0::PcmParameters{
              .sampleRate = static_cast<SampleRate>(tmpPcm.sampleRate),
              .channelMode = tmpPcm.channelMode,
              .bitsPerSample = tmpPcm.bitsPerSample
              /*dataIntervalUs is not passed to 2.0 */
          });
    }

    audio_session->OnSessionStarted(stack_iface, dataMQ, config);
  } else {
    LOG(FATAL) << " Not implemented yet!!";
  }
}

std::unique_ptr<BluetoothAudioSessionInstance_2_1>
    BluetoothAudioSessionInstance_2_1::instance_ptr =
        std::unique_ptr<BluetoothAudioSessionInstance_2_1>(
            new BluetoothAudioSessionInstance_2_1());

// API to fetch the session of A2DP / Hearing Aid
std::shared_ptr<BluetoothAudioSession_2_1>
BluetoothAudioSessionInstance_2_1::GetSessionInstance(
    const SessionType_2_1& session_type) {
  std::lock_guard<std::mutex> guard(instance_ptr->mutex_);
  if (!instance_ptr->sessions_map_.empty()) {
    auto entry = instance_ptr->sessions_map_.find(session_type);
    if (entry != instance_ptr->sessions_map_.end()) {
      return entry->second;
    }
  }
  std::shared_ptr<BluetoothAudioSession_2_1> session_ptr =
      std::make_shared<BluetoothAudioSession_2_1>(session_type);
  instance_ptr->sessions_map_[session_type] = session_ptr;
  return session_ptr;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
