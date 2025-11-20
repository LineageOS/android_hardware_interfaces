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

#include <sys/types.h>

#include <cstdint>
#include <optional>

#include "aidl/android/hardware/bluetooth/audio/ChannelMode.h"
#include "aidl/android/hardware/bluetooth/audio/CodecId.h"
#include "aidl/android/hardware/bluetooth/audio/CodecSpecificConfigurationLtv.h"
#include "aidl/android/hardware/bluetooth/audio/CodecType.h"
#include "aidl/android/hardware/bluetooth/audio/OpusConfiguration.h"
#define LOG_TAG "BTAudioSessionAidl"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android/binder_manager.h>
#include <com_android_btaudio_hal_flags.h>
#include <hardware/audio.h>

#include "BluetoothAudioSession.h"
#include "BluetoothAudioSwOffload.h"
#include "BluetoothAudioType.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

static constexpr int kFmqSendTimeoutMs = 1000;  // 1000 ms timeout for sending
static constexpr int kFmqReceiveTimeoutMs =
    1000;                               // 1000 ms timeout for receiving
static constexpr int kWritePollMs = 1;  // polled non-blocking interval
static constexpr int kReadPollMs = 1;   // polled non-blocking interval

constexpr char kPropertyLeaSwOffload[] =
    "persist.vendor.audio.leaudio_sw_offload";

static std::string toString(const std::vector<LatencyMode>& latencies) {
  std::stringstream latencyModesStr;
  for (LatencyMode mode : latencies) {
    latencyModesStr << " " << toString(mode);
  }
  return latencyModesStr.str();
}

BluetoothAudioSession::BluetoothAudioSession(const SessionType& session_type)
    : session_type_(session_type), stack_iface_(nullptr), data_mq_(nullptr) {}

/***
 *
 * Callback methods
 *
 ***/

void BluetoothAudioSession::OnSessionStarted(
    const std::shared_ptr<IBluetoothAudioPort> stack_iface,
    const DataMQDesc* mq_desc, const AudioConfiguration& audio_config,
    const std::vector<LatencyMode>& latency_modes) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (stack_iface == nullptr) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << ", IBluetoothAudioPort Invalid";
  } else if (!UpdateAudioConfig(audio_config)) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << ", AudioConfiguration=" << audio_config.toString()
               << " Invalid";
  } else if (!UpdateDataPath(mq_desc)) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << " MqDescriptor Invalid";
    audio_config_ = nullptr;
  } else {
    stack_iface_ = stack_iface;
    latency_modes_ = latency_modes;
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " - All LatencyModes=" << toString(latency_modes)
              << ", AudioConfiguration=" << audio_config.toString();
    ReportSessionStatus();
  }
}

void BluetoothAudioSession::OnSessionEnded() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  bool toggled = IsSessionReadyInternal();
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_);
  audio_config_ = nullptr;
  stack_iface_ = nullptr;
  UpdateDataPath(nullptr);
  if (com::android::btaudio::hal::flags::leaudio_sw_offload() &&
      ::android::base::GetBoolProperty(kPropertyLeaSwOffload, false)) {
    if (session_type_ ==
        SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
      LeAudioSwOffloadInstance::releaseSwOffload();
    }
  }
  if (toggled) {
    ReportSessionStatus();
  }
}

/***
 *
 * Util methods
 *
 ***/

