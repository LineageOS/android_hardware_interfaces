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

#define LOG_TAG "BTAudioProviderLeAudio"

#include "LeAudioAudioProvider.h"

#include <android-base/logging.h>

#include "AudioPort_2_0_to_2_2_Wrapper.h"
#include "BluetoothAudioSessionReport_2_2.h"
#include "BluetoothAudioSupportedCodecsDB_2_2.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_2 {
namespace implementation {

using ::android::bluetooth::audio::BluetoothAudioSessionReport_2_2;
using ::android::hardware::Void;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_1::SampleRate;

static constexpr uint32_t kBufferOutCount = 2;  // two frame buffer
static constexpr uint32_t kBufferInCount = 2;   // two frame buffer

LeAudioOutputAudioProvider::LeAudioOutputAudioProvider()
    : LeAudioAudioProvider() {
  session_type_ = V2_1::SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH;
}

LeAudioInputAudioProvider::LeAudioInputAudioProvider()
    : LeAudioAudioProvider() {
  session_type_ = V2_1::SessionType::LE_AUDIO_SOFTWARE_DECODED_DATAPATH;
}

LeAudioAudioProvider::LeAudioAudioProvider()
    : BluetoothAudioProvider(), mDataMQ(nullptr) {}

bool LeAudioAudioProvider::isValid(const V2_0::SessionType& sessionType) {
  LOG(ERROR) << __func__ << ", invalid session type for Le Audio provider: "
             << toString(sessionType);

  return false;
}

bool LeAudioAudioProvider::isValid(const V2_1::SessionType& sessionType) {
  return (sessionType == session_type_);
}

Return<void> LeAudioAudioProvider::startSession_2_1(
    const sp<V2_0::IBluetoothAudioPort>& hostIf,
    const V2_1::AudioConfiguration& audioConfig, startSession_cb _hidl_cb) {
  if (audioConfig.getDiscriminator() !=
      V2_1::AudioConfiguration::hidl_discriminator::pcmConfig) {
    LOG(WARNING) << __func__
                 << " - Invalid Audio Configuration=" << toString(audioConfig);
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  }

  AudioConfiguration audioConfig_2_2;
  audioConfig_2_2.pcmConfig(
      {.sampleRate =
           static_cast<V2_1::SampleRate>(audioConfig.pcmConfig().sampleRate),
       .channelMode = audioConfig.pcmConfig().channelMode,
       .bitsPerSample = audioConfig.pcmConfig().bitsPerSample,
       .dataIntervalUs = 0});

  sp<V2_2::IBluetoothAudioPort> hostIf_2_2 =
      new AudioPort_2_0_to_2_2_Wrapper(hostIf);
  return startSession_2_2(hostIf_2_2, audioConfig_2_2, _hidl_cb);
}

Return<void> LeAudioAudioProvider::startSession_2_2(
    const sp<V2_2::IBluetoothAudioPort>& hostIf,
    const AudioConfiguration& audioConfig, startSession_cb _hidl_cb) {
  /**
   * Initialize the audio platform if audioConfiguration is supported.
   * Save the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */
  if (audioConfig.getDiscriminator() !=
      AudioConfiguration::hidl_discriminator::pcmConfig) {
    LOG(WARNING) << __func__
                 << " - Invalid Audio Configuration=" << toString(audioConfig);
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  } else if (!android::bluetooth::audio::IsSoftwarePcmConfigurationValid_2_1(
                 audioConfig.pcmConfig())) {
    LOG(WARNING) << __func__ << " - Unsupported PCM Configuration="
                 << toString(audioConfig.pcmConfig());
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  }

  uint32_t kDataMqSize = 0;
  switch (audioConfig.pcmConfig().sampleRate) {
    case SampleRate::RATE_8000:
      kDataMqSize = 8000;
      break;
    case SampleRate::RATE_16000:
      kDataMqSize = 16000;
      break;
    case SampleRate::RATE_24000:
      kDataMqSize = 24000;
      break;
    case SampleRate::RATE_32000:
      kDataMqSize = 32000;
      break;
    case SampleRate::RATE_44100:
      kDataMqSize = 44100;
      break;
    case SampleRate::RATE_48000:
      kDataMqSize = 48000;
      break;
    default:
      LOG(WARNING) << __func__ << " - Unsupported sampling frequency="
                   << toString(audioConfig.pcmConfig());
      _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
               DataMQ::Descriptor());
      return Void();
  }

  /* Number of samples per millisecond */
  kDataMqSize = ceil(kDataMqSize / 1000);

  switch (audioConfig.pcmConfig().channelMode) {
    case ChannelMode::MONO:
      break;
    case ChannelMode::STEREO:
      kDataMqSize *= 2;
      break;
    default:
      /* This should never happen it would be caught while validating
       * parameters.
       */
      break;
  }

  switch (audioConfig.pcmConfig().bitsPerSample) {
    case BitsPerSample::BITS_16:
      kDataMqSize *= 2;
      break;
    case BitsPerSample::BITS_24:
      kDataMqSize *= 3;
      break;
    case BitsPerSample::BITS_32:
      kDataMqSize *= 4;
      break;
    default:
      /* This should never happen it would be caught while validating
       * parameters.
       */
      break;
  }

  if (session_type_ == V2_1::SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH)
    kDataMqSize *= kBufferOutCount;
  else if (session_type_ ==
           V2_1::SessionType::LE_AUDIO_SOFTWARE_DECODED_DATAPATH)
    kDataMqSize *= kBufferInCount;
  else
    LOG(WARNING) << __func__ << ", default single buffer used";

  kDataMqSize *= audioConfig.pcmConfig().dataIntervalUs / 1000;

  LOG(INFO) << __func__ << " - size of audio buffer " << kDataMqSize
            << " byte(s)";

  std::unique_ptr<DataMQ> tempDataMQ(
      new DataMQ(kDataMqSize, /* EventFlag */ true));
  if (tempDataMQ && tempDataMQ->isValid()) {
    mDataMQ = std::move(tempDataMQ);
  } else {
    ALOGE_IF(!tempDataMQ, "failed to allocate data MQ");
    ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(), "data MQ is invalid");
    _hidl_cb(BluetoothAudioStatus::FAILURE, DataMQ::Descriptor());
    return Void();
  }

  return BluetoothAudioProvider::startSession_2_2(hostIf, audioConfig,
                                                  _hidl_cb);
}

Return<void> LeAudioAudioProvider::onSessionReady(startSession_cb _hidl_cb) {
  if (mDataMQ && mDataMQ->isValid()) {
    BluetoothAudioSessionReport_2_2::OnSessionStarted(
        session_type_, stack_iface_, mDataMQ->getDesc(), audio_config_);
    _hidl_cb(BluetoothAudioStatus::SUCCESS, *mDataMQ->getDesc());
  } else {
    _hidl_cb(BluetoothAudioStatus::FAILURE, DataMQ::Descriptor());
  }
  return Void();
}

}  // namespace implementation
}  // namespace V2_2
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
