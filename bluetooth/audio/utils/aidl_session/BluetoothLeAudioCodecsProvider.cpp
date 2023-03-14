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

#define LOG_TAG "BTAudioCodecsProviderAidl"

#include "BluetoothLeAudioCodecsProvider.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

static const char* kLeAudioCodecCapabilitiesFile =
    "/vendor/etc/le_audio_codec_capabilities.xml";

static const AudioLocation kStereoAudio = static_cast<AudioLocation>(
    static_cast<uint8_t>(AudioLocation::FRONT_LEFT) |
    static_cast<uint8_t>(AudioLocation::FRONT_RIGHT));
static const AudioLocation kMonoAudio = AudioLocation::UNKNOWN;

static std::vector<LeAudioCodecCapabilitiesSetting> leAudioCodecCapabilities;

static bool isInvalidFileContent = false;

std::optional<setting::LeAudioOffloadSetting>
BluetoothLeAudioCodecsProvider::ParseFromLeAudioOffloadSettingFile() {
  if (!leAudioCodecCapabilities.empty() || isInvalidFileContent) {
    return std::nullopt;
  }
  auto le_audio_offload_setting =
      setting::readLeAudioOffloadSetting(kLeAudioCodecCapabilitiesFile);
  if (!le_audio_offload_setting.has_value()) {
    LOG(ERROR) << __func__ << ": Failed to read "
               << kLeAudioCodecCapabilitiesFile;
  }
  return le_audio_offload_setting;
}

std::vector<LeAudioCodecCapabilitiesSetting>
BluetoothLeAudioCodecsProvider::GetLeAudioCodecCapabilities(
    const std::optional<setting::LeAudioOffloadSetting>&
        le_audio_offload_setting) {
  if (!leAudioCodecCapabilities.empty()) {
    return leAudioCodecCapabilities;
  }

  if (!le_audio_offload_setting.has_value()) {
    LOG(ERROR)
        << __func__
        << ": input le_audio_offload_setting content need to be non empty";
    return {};
  }

  ClearLeAudioCodecCapabilities();
  isInvalidFileContent = true;

  std::vector<setting::Scenario> supported_scenarios =
      GetScenarios(le_audio_offload_setting);
  if (supported_scenarios.empty()) {
    LOG(ERROR) << __func__ << ": No scenarios in "
               << kLeAudioCodecCapabilitiesFile;
    return {};
  }

  UpdateConfigurationsToMap(le_audio_offload_setting);
  if (configuration_map_.empty()) {
    LOG(ERROR) << __func__ << ": No configurations in "
               << kLeAudioCodecCapabilitiesFile;
    return {};
  }

  UpdateCodecConfigurationsToMap(le_audio_offload_setting);
  if (codec_configuration_map_.empty()) {
    LOG(ERROR) << __func__ << ": No codec configurations in "
               << kLeAudioCodecCapabilitiesFile;
    return {};
  }

  UpdateStrategyConfigurationsToMap(le_audio_offload_setting);
  if (strategy_configuration_map_.empty()) {
    LOG(ERROR) << __func__ << ": No strategy configurations in "
               << kLeAudioCodecCapabilitiesFile;
    return {};
  }

  leAudioCodecCapabilities =
      ComposeLeAudioCodecCapabilities(supported_scenarios);
  isInvalidFileContent = leAudioCodecCapabilities.empty();

  return leAudioCodecCapabilities;
}

void BluetoothLeAudioCodecsProvider::ClearLeAudioCodecCapabilities() {
  leAudioCodecCapabilities.clear();
  configuration_map_.clear();
  codec_configuration_map_.clear();
  strategy_configuration_map_.clear();
}

std::vector<setting::Scenario> BluetoothLeAudioCodecsProvider::GetScenarios(
    const std::optional<setting::LeAudioOffloadSetting>&
        le_audio_offload_setting) {
  std::vector<setting::Scenario> supported_scenarios;
  if (le_audio_offload_setting->hasScenarioList()) {
    for (const auto& scenario_list :
         le_audio_offload_setting->getScenarioList()) {
      if (!scenario_list.hasScenario()) {
        continue;
      }
      for (const auto& scenario : scenario_list.getScenario()) {
        if (scenario.hasEncode() && scenario.hasDecode()) {
          supported_scenarios.push_back(scenario);
        }
      }
    }
  }
  return supported_scenarios;
}