std::vector<CodecSpecificConfigurationLtv>
getCodecConfigFromVendorCodecConfiguration(
    std::vector<uint8_t>& vendor_codec_config) {
  std::vector<CodecSpecificConfigurationLtv> codec_config;
  int i = 0;
  while (i < vendor_codec_config.size()) {
    auto opcode = vendor_codec_config[i++];
    auto subopcode = vendor_codec_config[i++];
    if (opcode == kCodecConfigOpcode) {
      if (subopcode == kSamplingFrequencySubOpcode) {
        auto p =
            codec_cfg_map_to_sampling_rate_ltv.find(vendor_codec_config[i++]);
        if (p != codec_cfg_map_to_sampling_rate_ltv.end()) {
          codec_config.push_back(p->second);
        }
      } else if (subopcode == kFrameDurationSubOpcode) {
        auto p =
            codec_cfg_map_to_frame_duration_ltv.find(vendor_codec_config[i++]);
        if (p != codec_cfg_map_to_frame_duration_ltv.end()) {
          codec_config.push_back(p->second);
        }
      } else if (subopcode == kFrameBlocksPerSDUSubOpcode) {
        auto frame_block =
            CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU();
        frame_block.value = vendor_codec_config[i++];
        codec_config.push_back(frame_block);
      }
    } else if (opcode == kAudioChannelAllocationOpcode) {
      auto allocation = CodecSpecificConfigurationLtv::AudioChannelAllocation();
      for (int b = 0; b < 4; ++b) {
        allocation.bitmask |= (vendor_codec_config[i++] << (b * 8));
      }
      codec_config.push_back(allocation);
    } else if (opcode == kOctetsPerCodecFrameOpcode) {
      auto octet = CodecSpecificConfigurationLtv::OctetsPerCodecFrame();
      for (int b = 0; b < 2; ++b) {
        octet.value |= (vendor_codec_config[i++] << (b * 8));
      }
      codec_config.push_back(octet);
    }
  }
  return codec_config;
}

OpusConfiguration getOpusConfigFromCodecConfig(
    std::vector<CodecSpecificConfigurationLtv> codecConfiguration) {
  OpusConfiguration opus_config;
  opus_config.pcmBitDepth = 16;
  for (auto ltv : codecConfiguration) {
    if (ltv.getTag() == CodecSpecificConfigurationLtv::samplingFrequency) {
      auto p = sampling_rate_ltv_map.find(
          ltv.get<CodecSpecificConfigurationLtv::samplingFrequency>());
      if (p != sampling_rate_ltv_map.end()) {
        opus_config.samplingFrequencyHz = p->second;
      }
    } else if (ltv.getTag() == CodecSpecificConfigurationLtv::frameDuration) {
      auto p = frame_duration_ltv_map.find(
          ltv.get<CodecSpecificConfigurationLtv::frameDuration>());
      if (p != frame_duration_ltv_map.end()) {
        opus_config.frameDurationUs = p->second;
      }
    } else if (ltv.getTag() ==
               CodecSpecificConfigurationLtv::octetsPerCodecFrame) {
      auto octet =
          ltv.get<CodecSpecificConfigurationLtv::octetsPerCodecFrame>();
      opus_config.octetsPerFrame = octet.value;
    } else if (ltv.getTag() ==
               CodecSpecificConfigurationLtv::codecFrameBlocksPerSDU) {
      auto block =
          ltv.get<CodecSpecificConfigurationLtv::codecFrameBlocksPerSDU>();
      opus_config.blocksPerSdu = block.value;
    }
  }
  opus_config.channelMode = ChannelMode::STEREO;
  return opus_config;
}

