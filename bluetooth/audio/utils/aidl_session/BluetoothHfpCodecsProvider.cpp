/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "BluetoothHfpCodecsProvider.h"

#include <unordered_map>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using hfp::setting::CodecType;
using hfp::setting::PathConfiguration;

static const char* kHfpCodecCapabilitiesFile =
    "/vendor/etc/aidl/hfp/hfp_codec_capabilities.xml";

std::optional<HfpOffloadSetting>
BluetoothHfpCodecsProvider::ParseFromHfpOffloadSettingFile() {
  auto hfp_offload_setting =
      hfp::setting::readHfpOffloadSetting(kHfpCodecCapabilitiesFile);
  if (!hfp_offload_setting.has_value()) {
    LOG(ERROR) << __func__ << ": Failed to read " << kHfpCodecCapabilitiesFile;
  }
  return hfp_offload_setting;
}

std::vector<CodecInfo> BluetoothHfpCodecsProvider::GetHfpAudioCodecInfo(
    const std::optional<HfpOffloadSetting>& hfp_offload_setting) {
  std::vector<CodecInfo> result;
  if (!hfp_offload_setting.has_value()) return result;

  // Convert path configuration into map
  // Currently transport configuration is unused
  if (!hfp_offload_setting.value().hasPathConfiguration() ||
      hfp_offload_setting.value().getPathConfiguration().empty()) {
    LOG(WARNING) << __func__ << ": path configurations is empty";
    return result;
  }
  auto path_configurations = hfp_offload_setting.value().getPathConfiguration();
  std::unordered_map<std::string, PathConfiguration> path_config_map;
  for (const auto& path_cfg : path_configurations)
    if (path_cfg.hasName() && path_cfg.hasDataPath())
      path_config_map.insert(make_pair(path_cfg.getName(), path_cfg));

  for (const auto& cfg : hfp_offload_setting.value().getConfiguration()) {
    auto input_path_cfg = path_config_map.find(cfg.getInputPathConfiguration());
    auto output_path_cfg =
        path_config_map.find(cfg.getOutputPathConfiguration());
    if (input_path_cfg == path_config_map.end()) {
      LOG(WARNING) << __func__ << ": Input path configuration not found: "
                   << cfg.getInputPathConfiguration();
      continue;
    }

    if (output_path_cfg == path_config_map.end()) {
      LOG(WARNING) << __func__ << ": Output path configuration not found: "
                   << cfg.getOutputPathConfiguration();
      continue;
    }

    CodecInfo codec_info;

    switch (cfg.getCodec()) {
      case CodecType::LC3:
        codec_info.id = CodecId::Core::LC3;
        break;
      case CodecType::MSBC:
        codec_info.id = CodecId::Core::MSBC;
        break;
      case CodecType::CVSD:
        codec_info.id = CodecId::Core::CVSD;
        break;
      default:
        LOG(WARNING) << __func__ << ": Unknown codec from " << cfg.getName();
        codec_info.id = CodecId::Vendor();
        break;
    }
    codec_info.name = cfg.getName();

    codec_info.transport =
        CodecInfo::Transport::make<CodecInfo::Transport::Tag::hfp>();

    auto& transport =
        codec_info.transport.get<CodecInfo::Transport::Tag::hfp>();
    transport.useControllerCodec = cfg.getUseControllerCodec();
    transport.inputDataPath = input_path_cfg->second.getDataPath();
    transport.outputDataPath = output_path_cfg->second.getDataPath();

    result.push_back(codec_info);
  }
  LOG(INFO) << __func__ << ": Has " << result.size() << " codec info";
  return result;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
