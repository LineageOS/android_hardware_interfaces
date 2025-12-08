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

#define LOG_TAG "bluetooth_hal.fw_config"

#include "bluetooth_hal/config/firmware_config_loader.h"

#include <asm-generic/fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/config/config_constants.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "bluetooth_hal/util/system_call_wrapper.h"
#include "firmware_config.pb.h"
#include "google/protobuf/util/json_util.h"

namespace bluetooth_hal {
namespace config {
namespace {

using ::bluetooth_hal::config::proto::FirmwareConfigForTransport;
using ::bluetooth_hal::config::proto::FirmwareConfigsContainer;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::transport::TransportInterface;
using ::bluetooth_hal::transport::TransportType;
using ::bluetooth_hal::util::SystemCallWrapper;

using ::google::protobuf::RepeatedField;
using ::google::protobuf::util::JsonParseOptions;
using ::google::protobuf::util::JsonStringToMessage;

namespace cfg_consts = ::bluetooth_hal::config::constants;

enum class DataLoadingType : int {
  kByPacket = 0,
  kByAccumulation,
};

enum class DataReadingMethod {
  kCommandBased,
  kFixedSize,
};

constexpr uint16_t GetOpcode(std::span<const uint8_t> packet) {
  if (packet.size() < 2) {
    // Invalid packet.
    return 0;
  }
  return (packet[1] << 8) | packet[0];
}

}  // namespace

class FirmwareConfigLoaderImpl : public FirmwareConfigLoader {
 public:
  FirmwareConfigLoaderImpl();
  ~FirmwareConfigLoaderImpl() override = default;

  bool LoadConfig() override;
  bool LoadConfigFromFile(std::string_view path) override;
  bool LoadConfigFromString(std::string_view content) override;

  bool SelectFirmwareConfiguration(TransportType transport_type) override;

  bool ResetFirmwareDataLoadingState() override;

  std::optional<DataPacket> GetNextFirmwareData() override;

  std::optional<std::reference_wrapper<const SetupCommandPacket>>
  GetSetupCommandPacket(SetupCommandType command_type) const override;

  int GetLoadMiniDrvDelayMs() const override;
  int GetLaunchRamDelayMs() const override;
  size_t GetFirmwareFileCount() const override;

  std::string DumpConfigToString() const override;

 private:
  void LoadSetupCommandsFromConfig(
      const FirmwareConfigForTransport& config,
      std::unordered_map<SetupCommandType, std::unique_ptr<SetupCommandPacket>>&
          target_map);

  bool OpenNextFirmwareFile();

  std::optional<DataPacket> GetNextPacketByCommand();
  std::optional<DataPacket> GetNextPacketByFixedSize();
  std::optional<DataPacket> GetNextSinglePacket();
  std::optional<DataPacket> GetNextFirmwareDataByAccumulation();
  std::optional<DataPacket> GetNextFirmwareDataByPacket();

  std::unordered_map<TransportType, FirmwareConfigForTransport>
      transport_specific_configs_;
  std::optional<std::reference_wrapper<const FirmwareConfigForTransport>>
      active_config_;
  std::unordered_map<SetupCommandType, std::unique_ptr<SetupCommandPacket>>
      active_setup_commands_;

  std::mutex firmware_data_mutex_;
  std::vector<std::string> current_firmware_filenames_;
  int current_firmware_file_index_;

  std::optional<DataPacket> previous_packet_;
  int firmware_file_fd_{-1};