void BluetoothLeAudioCodecsProvider::UpdateConfigurationsToMap(
    const std::optional<setting::LeAudioOffloadSetting>&
        le_audio_offload_setting) {
  if (le_audio_offload_setting->hasConfigurationList()) {
    for (const auto& configuration_list :
         le_audio_offload_setting->getConfigurationList()) {
      if (!configuration_list.hasConfiguration()) {
        continue;
      }
      for (const auto& configuration : configuration_list.getConfiguration()) {
        if (configuration.hasName() && configuration.hasCodecConfiguration() &&
            configuration.hasStrategyConfiguration()) {
          configuration_map_.insert(
              make_pair(configuration.getName(), configuration));
        }
      }
    }
  }
}

void BluetoothLeAudioCodecsProvider::UpdateCodecConfigurationsToMap(
    const std::optional<setting::LeAudioOffloadSetting>&
        le_audio_offload_setting) {
  if (le_audio_offload_setting->hasCodecConfigurationList()) {
    for (const auto& codec_configuration_list :
         le_audio_offload_setting->getCodecConfigurationList()) {
      if (!codec_configuration_list.hasCodecConfiguration()) {
        continue;
      }
      for (const auto& codec_configuration :
           codec_configuration_list.getCodecConfiguration()) {
        if (IsValidCodecConfiguration(codec_configuration)) {
          codec_configuration_map_.insert(
              make_pair(codec_configuration.getName(), codec_configuration));
        }
      }
    }
  }
}

void BluetoothLeAudioCodecsProvider::UpdateStrategyConfigurationsToMap(
    const std::optional<setting::LeAudioOffloadSetting>&
        le_audio_offload_setting) {
  if (le_audio_offload_setting->hasStrategyConfigurationList()) {
    for (const auto& strategy_configuration_list :
         le_audio_offload_setting->getStrategyConfigurationList()) {
      if (!strategy_configuration_list.hasStrategyConfiguration()) {
        continue;
      }
      for (const auto& strategy_configuration :
           strategy_configuration_list.getStrategyConfiguration()) {
        if (IsValidStrategyConfiguration(strategy_configuration)) {
          strategy_configuration_map_.insert(make_pair(
              strategy_configuration.getName(), strategy_configuration));
        }
      }
    }
  }
}

std::vector<LeAudioCodecCapabilitiesSetting>
BluetoothLeAudioCodecsProvider::ComposeLeAudioCodecCapabilities(
    const std::vector<setting::Scenario>& supported_scenarios) {
  std::vector<LeAudioCodecCapabilitiesSetting> le_audio_codec_capabilities;
  for (const auto& scenario : supported_scenarios) {
    UnicastCapability unicast_encode_capability =
        GetUnicastCapability(scenario.getEncode());
    UnicastCapability unicast_decode_capability =
        GetUnicastCapability(scenario.getDecode());
    // encode and decode cannot be unknown at the same time
    if (unicast_encode_capability.codecType == CodecType::UNKNOWN &&
        unicast_decode_capability.codecType == CodecType::UNKNOWN) {
      continue;
    }
    BroadcastCapability broadcast_capability = {.codecType =
                                                    CodecType::UNKNOWN};
    le_audio_codec_capabilities.push_back(
        {.unicastEncodeCapability = unicast_encode_capability,
         .unicastDecodeCapability = unicast_decode_capability,
         .broadcastCapability = broadcast_capability});
  }
  return le_audio_codec_capabilities;
}

