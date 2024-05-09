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

#define LOG_TAG "BTAudioProviderLeAudioHW"

#include "LeAudioOffloadAudioProvider.h"

#include <BluetoothAudioCodecs.h>
#include <BluetoothAudioSessionReport.h>
#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

constexpr uint8_t kLeAudioDirectionSink = 0x01;
constexpr uint8_t kLeAudioDirectionSource = 0x02;

const std::map<CodecSpecificConfigurationLtv::SamplingFrequency, uint32_t>
    freq_to_support_bitmask_map = {
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ8000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ8000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ11025,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ11025},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ16000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ22050,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ22050},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ24000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ32000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ32000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ48000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ88200,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ88200},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ96000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ176400,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ176400},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ192000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ192000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ384000,
         CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies::HZ384000},
};

// Helper map from capability's tag to configuration's tag
std::map<CodecSpecificCapabilitiesLtv::Tag, CodecSpecificConfigurationLtv::Tag>
    cap_to_cfg_tag_map = {
        {CodecSpecificCapabilitiesLtv::Tag::supportedSamplingFrequencies,
         CodecSpecificConfigurationLtv::Tag::samplingFrequency},
        {CodecSpecificCapabilitiesLtv::Tag::supportedMaxCodecFramesPerSDU,
         CodecSpecificConfigurationLtv::Tag::codecFrameBlocksPerSDU},
        {CodecSpecificCapabilitiesLtv::Tag::supportedFrameDurations,
         CodecSpecificConfigurationLtv::Tag::frameDuration},
        {CodecSpecificCapabilitiesLtv::Tag::supportedAudioChannelCounts,
         CodecSpecificConfigurationLtv::Tag::audioChannelAllocation},
        {CodecSpecificCapabilitiesLtv::Tag::supportedOctetsPerCodecFrame,
         CodecSpecificConfigurationLtv::Tag::octetsPerCodecFrame},
};

const std::map<CodecSpecificConfigurationLtv::FrameDuration, uint32_t>
    fduration_to_support_fduration_map = {
        {CodecSpecificConfigurationLtv::FrameDuration::US7500,
         CodecSpecificCapabilitiesLtv::SupportedFrameDurations::US7500},
        {CodecSpecificConfigurationLtv::FrameDuration::US10000,
         CodecSpecificCapabilitiesLtv::SupportedFrameDurations::US10000},
};

std::map<int32_t, CodecSpecificConfigurationLtv::SamplingFrequency>
    sampling_freq_map = {
        {16000, CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000},
        {48000, CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000},
        {96000, CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000},
};

std::map<int32_t, CodecSpecificConfigurationLtv::FrameDuration>
    frame_duration_map = {
        {7500, CodecSpecificConfigurationLtv::FrameDuration::US7500},
        {10000, CodecSpecificConfigurationLtv::FrameDuration::US10000},
};