  DataReadingMethod data_reading_method_{DataReadingMethod::kCommandBased};
  uint16_t launch_ram_opcode_{cfg_consts::kDefaultHciVscLaunchRamOpcode};
  size_t fixed_chunk_size_{cfg_consts::kDefaultFixedChunkSize};
};

bool FirmwareConfigLoaderImpl::ResetFirmwareDataLoadingState() {
  if (!active_config_ || !active_config_->get().has_firmware_folder_name() ||
      current_firmware_filenames_.empty()) {
    LOG(ERROR) << __func__
               << ": No active config, firmware folder not set, or firmware "
                  "file list is empty.";
    return false;
  }

  std::scoped_lock lock(firmware_data_mutex_);
  if (firmware_file_fd_ != -1) {
    SystemCallWrapper::GetWrapper().Close(firmware_file_fd_);
    firmware_file_fd_ = -1;
  }

  current_firmware_file_index_ = -1;
  return OpenNextFirmwareFile();  // Attempt to open the first file.
}

bool FirmwareConfigLoaderImpl::OpenNextFirmwareFile() {
  if (firmware_file_fd_ != -1) {
    SystemCallWrapper::GetWrapper().Close(firmware_file_fd_);
    firmware_file_fd_ = -1;
  }
  previous_packet_.reset();  // Reset for accumulation mode for the new file.

  current_firmware_file_index_++;
  if (static_cast<size_t>(current_firmware_file_index_) >=
      current_firmware_filenames_.size()) {
    LOG(INFO) << __func__ << ": All firmware files processed.";
    return false;  // No more files
  }

  const std::string& current_file_name =
      current_firmware_filenames_[current_firmware_file_index_];
  const std::string firmware_path =
      active_config_->get().firmware_folder_name() + current_file_name;

  LOG(INFO) << __func__
            << ": Attempting to open firmware file: " << firmware_path;
  firmware_file_fd_ =
      SystemCallWrapper::GetWrapper().Open(firmware_path.c_str(), O_RDONLY);
  if (firmware_file_fd_ < 0) {
    LOG(ERROR) << __func__ << ": Cannot open firmware file: " << firmware_path
               << ". Error: " << strerror(errno);
    return false;
  }
  LOG(INFO) << __func__
            << ": Successfully opened firmware file: " << firmware_path;
  return true;
}

std::optional<DataPacket> FirmwareConfigLoaderImpl::GetNextFirmwareData() {
  std::scoped_lock lock(firmware_data_mutex_);

  if (!active_config_) {
    LOG(ERROR) << __func__ << ": No active firmware configuration selected.";
    return std::nullopt;
  }

  // If no file is currently open (or previous one ended), try opening the
  // next one.
  if (!previous_packet_ && firmware_file_fd_ == -1 && !OpenNextFirmwareFile()) {
    return std::nullopt;
  }

  if (!active_config_ ||
      !active_config_->get().has_firmware_data_loading_type()) {
    LOG(WARNING)
        << __func__
        << ": Data loading type not set, defaulting to PACKET_BY_PACKET.";
    return GetNextFirmwareDataByPacket();
  }

  DataLoadingType data_loading_type = static_cast<DataLoadingType>(
      active_config_->get().firmware_data_loading_type());

  switch (data_loading_type) {
    case DataLoadingType::kByAccumulation:
      return GetNextFirmwareDataByAccumulation();
    case DataLoadingType::kByPacket:
    default:
      return GetNextFirmwareDataByPacket();
  }
}

std::optional<std::reference_wrapper<const SetupCommandPacket>>
FirmwareConfigLoaderImpl::GetSetupCommandPacket(
    SetupCommandType command_type) const {
  if (!active_config_) {
    LOG(ERROR) << __func__ << ": No active firmware configuration selected.";
    return std::nullopt;
  }

  const auto iter = active_setup_commands_.find(command_type);
  if (iter == active_setup_commands_.end()) {
    return std::nullopt;
  }

  return std::cref(*iter->second);
}

int FirmwareConfigLoaderImpl::GetLoadMiniDrvDelayMs() const {
  if (!active_config_) {
    LOG(ERROR) << __func__ << ": No active firmware configuration selected.";
    return cfg_consts::kDefaultLoadMiniDrvDelayMs;
  }
  const auto& config = active_config_->get();
  return config.has_load_mini_drv_delay_ms()
             ? config.load_mini_drv_delay_ms()
             : cfg_consts::kDefaultLoadMiniDrvDelayMs;
}

int FirmwareConfigLoaderImpl::GetLaunchRamDelayMs() const {
  if (!active_config_) {
    LOG(ERROR) << __func__ << ": No active firmware configuration selected.";
    return cfg_consts::kDefaultLaunchRamDelayMs;
  }
  const auto& config = active_config_->get();
  return config.has_launch_ram_delay_ms()
             ? config.launch_ram_delay_ms()
             : cfg_consts::kDefaultLaunchRamDelayMs;
}

size_t FirmwareConfigLoaderImpl::GetFirmwareFileCount() const {
  if (!active_config_) {
    return 0;
  }
  return current_firmware_filenames_.size();
}

std::string FirmwareConfigLoaderImpl::DumpConfigToString() const {
  std::stringstream ss;
  ss << "--- FirmwareConfigLoaderImpl State ---\n";
  ss << "Loaded Transport Specific Configurations: "
     << transport_specific_configs_.size() << "\n";

  for (const auto& [transport_type, config] : transport_specific_configs_) {
    ss << "  Transport Type: " << static_cast<int>(transport_type) << "\n";
    ss << "    Firmware Folder: \"" << config.firmware_folder_name() << "\"\n";
    ss << "    Firmware Files:\n";
    if (config.firmware_file_name_size() > 0) {
      for (const std::string& fname : config.firmware_file_name()) {
        ss << "      - \"" << fname << "\"\n";
      }
    } else {
      ss << "      (None)\n";
    }
    ss << "    Chip ID: " << config.chip_id() << "\n";
    ss << "    Load MiniDrv Delay (ms): " << config.load_mini_drv_delay_ms()
       << "\n";
    ss << "    Launch RAM Delay (ms): " << config.launch_ram_delay_ms() << "\n";
    ss << "    Data Loading Type: "
       << FirmwareDataLoadingType_Name(config.firmware_data_loading_type())
       << "\n";
    switch (config.data_reading_method_case()) {
      case FirmwareConfigForTransport::kCommandBasedReading:
        ss << "    Data Reading Method: COMMAND_BASED\n";
        ss << "      Launch RAM Opcode: 0x" << std::hex
           << config.command_based_reading().launch_ram_opcode() << std::dec
           << "\n";
        break;

      case FirmwareConfigForTransport::kFixedSizeReading:
        ss << "    Data Reading Method: FIXED_SIZE\n";
        ss << "      Chunk Size: " << config.fixed_size_reading().chunk_size()
           << " bytes\n";
        break;

      case FirmwareConfigForTransport::DATA_READING_METHOD_NOT_SET:
      default:
        ss << "    Data Reading Method: (Default) COMMAND_BASED\n";
        break;
    }
    ss << "    Setup Commands Loaded:\n";
    if (active_setup_commands_.empty()) {
      ss << "      (None)\n";
    } else {
      for (const auto& [command_type, packet_ptr] : active_setup_commands_) {
        ss << "      - " << SetupCommandTypeToString(command_type) << ": "
           << (packet_ptr ? "Present" : "Absent") << "\n";
      }
    }
  }

  if (active_config_) {
    ss << "Active Configuration for Transport Type: "
       << active_config_->get().transport_type() << "\n";
  } else {
    ss << "No Active Firmware Configuration Selected.\n";
  }
  ss << "-------------------------------------\n";

  return ss.str();
}

FirmwareConfigLoaderImpl::FirmwareConfigLoaderImpl() {
#ifndef UNIT_TEST
  LoadConfig();
#endif
}

bool FirmwareConfigLoaderImpl::LoadConfig() {
  return LoadConfigFromFile(cfg_consts::kFirmwareConfigFile);
}

bool FirmwareConfigLoaderImpl::LoadConfigFromFile(std::string_view path) {
  std::ifstream json_file(path.data());
  if (!json_file.is_open()) {
    LOG(ERROR) << __func__ << ": Failed to open json file " << path.data();
    return false;
  }

  std::string json_str((std::istreambuf_iterator<char>(json_file)),
                       std::istreambuf_iterator<char>());

  return LoadConfigFromString(json_str);
}

bool FirmwareConfigLoaderImpl::LoadConfigFromString(std::string_view content) {
  FirmwareConfigsContainer container;
  JsonParseOptions options;
  options.ignore_unknown_fields = true;

  auto status = JsonStringToMessage(content, &container, options);
  if (!status.ok()) {
    LOG(ERROR) << __func__
               << ": Failed to parse json file, error: " << status.message();
    return false;
  }

  transport_specific_configs_.clear();
  for (const auto& config_entry : container.firmware_configs()) {
    auto type = static_cast<TransportType>(config_entry.transport_type());
    transport_specific_configs_[type] = config_entry;
  }

  const TransportType active_transport_type =
      TransportInterface::GetTransportType();
  if (active_transport_type != TransportType::kUnknown) {
    SelectFirmwareConfiguration(active_transport_type);
  } else {
    const auto& transport_type_priorities =
        HalConfigLoader::GetLoader().GetTransportTypePriority();
    for (auto type : transport_type_priorities) {
      if (transport_specific_configs_.find(type) !=
          transport_specific_configs_.end()) {
        SelectFirmwareConfiguration(type);
        break;
      }
    }
  }

  LOG(INFO) << DumpConfigToString();

  return true;
}

bool FirmwareConfigLoaderImpl::SelectFirmwareConfiguration(
    TransportType transport_type) {
  auto it = transport_specific_configs_.find(transport_type);
  if (it == transport_specific_configs_.end()) {
    LOG(ERROR) << __func__
               << ": No firmware configuration found for transport type "
               << static_cast<int>(transport_type);
    active_config_ = std::nullopt;
    active_setup_commands_.clear();
    return false;
  }

  active_config_ = std::cref(it->second);
  LOG(INFO) << __func__
            << ": Selected firmware configuration for transport type "
            << static_cast<int>(transport_type);

  current_firmware_filenames_.clear();
  const auto& config = active_config_->get();
  for (const std::string& fname : config.firmware_file_name()) {
    current_firmware_filenames_.push_back(fname);
  }
  current_firmware_file_index_ = -1;

  active_setup_commands_.clear();
  if (active_config_->get().has_setup_commands()) {
    LoadSetupCommandsFromConfig(active_config_->get(), active_setup_commands_);
  }

  // Configure data reading method.
  switch (config.data_reading_method_case()) {
    case FirmwareConfigForTransport::kCommandBasedReading:
      data_reading_method_ = DataReadingMethod::kCommandBased;
      launch_ram_opcode_ =
          config.command_based_reading().has_launch_ram_opcode()
              ? config.command_based_reading().launch_ram_opcode()
              : cfg_consts::kDefaultHciVscLaunchRamOpcode;
      LOG(INFO)
          << __func__
          << ": Data reading method set to COMMAND_BASED, Launch RAM Opcode: 0x"
          << std::hex << launch_ram_opcode_ << std::dec;
      break;

    case FirmwareConfigForTransport::kFixedSizeReading:
      data_reading_method_ = DataReadingMethod::kFixedSize;
      fixed_chunk_size_ = config.fixed_size_reading().has_chunk_size()
                              ? config.fixed_size_reading().chunk_size()
                              : cfg_consts::kDefaultFixedChunkSize;
      LOG(INFO) << __func__
                << ": Data reading method set to FIXED_SIZE, Chunk Size: "
                << fixed_chunk_size_ << " bytes";
      break;

    case FirmwareConfigForTransport::DATA_READING_METHOD_NOT_SET:
    default:
      data_reading_method_ = DataReadingMethod::kCommandBased;
      launch_ram_opcode_ = cfg_consts::kDefaultHciVscLaunchRamOpcode;
      LOG(INFO) << __func__
                << ": Data reading method not specified, defaulting to "
                   "COMMAND_BASED, Launch RAM Opcode: 0x"
                << std::hex << launch_ram_opcode_ << std::dec;
      break;
  }

  return true;
}

void FirmwareConfigLoaderImpl::LoadSetupCommandsFromConfig(
    const FirmwareConfigForTransport& config,
    std::unordered_map<SetupCommandType, std::unique_ptr<SetupCommandPacket>>&
        target_map) {
  const auto& commands = config.setup_commands();

  auto to_vector = [](const RepeatedField<uint32_t>& field) {
    return std::vector<uint8_t>(field.begin(), field.end());
  };

  auto add = [&](SetupCommandType type, const RepeatedField<uint32_t>& data) {
    target_map.emplace(
        type, std::make_unique<SetupCommandPacket>(type, to_vector(data)));
  };

  if (commands.hci_reset_size()) {
    add(SetupCommandType::kReset, commands.hci_reset());
  }
  if (commands.hci_read_chip_id_size()) {
    add(SetupCommandType::kReadChipId, commands.hci_read_chip_id());
  }
  if (commands.hci_update_chip_baud_rate_size()) {
    add(SetupCommandType::kUpdateChipBaudRate,
        commands.hci_update_chip_baud_rate());
  }
  if (commands.hci_set_fast_download_size()) {
    add(SetupCommandType::kSetFastDownload, commands.hci_set_fast_download());
  }
  if (commands.hci_download_minidrv_size()) {
    add(SetupCommandType::kDownloadMinidrv, commands.hci_download_minidrv());
  }
  if (commands.hci_read_fw_version_size()) {
    add(SetupCommandType::kReadFwVersion, commands.hci_read_fw_version());
  }
  if (commands.hci_setup_low_power_mode_size()) {
    add(SetupCommandType::kSetupLowPowerMode,
        commands.hci_setup_low_power_mode());
  }
  if (commands.hci_write_bd_address_size()) {
    add(SetupCommandType::kWriteBdAddress, commands.hci_write_bd_address());
  }
}

std::optional<DataPacket> FirmwareConfigLoaderImpl::GetNextPacketByCommand() {
  while (true) {
    if (firmware_file_fd_ == -1 && !OpenNextFirmwareFile()) {
      return std::nullopt;  // No more files or error opening.
    }

    // Read packet header (opcode and length).
    std::vector<uint8_t> header(3);
    ssize_t bytes_read =
        TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Read(
            firmware_file_fd_, header.data(), header.size()));

    if (bytes_read <= 0) {
      // End of current file or error.
      LOG(ERROR) << __func__ << ": Failed to read full header for packet in "
                 << current_firmware_filenames_[current_firmware_file_index_];
      SystemCallWrapper::GetWrapper().Close(firmware_file_fd_);
      firmware_file_fd_ = -1;
      // Attempt to open the next file.
      continue;
    }

    // Read remaining packet data.
    const size_t payload_size = header[2];
    std::vector<uint8_t> packet_payload(1 + header.size() + payload_size);
    packet_payload[0] = static_cast<uint8_t>(HciPacketType::kCommand);
    std::copy(header.begin(), header.end(), packet_payload.begin() + 1);

    ssize_t payload_bytes_read =
        TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Read(
            firmware_file_fd_, packet_payload.data() + 1 + header.size(),
            payload_size));

