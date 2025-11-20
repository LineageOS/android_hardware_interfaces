/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <cstdint>
#include <vector>
#define STREAM_TO_UINT8(u8, p) \
  {                            \
    (u8) = (uint8_t)(*(p));    \
    (p) += 1;                  \
  }
#define STREAM_TO_UINT16(u16, p)                                  \
  {                                                               \
    (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); \
    (p) += 2;                                                     \
  }
#define STREAM_TO_UINT32(u32, p)                                      \
  {                                                                   \
    (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + \
             ((((uint32_t)(*((p) + 2)))) << 16) +                     \
             ((((uint32_t)(*((p) + 3)))) << 24));                     \
    (p) += 4;                                                         \
  }

#define LOG_TAG "BTAudioAseConfigAidl"

#include <aidl/android/hardware/bluetooth/audio/AudioConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/AudioContext.h>
#include <aidl/android/hardware/bluetooth/audio/BluetoothAudioStatus.h>
#include <aidl/android/hardware/bluetooth/audio/CodecId.h>
#include <aidl/android/hardware/bluetooth/audio/CodecSpecificCapabilitiesLtv.h>
#include <aidl/android/hardware/bluetooth/audio/CodecSpecificConfigurationLtv.h>
#include <aidl/android/hardware/bluetooth/audio/ConfigurationFlags.h>
#include <aidl/android/hardware/bluetooth/audio/LeAudioAseConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/Phy.h>
#include <android-base/logging.h>

#include <optional>

#include "BluetoothAudioType.h"
#include "BluetoothLeAudioAseConfigurationSettingProvider.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

/* Internal structure definition */
std::map<std::string,
         std::tuple<std::vector<std::optional<AseDirectionConfiguration>>,
                    std::vector<std::optional<AseDirectionConfiguration>>,
                    ConfigurationFlags>>
    configurations_;

std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>
    ase_configuration_settings_;

// Set configuration and scenario files with fallback default
static const std::vector<
    std::pair<const char* /*schema*/, const char* /*content*/>>
    kLeAudioSetConfigs = {
        {"/vendor/etc/aidl/le_audio/"
         "aidl_audio_set_configurations.bfbs",
         "/vendor/etc/aidl/le_audio/"
         "aidl_audio_set_configurations.json"},

        {"/vendor/etc/aidl/le_audio/"
         "aidl_audio_set_configurations.bfbs",
         "/vendor/etc/aidl/le_audio/"
         "aidl_default_audio_set_configurations.json"},
};
static const std::vector<
    std::pair<const char* /*schema*/, const char* /*content*/>>
    kLeAudioSetScenarios = {{"/vendor/etc/aidl/le_audio/"
                             "aidl_audio_set_scenarios.bfbs",
                             "/vendor/etc/aidl/le_audio/"
                             "aidl_audio_set_scenarios.json"},

                            {"/vendor/etc/aidl/le_audio/"
                             "aidl_audio_set_scenarios.bfbs",
                             "/vendor/etc/aidl/le_audio/"
                             "aidl_default_audio_set_scenarios.json"}};

/* Implementation */

std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>
AudioSetConfigurationProviderJson::GetLeAudioAseConfigurationSettings() {
  AudioSetConfigurationProviderJson::LoadAudioSetConfigurationProviderJson();
  return ase_configuration_settings_;
}

void AudioSetConfigurationProviderJson::
    LoadAudioSetConfigurationProviderJson() {
  if (configurations_.empty() || ase_configuration_settings_.empty()) {
    ase_configuration_settings_.clear();
    configurations_.clear();
    auto loaded = LoadContent(kLeAudioSetConfigs, kLeAudioSetScenarios,
                              CodecLocation::ADSP);
    if (!loaded)
      LOG(ERROR) << ": Unable to load le audio set configuration files.";
  } else
    LOG(INFO) << ": Reusing loaded le audio set configuration";
}