std::optional<AudioConfiguration> convertToOpusAudioConfiguration(
    const AudioConfiguration& audio_config_,
    std::optional<bool>& is_stream_active) {
  if (audio_config_.getTag() == AudioConfiguration::leAudioConfig) {
    is_stream_active = false;
    auto le_audio_config =
        audio_config_.get<AudioConfiguration::leAudioConfig>();
    // Conversion from extension to Lc3Configuration
    LOG(DEBUG) << __func__ << ": leAudioConfig detected, len = "
               << le_audio_config.streamMap.size();
    for (auto info : le_audio_config.streamMap) {
      LOG(DEBUG) << __func__ << ": info is " << info.toString();
      if (info.streamHandle != 0) {
        is_stream_active = true;
      }
      if (info.aseConfiguration.has_value()) {
        auto ase_config = info.aseConfiguration.value();
        auto codec_id = ase_config.codecId;
        if (codec_id.has_value() &&
            codec_id.value().getTag() == CodecId::vendor) {
          auto cid = codec_id.value().get<CodecId::vendor>();
          if (cid == opus_codec &&
              ase_config.vendorCodecConfiguration.has_value()) {
            OpusConfiguration opus_config = getOpusConfigFromCodecConfig(
                getCodecConfigFromVendorCodecConfiguration(
                    ase_config.vendorCodecConfiguration.value()));
            LOG(DEBUG) << __func__ << ": converted and set to OPUS config: "
                       << opus_config.toString();
            if (com::android::btaudio::hal::flags::leaudio_sw_offload() &&
                ::android::base::GetBoolProperty(kPropertyLeaSwOffload,
                                                 false) &&
                opus_config.samplingFrequencyHz ==
                    kOpusHiresSamplingFrequency) {
              LOG(INFO) << __func__
                        << ": Detect premium audio, use software offload path.";

              if (info.streamHandle != 0) {
                swoff::AudioConfig audio_config_sw_off = {
                    .bitdepth = kOpusHiresBitPerSample,
                    .sample_rate = kOpusHiresSamplingFrequency,
                    .frame_duration_us = opus_config.frameDurationUs,
                    .codec_type = swoff::OPUS,
                    .codec_config.opus = {opus_config.octetsPerFrame,
                                          kOpusHiresVbr, kOpusHiresComplexity}};

                std::vector<swoff::IsoStream> iso_streams = {
                    {info.streamHandle,
                     static_cast<uint32_t>(info.audioChannelAllocation)}};

                LeAudioSwOffloadInstance::sw_offload_cbacks_ =
                    std::make_shared<LeAudioSwOffloadCallbacks>();
                LeAudioSwOffloadInstance::sw_offload_streams_ =
                    std::make_shared<swoff::LeAudioStream>(
                        iso_streams, audio_config_sw_off,
                        LeAudioSwOffloadInstance::sw_offload_cbacks_);
              } else {
                LOG(WARNING) << __func__
                             << ": ISO stream handle is 0, do not initialte "
                                "stream in software offload library.";
              }

              PcmConfiguration pcm_config{
                  .sampleRateHz = kOpusHiresSamplingFrequency,
                  .channelMode = ChannelMode::STEREO,
                  .bitsPerSample = kOpusHiresBitPerSample,
                  .dataIntervalUs = opus_config.frameDurationUs};

              LeAudioSwOffloadInstance::is_using_swoffload_ = true;
              return pcm_config;
            } else {
              LeAudioConfiguration audio_config;
              audio_config.leAudioCodecConfig = opus_config;
              audio_config.codecType = CodecType::OPUS;
              audio_config.streamMap = le_audio_config.streamMap;
              audio_config.peerDelayUs = le_audio_config.peerDelayUs;
              audio_config.vendorSpecificMetadata =
                  le_audio_config.vendorSpecificMetadata;
              return audio_config;
            }
          }
        }
      }
    }
  }
  return std::nullopt;
}

const AudioConfiguration BluetoothAudioSession::GetAudioConfig() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    switch (session_type_) {
      case SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH:
        return AudioConfiguration(CodecConfiguration{});
      case SessionType::HFP_HARDWARE_OFFLOAD_DATAPATH:
        return AudioConfiguration(HfpConfiguration{});
      case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH:
        return AudioConfiguration(LeAudioConfiguration{});
      case SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
        return AudioConfiguration(LeAudioBroadcastConfiguration{});
      default:
        return AudioConfiguration(PcmConfiguration{});
    }
  }
  return *audio_config_;
}