    if (payload_bytes_read != static_cast<ssize_t>(payload_size)) {
      // Incomplete packet or error.
      LOG(ERROR) << __func__ << ": Failed to read full payload for packet in "
                 << current_firmware_filenames_[current_firmware_file_index_];
      SystemCallWrapper::GetWrapper().Close(firmware_file_fd_);
      firmware_file_fd_ = -1;
      // Attempt to open the next file.
      continue;
    }

    bool is_launch_ram = (GetOpcode(std::span(header)) == launch_ram_opcode_);

    DataType packet_data_type = DataType::kDataFragment;
    if (is_launch_ram) {
      LOG(INFO) << __func__ << ": Launch RAM command found in file "
                << current_firmware_filenames_[current_firmware_file_index_];
      // This launch_ram packet is the end of the current file.
      SystemCallWrapper::GetWrapper().Close(firmware_file_fd_);
      firmware_file_fd_ = -1;
      packet_data_type = DataType::kDataEnd;
    }

    return DataPacket(packet_data_type, packet_payload);
  }
}

std::optional<DataPacket> FirmwareConfigLoaderImpl::GetNextPacketByFixedSize() {
  if (firmware_file_fd_ == -1) {
    if (!OpenNextFirmwareFile()) {
      return std::nullopt;
    }
  }

  std::vector<uint8_t> buffer(fixed_chunk_size_);
  ssize_t bytes_read = TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Read(
      firmware_file_fd_, buffer.data(), fixed_chunk_size_));

  if (bytes_read <= 0) {
    // End of stream or error.
    SystemCallWrapper::GetWrapper().Close(firmware_file_fd_);
    firmware_file_fd_ = -1;
    return std::nullopt;
  }

  buffer.resize(bytes_read);

  // If we read less than the requested chunk size, it means we hit EOF.
  bool is_end_of_current_file =
      (bytes_read < static_cast<ssize_t>(fixed_chunk_size_));

  if (is_end_of_current_file) {
    SystemCallWrapper::GetWrapper().Close(firmware_file_fd_);
    firmware_file_fd_ = -1;
    return DataPacket(DataType::kDataEnd, std::move(buffer));
  }

  return DataPacket(DataType::kDataFragment, std::move(buffer));
}