const le_audio::CodecSpecificConfiguration*
AudioSetConfigurationProviderJson::LookupCodecSpecificParam(
    const flatbuffers::Vector<flatbuffers::Offset<
        le_audio::CodecSpecificConfiguration>>* flat_codec_specific_params,
    le_audio::CodecSpecificLtvGenericTypes type) {
  auto it = std::find_if(
      flat_codec_specific_params->cbegin(), flat_codec_specific_params->cend(),
      [&type](const auto& csc) { return (csc->type() == type); });
  return (it != flat_codec_specific_params->cend()) ? *it : nullptr;
}

void AudioSetConfigurationProviderJson::populateAudioChannelAllocation(
    CodecSpecificConfigurationLtv::AudioChannelAllocation&
        audio_channel_allocation,
    uint32_t audio_location) {
  audio_channel_allocation.bitmask = 0;
  for (auto [allocation, bitmask] : audio_channel_allocation_map) {
    if (audio_location & allocation)
      audio_channel_allocation.bitmask |= bitmask;
  }
}

void AudioSetConfigurationProviderJson::populateConfigurationData(
    LeAudioAseConfiguration& ase,
    const flatbuffers::Vector<
        flatbuffers::Offset<le_audio::CodecSpecificConfiguration>>*
        flat_codec_specific_params) {
  uint8_t sampling_frequency = 0;
  uint8_t frame_duration = 0;
  uint32_t audio_channel_allocation = 0;
  uint16_t octets_per_codec_frame = 0;
  uint8_t codec_frames_blocks_per_sdu = 0;

  auto param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_SAMPLING_FREQUENCY);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT8(sampling_frequency, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_FRAME_DURATION);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT8(frame_duration, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::
          CodecSpecificLtvGenericTypes_SUPPORTED_AUDIO_CHANNEL_ALLOCATION);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT32(audio_channel_allocation, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_OCTETS_PER_CODEC_FRAME);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT16(octets_per_codec_frame, ptr);
  }

  param = LookupCodecSpecificParam(
      flat_codec_specific_params,
      le_audio::
          CodecSpecificLtvGenericTypes_SUPPORTED_CODEC_FRAME_BLOCKS_PER_SDU);
  if (param) {
    auto ptr = param->compound_value()->value()->data();
    STREAM_TO_UINT8(codec_frames_blocks_per_sdu, ptr);
  }

  // Make the correct value
  ase.codecConfiguration = std::vector<CodecSpecificConfigurationLtv>();

  auto sampling_freq_it = sampling_freq_map.find(sampling_frequency);
  if (sampling_freq_it != sampling_freq_map.end())
    ase.codecConfiguration.push_back(sampling_freq_it->second);
  auto frame_duration_it = frame_duration_map.find(frame_duration);
  if (frame_duration_it != frame_duration_map.end())
    ase.codecConfiguration.push_back(frame_duration_it->second);

  CodecSpecificConfigurationLtv::AudioChannelAllocation channel_allocation;
  populateAudioChannelAllocation(channel_allocation, audio_channel_allocation);
  ase.codecConfiguration.push_back(channel_allocation);

  auto octet_structure = CodecSpecificConfigurationLtv::OctetsPerCodecFrame();
  octet_structure.value = octets_per_codec_frame;
  ase.codecConfiguration.push_back(octet_structure);

  auto frame_sdu_structure =
      CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU();
  frame_sdu_structure.value = codec_frames_blocks_per_sdu;
  ase.codecConfiguration.push_back(frame_sdu_structure);
}

