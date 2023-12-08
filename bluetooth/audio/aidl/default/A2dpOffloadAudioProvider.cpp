/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "BTAudioProviderA2dpHW"

#include "A2dpOffloadAudioProvider.h"

#include <BluetoothAudioCodecs.h>
#include <BluetoothAudioSessionReport.h>
#include <android-base/logging.h>

#include "A2dpOffloadCodecAac.h"
#include "A2dpOffloadCodecFactory.h"
#include "A2dpOffloadCodecSbc.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

A2dpOffloadEncodingAudioProvider::A2dpOffloadEncodingAudioProvider()
    : A2dpOffloadAudioProvider() {
  session_type_ = SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

A2dpOffloadDecodingAudioProvider::A2dpOffloadDecodingAudioProvider()
    : A2dpOffloadAudioProvider() {
  session_type_ = SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH;
}

A2dpOffloadAudioProvider::A2dpOffloadAudioProvider() {}

bool A2dpOffloadAudioProvider::isValid(const SessionType& session_type) {
  return (session_type == session_type_);
}

ndk::ScopedAStatus A2dpOffloadAudioProvider::startSession(
    const std::shared_ptr<IBluetoothAudioPort>& host_if,
    const AudioConfiguration& audio_config,
    const std::vector<LatencyMode>& latency_modes, DataMQDesc* _aidl_return) {
  if (audio_config.getTag() == AudioConfiguration::Tag::a2dp) {
    auto a2dp_config = audio_config.get<AudioConfiguration::Tag::a2dp>();
    A2dpStatus a2dp_status = A2dpStatus::NOT_SUPPORTED_CODEC_TYPE;

    if (a2dp_config.codecId ==
        A2dpOffloadCodecSbc::GetInstance()->GetCodecId()) {
      SbcParameters sbc_parameters;
      a2dp_status = A2dpOffloadCodecSbc::GetInstance()->ParseConfiguration(
          a2dp_config.configuration, &sbc_parameters);

    } else if (a2dp_config.codecId ==
               A2dpOffloadCodecAac::GetInstance()->GetCodecId()) {
      AacParameters aac_parameters;
      a2dp_status = A2dpOffloadCodecAac::GetInstance()->ParseConfiguration(
          a2dp_config.configuration, &aac_parameters);
    }
    if (a2dp_status != A2dpStatus::OK) {
      LOG(WARNING) << __func__ << " - Invalid Audio Configuration="
                   << audio_config.toString();
      *_aidl_return = DataMQDesc();
      return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
  } else if (audio_config.getTag() == AudioConfiguration::Tag::a2dpConfig) {
    if (!BluetoothAudioCodecs::IsOffloadCodecConfigurationValid(
            session_type_,
            audio_config.get<AudioConfiguration::a2dpConfig>())) {
      LOG(WARNING) << __func__ << " - Invalid Audio Configuration="
                   << audio_config.toString();
      *_aidl_return = DataMQDesc();
      return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
  } else {
    LOG(WARNING) << __func__ << " - Invalid Audio Configuration="
                 << audio_config.toString();
    *_aidl_return = DataMQDesc();
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  return BluetoothAudioProvider::startSession(
      host_if, audio_config, latency_modes, _aidl_return);
}

ndk::ScopedAStatus A2dpOffloadAudioProvider::onSessionReady(
    DataMQDesc* _aidl_return) {
  *_aidl_return = DataMQDesc();
  BluetoothAudioSessionReport::OnSessionStarted(
      session_type_, stack_iface_, nullptr, *audio_config_, latency_modes_);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus A2dpOffloadAudioProvider::parseA2dpConfiguration(
    const CodecId& codec_id, const std::vector<uint8_t>& configuration,
    CodecParameters* codec_parameters, A2dpStatus* _aidl_return) {
  auto codec = A2dpOffloadCodecFactory::GetInstance()->GetCodec(codec_id);
  if (!codec) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " - CodecId=" << codec_id.toString() << " is not found";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  *_aidl_return = codec->ParseConfiguration(configuration, codec_parameters);

  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus A2dpOffloadAudioProvider::getA2dpConfiguration(
    const std::vector<A2dpRemoteCapabilities>& remote_a2dp_capabilities,
    const A2dpConfigurationHint& hint,
    std::optional<audio::A2dpConfiguration>* _aidl_return) {
  *_aidl_return = std::nullopt;
  A2dpConfiguration avdtp_configuration;

  if (A2dpOffloadCodecFactory::GetInstance()->GetConfiguration(
          remote_a2dp_capabilities, hint, &avdtp_configuration))
    *_aidl_return =
        std::make_optional<A2dpConfiguration>(std::move(avdtp_configuration));

  return ndk::ScopedAStatus::ok();
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