LeAudioOffloadOutputAudioProvider::LeAudioOffloadOutputAudioProvider()
    : LeAudioOffloadAudioProvider() {
  session_type_ = SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

LeAudioOffloadInputAudioProvider::LeAudioOffloadInputAudioProvider()
    : LeAudioOffloadAudioProvider() {
  session_type_ = SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH;
}

LeAudioOffloadBroadcastAudioProvider::LeAudioOffloadBroadcastAudioProvider()
    : LeAudioOffloadAudioProvider() {
  session_type_ =
      SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

LeAudioOffloadAudioProvider::LeAudioOffloadAudioProvider()
    : BluetoothAudioProvider() {}

bool LeAudioOffloadAudioProvider::isValid(const SessionType& sessionType) {
  return (sessionType == session_type_);
}

ndk::ScopedAStatus LeAudioOffloadAudioProvider::startSession(
    const std::shared_ptr<IBluetoothAudioPort>& host_if,
    const AudioConfiguration& audio_config,
    const std::vector<LatencyMode>& latency_modes, DataMQDesc* _aidl_return) {
  if (session_type_ ==
      SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    if (audio_config.getTag() != AudioConfiguration::leAudioBroadcastConfig) {
      LOG(WARNING) << __func__ << " - Invalid Audio Configuration="
                   << audio_config.toString();
      *_aidl_return = DataMQDesc();
      return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
  } else if (audio_config.getTag() != AudioConfiguration::leAudioConfig) {
    LOG(WARNING) << __func__ << " - Invalid Audio Configuration="
                 << audio_config.toString();
    *_aidl_return = DataMQDesc();
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  return BluetoothAudioProvider::startSession(host_if, audio_config,
                                              latency_modes, _aidl_return);
}

ndk::ScopedAStatus LeAudioOffloadAudioProvider::onSessionReady(
    DataMQDesc* _aidl_return) {
  BluetoothAudioSessionReport::OnSessionStarted(
      session_type_, stack_iface_, nullptr, *audio_config_, latency_modes_);
  *_aidl_return = DataMQDesc();
  return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus LeAudioOffloadAudioProvider::setCodecPriority(
    const CodecId& in_codecId, int32_t in_priority) {
  codec_priority_map_[in_codecId] = in_priority;
  return ndk::ScopedAStatus::ok();
};

bool LeAudioOffloadAudioProvider::isMatchedValidCodec(CodecId cfg_codec,
                                                      CodecId req_codec) {
  auto priority = codec_priority_map_.find(cfg_codec);
  if (priority != codec_priority_map_.end() &&
      priority->second ==
          LeAudioOffloadAudioProvider::CODEC_PRIORITY_DISABLED) {
    return false;
  }
  return cfg_codec == req_codec;
}

bool LeAudioOffloadAudioProvider::filterCapabilitiesMatchedContext(
    AudioContext& setting_context,
    const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities) {
  // If has no metadata, assume match
  if (!capabilities.metadata.has_value()) return true;

  for (auto metadata : capabilities.metadata.value()) {
    if (!metadata.has_value()) continue;
    if (metadata.value().getTag() == MetadataLtv::Tag::preferredAudioContexts) {
      // Check all pref audio context to see if anything matched
      auto& context = metadata.value()
                          .get<MetadataLtv::Tag::preferredAudioContexts>()
                          .values;
      if (setting_context.bitmask & context.bitmask) {
        // New mask with matched capability
        setting_context.bitmask &= context.bitmask;
        return true;
      }
    }
  }

  return false;
}

bool LeAudioOffloadAudioProvider::isMatchedSamplingFreq(
    CodecSpecificConfigurationLtv::SamplingFrequency& cfg_freq,
    CodecSpecificCapabilitiesLtv::SupportedSamplingFrequencies&
        capability_freq) {
  auto p = freq_to_support_bitmask_map.find(cfg_freq);
  if (p != freq_to_support_bitmask_map.end()) {
    if (capability_freq.bitmask & p->second) {
      return true;
    }
  }
  return false;
}

bool LeAudioOffloadAudioProvider::isMatchedFrameDuration(
    CodecSpecificConfigurationLtv::FrameDuration& cfg_fduration,
    CodecSpecificCapabilitiesLtv::SupportedFrameDurations&
        capability_fduration) {
  auto p = fduration_to_support_fduration_map.find(cfg_fduration);
  if (p != fduration_to_support_fduration_map.end())
    if (capability_fduration.bitmask & p->second) {
      return true;
    }
  return false;
}

bool LeAudioOffloadAudioProvider::isMatchedAudioChannel(
    CodecSpecificConfigurationLtv::AudioChannelAllocation&
    /*cfg_channel*/,
    CodecSpecificCapabilitiesLtv::SupportedAudioChannelCounts&
    /*capability_channel*/) {
  bool isMatched = true;
  // TODO: how to match?
  return isMatched;
}

bool LeAudioOffloadAudioProvider::isMatchedCodecFramesPerSDU(
    CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU& cfg_frame_sdu,
    CodecSpecificCapabilitiesLtv::SupportedMaxCodecFramesPerSDU&
        capability_frame_sdu) {
  return cfg_frame_sdu.value <= capability_frame_sdu.value;
}

bool LeAudioOffloadAudioProvider::isMatchedOctetsPerCodecFrame(
    CodecSpecificConfigurationLtv::OctetsPerCodecFrame& cfg_octets,
    CodecSpecificCapabilitiesLtv::SupportedOctetsPerCodecFrame&
        capability_octets) {
  return cfg_octets.value >= capability_octets.min &&
         cfg_octets.value <= capability_octets.max;
}

bool LeAudioOffloadAudioProvider::isCapabilitiesMatchedCodecConfiguration(
    std::vector<CodecSpecificConfigurationLtv>& codec_cfg,
    std::vector<CodecSpecificCapabilitiesLtv> codec_capabilities) {
  // Convert all codec_cfg into a map of tags -> correct data
  std::map<CodecSpecificConfigurationLtv::Tag, CodecSpecificConfigurationLtv>
      cfg_tag_map;
  for (auto codec_cfg_data : codec_cfg)
    cfg_tag_map[codec_cfg_data.getTag()] = codec_cfg_data;

  for (auto& codec_capability : codec_capabilities) {
    auto cfg = cfg_tag_map.find(cap_to_cfg_tag_map[codec_capability.getTag()]);
    // If capability has this tag, but our configuration doesn't
    // Then we will assume it is matched
    if (cfg == cfg_tag_map.end()) {
      continue;
    }

    switch (codec_capability.getTag()) {
      case CodecSpecificCapabilitiesLtv::Tag::supportedSamplingFrequencies: {
        if (!isMatchedSamplingFreq(
                cfg->second.get<
                    CodecSpecificConfigurationLtv::Tag::samplingFrequency>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedSamplingFrequencies>())) {
          return false;
        }
        break;
      }

      case CodecSpecificCapabilitiesLtv::Tag::supportedFrameDurations: {
        if (!isMatchedFrameDuration(
                cfg->second
                    .get<CodecSpecificConfigurationLtv::Tag::frameDuration>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedFrameDurations>())) {
          return false;
        }
        break;
      }

      case CodecSpecificCapabilitiesLtv::Tag::supportedAudioChannelCounts: {
        if (!isMatchedAudioChannel(
                cfg->second.get<CodecSpecificConfigurationLtv::Tag::
                                    audioChannelAllocation>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedAudioChannelCounts>())) {
          return false;
        }
        break;
      }

      case CodecSpecificCapabilitiesLtv::Tag::supportedMaxCodecFramesPerSDU: {
        if (!isMatchedCodecFramesPerSDU(
                cfg->second.get<CodecSpecificConfigurationLtv::Tag::
                                    codecFrameBlocksPerSDU>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedMaxCodecFramesPerSDU>())) {
          return false;
        }
        break;
      }

      case CodecSpecificCapabilitiesLtv::Tag::supportedOctetsPerCodecFrame: {
        if (!isMatchedOctetsPerCodecFrame(
                cfg->second.get<
                    CodecSpecificConfigurationLtv::Tag::octetsPerCodecFrame>(),
                codec_capability.get<CodecSpecificCapabilitiesLtv::Tag::
                                         supportedOctetsPerCodecFrame>())) {
          return false;
        }
        break;
      }
    }
  }

  return true;
}

bool LeAudioOffloadAudioProvider::isMatchedAseConfiguration(
    LeAudioAseConfiguration setting_cfg,
    LeAudioAseConfiguration requirement_cfg) {
  // Check matching for codec configuration <=> requirement ASE codec
  // Also match if no CodecId requirement
  if (requirement_cfg.codecId.has_value()) {
    if (!setting_cfg.codecId.has_value()) return false;
    if (!isMatchedValidCodec(setting_cfg.codecId.value(),
                             requirement_cfg.codecId.value())) {
      return false;
    }
  }

  if (requirement_cfg.targetLatency ==
          LeAudioAseConfiguration::TargetLatency::UNDEFINED ||
      setting_cfg.targetLatency != requirement_cfg.targetLatency) {
    return false;
  }
  // Ignore PHY requirement

  // Check all codec configuration
  std::map<CodecSpecificConfigurationLtv::Tag, CodecSpecificConfigurationLtv>
      cfg_tag_map;
  for (auto cfg : setting_cfg.codecConfiguration)
    cfg_tag_map[cfg.getTag()] = cfg;

  for (auto requirement_cfg : requirement_cfg.codecConfiguration) {
    // Directly compare CodecSpecificConfigurationLtv
    auto cfg = cfg_tag_map.find(requirement_cfg.getTag());
    if (cfg == cfg_tag_map.end()) {
      return false;
    }

    if (cfg->second != requirement_cfg) {
      return false;
    }
  }
  // Ignore vendor configuration and metadata requirement

  return true;
}

bool LeAudioOffloadAudioProvider::isMatchedBISConfiguration(
    LeAudioBisConfiguration bis_cfg,
    const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities) {
  if (!isMatchedValidCodec(bis_cfg.codecId, capabilities.codecId)) {
    return false;
  }
  if (!isCapabilitiesMatchedCodecConfiguration(
          bis_cfg.codecConfiguration, capabilities.codecSpecificCapabilities)) {
    return false;
  }
  return true;
}

void LeAudioOffloadAudioProvider::filterCapabilitiesAseDirectionConfiguration(
    std::vector<std::optional<AseDirectionConfiguration>>&
        direction_configurations,
    const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities,
    std::vector<std::optional<AseDirectionConfiguration>>&
        valid_direction_configurations) {
  for (auto direction_configuration : direction_configurations) {
    if (!direction_configuration.has_value()) continue;
    if (!direction_configuration.value().aseConfiguration.codecId.has_value())
      continue;
    if (!isMatchedValidCodec(
            direction_configuration.value().aseConfiguration.codecId.value(),
            capabilities.codecId))
      continue;
    // Check matching for codec configuration <=> codec capabilities
    if (!isCapabilitiesMatchedCodecConfiguration(
            direction_configuration.value().aseConfiguration.codecConfiguration,
            capabilities.codecSpecificCapabilities))
      continue;
    valid_direction_configurations.push_back(direction_configuration);
  }
}

void LeAudioOffloadAudioProvider::filterRequirementAseDirectionConfiguration(
    std::optional<std::vector<std::optional<AseDirectionConfiguration>>>&
        direction_configurations,
    const std::vector<std::optional<AseDirectionRequirement>>& requirements,
    std::optional<std::vector<std::optional<AseDirectionConfiguration>>>&
        valid_direction_configurations) {
  if (!valid_direction_configurations.has_value()) {
    valid_direction_configurations =
        std::vector<std::optional<AseDirectionConfiguration>>();
  }
  // For every requirement, find the matched ase configuration
  if (!direction_configurations.has_value()) return;
  for (auto& requirement : requirements) {
    if (!requirement.has_value()) continue;
    for (auto direction_configuration : direction_configurations.value()) {
      if (!direction_configuration.has_value()) continue;
      if (!isMatchedAseConfiguration(
              direction_configuration.value().aseConfiguration,
              requirement.value().aseConfiguration))
        continue;
      // Valid if match any requirement.
      valid_direction_configurations.value().push_back(direction_configuration);
      break;
    }
  }
  // Ensure that each requirement will have one direction configurations
  if (valid_direction_configurations.value().empty() ||
      (valid_direction_configurations.value().size() != requirements.size())) {
    valid_direction_configurations = std::nullopt;
  }
}

/* Get a new LeAudioAseConfigurationSetting by matching a setting with a
 * capabilities. The new setting will have a filtered list of
 * AseDirectionConfiguration that matched the capabilities */
std::optional<LeAudioAseConfigurationSetting>
LeAudioOffloadAudioProvider::getCapabilitiesMatchedAseConfigurationSettings(
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
    const IBluetoothAudioProvider::LeAudioDeviceCapabilities& capabilities,
    uint8_t direction) {
  // Create a new LeAudioAseConfigurationSetting and return
  // For other direction will contain all settings
  LeAudioAseConfigurationSetting filtered_setting{
      .audioContext = setting.audioContext,
      .sinkAseConfiguration = setting.sinkAseConfiguration,
      .sourceAseConfiguration = setting.sourceAseConfiguration,
      .flags = setting.flags,
      .packing = setting.packing,
  };
  // Try to match context in metadata.
  if (!filterCapabilitiesMatchedContext(filtered_setting.audioContext,
                                        capabilities))
    return std::nullopt;

  // Get a list of all matched AseDirectionConfiguration
  // for the input direction
  std::vector<std::optional<AseDirectionConfiguration>>*
      direction_configuration = nullptr;
  if (direction == kLeAudioDirectionSink) {
    if (!filtered_setting.sinkAseConfiguration.has_value()) return std::nullopt;
    direction_configuration = &filtered_setting.sinkAseConfiguration.value();
  } else {
    if (!filtered_setting.sourceAseConfiguration.has_value())
      return std::nullopt;
    direction_configuration = &filtered_setting.sourceAseConfiguration.value();
  }
  std::vector<std::optional<AseDirectionConfiguration>>
      valid_direction_configuration;
  filterCapabilitiesAseDirectionConfiguration(
      *direction_configuration, capabilities, valid_direction_configuration);

  // No valid configuration for this direction
  if (valid_direction_configuration.empty()) {
    return std::nullopt;
  }

  // Create a new LeAudioAseConfigurationSetting and return
  // For other direction will contain all settings
  if (direction == kLeAudioDirectionSink) {
    filtered_setting.sinkAseConfiguration = valid_direction_configuration;
  } else {
    filtered_setting.sourceAseConfiguration = valid_direction_configuration;
  }

  return filtered_setting;
}

/* Get a new LeAudioAseConfigurationSetting by matching a setting with a
 * requirement. The new setting will have a filtered list of
 * AseDirectionConfiguration that matched the requirement */
std::optional<LeAudioAseConfigurationSetting>
LeAudioOffloadAudioProvider::getRequirementMatchedAseConfigurationSettings(
    IBluetoothAudioProvider::LeAudioAseConfigurationSetting& setting,
    const IBluetoothAudioProvider::LeAudioConfigurationRequirement&
        requirement) {
  // Try to match context in metadata.
  if ((setting.audioContext.bitmask & requirement.audioContext.bitmask) !=
      requirement.audioContext.bitmask)
    return std::nullopt;

  // Further filter setting's context
  setting.audioContext.bitmask &= requirement.audioContext.bitmask;

  // Check requirement for the correct direction
  const std::optional<std::vector<std::optional<AseDirectionRequirement>>>*
      direction_requirement;
  std::vector<std::optional<AseDirectionConfiguration>>*
      direction_configuration;

  // Create a new LeAudioAseConfigurationSetting to return
  LeAudioAseConfigurationSetting filtered_setting{
      .audioContext = setting.audioContext,
      .packing = setting.packing,
      .flags = setting.flags,
  };

  if (requirement.sinkAseRequirement.has_value()) {
    filterRequirementAseDirectionConfiguration(
        setting.sinkAseConfiguration, requirement.sinkAseRequirement.value(),
        filtered_setting.sinkAseConfiguration);
    if (!filtered_setting.sinkAseConfiguration.has_value()) return std::nullopt;
  }

  if (requirement.sourceAseRequirement.has_value()) {
    filterRequirementAseDirectionConfiguration(
        setting.sourceAseConfiguration,
        requirement.sourceAseRequirement.value(),
        filtered_setting.sourceAseConfiguration);
    if (!filtered_setting.sourceAseConfiguration.has_value())
      return std::nullopt;
  }

  return filtered_setting;
}

// For each requirement, a valid ASE configuration will satify:
// - matched with any sink capability (if presented)
// - OR matched with any source capability (if presented)
// - and the setting need to pass the requirement
ndk::ScopedAStatus LeAudioOffloadAudioProvider::getLeAudioAseConfiguration(
    const std::optional<std::vector<
        std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
        in_remoteSinkAudioCapabilities,
    const std::optional<std::vector<
        std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
        in_remoteSourceAudioCapabilities,
    const std::vector<IBluetoothAudioProvider::LeAudioConfigurationRequirement>&
        in_requirements,
    std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>*
        _aidl_return) {
  // Get all configuration settings
  std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>
      ase_configuration_settings =
          BluetoothAudioCodecs::GetLeAudioAseConfigurationSettings();

  if (!in_remoteSinkAudioCapabilities.has_value() &&
      !in_remoteSourceAudioCapabilities.has_value()) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  // Each setting consist of source and sink AseDirectionConfiguration vector
  // Filter every sink capability
  std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>
      matched_ase_configuration_settings;

  if (in_remoteSinkAudioCapabilities.has_value()) {
    // Matching each setting with any remote capabilities
    for (auto& setting : ase_configuration_settings) {
      for (auto& capability : in_remoteSinkAudioCapabilities.value()) {
        if (!capability.has_value()) continue;
        auto filtered_ase_configuration_setting =
            getCapabilitiesMatchedAseConfigurationSettings(
                setting, capability.value(), kLeAudioDirectionSink);
        if (filtered_ase_configuration_setting.has_value()) {
          matched_ase_configuration_settings.push_back(
              filtered_ase_configuration_setting.value());
        }
      }
    }
  }

  // Combine filter every source capability
  if (in_remoteSourceAudioCapabilities.has_value()) {
    // Matching each setting with any remote capabilities
    for (auto& setting : ase_configuration_settings) {
      for (auto& capability : in_remoteSourceAudioCapabilities.value()) {
        if (!capability.has_value()) continue;
        auto filtered_ase_configuration_setting =
            getCapabilitiesMatchedAseConfigurationSettings(
                setting, capability.value(), kLeAudioDirectionSource);
        if (filtered_ase_configuration_setting.has_value()) {
          // Put into the same list
          // possibly duplicated, filtered by requirement later
          matched_ase_configuration_settings.push_back(
              filtered_ase_configuration_setting.value());
        }
      }
    }
  }

  if (matched_ase_configuration_settings.empty()) {
    LOG(WARNING) << __func__ << ": No setting matched the capability";
    return ndk::ScopedAStatus::ok();
  }
  // Each requirement will match with a valid setting
  std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting> result;
  for (auto& requirement : in_requirements) {
    LOG(INFO) << __func__ << ": Trying to match for the requirement "
              << requirement.toString();
    bool is_matched = false;

    for (auto& setting : matched_ase_configuration_settings) {
      auto filtered_ase_configuration_setting =
          getRequirementMatchedAseConfigurationSettings(setting, requirement);
      if (filtered_ase_configuration_setting.has_value()) {
        result.push_back(filtered_ase_configuration_setting.value());
        LOG(INFO) << __func__ << ": Result = "
                  << filtered_ase_configuration_setting.value().toString();
        // Found a matched setting, ignore other settings
        is_matched = true;
        break;
      }
    }
    if (!is_matched) {
      // If cannot satisfy this requirement, return an empty result
      LOG(WARNING) << __func__ << ": Cannot match the requirement "
                   << requirement.toString();
      result.clear();
      break;
    }
  }

  *_aidl_return = result;
  return ndk::ScopedAStatus::ok();
};

bool LeAudioOffloadAudioProvider::isMatchedQosRequirement(
    LeAudioAseQosConfiguration setting_qos,
    AseQosDirectionRequirement requirement_qos) {
  if (setting_qos.retransmissionNum !=
      requirement_qos.preferredRetransmissionNum) {
    return false;
  }
  if (setting_qos.maxTransportLatencyMs >
      requirement_qos.maxTransportLatencyMs) {
    return false;
  }
  // Ignore other parameters, as they are not populated in the setting_qos
  return true;
}

bool isValidQosRequirement(AseQosDirectionRequirement qosRequirement) {
  return ((qosRequirement.maxTransportLatencyMs > 0) &&
          (qosRequirement.presentationDelayMaxUs > 0) &&
          (qosRequirement.presentationDelayMaxUs >=
           qosRequirement.presentationDelayMinUs));
}

std::optional<LeAudioAseQosConfiguration>
LeAudioOffloadAudioProvider::getDirectionQosConfiguration(
    uint8_t direction,
    const IBluetoothAudioProvider::LeAudioAseQosConfigurationRequirement&
        qosRequirement,
    std::vector<LeAudioAseConfigurationSetting>& ase_configuration_settings) {
  std::optional<AseQosDirectionRequirement> direction_qos_requirement =
      std::nullopt;

  // Get the correct direction
  if (direction == kLeAudioDirectionSink) {
    direction_qos_requirement = qosRequirement.sinkAseQosRequirement.value();
  } else {
    direction_qos_requirement = qosRequirement.sourceAseQosRequirement.value();
  }

  for (auto& setting : ase_configuration_settings) {
    // Context matching
    if ((setting.audioContext.bitmask & qosRequirement.audioContext.bitmask) !=
        qosRequirement.audioContext.bitmask)
      continue;

    // Match configuration flags
    // Currently configuration flags are not populated, ignore.

    // Get a list of all matched AseDirectionConfiguration
    // for the input direction
    std::vector<std::optional<AseDirectionConfiguration>>*
        direction_configuration = nullptr;
    if (direction == kLeAudioDirectionSink) {
      if (!setting.sinkAseConfiguration.has_value()) continue;
      direction_configuration = &setting.sinkAseConfiguration.value();
    } else {
      if (!setting.sourceAseConfiguration.has_value()) continue;
      direction_configuration = &setting.sourceAseConfiguration.value();
    }

    for (auto cfg : *direction_configuration) {
      if (!cfg.has_value()) continue;
      // If no requirement, return the first QoS
      if (!direction_qos_requirement.has_value()) {
        return cfg.value().qosConfiguration;
      }

      // If has requirement, return the first matched QoS
      // Try to match the ASE configuration
      // and QoS with requirement
      if (!cfg.value().qosConfiguration.has_value()) continue;
      if (isMatchedAseConfiguration(
              cfg.value().aseConfiguration,
              direction_qos_requirement.value().aseConfiguration) &&
          isMatchedQosRequirement(cfg.value().qosConfiguration.value(),
                                  direction_qos_requirement.value())) {
        return cfg.value().qosConfiguration;
      }
    }
  }

  return std::nullopt;
}

ndk::ScopedAStatus LeAudioOffloadAudioProvider::getLeAudioAseQosConfiguration(
    const IBluetoothAudioProvider::LeAudioAseQosConfigurationRequirement&
        in_qosRequirement,
    IBluetoothAudioProvider::LeAudioAseQosConfigurationPair* _aidl_return) {
  IBluetoothAudioProvider::LeAudioAseQosConfigurationPair result;

  // Get all configuration settings
  std::vector<IBluetoothAudioProvider::LeAudioAseConfigurationSetting>
      ase_configuration_settings =
          BluetoothAudioCodecs::GetLeAudioAseConfigurationSettings();

  // Direction QoS matching
  // Only handle one direction input case
  if (in_qosRequirement.sinkAseQosRequirement.has_value()) {
    if (!isValidQosRequirement(in_qosRequirement.sinkAseQosRequirement.value()))
      return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    result.sinkQosConfiguration = getDirectionQosConfiguration(
        kLeAudioDirectionSink, in_qosRequirement, ase_configuration_settings);
  }
  if (in_qosRequirement.sourceAseQosRequirement.has_value()) {
    if (!isValidQosRequirement(
            in_qosRequirement.sourceAseQosRequirement.value()))
      return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    result.sourceQosConfiguration = getDirectionQosConfiguration(
        kLeAudioDirectionSource, in_qosRequirement, ase_configuration_settings);
  }

  *_aidl_return = result;
  return ndk::ScopedAStatus::ok();
};

ndk::ScopedAStatus LeAudioOffloadAudioProvider::onSinkAseMetadataChanged(
    IBluetoothAudioProvider::AseState in_state, int32_t /*in_cigId*/,
    int32_t /*in_cisId*/,
    const std::optional<std::vector<std::optional<MetadataLtv>>>& in_metadata) {
  (void)in_state;
  (void)in_metadata;
  return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
};

ndk::ScopedAStatus LeAudioOffloadAudioProvider::onSourceAseMetadataChanged(
    IBluetoothAudioProvider::AseState in_state, int32_t /*in_cigId*/,
    int32_t /*in_cisId*/,
    const std::optional<std::vector<std::optional<MetadataLtv>>>& in_metadata) {
  (void)in_state;
  (void)in_metadata;
  return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
};

void LeAudioOffloadAudioProvider::getBroadcastSettings() {
  if (!broadcast_settings.empty()) return;

  LOG(INFO) << __func__ << ": Loading broadcast settings from provider info";

  std::vector<CodecInfo> db_codec_info =
      BluetoothAudioCodecs::GetLeAudioOffloadCodecInfo(
          SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
  broadcast_settings.clear();

  // Default value for unmapped fields
  CodecSpecificConfigurationLtv::AudioChannelAllocation default_allocation;
  default_allocation.bitmask =
      CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_CENTER;
  CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU default_frame;
  default_frame.value = 2;

  for (auto& codec_info : db_codec_info) {
    if (codec_info.transport.getTag() != CodecInfo::Transport::leAudio)
      continue;
    auto& transport = codec_info.transport.get<CodecInfo::Transport::leAudio>();
    LeAudioBroadcastConfigurationSetting setting;
    // Default setting
    setting.numBis = 1;
    setting.phy = {Phy::TWO_M};
    // Populate BIS configuration info using codec_info
    LeAudioBisConfiguration bis_cfg;
    bis_cfg.codecId = codec_info.id;

    CodecSpecificConfigurationLtv::OctetsPerCodecFrame octets;
    octets.value = transport.bitdepth[0];

    bis_cfg.codecConfiguration = {
        sampling_freq_map[transport.samplingFrequencyHz[0]],
        octets,
        frame_duration_map[transport.frameDurationUs[0]],
        default_allocation,
        default_frame,
    };

    // Add information to structure
    IBluetoothAudioProvider::LeAudioSubgroupBisConfiguration sub_bis_cfg;
    sub_bis_cfg.numBis = 2;
    sub_bis_cfg.bisConfiguration = bis_cfg;
    IBluetoothAudioProvider::LeAudioBroadcastSubgroupConfiguration sub_cfg;
    // Populate the same sub config
    sub_cfg.bisConfigurations = {sub_bis_cfg, sub_bis_cfg};
    setting.subgroupsConfigurations = {sub_cfg};

    broadcast_settings.push_back(setting);
  }

  LOG(INFO) << __func__
            << ": Done loading broadcast settings from provider info";
}

/* Get a new LeAudioAseConfigurationSetting by matching a setting with a
 * capabilities. The new setting will have a filtered list of
 * AseDirectionConfiguration that matched the capabilities */
std::optional<LeAudioBroadcastConfigurationSetting>
LeAudioOffloadAudioProvider::
    getCapabilitiesMatchedBroadcastConfigurationSettings(
        LeAudioBroadcastConfigurationSetting& setting,
        const IBluetoothAudioProvider::LeAudioDeviceCapabilities&
            capabilities) {
  std::vector<IBluetoothAudioProvider::LeAudioBroadcastSubgroupConfiguration>
      filter_subgroup;
  for (auto& sub_cfg : setting.subgroupsConfigurations) {
    std::vector<IBluetoothAudioProvider::LeAudioSubgroupBisConfiguration>
        filtered_bis_cfg;
    for (auto& bis_cfg : sub_cfg.bisConfigurations)
      if (isMatchedBISConfiguration(bis_cfg.bisConfiguration, capabilities)) {
        filtered_bis_cfg.push_back(bis_cfg);
      }
    if (!filtered_bis_cfg.empty()) {
      IBluetoothAudioProvider::LeAudioBroadcastSubgroupConfiguration
          subgroup_cfg;
      subgroup_cfg.bisConfigurations = filtered_bis_cfg;
      filter_subgroup.push_back(subgroup_cfg);
    }
  }
  if (filter_subgroup.empty()) return std::nullopt;

  // Create a new LeAudioAseConfigurationSetting and return
  LeAudioBroadcastConfigurationSetting filtered_setting(setting);
  filtered_setting.subgroupsConfigurations = filter_subgroup;

  return filtered_setting;
}

bool LeAudioOffloadAudioProvider::isSubgroupConfigurationMatchedContext(
    AudioContext setting_context,
    LeAudioBroadcastSubgroupConfiguration configuration) {
  // Find any valid context metadata in the bisConfigurations
  // assuming the bis configuration in the same bis subgroup
  // will have the same context metadata
  std::optional<AudioContext> config_context = std::nullopt;

  for (auto& p : configuration.bisConfigurations)
    if (p.bisConfiguration.metadata.has_value()) {
      bool is_context_found = false;
      for (auto& metadata : p.bisConfiguration.metadata.value()) {
        if (!metadata.has_value()) continue;
        if (metadata.value().getTag() ==
            MetadataLtv::Tag::preferredAudioContexts) {
          config_context = metadata.value()
                               .get<MetadataLtv::Tag::preferredAudioContexts>()
                               .values;
          is_context_found = true;
          break;
        }
      }
      if (is_context_found) break;
    }

  // Not found context metadata in any of the bis configuration, assume matched
  if (!config_context.has_value()) return true;
  return (setting_context.bitmask & config_context.value().bitmask);
}

ndk::ScopedAStatus
LeAudioOffloadAudioProvider::getLeAudioBroadcastConfiguration(
    const std::optional<std::vector<
        std::optional<IBluetoothAudioProvider::LeAudioDeviceCapabilities>>>&
        in_remoteSinkAudioCapabilities,
    const IBluetoothAudioProvider::LeAudioBroadcastConfigurationRequirement&
        in_requirement,
    LeAudioBroadcastConfigurationSetting* _aidl_return) {
  if (in_requirement.subgroupConfigurationRequirements.empty()) {
    LOG(WARNING) << __func__ << ": Empty requirement";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  // Broadcast setting are from provider info
  // We will allow empty capability input, match all settings with requirements.
  getBroadcastSettings();
  std::vector<LeAudioBroadcastConfigurationSetting> filtered_settings;
  if (!in_remoteSinkAudioCapabilities.has_value()) {
    LOG(INFO) << __func__ << ": Empty capability, get all broadcast settings";
    filtered_settings = broadcast_settings;
  } else {
    for (auto& setting : broadcast_settings) {
      for (auto& capability : in_remoteSinkAudioCapabilities.value()) {
        if (!capability.has_value()) continue;
        auto filtered_setting =
            getCapabilitiesMatchedBroadcastConfigurationSettings(
                setting, capability.value());
        if (filtered_setting.has_value())
          filtered_settings.push_back(filtered_setting.value());
      }
    }
  }

  if (filtered_settings.empty()) {
    LOG(WARNING) << __func__ << ": Cannot match any remote capability";
    return ndk::ScopedAStatus::ok();
  }

  if (in_requirement.subgroupConfigurationRequirements.empty()) {
    LOG(INFO) << __func__ << ": Empty requirement";
    *_aidl_return = filtered_settings[0];
    return ndk::ScopedAStatus::ok();
  }

  // For each subgroup config requirement, find a suitable subgroup config.
  // Gather these suitable subgroup config in an array.
  // If the setting can satisfy all requirement, we can return the setting
  // with the filtered array.
  for (auto& setting : filtered_settings) {
    LeAudioBroadcastConfigurationSetting matched_setting(setting);
    matched_setting.subgroupsConfigurations.clear();
    auto total_num_bis = 0;

    bool matched_all_requirement = true;

    for (auto& sub_req : in_requirement.subgroupConfigurationRequirements) {
      bool is_matched = false;
      for (auto& sub_cfg : setting.subgroupsConfigurations) {
        // Match the context
        if (!isSubgroupConfigurationMatchedContext(sub_req.audioContext,
                                                   sub_cfg))
          continue;
        // Matching number of BIS
        if (sub_req.bisNumPerSubgroup != sub_cfg.bisConfigurations.size())
          continue;
        // Currently will ignore quality matching.
        matched_setting.subgroupsConfigurations.push_back(sub_cfg);
        total_num_bis += sub_cfg.bisConfigurations.size();
        is_matched = true;
        break;
      }
      // There is an unmatched requirement, this setting cannot be used
      if (!is_matched) {
        matched_all_requirement = false;
        break;
      }
    }

    if (!matched_all_requirement) continue;

    matched_setting.numBis = total_num_bis;
    // Return the filtered setting if all requirement satified
    *_aidl_return = matched_setting;
    return ndk::ScopedAStatus::ok();
  }

  LOG(WARNING) << __func__ << ": Cannot match any requirement";
  return ndk::ScopedAStatus::ok();
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