void AudioSetConfigurationProviderJson::populateAseConfiguration(
    LeAudioAseConfiguration& ase,
    const le_audio::AudioSetSubConfiguration* flat_subconfig,
    const le_audio::QosConfiguration* qos_cfg,
    ConfigurationFlags& configurationFlags) {
  // Target latency
  switch (qos_cfg->target_latency()) {
    case le_audio::AudioSetConfigurationTargetLatency::
        AudioSetConfigurationTargetLatency_BALANCED_RELIABILITY:
      ase.targetLatency =
          LeAudioAseConfiguration::TargetLatency::BALANCED_LATENCY_RELIABILITY;
      break;
    case le_audio::AudioSetConfigurationTargetLatency::
        AudioSetConfigurationTargetLatency_HIGH_RELIABILITY:
      ase.targetLatency =
          LeAudioAseConfiguration::TargetLatency::HIGHER_RELIABILITY;
      break;
    case le_audio::AudioSetConfigurationTargetLatency::
        AudioSetConfigurationTargetLatency_LOW:
      ase.targetLatency = LeAudioAseConfiguration::TargetLatency::LOWER;
      configurationFlags.bitmask |= ConfigurationFlags::LOW_LATENCY;
      break;
    default:
      ase.targetLatency = LeAudioAseConfiguration::TargetLatency::UNDEFINED;
      break;
  };

  ase.targetPhy = Phy::TWO_M;
  // Making CodecId
  if (flat_subconfig->codec_id()->coding_format() ==
      (uint8_t)CodecId::Core::LC3) {
    ase.codecId = CodecId::Core::LC3;
  } else {
    auto vendorC = CodecId::Vendor();
    vendorC.codecId = flat_subconfig->codec_id()->vendor_codec_id();
    vendorC.id = flat_subconfig->codec_id()->vendor_company_id();
    ase.codecId = vendorC;
  }
  // Codec configuration data
  populateConfigurationData(ase, flat_subconfig->codec_configuration());
}

void AudioSetConfigurationProviderJson::populateAseQosConfiguration(
    LeAudioAseQosConfiguration& qos, const le_audio::QosConfiguration* qos_cfg,
    LeAudioAseConfiguration& ase, uint8_t ase_channel_cnt) {
  std::optional<CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU>
      frameBlock = std::nullopt;
  std::optional<CodecSpecificConfigurationLtv::FrameDuration> frameDuration =
      std::nullopt;
  std::optional<CodecSpecificConfigurationLtv::OctetsPerCodecFrame> octet =
      std::nullopt;

  // Put back allocation
  CodecSpecificConfigurationLtv::AudioChannelAllocation allocation =
      CodecSpecificConfigurationLtv::AudioChannelAllocation();
  if (ase_channel_cnt == 1) {
    if (ase.codecId.value().getTag() == CodecId::vendor) {
      allocation.bitmask =
          CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_LEFT;
    } else {
      allocation.bitmask |=
          CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_CENTER;
    }
  } else {
    allocation.bitmask |=
        CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_LEFT |
        CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_RIGHT;
  }
  for (auto& cfg_ltv : ase.codecConfiguration) {
    auto tag = cfg_ltv.getTag();
    if (tag == CodecSpecificConfigurationLtv::codecFrameBlocksPerSDU) {
      frameBlock =
          cfg_ltv.get<CodecSpecificConfigurationLtv::codecFrameBlocksPerSDU>();
    } else if (tag == CodecSpecificConfigurationLtv::frameDuration) {
      frameDuration =
          cfg_ltv.get<CodecSpecificConfigurationLtv::frameDuration>();
    } else if (tag == CodecSpecificConfigurationLtv::octetsPerCodecFrame) {
      octet = cfg_ltv.get<CodecSpecificConfigurationLtv::octetsPerCodecFrame>();
    } else if (tag == CodecSpecificConfigurationLtv::audioChannelAllocation) {
      // Change to the old hack allocation
      cfg_ltv.set<CodecSpecificConfigurationLtv::audioChannelAllocation>(
          allocation);
    }
  }

  int frameBlockValue = 1;
  if (frameBlock.has_value()) frameBlockValue = frameBlock.value().value;

  // Populate maxSdu
  if (octet.has_value()) {
    // Vendor logic: maxSdu = octet.value().value to allow set directly.
    if (ase.codecId.has_value() &&
        ase.codecId.value().getTag() == CodecId::vendor) {
      qos.maxSdu = octet.value().value * frameBlockValue;
    } else {
      qos.maxSdu = ase_channel_cnt * octet.value().value * frameBlockValue;
    }
  }
  // Populate sduIntervalUs
  if (frameDuration.has_value()) {
    switch (frameDuration.value()) {
      case CodecSpecificConfigurationLtv::FrameDuration::US7500:
        qos.sduIntervalUs = 7500;
        break;
      case CodecSpecificConfigurationLtv::FrameDuration::US10000:
        qos.sduIntervalUs = 10000;
        break;
      case CodecSpecificConfigurationLtv::FrameDuration::US20000:
        qos.sduIntervalUs = 20000;
        break;
    }
    qos.sduIntervalUs *= frameBlockValue;
  }
  qos.maxTransportLatencyMs = qos_cfg->max_transport_latency();
  qos.retransmissionNum = qos_cfg->retransmission_number();
}