std::optional<DataPacket> FirmwareConfigLoaderImpl::GetNextSinglePacket() {
  return data_reading_method_ == DataReadingMethod::kCommandBased
             ? GetNextPacketByCommand()
             : GetNextPacketByFixedSize();
}

std::optional<DataPacket>
FirmwareConfigLoaderImpl::GetNextFirmwareDataByPacket() {
  return GetNextSinglePacket();
}

std::optional<DataPacket>
FirmwareConfigLoaderImpl::GetNextFirmwareDataByAccumulation() {
  std::vector<uint8_t> accumulated_buffer;
  constexpr int kBufferSize = 32 * 1024;
  accumulated_buffer.reserve(kBufferSize);

  if (previous_packet_.has_value()) {
    if (previous_packet_->GetDataType() == DataType::kDataEnd) {
      DataPacket result = std::move(previous_packet_.value());
      previous_packet_.reset();
      return result;
    }

    accumulated_buffer.insert(accumulated_buffer.end(),
                              previous_packet_->GetPayload().begin(),
                              previous_packet_->GetPayload().end());
    previous_packet_.reset();
  }

  while (accumulated_buffer.size() <= kBufferSize) {
    std::optional<DataPacket> next_packet = GetNextSinglePacket();

    if (!next_packet.has_value()) {
      break;
    }

    if ((next_packet->GetDataType() == DataType::kDataEnd &&
         !accumulated_buffer.empty()) ||
        (accumulated_buffer.size() + next_packet->GetPayload().size() >
         kBufferSize)) {
      previous_packet_ = std::move(next_packet);
      break;
    }

    if (next_packet->GetDataType() == DataType::kDataEnd) {
      return DataPacket(DataType::kDataEnd,
                        std::move(next_packet->GetPayload()));
    }

    accumulated_buffer.insert(accumulated_buffer.end(),
                              next_packet->GetPayload().begin(),
                              next_packet->GetPayload().end());
  }

  if (!accumulated_buffer.empty()) {
    return DataPacket(DataType::kDataFragment, std::move(accumulated_buffer));
  }

  return std::nullopt;
}

FirmwareConfigLoader& FirmwareConfigLoader::GetLoader() {
  std::lock_guard<std::mutex> lock(loader_mutex_);
  if (loader_ == nullptr) {
    loader_ = new FirmwareConfigLoaderImpl();
  }
  return *loader_;
}

void FirmwareConfigLoader::ResetLoader() {
  std::lock_guard<std::mutex> lock(loader_mutex_);
  if (loader_ != nullptr) {
    delete loader_;
    loader_ = nullptr;
  }
}

}  // namespace config
}  // namespace bluetooth_hal