void BluetoothAudioSession::ReportAudioConfigChanged(
    const AudioConfiguration& audio_config) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (com::android::btaudio::hal::flags::leaudio_report_broadcast_ac_to_hal()) {
    if (session_type_ ==
            SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
        session_type_ ==
            SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
      if (audio_config.getTag() != AudioConfiguration::leAudioConfig) {
        LOG(ERROR) << __func__ << " invalid audio config type for SessionType ="
                   << toString(session_type_);
        return;
      }
    } else if (session_type_ ==
               SessionType::
                   LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
      if (audio_config.getTag() != AudioConfiguration::leAudioBroadcastConfig) {
        LOG(ERROR) << __func__ << " invalid audio config type for SessionType ="
                   << toString(session_type_);
        return;
      }
    } else if (session_type_ == SessionType::HFP_HARDWARE_OFFLOAD_DATAPATH) {
      if (audio_config.getTag() != AudioConfiguration::hfpConfig) {
        LOG(ERROR) << __func__ << " invalid audio config type for SessionType ="
                   << toString(session_type_);
        return;
      }
    } else if (session_type_ == SessionType::HFP_SOFTWARE_DECODING_DATAPATH ||
               session_type_ == SessionType::HFP_SOFTWARE_ENCODING_DATAPATH) {
      if (audio_config.getTag() != AudioConfiguration::pcmConfig) {
        LOG(ERROR) << __func__ << " invalid audio config type for SessionType ="
                   << toString(session_type_);
        return;
      }
    } else {
      LOG(ERROR) << __func__
                 << " invalid SessionType =" << toString(session_type_);
      return;
    }
  } else {
    if (session_type_ ==
            SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
        session_type_ ==
            SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
      if (audio_config.getTag() != AudioConfiguration::leAudioConfig) {
        LOG(ERROR) << __func__ << " invalid audio config type for SessionType ="
                   << toString(session_type_);
        return;
      }
    } else if (session_type_ == SessionType::HFP_HARDWARE_OFFLOAD_DATAPATH) {
      if (audio_config.getTag() != AudioConfiguration::hfpConfig) {
        LOG(ERROR) << __func__ << " invalid audio config type for SessionType ="
                   << toString(session_type_);
        return;
      }
    } else if (session_type_ == SessionType::HFP_SOFTWARE_DECODING_DATAPATH ||
               session_type_ == SessionType::HFP_SOFTWARE_ENCODING_DATAPATH) {
      if (audio_config.getTag() != AudioConfiguration::pcmConfig) {
        LOG(ERROR) << __func__ << " invalid audio config type for SessionType ="
                   << toString(session_type_);
        return;
      }
    } else {
      LOG(ERROR) << __func__
                 << " invalid SessionType =" << toString(session_type_);
      return;
    }
  }

  if (session_type_ ==
      SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    // reset swoffload when new config is coming with offload session
    // type
    LeAudioSwOffloadInstance::releaseSwOffload();
  }

  audio_config_ = std::make_unique<AudioConfiguration>(audio_config);

  std::optional<bool> is_stream_active;
  auto opus_audio_config =
      convertToOpusAudioConfiguration(audio_config, is_stream_active);

  if (opus_audio_config.has_value()) {
    audio_config_ =
        std::make_unique<AudioConfiguration>(opus_audio_config.value());
  }

  if (is_stream_active.has_value() && !is_stream_active.value()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO active stream.";
    return;
  }

  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<struct PortStatusCallbacks> cb = observer.second;
    LOG(INFO) << __func__ << " for SessionType=" << toString(session_type_)
              << ", bluetooth_audio=0x"
              << ::android::base::StringPrintf("%04x", cookie);
    if (cb->audio_configuration_changed_cb_ != nullptr) {
      cb->audio_configuration_changed_cb_(cookie);
    }
  }
}

bool BluetoothAudioSession::IsSessionReady(bool is_primary_hal) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);

  bool is_mq_valid =
      (session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
       session_type_ == SessionType::HFP_HARDWARE_OFFLOAD_DATAPATH ||
       (data_mq_ != nullptr && data_mq_->isValid()));

  if (com::android::btaudio::hal::flags::leaudio_sw_offload() &&
      ::android::base::GetBoolProperty(kPropertyLeaSwOffload, false)) {
    if (session_type_ ==
        SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
      if (!is_primary_hal) {
        is_mq_valid &= LeAudioSwOffloadInstance::is_using_swoffload_.load();
      }
    }
  }

  return stack_iface_ != nullptr && is_mq_valid && audio_config_ != nullptr;
}

bool BluetoothAudioSession::IsSessionReadyInternal() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);

  bool is_mq_valid =
      (session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
       session_type_ == SessionType::HFP_HARDWARE_OFFLOAD_DATAPATH ||
       (data_mq_ != nullptr && data_mq_->isValid()));
  return stack_iface_ != nullptr && is_mq_valid && audio_config_ != nullptr;
}

/***
 *
 * Status callback methods
 *
 ***/

uint16_t BluetoothAudioSession::RegisterStatusCback(
    const PortStatusCallbacks& callbacks) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  uint16_t cookie = ObserversCookieGetInitValue(session_type_);
  uint16_t cookie_upper_bound = ObserversCookieGetUpperBound(session_type_);

  while (cookie < cookie_upper_bound) {
    if (observers_.find(cookie) == observers_.end()) {
      break;
    }
    ++cookie;
  }
  if (cookie >= cookie_upper_bound) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << " has " << observers_.size()
               << " observers already (No Resource)";
    return kObserversCookieUndefined;
  }
  std::shared_ptr<PortStatusCallbacks> cb =
      std::make_shared<PortStatusCallbacks>();
  *cb = callbacks;
  observers_[cookie] = cb;
  return cookie;
}

