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

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bluetooth_hal/config/config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace config {

enum class FirmwarePacketType : int {
  kSetupCommand = 0,
  kData,
};

enum class SetupCommandType : int {
  kReset = 0,
  kReadChipId,
  kUpdateChipBaudRate,
  kSetFastDownload,
  kDownloadMinidrv,
  kReadFwVersion,
  kSetupLowPowerMode,
  kWriteBdAddress,
};

inline constexpr std::string_view SetupCommandTypeToString(
    SetupCommandType type) {
  switch (type) {
    case SetupCommandType::kReset:
      return "Reset";
    case SetupCommandType::kReadChipId:
      return "ReadChipId";
    case SetupCommandType::kUpdateChipBaudRate:
      return "UpdateChipBaudRate";
    case SetupCommandType::kSetFastDownload:
      return "SetFastDownload";
    case SetupCommandType::kDownloadMinidrv:
      return "DownloadMinidrv";
    case SetupCommandType::kReadFwVersion:
      return "ReadFwVersion";
    case SetupCommandType::kSetupLowPowerMode:
      return "SetupLowPowerMode";
    case SetupCommandType::kWriteBdAddress:
      return "WriteBdAddress";
    default:
      return "Unknown";
  }
}

enum class DataType : int { kDataFragment = 0, kDataEnd };

class FirmwarePacket {
 public:
  FirmwarePacket(FirmwarePacketType packet_type, std::vector<uint8_t> payload)
      : packet_type_(packet_type), payload_(std::move(payload)) {}

  virtual ~FirmwarePacket() = default;

  FirmwarePacketType GetPacketType() const { return packet_type_; }

  const ::bluetooth_hal::hci::HalPacket& GetPayload() const { return payload_; }

 protected:
  FirmwarePacketType packet_type_;
  ::bluetooth_hal::hci::HalPacket payload_;
};

class SetupCommandPacket : public FirmwarePacket {
 public:
  SetupCommandPacket(SetupCommandType command_type,
                     std::vector<uint8_t> payload)
      : FirmwarePacket(FirmwarePacketType::kSetupCommand, std::move(payload)),
        command_type_(command_type) {}

  SetupCommandType GetCommandType() const { return command_type_; }

 private:
  SetupCommandType command_type_;
};

class DataPacket : public FirmwarePacket {
 public:
  DataPacket(DataType data_type, std::vector<uint8_t> payload)
      : FirmwarePacket(FirmwarePacketType::kData, std::move(payload)),
        data_type_(data_type) {}

  DataType GetDataType() const { return data_type_; }

 private:
  DataType data_type_;
};

/**
 * @brief Manages the configuration and loading process for firmware.
 *
 * This class provides an interface for loading firmware data, retrieving
 * setup commands, and managing the state of the firmware loading process.
 * It follows the Singleton design pattern to ensure a single point of
 * access for firmware configuration.
 */
class FirmwareConfigLoader : public ConfigLoader {
 public:
  virtual ~FirmwareConfigLoader() = default;

  virtual bool LoadConfig() override = 0;
  virtual std::string DumpConfigToString() const override = 0;

  /**
   * @brief Selects the firmware configuration for a given transport type.
   *
   * This method sets the internal active configuration to the one matching
   * the provided transport_type. Subsequent calls to getters will use this
   * active configuration.
   *
   * @param transport_type The transport type to select configuration for.
   *
   * @return true if a configuration was found and selected, false otherwise.
   */
  virtual bool SelectFirmwareConfiguration(
      ::bluetooth_hal::transport::TransportType transport_type) = 0;

  /**
   * @brief Resets the state of the firmware data loading process.
   *
   * This function should be called before attempting to load firmware data
   * using GetNextFirmwareData(). It reinitializes the internal state
   * of the firmware loading mechanism, allowing for a fresh start. If the
   * firmware file cannot be opened, this function will return false.
   *
   * @note If ResetFirmwareDataLoadingState() is not called,
   * GetNextFirmwareData() will always return null.
   * @note ResetFirmwareDataLoadingState() = false indicates that the
   * firmware file could not be opened.
   *
   * @return True if the state was successfully reset, false otherwise.
   */
  virtual bool ResetFirmwareDataLoadingState() = 0;

  /**
   * @brief Retrieves the next chunk of firmware data.
   *
   * This function returns the next available chunk of data from the
   * firmware file. It should be called repeatedly until it returns null,
   * indicating that all firmware data has been read.
   *
   * @note This function will always return null if
   * ResetFirmwareDataLoadingState() has not been called or if it has
   * returned false.
   * @note After all data from the firmware file has been read, this
   * function will return null. To read the data again, you must call
   * ResetFirmwareDataLoadingState().
   *
   * @return An optional DataPacket containing the next chunk of firmware
   *         data, or null if no more data is available or if the loading
   *         state has not been reset.
   */
  virtual std::optional<DataPacket> GetNextFirmwareData() = 0;

  /**
   * @brief Retrieves a specific setup command packet.
   *
   * This function returns the setup command packet associated with the
   * given command type.
   *
   * @param command_type The type of the setup command to retrieve.
   *
   * @return An optional const reference to the SetupCommandPacket, or
   *         nullopt if no command is found for the given type.
   */
  virtual std::optional<std::reference_wrapper<const SetupCommandPacket>>
  GetSetupCommandPacket(SetupCommandType command_type) const = 0;

  /**
   * @brief Retrieves the delay in ms for loading the minidriver.
   *
   * @return The delay in milliseconds.
   */
  virtual int GetLoadMiniDrvDelayMs() const = 0;

  /**
   * @brief Retrieves the delay in ms for launching RAM.
   *
   * @return The delay in milliseconds.
   */
  virtual int GetLaunchRamDelayMs() const = 0;

  /**
   * @brief Retrieves the number of configured firmware files.
   *
   * @return The count of firmware files for the active configuration.
   */
  virtual size_t GetFirmwareFileCount() const = 0;

  static FirmwareConfigLoader& GetLoader();

  static void ResetLoader();

 private:
  static inline FirmwareConfigLoader* loader_{nullptr};
  static inline std::mutex loader_mutex_;
};

}  // namespace config
}  // namespace bluetooth_hal
