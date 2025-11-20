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

#define LOG_TAG "bthal.chip_provisioner"

#include "bluetooth_hal/chip/chip_provisioner.h"

#include <chrono>
#include <fstream>
#include <future>
#include <sstream>
#include <thread>

#include "android-base/logging.h"
#include "android-base/properties.h"
#include "android-base/stringprintf.h"
#include "bluetooth_hal/config/firmware_config_loader.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_router.h"

namespace bluetooth_hal {
namespace chip {
namespace {

using ::android::base::GetProperty;
using ::android::base::StringPrintf;
using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::DataPacket;
using ::bluetooth_hal::config::DataType;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::config::SetupCommandType;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::HciRouter;

constexpr char kDevinfoNodePath[] = "/proc/device-tree/chosen/config/bt_addr";
constexpr char kRandGenBdaddrPath[] =
    "/mnt/vendor/persist/bluetooth/bdaddr.txt";
constexpr char kEvbDefaultBdaddrProp[] = "ro.vendor.bluetooth.evb_bdaddr";
constexpr uint16_t kHciVscWriteBdAddress = 0xfc01;
constexpr uint16_t kHciVscWriteBdAddressLength = 0x0A;

}  // namespace

void ChipProvisioner::Initialize(
    const std::function<void(HalState)> on_hal_state_update) {
  on_hal_state_update_ = std::move(on_hal_state_update);
}

bool ChipProvisioner::DownloadFirmware() {
  LOG(INFO) << __func__;

  HandleNextSetupStep(SetupCommandType::kReset);

  if (HciRouter::GetRouter().GetHalState() < HalState::kBtChipReady) {
    // TODO: b/372148907 - Need to report error (kill self if needed).
    LOG(FATAL) << __func__ << ": Failed to complete download firmware.";
    return false;
  }
  return true;
}

bool ChipProvisioner::ResetFirmware() {
  LOG(INFO) << __func__;
  if (!ExecuteCurrentSetupStep(SetupCommandType::kReset)) {
    LOG(ERROR) << __func__ << ": Failed to reset firmware.";
    // TODO: b/372148907 - Need to report error (kill self if needed).
    return false;
  }

  switch (HciRouter::GetRouter().GetHalState()) {
    case HalState::kBtChipReady:
      UpdateHalState(HalState::kRunning);
      break;
    case HalState::kRunning:
      UpdateHalState(HalState::kBtChipReady);
      break;
    default:
      // TODO: b/372148907 - Need to report error (kill self if needed).
      return false;
  }

  return true;
}

bool ChipProvisioner::ExecuteCurrentSetupStep(SetupCommandType command_type) {
  // Get the next firmware setup command via command type.
  auto next_setup_command = config_loader_.GetSetupCommandPacket(command_type);
  if (!next_setup_command.has_value()) {
    LOG(ERROR) << __func__ << ": Failed to get next setup command.";
    return false;
  }

  return SendCommandAndWait(next_setup_command->get().GetPayload());
}

bool ChipProvisioner::SendCommandAndWait(const HalPacket& packet) {
  if (!SendCommand(packet)) {
    LOG(ERROR) << __func__ << ": Failed to send next setup command.";
    return false;
  }

  std::future_status status =
      command_promise_.get_future().wait_for(std::chrono::milliseconds(1000));
  if (status != std::future_status::ready) {
    LOG(ERROR) << __func__ << ": Command timeout during download firmware.";
    return false;
  }
  command_promise_ = std::promise<void>();
  return firmware_command_success_;
}

void ChipProvisioner::OnCommandCallback(const HalPacket& callback_event) {
  bool success = (callback_event.GetCommandCompleteEventResult() ==
                  static_cast<uint8_t>(EventResultCode::kSuccess));
  LOG(success ? INFO : WARNING)
      << __func__ << ": Recv VSE <" << callback_event.ToString() << "> "
      << (success ? "[Success]" : "[Failed]");
  firmware_command_success_ = success;
  command_promise_.set_value();
}

bool ChipProvisioner::ProvisionBluetoothAddress() {
  LOG(INFO) << __func__;
  std::string bdaddr_str;
  std::fstream devinfo(kDevinfoNodePath, std::ios::in);
  std::fstream randgen(kRandGenBdaddrPath, std::ios::in);
  if (devinfo.is_open()) {
    std::getline(devinfo, bdaddr_str);
  } else if (randgen.is_open()) {
    std::getline(randgen, bdaddr_str);
  } else {
    bdaddr_str = GetProperty(kEvbDefaultBdaddrProp, "");
  }

  if (bdaddr_str.empty()) {
    LOG(ERROR) << __func__ << "Can't fetch the provisioning BDA (empty string)";
    return false;
  }

  unsigned char trailing_char = '\0';
  auto try_parse = [&](const char* fmt) {
    return std::sscanf(bdaddr_str.data(), fmt, &bdaddr_[5], &bdaddr_[4],
                       &bdaddr_[3], &bdaddr_[2], &bdaddr_[1], &bdaddr_[0],
                       &trailing_char) == kBluetoothAddressLength;
  };

  bool success = try_parse("%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%c") ||
                 try_parse("%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%c");

  if (!success) {
    LOG(ERROR) << __func__
               << "Can't fetch the provisioning BDA (invalid format)";
    return false;
  }

  LOG(INFO) << __func__ << ": BDADDR <"
            << StringPrintf("xx:xx:xx:xx:%02x:%02x", bdaddr_[1], bdaddr_[0])
            << ">";

  std::optional<HalPacket> write_bda_packet = PrepareWriteBdAddressPacket();
  if (write_bda_packet.has_value()) {
    if (!SendCommandAndWait(write_bda_packet.value())) {
      LOG(ERROR) << __func__
                 << "Failed to send write Bluetooth address command.";
      return false;
    }
    return true;
  } else {
    LOG(ERROR) << __func__
               << "Failed to prepare write Bluetooth address packet.";
    return false;
  }
}

std::optional<HalPacket> ChipProvisioner::PrepareWriteBdAddressPacket() {
  if (bdaddr_.size() != kBluetoothAddressLength) {
    LOG(ERROR) << __func__ << ": Invalid Bluetooth address length";
    return std::nullopt;
  }

  HalPacket write_bda_vsc;
  write_bda_vsc.resize(kHciVscWriteBdAddressLength);

  // Prepare the HalPacket elements for the WriteBdAddress command.
  write_bda_vsc[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  write_bda_vsc[1] = kHciVscWriteBdAddress & 0xff;
  write_bda_vsc[2] = (kHciVscWriteBdAddress >> 8u) & 0xff;
  write_bda_vsc[3] = kBluetoothAddressLength;

  memcpy(write_bda_vsc.data() + 4, bdaddr_.data(), kBluetoothAddressLength);

  std::stringstream ss;
  for (uint8_t byte : write_bda_vsc) {
    ss << StringPrintf("%02x", byte);
  }
  LOG(INFO) << __func__ << ": Prepared VSC <" << ss.str() << ">";

  return write_bda_vsc;
}

void ChipProvisioner::UpdateHalState(HalState status) {
  // Check if a callback is present.
  if (on_hal_state_update_.has_value()) {
    on_hal_state_update_.value()(status);
  } else {
    LOG(WARNING) << "No download callback registered.";
  }
}

bool ChipProvisioner::SendCommandNoAck(const HalPacket& packet) {
  if (packet.GetType() != HciPacketType::kCommand) {
    LOG(WARNING) << __func__ << ": Invalid Packet Type.";
    return false;
  }
  return HciRouter::GetRouter().SendCommandNoAck(packet);
}

void ChipProvisioner::HandleNextSetupStep(SetupCommandType command) {
  switch (command) {
    case SetupCommandType::kReset:
      LOG(INFO) << __func__ << ": Reset";
      if (ExecuteCurrentSetupStep(command)) {
        HandleNextSetupStep(SetupCommandType::kReadChipId);
      }
      break;
    case SetupCommandType::kReadChipId:
      LOG(INFO) << __func__ << ": ReadChipId";
      if (ExecuteCurrentSetupStep(command)) {
        HandleNextSetupStep(SetupCommandType::kUpdateChipBaudRate);
      }
      break;
    case SetupCommandType::kUpdateChipBaudRate:
      if (ExecuteCurrentSetupStep(command)) {
        if (HciRouter::GetRouter().GetHalState() ==
            HalState::kFirmwareDownloadCompleted) {
          LOG(INFO) << __func__ << ": [ FirmwareReady ]";
          UpdateHalState(HalState::kFirmwareReady);
          HandleNextSetupStep(SetupCommandType::kReadFwVersion);
        } else {
          LOG(INFO) << __func__ << ": [ FirmwareDownloading ]";
          UpdateHalState(HalState::kFirmwareDownloading);
          HandleNextSetupStep(SetupCommandType::kSetFastDownload);
        }
      }
      break;
    case SetupCommandType::kSetFastDownload:
      LOG(INFO) << __func__ << ": SetFastDownload";
      if (ExecuteCurrentSetupStep(command)) {
        HandleNextSetupStep(SetupCommandType::kDownloadMinidrv);
      }
      break;
    case SetupCommandType::kDownloadMinidrv: {
      if (ExecuteCurrentSetupStep(command)) {
        // Add delay time for placing firmware in download mode
        int mini_drv_delay_ms = config_loader_.GetLoadMiniDrvDelayMs();
        std::this_thread::sleep_for(
            std::chrono::milliseconds(mini_drv_delay_ms));
        LOG(INFO) << __func__ << ":Writing firmware patchram";
        // Write firmware patchram packets.
        if (!WriteFwPatchramPacket()) {
          LOG(ERROR) << __func__
                     << ": Failed to write Firmware PatchRam Packets.";
          return;
        }
        HandleNextSetupStep(SetupCommandType::kLaunchRam);
      }
      break;
    }
    case SetupCommandType::kLaunchRam: {
      if (ExecuteCurrentSetupStep(command)) {
        // Add settlement time after launching ram.
        int launch_ram_delay_ms = config_loader_.GetLaunchRamDelayMs();
        std::this_thread::sleep_for(
            std::chrono::milliseconds(launch_ram_delay_ms));

        LOG(INFO) << "[ FirmwareDownloadCompleted ]";
        UpdateHalState(HalState::kFirmwareDownloadCompleted);
        HandleNextSetupStep(SetupCommandType::kReset);
      }
      break;
    }
    case SetupCommandType::kReadFwVersion:
      if (ExecuteCurrentSetupStep(command)) {
        // TODO: Extract firmware version from the callback event.
        LOG(INFO) << "ReadFwVersion";
        HandleNextSetupStep(SetupCommandType::kWriteBdAddress);
      }
      break;
    case SetupCommandType::kWriteBdAddress:
      LOG(INFO) << "writing BDA to controller";
      if (!ProvisionBluetoothAddress()) {
        LOG(ERROR) << __func__
                   << ": Failed to provision and write Bluetooth address.";
        // TODO: b/409658769 - Force to abort hal service and report issue.
      }
      HandleNextSetupStep(SetupCommandType::kSetupLowPowerMode);
      break;
    case SetupCommandType::kSetupLowPowerMode:
      if (HalConfigLoader::GetLoader().IsLowPowerModeSupported()) {
        if (!ExecuteCurrentSetupStep(command)) {
          // Command failed, no need to proceed to BtChipReady
          LOG(ERROR) << __func__ << ": Failed to send low power mode command.";
          break;  // Exit the case early
        }
      } else {
        LOG(WARNING) << "Low power mode is disabled!";
      }
      // End of the firmware downloading process.
      LOG(INFO) << "[ BtChipReady ]";
      UpdateHalState(HalState::kBtChipReady);
      break;
    default:
      LOG(ERROR) << __func__ << ": Unknown setup command type.";
      return;
  }
}

bool ChipProvisioner::WriteFwPatchramPacket() {
  if (!config_loader_.ResetFirmwareDataLoadingState()) {
    LOG(ERROR) << __func__ << ": Failed to load firmware data";
    return false;
  }

  std::optional<DataPacket> data_packet;
  while ((data_packet = config_loader_.GetNextFirmwareData()).has_value()) {
    if ((*data_packet).GetDataType() == config::DataType::kDataEnd) {
      // The last firmware patchram packet is expected be launchram command.
      LOG(INFO) << " the latest patchram packet: "
                << (*data_packet).GetPayload().ToString();
      break;
    }
    if (!SendCommandNoAck((*data_packet).GetPayload())) {
      LOG(ERROR) << __func__ << ": Failed to send firmware data packet";
      return false;
    }
  }
  return true;
}

}  // namespace chip
}  // namespace bluetooth_hal