void BluetoothAudioSession::UnregisterStatusCback(uint16_t cookie) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (observers_.erase(cookie) != 1) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " no such provider=0x"
                 << ::android::base::StringPrintf("%04x", cookie);
  }
}

/***
 *
 * Stream methods
 *
 ***/

bool BluetoothAudioSession::StartStream(bool is_low_latency) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  auto hal_retval = stack_iface_->startStream(is_low_latency);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
    return false;
  }
  return true;
}

bool BluetoothAudioSession::SuspendStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  auto hal_retval = stack_iface_->suspendStream();
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
    return false;
  }
  return true;
}

void BluetoothAudioSession::StopStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    return;
  }
  auto hal_retval = stack_iface_->stopStream();
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
  }
}

/***
 *
 * Private methods
 *
 ***/

bool BluetoothAudioSession::UpdateDataPath(const DataMQDesc* mq_desc) {
  if (mq_desc == nullptr) {
    // usecase of reset by nullptr
    data_mq_ = nullptr;
    return true;
  }
  std::unique_ptr<DataMQ> temp_mq;
  temp_mq.reset(new DataMQ(*mq_desc));
  if (!temp_mq || !temp_mq->isValid()) {
    data_mq_ = nullptr;
    return false;
  }
  data_mq_ = std::move(temp_mq);
  return true;
}

bool BluetoothAudioSession::UpdateAudioConfig(
    const AudioConfiguration& audio_config) {
  bool is_software_session =
      (session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ == SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ == SessionType::HFP_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ == SessionType::HFP_SOFTWARE_DECODING_DATAPATH ||
       session_type_ == SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH ||
       session_type_ == SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ == SessionType::A2DP_SOFTWARE_DECODING_DATAPATH);
  bool is_offload_a2dp_session =
      (session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH);
  bool is_offload_hfp_session =
      session_type_ == SessionType::HFP_HARDWARE_OFFLOAD_DATAPATH;
  bool is_offload_le_audio_unicast_session =
      (session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH);
  bool is_offload_le_audio_broadcast_session =
      (session_type_ ==
       SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
  auto audio_config_tag = audio_config.getTag();
  bool is_software_audio_config =
      (is_software_session &&
       audio_config_tag == AudioConfiguration::pcmConfig);
  bool is_a2dp_offload_audio_config =
      (is_offload_a2dp_session &&
       (audio_config_tag == AudioConfiguration::a2dp ||
        audio_config_tag == AudioConfiguration::a2dpConfig));
  bool is_hfp_offload_audio_config =
      (is_offload_hfp_session &&
       audio_config_tag == AudioConfiguration::hfpConfig);
  bool is_le_audio_offload_unicast_audio_config =
      (is_offload_le_audio_unicast_session &&
       audio_config_tag == AudioConfiguration::leAudioConfig);
  bool is_le_audio_offload_broadcast_audio_config =
      (is_offload_le_audio_broadcast_session &&
       audio_config_tag == AudioConfiguration::leAudioBroadcastConfig);
  if (!is_software_audio_config && !is_a2dp_offload_audio_config &&
      !is_hfp_offload_audio_config &&
      !is_le_audio_offload_unicast_audio_config &&
      !is_le_audio_offload_broadcast_audio_config) {
    return false;
  }

  if (session_type_ ==
      SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    // reset swoffload when new config is coming with offload session
    // type
    LeAudioSwOffloadInstance::releaseSwOffload();
  }

  audio_config_ = std::make_unique<AudioConfiguration>(audio_config);

  std::optional<bool> is_stream_active;
  auto opus_audio_config =
      convertToOpusAudioConfiguration(audio_config, is_stream_active);
  if (opus_audio_config.has_value()) {
    audio_config_ =
        std::make_unique<AudioConfiguration>(opus_audio_config.value());
  }
  return true;
}

void BluetoothAudioSession::ReportSessionStatus() {
  // This is locked already by OnSessionStarted / OnSessionEnded
  if (observers_.empty()) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<PortStatusCallbacks> callback = observer.second;
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " notify to bluetooth_audio=0x"
              << ::android::base::StringPrintf("%04x", cookie);
    callback->session_changed_cb_(cookie);
  }
}

/***
 *
 * PCM methods
 *
 ***/