void populateVendorCodecConfiguration(LeAudioAseConfiguration& ase) {
  if (ase.codecId.has_value() &&
      ase.codecId.value().getTag() == CodecId::vendor) {
    // Only populate for vendor codec.
    std::vector<uint8_t> codec_config;
    for (auto ltv : ase.codecConfiguration) {
      if (ltv.getTag() == CodecSpecificConfigurationLtv::samplingFrequency) {
        auto p = sampling_rate_ltv_to_codec_cfg_map.find(
            ltv.get<CodecSpecificConfigurationLtv::samplingFrequency>());
        if (p != sampling_rate_ltv_to_codec_cfg_map.end()) {
          codec_config.push_back(kCodecConfigOpcode);
          codec_config.push_back(kSamplingFrequencySubOpcode);
          codec_config.push_back(p->second);
        }
      } else if (ltv.getTag() == CodecSpecificConfigurationLtv::frameDuration) {
        auto p = frame_duration_ltv_to_codec_cfg_map.find(
            ltv.get<CodecSpecificConfigurationLtv::frameDuration>());
        if (p != frame_duration_ltv_to_codec_cfg_map.end()) {
          codec_config.push_back(kCodecConfigOpcode);
          codec_config.push_back(kFrameDurationSubOpcode);
          codec_config.push_back(p->second);
        }
      } else if (ltv.getTag() ==
                 CodecSpecificConfigurationLtv::audioChannelAllocation) {
        auto allocation =
            ltv.get<CodecSpecificConfigurationLtv::audioChannelAllocation>();
        codec_config.push_back(kAudioChannelAllocationOpcode);
        codec_config.push_back(kAudioChannelAllocationSubOpcode);
        for (int b = 0; b < 4; ++b) {
          codec_config.push_back((allocation.bitmask >> (b * 8)) & 0xff);
        }
      } else if (ltv.getTag() ==
                 CodecSpecificConfigurationLtv::octetsPerCodecFrame) {
        auto octet =
            ltv.get<CodecSpecificConfigurationLtv::octetsPerCodecFrame>();
        codec_config.push_back(kOctetsPerCodecFrameOpcode);
        codec_config.push_back(kOctetsPerCodecFrameSubOpcode);
        for (int b = 0; b < 2; ++b) {
          codec_config.push_back((octet.value >> (b * 8)) & 0xff);
        }
      } else if (ltv.getTag() ==
                 CodecSpecificConfigurationLtv::codecFrameBlocksPerSDU) {
        auto frame_block =
            ltv.get<CodecSpecificConfigurationLtv::codecFrameBlocksPerSDU>();
        codec_config.push_back(kCodecConfigOpcode);
        codec_config.push_back(kFrameBlocksPerSDUSubOpcode);
        codec_config.push_back(frame_block.value);
      }
    }
    ase.vendorCodecConfiguration = codec_config;
  }
}

