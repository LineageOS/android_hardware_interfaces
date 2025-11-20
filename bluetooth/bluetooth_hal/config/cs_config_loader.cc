/*
 * Copyright 2025 The Android Open Source Project
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

#define LOG_TAG "bthal.cs_config"

#include "bluetooth_hal/config/cs_config_loader.h"

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "cs_config.pb.h"
#include "google/protobuf/util/json_util.h"

namespace bluetooth_hal {
namespace config {

using ::bluetooth_hal::config::proto::CalibrationCommands;
using ::bluetooth_hal::hci::HalPacket;

using ::google::protobuf::util::JsonParseOptions;
using ::google::protobuf::util::JsonStringToMessage;
using ::google::protobuf::util::Status;

constexpr std::string_view kCsConfigFile =
    "/vendor/etc/bluetooth/bluetooth_channel_sounding_calibration.json";

class CsConfigLoaderImpl : public CsConfigLoader {
 public:
  CsConfigLoaderImpl();

  bool LoadConfig() override;
  bool LoadConfigFromFile(std::string_view path) override;
  bool LoadConfigFromString(std::string_view content) override;

  const std::vector<HalPacket>& GetCsCalibrationCommands() const override;

  std::string DumpConfigToString() const override;

 private:
  std::vector<HalPacket> cs_calibration_commands_;
};

CsConfigLoaderImpl::CsConfigLoaderImpl() {
#ifndef UNIT_TEST
  LoadConfig();
#endif
}

bool CsConfigLoaderImpl::LoadConfig() {
  return LoadConfigFromFile(kCsConfigFile);
}

bool CsConfigLoaderImpl::LoadConfigFromFile(std::string_view path) {
  std::ifstream json_file(path.data());
  if (!json_file.is_open()) {
    LOG(ERROR) << __func__ << ": Failed to open json file " << path.data();
    return false;
  }

  std::string json_str((std::istreambuf_iterator<char>(json_file)),
                       std::istreambuf_iterator<char>());

  return LoadConfigFromString(json_str);
}

bool CsConfigLoaderImpl::LoadConfigFromString(std::string_view content) {
  CalibrationCommands calibration_commands;
  JsonParseOptions options;
  options.ignore_unknown_fields = true;

  Status status = JsonStringToMessage(content, &calibration_commands, options);
  if (!status.ok()) {
    LOG(ERROR) << __func__
               << ": Failed to parse json content, error: " << status.message();
    return false;
  }

  cs_calibration_commands_.clear();

  for (const auto& command : calibration_commands.commands()) {
    HalPacket packet;

    packet.push_back(command.packet_type());
    packet.push_back(command.opcode() & 0xff);
    packet.push_back((command.opcode() >> 8u) & 0xff);
    packet.push_back(command.payload_length());

    packet.insert(packet.end(), command.sub_opcode().begin(),
                  command.sub_opcode().end());
    packet.insert(packet.end(), command.data().begin(), command.data().end());

    cs_calibration_commands_.emplace_back(packet);
  }

  LOG(INFO) << DumpConfigToString();

  return true;
}

const std::vector<HalPacket>& CsConfigLoaderImpl::GetCsCalibrationCommands()
    const {
  return cs_calibration_commands_;
}

std::string CsConfigLoaderImpl::DumpConfigToString() const {
  std::stringstream ss;
  ss << "--- CsConfigLoaderImpl State ---\n";
  ss << "CS Calibration Commands Loaded: " << cs_calibration_commands_.size()
     << " command(s)\n";
  ss << "-------------------------------\n";
  return ss.str();
}

CsConfigLoader& CsConfigLoader::GetLoader() {
  static CsConfigLoaderImpl loader;
  return loader;
}

}  // namespace config
}  // namespace bluetooth_hal