size_t BluetoothAudioSession::OutWritePcmData(const void* buffer,
                                              size_t bytes) {
  if (buffer == nullptr || bytes <= 0) {
    return 0;
  }

  if (session_type_ ==
      SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    if (LeAudioSwOffloadInstance::is_using_swoffload_) {
      if (!LeAudioSwOffloadInstance::is_swoff_stream_running_) {
        return 0;
      }
      size_t total_written =
          LeAudioSwOffloadInstance::sw_offload_streams_->write(buffer, bytes);

      if (total_written != bytes) {
        LOG(WARNING) << "Software offload write not complete.";
      }
      return total_written;
    } else {
      return 0;
    }
  } else {
    size_t total_written = 0;
    int timeout_ms = kFmqSendTimeoutMs;
    do {
      std::unique_lock<std::recursive_mutex> lock(mutex_);
      if (!IsSessionReadyInternal()) {
        break;
      }
      size_t num_bytes_to_write = data_mq_->availableToWrite();
      if (num_bytes_to_write) {
        if (num_bytes_to_write > (bytes - total_written)) {
          num_bytes_to_write = bytes - total_written;
        }

        if (!data_mq_->write(
                static_cast<const MQDataType*>(buffer) + total_written,
                num_bytes_to_write)) {
          LOG(ERROR) << "FMQ datapath writing " << total_written << "/" << bytes
                     << " failed";
          return total_written;
        }
        total_written += num_bytes_to_write;
      } else if (timeout_ms >= kWritePollMs) {
        lock.unlock();
        usleep(kWritePollMs * 1000);
        timeout_ms -= kWritePollMs;
      } else {
        LOG(DEBUG) << "Data " << total_written << "/" << bytes << " overflow "
                   << (kFmqSendTimeoutMs - timeout_ms) << " ms";
        return total_written;
      }
    } while (total_written < bytes);
    return total_written;
  }
}

size_t BluetoothAudioSession::InReadPcmData(void* buffer, size_t bytes) {
  if (buffer == nullptr || bytes <= 0) {
    return 0;
  }
  size_t total_read = 0;
  int timeout_ms = kFmqReceiveTimeoutMs;
  do {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    if (!IsSessionReadyInternal()) {
      break;
    }
    size_t num_bytes_to_read = data_mq_->availableToRead();
    if (num_bytes_to_read) {
      if (num_bytes_to_read > (bytes - total_read)) {
        num_bytes_to_read = bytes - total_read;
      }
      if (!data_mq_->read(static_cast<MQDataType*>(buffer) + total_read,
                          num_bytes_to_read)) {
        LOG(ERROR) << "FMQ datapath reading " << total_read << "/" << bytes
                   << " failed";
        return total_read;
      }
      total_read += num_bytes_to_read;
    } else if (timeout_ms >= kReadPollMs) {
      lock.unlock();
      usleep(kReadPollMs * 1000);
      timeout_ms -= kReadPollMs;
      continue;
    } else {
      LOG(DEBUG) << "Data " << total_read << "/" << bytes << " overflow "
                 << (kFmqReceiveTimeoutMs - timeout_ms) << " ms";
      return total_read;
    }
  } while (total_read < bytes);
  return total_read;
}

/***
 *
 * Other methods
 *
 ***/

void BluetoothAudioSession::ReportControlStatus(bool start_resp,
                                                BluetoothAudioStatus status) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<PortStatusCallbacks> callback = observer.second;
    LOG(INFO) << __func__ << " - status=" << toString(status)
              << " for SessionType=" << toString(session_type_)
              << ", bluetooth_audio=0x"
              << ::android::base::StringPrintf("%04x", cookie)
              << (start_resp ? " started" : " suspended");
    callback->control_result_cb_(cookie, start_resp, status);
  }
}

void BluetoothAudioSession::ReportLowLatencyModeAllowedChanged(bool allowed) {
  if (session_type_ != SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    return;
  }
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  low_latency_allowed_ = allowed;
  // TODO(b/294498919): Remove this after there is API to update latency mode
  // after audio session started. If low_latency_allowed_ is true, the session
  // can support LOW_LATENCY and FREE LatencyMode.
  if (low_latency_allowed_) {
    if (std::find(latency_modes_.begin(), latency_modes_.end(),
                  LatencyMode::LOW_LATENCY) == latency_modes_.end()) {
      LOG(INFO) << __func__ << " - insert LOW_LATENCY LatencyMode";
      latency_modes_.push_back(LatencyMode::LOW_LATENCY);
    }
  }
  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<PortStatusCallbacks> callback = observer.second;
    LOG(INFO) << __func__
              << " - allowed=" << (allowed ? " allowed" : " disallowed");
    if (callback->low_latency_mode_allowed_cb_ != nullptr) {
      callback->low_latency_mode_allowed_cb_(cookie, allowed);
    }
  }
}