bool isOpusHiResCodec(const LeAudioAseConfiguration& ase) {
  if (ase.codecId.has_value() &&
      ase.codecId.value().getTag() == CodecId::vendor) {
    auto cid = ase.codecId.value().get<CodecId::vendor>();
    if (cid == opus_codec) {
      // Based on the sampling freq
      for (auto ltv : ase.codecConfiguration) {
        if (ltv.getTag() == CodecSpecificConfigurationLtv::samplingFrequency) {
          if (ltv.get<CodecSpecificConfigurationLtv::samplingFrequency>() ==
              CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

LeAudioDataPathConfiguration populateDatapath(
    const CodecLocation& location, const LeAudioAseConfiguration& ase) {
  LeAudioDataPathConfiguration path;
  // Move codecId to iso data path
  path.isoDataPathConfiguration.codecId = ase.codecId.value();
  // Specific vendor datapath logic
  if (isOpusHiResCodec(ase)) {
    path.isoDataPathConfiguration.isTransparent = true;
    path.dataPathId = kIsoDataPathHciLinkFeedback;
    return path;
  }
  // Translate location to data path id
  switch (location) {
    case CodecLocation::ADSP:
      path.isoDataPathConfiguration.isTransparent = true;
      path.dataPathId = kIsoDataPathPlatformDefault;
      break;
    case CodecLocation::HOST:
      path.isoDataPathConfiguration.isTransparent = true;
      path.dataPathId = kIsoDataPathHci;
      break;
    case CodecLocation::CONTROLLER:
      path.isoDataPathConfiguration.isTransparent = false;
      path.dataPathId = kIsoDataPathPlatformDefault;
      break;
  }
  return path;
}

// Parse into AseDirectionConfiguration
AseDirectionConfiguration
AudioSetConfigurationProviderJson::SetConfigurationFromFlatSubconfig(
    const le_audio::AudioSetSubConfiguration* flat_subconfig,
    const le_audio::QosConfiguration* qos_cfg, CodecLocation location,
    ConfigurationFlags& configurationFlags) {
  AseDirectionConfiguration direction_conf;

  LeAudioAseConfiguration ase;
  LeAudioAseQosConfiguration qos;

  // Translate into LeAudioAseConfiguration
  populateAseConfiguration(ase, flat_subconfig, qos_cfg, configurationFlags);

  // Translate into LeAudioAseQosConfiguration
  populateAseQosConfiguration(qos, qos_cfg, ase,
                              flat_subconfig->ase_channel_cnt());

  // Populate vendorCodecConfiguration using the correct LTV
  populateVendorCodecConfiguration(ase);

  direction_conf.aseConfiguration = ase;
  direction_conf.qosConfiguration = qos;
  // Populate the correct datapath.
  direction_conf.dataPathConfiguration = populateDatapath(location, ase);

  return direction_conf;
}

// Parse into AseDirectionConfiguration and the ConfigurationFlags
// and put them in the given list.
void AudioSetConfigurationProviderJson::processSubconfig(
    const le_audio::AudioSetSubConfiguration* subconfig,
    const le_audio::QosConfiguration* qos_cfg,
    std::vector<std::optional<AseDirectionConfiguration>>&
        directionAseConfiguration,
    CodecLocation location, ConfigurationFlags& configurationFlags) {
  auto ase_cnt = subconfig->ase_cnt();
  auto config = SetConfigurationFromFlatSubconfig(subconfig, qos_cfg, location,
                                                  configurationFlags);
  directionAseConfiguration.push_back(config);
  // Put the same setting again.
  if (ase_cnt == 2) directionAseConfiguration.push_back(config);
}

// Comparing if 2 AseDirectionConfiguration is asymmetrical.
bool isAseConfigurationAsymmetrical(AseDirectionConfiguration cfg_a,
                                    AseDirectionConfiguration cfg_b) {
  // Comparing samplingFrequency of these 2 config.
  std::optional<CodecSpecificConfigurationLtv> cfg_a_fr = std::nullopt;
  std::optional<CodecSpecificConfigurationLtv> cfg_b_fr = std::nullopt;
  for (auto ltv : cfg_a.aseConfiguration.codecConfiguration) {
    if (ltv.getTag() == CodecSpecificConfigurationLtv::samplingFrequency) {
      cfg_a_fr = ltv.get<CodecSpecificConfigurationLtv::samplingFrequency>();
      break;
    }
  }
  for (auto ltv : cfg_b.aseConfiguration.codecConfiguration) {
    if (ltv.getTag() == CodecSpecificConfigurationLtv::samplingFrequency) {
      cfg_b_fr = ltv.get<CodecSpecificConfigurationLtv::samplingFrequency>();
      break;
    }
  }
  if (cfg_a_fr.has_value() && cfg_b_fr.has_value()) {
    return cfg_a_fr.value() != cfg_b_fr.value();
  }
  return false;
}

void AudioSetConfigurationProviderJson::PopulateAseConfigurationFromFlat(
    const le_audio::AudioSetConfiguration* flat_cfg,
    std::vector<const le_audio::CodecConfiguration*>* codec_cfgs,
    std::vector<const le_audio::QosConfiguration*>* qos_cfgs,
    CodecLocation location,
    std::vector<std::optional<AseDirectionConfiguration>>&
        sourceAseConfiguration,
    std::vector<std::optional<AseDirectionConfiguration>>& sinkAseConfiguration,
    ConfigurationFlags& configurationFlags) {
  if (flat_cfg == nullptr) {
    LOG(ERROR) << "flat_cfg cannot be null";
    return;
  }
  std::string codec_config_key = flat_cfg->codec_config_name()->str();
  auto* qos_config_key_array = flat_cfg->qos_config_name();

  constexpr std::string_view default_qos = "QoS_Config_Balanced_Reliability";

  std::string qos_sink_key(default_qos);
  std::string qos_source_key(default_qos);

  /* We expect maximum two QoS settings. First for Sink and second for Source
   */
  if (qos_config_key_array->size() > 0) {
    qos_sink_key = qos_config_key_array->Get(0)->str();
    if (qos_config_key_array->size() > 1) {
      qos_source_key = qos_config_key_array->Get(1)->str();
    } else {
      qos_source_key = qos_sink_key;
    }
  }

  LOG(INFO) << "Audio set config " << flat_cfg->name()->c_str()
            << ": codec config " << codec_config_key.c_str() << ", qos_sink "
            << qos_sink_key.c_str() << ", qos_source "
            << qos_source_key.c_str();

  // Find the first qos config that match the name
  const le_audio::QosConfiguration* qos_sink_cfg = nullptr;
  for (auto i = qos_cfgs->begin(); i != qos_cfgs->end(); ++i) {
    if ((*i)->name()->str() == qos_sink_key) {
      qos_sink_cfg = *i;
      break;
    }
  }

  const le_audio::QosConfiguration* qos_source_cfg = nullptr;
  for (auto i = qos_cfgs->begin(); i != qos_cfgs->end(); ++i) {
    if ((*i)->name()->str() == qos_source_key) {
      qos_source_cfg = *i;
      break;
    }
  }

  // First codec_cfg with the same name
  const le_audio::CodecConfiguration* codec_cfg = nullptr;
  for (auto i = codec_cfgs->begin(); i != codec_cfgs->end(); ++i) {
    if ((*i)->name()->str() == codec_config_key) {
      codec_cfg = *i;
      break;
    }
  }

  // Process each subconfig and put it into the correct list
  if (codec_cfg != nullptr && codec_cfg->subconfigurations()) {
    /* Load subconfigurations */
    for (auto subconfig : *codec_cfg->subconfigurations()) {
      if (subconfig->direction() == kLeAudioDirectionSink) {
        processSubconfig(subconfig, qos_sink_cfg, sinkAseConfiguration,
                         location, configurationFlags);
      } else {
        processSubconfig(subconfig, qos_source_cfg, sourceAseConfiguration,
                         location, configurationFlags);
      }
    }

    // After putting all subconfig, check if it's an asymmetric configuration
    // and populate information for ConfigurationFlags
    if (!sinkAseConfiguration.empty() && !sourceAseConfiguration.empty()) {
      for (int i = 0; i < sinkAseConfiguration.size(); ++i) {
        // Only check for comparable source and sink configuration.
        if (sourceAseConfiguration.size() <= i) break;
        if (sinkAseConfiguration[i].has_value() &&
            sourceAseConfiguration[i].has_value()) {
          // Has both direction, comparing inner fields:
          if (isAseConfigurationAsymmetrical(
                  sinkAseConfiguration[i].value(),
                  sourceAseConfiguration[i].value())) {
            configurationFlags.bitmask |=
                ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS;
            // Already detect asymmetrical config.
            break;
          }
        }
      }
    }
  } else {
    if (codec_cfg == nullptr) {
      LOG(ERROR) << "No codec config matching key " << codec_config_key.c_str()
                 << " found";
    } else {
      LOG(ERROR) << "Configuration '" << flat_cfg->name()->c_str()
                 << "' has no valid subconfigurations.";
    }
  }
}

bool AudioSetConfigurationProviderJson::LoadConfigurationsFromFiles(
    const char* schema_file, const char* content_file, CodecLocation location) {
  flatbuffers::Parser configurations_parser_;
  std::string configurations_schema_binary_content;
  bool ok = flatbuffers::LoadFile(schema_file, true,
                                  &configurations_schema_binary_content);
  LOG(INFO) << __func__ << ": Loading file " << schema_file;
  if (!ok) return ok;

  /* Load the binary schema */
  ok = configurations_parser_.Deserialize(
      (uint8_t*)configurations_schema_binary_content.c_str(),
      configurations_schema_binary_content.length());
  if (!ok) return ok;

  /* Load the content from JSON */
  std::string configurations_json_content;
  LOG(INFO) << __func__ << ": Loading file " << content_file;
  ok = flatbuffers::LoadFile(content_file, false, &configurations_json_content);
  if (!ok) return ok;

  /* Parse */
  LOG(INFO) << __func__ << ": Parse JSON content";
  ok = configurations_parser_.Parse(configurations_json_content.c_str());
  if (!ok) return ok;

  /* Import from flatbuffers */
  LOG(INFO) << __func__ << ": Build flat buffer structure";
  auto configurations_root = le_audio::GetAudioSetConfigurations(
      configurations_parser_.builder_.GetBufferPointer());
  if (!configurations_root) return false;

  auto flat_qos_configs = configurations_root->qos_configurations();
  if ((flat_qos_configs == nullptr) || (flat_qos_configs->size() == 0))
    return false;

  LOG(DEBUG) << ": Updating " << flat_qos_configs->size()
             << " qos config entries.";
  std::vector<const le_audio::QosConfiguration*> qos_cfgs;
  for (auto const& flat_qos_cfg : *flat_qos_configs) {
    qos_cfgs.push_back(flat_qos_cfg);
  }

  auto flat_codec_configs = configurations_root->codec_configurations();
  if ((flat_codec_configs == nullptr) || (flat_codec_configs->size() == 0))
    return false;

  LOG(DEBUG) << ": Updating " << flat_codec_configs->size()
             << " codec config entries.";
  std::vector<const le_audio::CodecConfiguration*> codec_cfgs;
  for (auto const& flat_codec_cfg : *flat_codec_configs) {
    codec_cfgs.push_back(flat_codec_cfg);
  }

  auto flat_configs = configurations_root->configurations();
  if ((flat_configs == nullptr) || (flat_configs->size() == 0)) return false;

  LOG(DEBUG) << ": Updating " << flat_configs->size() << " config entries.";
  for (auto const& flat_cfg : *flat_configs) {
    // Create 3 vector to use
    std::vector<std::optional<AseDirectionConfiguration>>
        sourceAseConfiguration;
    std::vector<std::optional<AseDirectionConfiguration>> sinkAseConfiguration;
    ConfigurationFlags configurationFlags;
    PopulateAseConfigurationFromFlat(flat_cfg, &codec_cfgs, &qos_cfgs, location,
                                     sourceAseConfiguration,
                                     sinkAseConfiguration, configurationFlags);
    if (sourceAseConfiguration.empty() && sinkAseConfiguration.empty())
      continue;
    configurations_[flat_cfg->name()->str()] = std::make_tuple(
        sourceAseConfiguration, sinkAseConfiguration, configurationFlags);
  }

  return true;
}

bool AudioSetConfigurationProviderJson::LoadScenariosFromFiles(
    const char* schema_file, const char* content_file) {
  flatbuffers::Parser scenarios_parser_;
  std::string scenarios_schema_binary_content;
  bool ok = flatbuffers::LoadFile(schema_file, true,
                                  &scenarios_schema_binary_content);
  LOG(INFO) << __func__ << ": Loading file " << schema_file;
  if (!ok) return ok;

  /* Load the binary schema */
  ok = scenarios_parser_.Deserialize(
      (uint8_t*)scenarios_schema_binary_content.c_str(),
      scenarios_schema_binary_content.length());
  if (!ok) return ok;

  /* Load the content from JSON */
  LOG(INFO) << __func__ << ": Loading file " << content_file;
  std::string scenarios_json_content;
  ok = flatbuffers::LoadFile(content_file, false, &scenarios_json_content);
  if (!ok) return ok;

  /* Parse */
  LOG(INFO) << __func__ << ": Parse json content";
  ok = scenarios_parser_.Parse(scenarios_json_content.c_str());
  if (!ok) return ok;

  /* Import from flatbuffers */
  LOG(INFO) << __func__ << ": Build flat buffer structure";
  auto scenarios_root = le_audio::GetAudioSetScenarios(
      scenarios_parser_.builder_.GetBufferPointer());
  if (!scenarios_root) return false;

  auto flat_scenarios = scenarios_root->scenarios();
  if ((flat_scenarios == nullptr) || (flat_scenarios->size() == 0))
    return false;

  LOG(INFO) << __func__ << ": Turn flat buffer into structure";
  AudioContext media_context = AudioContext();
  media_context.bitmask =
      (AudioContext::ALERTS | AudioContext::INSTRUCTIONAL |
       AudioContext::NOTIFICATIONS | AudioContext::EMERGENCY_ALARM |
       AudioContext::UNSPECIFIED | AudioContext::MEDIA |
       AudioContext::SOUND_EFFECTS);

  AudioContext conversational_context = AudioContext();
  conversational_context.bitmask =
      (AudioContext::RINGTONE_ALERTS | AudioContext::CONVERSATIONAL);

  AudioContext live_context = AudioContext();
  live_context.bitmask = AudioContext::LIVE_AUDIO;

  AudioContext game_context = AudioContext();
  game_context.bitmask = AudioContext::GAME;

  AudioContext voice_assistants_context = AudioContext();
  voice_assistants_context.bitmask = AudioContext::VOICE_ASSISTANTS;

  LOG(DEBUG) << "Updating " << flat_scenarios->size() << " scenarios.";
  for (auto const& scenario : *flat_scenarios) {
    if (!scenario->configurations()) continue;
    std::string scenario_name = scenario->name()->c_str();
    AudioContext context;
    if (scenario_name == "Media")
      context = AudioContext(media_context);
    else if (scenario_name == "Conversational")
      context = AudioContext(conversational_context);
    else if (scenario_name == "Live")
      context = AudioContext(live_context);
    else if (scenario_name == "Game")
      context = AudioContext(game_context);
    else if (scenario_name == "VoiceAssistants")
      context = AudioContext(voice_assistants_context);
    LOG(DEBUG) << "Scenario " << scenario->name()->c_str()
               << " configs: " << scenario->configurations()->size()
               << " context: " << context.toString();

    for (auto it = scenario->configurations()->begin();
         it != scenario->configurations()->end(); ++it) {
      auto config_name = it->str();
      auto configuration = configurations_.find(config_name);
      if (configuration == configurations_.end()) continue;
      LOG(DEBUG) << "Getting configuration with name: " << config_name;
      auto [source, sink, flags] = configuration->second;
      // Each configuration will create a LeAudioAseConfigurationSetting
      // with the same {context, packing}
      // and different data
      LeAudioAseConfigurationSetting setting;
      setting.audioContext = context;
      // TODO: Packing
      setting.sourceAseConfiguration = source;
      setting.sinkAseConfiguration = sink;
      setting.flags = flags;
      // Add to list of setting
      LOG(DEBUG) << "Pushing configuration to list: " << config_name;
      ase_configuration_settings_.push_back({config_name, setting});
    }
  }

  return true;
}

bool AudioSetConfigurationProviderJson::LoadContent(
    std::vector<std::pair<const char* /*schema*/, const char* /*content*/>>
        config_files,
    std::vector<std::pair<const char* /*schema*/, const char* /*content*/>>
        scenario_files,
    CodecLocation location) {
  bool is_loaded_config = false;
  for (auto [schema, content] : config_files) {
    if (LoadConfigurationsFromFiles(schema, content, location)) {
      is_loaded_config = true;
      break;
    }
  }

  bool is_loaded_scenario = false;
  for (auto [schema, content] : scenario_files) {
    if (LoadScenariosFromFiles(schema, content)) {
      is_loaded_scenario = true;
      break;
    }
  }
  return is_loaded_config && is_loaded_scenario;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
