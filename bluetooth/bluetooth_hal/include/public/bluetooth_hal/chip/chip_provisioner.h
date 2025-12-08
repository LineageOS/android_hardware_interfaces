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

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <optional>

#include "bluetooth_hal/chip/chip_provisioner_interface.h"
#include "bluetooth_hal/config/firmware_config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace chip {

class ChipProvisioner : public ChipProvisionerInterface,
                        public ::bluetooth_hal::hci::HciRouterClient {
 public:
  // Defines the states for the firmware provisioning state machine.
  enum class ProvisioningState {
    kIdle,
    kInitialReset,
    kReadChipId,
    kSetRuntimeBaudRate,
    kCheckFirmwareStatus,
    kSetFastDownload,
    kDownloadMinidrv,
    kWriteFirmware,
    kFinalReset,
    kReadFwVersion,
    kWriteBdAddress,
    kSetupLowPowerMode,
    kDone,
    kError,
  };

  ChipProvisioner()
      : config_loader_(
            ::bluetooth_hal::config::FirmwareConfigLoader::GetLoader()) {}

  /**
   * Initializes the HAL state update mechanism.
   *
   * @param on_hal_state_update A callback function that is invoked
   *        when the HAL state changes.
   */
  void Initialize(const std::function<void(::bluetooth_hal::HalState)>
                      on_hal_state_update) override;

  /**
   * Downloads the chip firmware.
   *
   * This function initiates the firmware download process for the chip.
   * It is a blocking call and returns only after the firmware download
   * has completed.
   *
   * @return `true` if the firmware download completes successfully.
   *         `false` if the firmware download fails.
   */
  bool DownloadFirmware() override;

  /**
   * Resets the chip firmware.
   *
   * This function resets the firmware on the chip using an HCI reset command.
   * It is a blocking call and returns only after receiving events from the
   * chip.
   *
   * @return `true` if the firmware reset is successful.
   *         `false` if the firmware reset fails.
   */
  bool ResetFirmware() override;

 protected:
  // HciRouterClient overrides.
  void OnCommandCallback(
      const ::bluetooth_hal::hci::HalPacket& callback_event) override;
  void OnBluetoothEnabled() override {};
  void OnBluetoothDisabled() override {};
  void OnBluetoothChipReady() override {};
  void OnBluetoothChipClosed() override {};
  void OnMonitorPacketCallback(
      [[maybe_unused]] ::bluetooth_hal::hci::MonitorMode mode,
      [[maybe_unused]] const ::bluetooth_hal::hci::HalPacket& packet) override {
  };

  void UpdateHalState(::bluetooth_hal::HalState state);
  bool ExecuteCurrentSetupStep(
      ::bluetooth_hal::config::SetupCommandType next_command_type);
  bool SendCommandNoAck(const ::bluetooth_hal::hci::HalPacket& packet);
  bool SendCommandAndWait(const ::bluetooth_hal::hci::HalPacket& packet);
  bool ProvisionBluetoothAddress();
  std::optional<::bluetooth_hal::hci::HalPacket> PrepareWriteBdAddressPacket();

  virtual bool WriteFwPatchramPacket();

 private:
  void RunProvisioningSequence();

  std::optional<std::function<void(::bluetooth_hal::HalState)>>
      on_hal_state_update_;
  ::bluetooth_hal::config::FirmwareConfigLoader& config_loader_;
  static constexpr size_t kBluetoothAddressLength = 6;
  std::array<uint8_t, kBluetoothAddressLength> bdaddr_;

  std::promise<void> command_promise_;
  bool firmware_command_success_;
  ProvisioningState state_{ProvisioningState::kIdle};
};

}  // namespace chip
}  // namespace bluetooth_hal