bool BluetoothAudioSession::GetPresentationPosition(
    PresentationPosition& presentation_position) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  if (!stack_iface_->getPresentationPosition(&presentation_position).isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
    return false;
  }
  return true;
}

void BluetoothAudioSession::UpdateSourceMetadata(
    const struct source_metadata& source_metadata) {
  ssize_t track_count = source_metadata.track_count;
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_) << ","
            << track_count << " track(s)";
  SourceMetadata hal_source_metadata;
  hal_source_metadata.tracks.resize(track_count);
  for (int i = 0; i < track_count; i++) {
    hal_source_metadata.tracks[i].usage =
        static_cast<media::audio::common::AudioUsage>(
            source_metadata.tracks[i].usage);
    hal_source_metadata.tracks[i].contentType =
        static_cast<media::audio::common::AudioContentType>(
            source_metadata.tracks[i].content_type);
    hal_source_metadata.tracks[i].gain = source_metadata.tracks[i].gain;
    LOG(VERBOSE) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", usage=" << toString(hal_source_metadata.tracks[i].usage)
                 << ", content="
                 << toString(hal_source_metadata.tracks[i].contentType)
                 << ", gain=" << hal_source_metadata.tracks[i].gain;
  }
  UpdateSourceMetadata(hal_source_metadata);
}

void BluetoothAudioSession::UpdateSinkMetadata(
    const struct sink_metadata& sink_metadata) {
  ssize_t track_count = sink_metadata.track_count;
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_) << ","
            << track_count << " track(s)";
  SinkMetadata hal_sink_metadata;
  hal_sink_metadata.tracks.resize(track_count);
  for (int i = 0; i < track_count; i++) {
    hal_sink_metadata.tracks[i].source =
        static_cast<media::audio::common::AudioSource>(
            sink_metadata.tracks[i].source);
    hal_sink_metadata.tracks[i].gain = sink_metadata.tracks[i].gain;
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << ", source=" << sink_metadata.tracks[i].source
              << ", dest_device=" << sink_metadata.tracks[i].dest_device
              << ", gain=" << sink_metadata.tracks[i].gain
              << ", dest_device_address="
              << sink_metadata.tracks[i].dest_device_address;
  }
  UpdateSinkMetadata(hal_sink_metadata);
}

bool BluetoothAudioSession::UpdateSourceMetadata(
    const SourceMetadata& hal_source_metadata) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }

  if (session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_SOFTWARE_DECODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
      session_type_ == SessionType::HFP_SOFTWARE_ENCODING_DATAPATH ||
      session_type_ == SessionType::HFP_SOFTWARE_DECODING_DATAPATH) {
    return false;
  }

  auto hal_retval = stack_iface_->updateSourceMetadata(hal_source_metadata);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
    return false;
  }
  return true;
}

bool BluetoothAudioSession::UpdateSinkMetadata(
    const SinkMetadata& hal_sink_metadata) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }

  if (session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_SOFTWARE_DECODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
      session_type_ == SessionType::HFP_SOFTWARE_ENCODING_DATAPATH ||
      session_type_ == SessionType::HFP_SOFTWARE_DECODING_DATAPATH) {
    return false;
  }

  auto hal_retval = stack_iface_->updateSinkMetadata(hal_sink_metadata);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
    return false;
  }
  return true;
}

