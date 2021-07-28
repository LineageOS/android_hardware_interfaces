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

#define LOG_TAG "BTAudioProviderA2dpSoftware"

#include "A2dpSoftwareAudioProvider.h"

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
using ::android::hardware::Void;
using ::android::hardware::bluetooth::audio::V2_0::AudioConfiguration;

// Here the buffer size is based on SBC
static constexpr uint32_t kPcmFrameSize = 4;  // 16 bits per sample / stereo
// SBC is 128, and here we choose the LCM of 16, 24, and 32
static constexpr uint32_t kPcmFrameCount = 96;
static constexpr uint32_t kRtpFrameSize = kPcmFrameSize * kPcmFrameCount;
// The max counts by 1 tick (20ms) for SBC is about 7. Since using 96 for the
// PCM counts, here we just choose a greater number
static constexpr uint32_t kRtpFrameCount = 10;
static constexpr uint32_t kBufferSize = kRtpFrameSize * kRtpFrameCount;
static constexpr uint32_t kBufferCount = 2;  // double buffer
static constexpr uint32_t kDataMqSize = kBufferSize * kBufferCount;

A2dpSoftwareAudioProvider::A2dpSoftwareAudioProvider()
    : BluetoothAudioProvider(), mDataMQ(nullptr) {
  LOG(INFO) << __func__ << " - size of audio buffer " << kDataMqSize
            << " byte(s)";
  std::unique_ptr<DataMQ> tempDataMQ(
      new DataMQ(kDataMqSize, /* EventFlag */ true));
  if (tempDataMQ && tempDataMQ->isValid()) {
    mDataMQ = std::move(tempDataMQ);
    session_type_ = SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH;
  } else {
    ALOGE_IF(!tempDataMQ, "failed to allocate data MQ");
    ALOGE_IF(tempDataMQ && !tempDataMQ->isValid(), "data MQ is invalid");
  }
}

bool A2dpSoftwareAudioProvider::isValid(const V2_0::SessionType& sessionType) {
  return isValid(static_cast<SessionType>(sessionType));
}

bool A2dpSoftwareAudioProvider::isValid(const SessionType& sessionType) {
  return (sessionType == session_type_ && mDataMQ && mDataMQ->isValid());
}

Return<void> A2dpSoftwareAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const V2_0::AudioConfiguration& audioConfig, startSession_cb _hidl_cb) {
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
  } else if (!android::bluetooth::audio::IsSoftwarePcmConfigurationValid(
                 audioConfig.pcmConfig())) {
    LOG(WARNING) << __func__ << " - Unsupported PCM Configuration="
                 << toString(audioConfig.pcmConfig());
    _hidl_cb(BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION,
             DataMQ::Descriptor());
    return Void();
  }

  return BluetoothAudioProvider::startSession(hostIf, audioConfig, _hidl_cb);
}

Return<void> A2dpSoftwareAudioProvider::onSessionReady(
    startSession_cb _hidl_cb) {
  if (mDataMQ && mDataMQ->isValid()) {
    BluetoothAudioSessionReport_2_1::OnSessionStarted(
        session_type_, stack_iface_, mDataMQ->getDesc(), audio_config_);
    _hidl_cb(BluetoothAudioStatus::SUCCESS, *mDataMQ->getDesc());
  } else {
    _hidl_cb(BluetoothAudioStatus::FAILURE, DataMQ::Descriptor());
  }
  return Void();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