UnicastCapability BluetoothLeAudioCodecsProvider::GetUnicastCapability(
    const std::string& coding_direction) {
  if (coding_direction == "invalid") {
    return {.codecType = CodecType::UNKNOWN};
  }

  auto configuration_iter = configuration_map_.find(coding_direction);
  if (configuration_iter == configuration_map_.end()) {
    return {.codecType = CodecType::UNKNOWN};
  }

  auto codec_configuration_iter = codec_configuration_map_.find(
      configuration_iter->second.getCodecConfiguration());
  if (codec_configuration_iter == codec_configuration_map_.end()) {
    return {.codecType = CodecType::UNKNOWN};
  }

  auto strategy_configuration_iter = strategy_configuration_map_.find(
      configuration_iter->second.getStrategyConfiguration());
  if (strategy_configuration_iter == strategy_configuration_map_.end()) {
    return {.codecType = CodecType::UNKNOWN};
  }

  CodecType codec_type =
      GetCodecType(codec_configuration_iter->second.getCodec());
  if (codec_type == CodecType::LC3) {
    return ComposeUnicastCapability(
        codec_type,
        GetAudioLocation(
            strategy_configuration_iter->second.getAudioLocation()),
        strategy_configuration_iter->second.getConnectedDevice(),
        strategy_configuration_iter->second.getChannelCount(),
        ComposeLc3Capability(codec_configuration_iter->second));
  }
  return {.codecType = CodecType::UNKNOWN};
}

template <class T>
UnicastCapability BluetoothLeAudioCodecsProvider::ComposeUnicastCapability(
    const CodecType& codec_type, const AudioLocation& audio_location,
    const uint8_t& device_cnt, const uint8_t& channel_count,
    const T& capability) {
  return {
      .codecType = codec_type,
      .supportedChannel = audio_location,
      .deviceCount = device_cnt,
      .channelCountPerDevice = channel_count,
      .leAudioCodecCapabilities =
          UnicastCapability::LeAudioCodecCapabilities(capability),
  };
}

Lc3Capabilities BluetoothLeAudioCodecsProvider::ComposeLc3Capability(
    const setting::CodecConfiguration& codec_configuration) {
  return {.samplingFrequencyHz = {codec_configuration.getSamplingFrequency()},
          .frameDurationUs = {codec_configuration.getFrameDurationUs()},
          .octetsPerFrame = {codec_configuration.getOctetsPerCodecFrame()}};
}

AudioLocation BluetoothLeAudioCodecsProvider::GetAudioLocation(
    const setting::AudioLocation& audio_location) {
  switch (audio_location) {
    case setting::AudioLocation::MONO:
      return kMonoAudio;
    case setting::AudioLocation::STEREO:
      return kStereoAudio;
    default:
      return AudioLocation::UNKNOWN;
  }
}

CodecType BluetoothLeAudioCodecsProvider::GetCodecType(
    const setting::CodecType& codec_type) {
  switch (codec_type) {
    case setting::CodecType::LC3:
      return CodecType::LC3;
    default:
      return CodecType::UNKNOWN;
  }
}

bool BluetoothLeAudioCodecsProvider::IsValidCodecConfiguration(
    const setting::CodecConfiguration& codec_configuration) {
  return codec_configuration.hasName() && codec_configuration.hasCodec() &&
         codec_configuration.hasSamplingFrequency() &&
         codec_configuration.hasFrameDurationUs() &&
         codec_configuration.hasOctetsPerCodecFrame();
}

bool BluetoothLeAudioCodecsProvider::IsValidStrategyConfiguration(
    const setting::StrategyConfiguration& strategy_configuration) {
  if (!strategy_configuration.hasName() ||
      !strategy_configuration.hasAudioLocation() ||
      !strategy_configuration.hasConnectedDevice() ||
      !strategy_configuration.hasChannelCount()) {
    return false;
  }
  if (strategy_configuration.getAudioLocation() ==
      setting::AudioLocation::STEREO) {
    if ((strategy_configuration.getConnectedDevice() == 2 &&
         strategy_configuration.getChannelCount() == 1) ||
        (strategy_configuration.getConnectedDevice() == 1 &&
         strategy_configuration.getChannelCount() == 2)) {
      // Stereo
      // 1. two connected device, one for L one for R
      // 2. one connected device for both L and R
      return true;
    }
  } else if (strategy_configuration.getAudioLocation() ==
             setting::AudioLocation::MONO) {
    if (strategy_configuration.getConnectedDevice() == 1 &&
        strategy_configuration.getChannelCount() == 1) {
      // Mono
      return true;
    }
  }
  return false;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