std::vector<LatencyMode> BluetoothAudioSession::GetSupportedLatencyModes() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return std::vector<LatencyMode>();
  }

  if (com::android::btaudio::hal::flags::dsa_lea()) {
    std::vector<LatencyMode> supported_latency_modes;
    if (session_type_ ==
        SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
      for (LatencyMode mode : latency_modes_) {
        if (mode == LatencyMode::LOW_LATENCY) {
          // LOW_LATENCY is not supported for LE_HARDWARE_OFFLOAD_ENC sessions
          continue;
        }
        supported_latency_modes.push_back(mode);
      }
    } else {
      for (LatencyMode mode : latency_modes_) {
        if (!low_latency_allowed_ && mode == LatencyMode::LOW_LATENCY) {
          // ignore LOW_LATENCY mode if Bluetooth stack doesn't allow
          continue;
        }
        if (mode == LatencyMode::DYNAMIC_SPATIAL_AUDIO_SOFTWARE ||
            mode == LatencyMode::DYNAMIC_SPATIAL_AUDIO_HARDWARE) {
          // DSA_SW and DSA_HW only supported for LE_HARDWARE_OFFLOAD_ENC
          // sessions
          continue;
        }
        supported_latency_modes.push_back(mode);
      }
    }
    LOG(DEBUG) << __func__ << " - Supported LatencyMode="
               << toString(supported_latency_modes);
    return supported_latency_modes;
  }

  if (low_latency_allowed_) return latency_modes_;
  std::vector<LatencyMode> modes;
  for (LatencyMode mode : latency_modes_) {
    if (mode == LatencyMode::LOW_LATENCY)
      // ignore those low latency mode if Bluetooth stack doesn't allow
      continue;
    modes.push_back(mode);
  }
  return modes;
}

void BluetoothAudioSession::SetLatencyMode(const LatencyMode& latency_mode) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReadyInternal()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return;
  }

  auto hal_retval = stack_iface_->setLatencyMode(latency_mode);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
  }
}

bool BluetoothAudioSession::IsAidlAvailable() {
  if (is_aidl_checked) return is_aidl_available;
  is_aidl_available =
      (AServiceManager_checkService(
           kDefaultAudioProviderFactoryInterface.c_str()) != nullptr);
  is_aidl_checked = true;
  return is_aidl_available;
}

/***
 *
 * BluetoothAudioSessionInstance
 *
 ***/
std::mutex BluetoothAudioSessionInstance::mutex_;
std::unordered_map<SessionType, std::shared_ptr<BluetoothAudioSession>>
    BluetoothAudioSessionInstance::sessions_map_;

std::shared_ptr<BluetoothAudioSession>
BluetoothAudioSessionInstance::GetSessionInstance(
    const SessionType& session_type) {
  std::lock_guard<std::mutex> guard(mutex_);

  if (!sessions_map_.empty()) {
    auto entry = sessions_map_.find(session_type);
    if (entry != sessions_map_.end()) {
      return entry->second;
    }
  }
  std::shared_ptr<BluetoothAudioSession> session_ptr =
      std::make_shared<BluetoothAudioSession>(session_type);
  sessions_map_[session_type] = session_ptr;
  return session_ptr;
}

/***
 *
 * LeAudioSwOffload
 *
 ***/
std::shared_ptr<LeAudioSwOffloadCallbacks>
    LeAudioSwOffloadInstance::sw_offload_cbacks_;
std::shared_ptr<swoff::LeAudioStream>
    LeAudioSwOffloadInstance::sw_offload_streams_;
std::atomic<bool> LeAudioSwOffloadInstance::is_swoff_stream_running_ = false;
std::atomic<bool> LeAudioSwOffloadInstance::is_using_swoffload_ = false;

void LeAudioSwOffloadInstance::releaseSwOffload() {
  if (com::android::btaudio::hal::flags::leaudio_sw_offload() &&
      ::android::base::GetBoolProperty(kPropertyLeaSwOffload, false)) {
    if (LeAudioSwOffloadInstance::sw_offload_streams_) {
      LeAudioSwOffloadInstance::is_using_swoffload_ = false;
      LeAudioSwOffloadInstance::is_swoff_stream_running_ = false;
      LeAudioSwOffloadInstance::sw_offload_streams_ = nullptr;
      LeAudioSwOffloadInstance::sw_offload_cbacks_ = nullptr;
    }
  }
}

LeAudioSwOffloadCallbacks::LeAudioSwOffloadCallbacks() {}

void LeAudioSwOffloadCallbacks::start() {
  LOG(INFO) << __func__ << "Stream started";
  LeAudioSwOffloadInstance::is_swoff_stream_running_ = true;
}
void LeAudioSwOffloadCallbacks::stop() {
  LOG(INFO) << __func__ << "Stream stopped";
  LeAudioSwOffloadInstance::is_swoff_stream_running_ = false;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
