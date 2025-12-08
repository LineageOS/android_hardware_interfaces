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

#define LOG_TAG "bluetooth_hal.chip_provisioner"

#include "bluetooth_hal/chip/async_chip_provisioner.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "android-base/logging.h"
#include "bluetooth_hal/chip/chip_provisioner.h"
#include "bluetooth_hal/chip/chip_provisioner_interface.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/worker.h"

namespace bluetooth_hal {
namespace chip {
namespace {

using ::bluetooth_hal::HalState;

std::string MessageTypeToString(ChipProvisionMessageType type) {
  switch (type) {
    case ChipProvisionMessageType::kInitialize:
      return "Initialize";
    case ChipProvisionMessageType::kDownloadFirmware:
      return "DownloadFirmware";
    case ChipProvisionMessageType::kResetFirmware:
      return "ResetFirmware";
    default:
      return "Unknown";
  }
}

}  // namespace

ChipProvisionMessage ChipProvisionMessage::CreateInitialize(
    InitializePayload payload) {
  ChipProvisionMessage msg;
  msg.type = ChipProvisionMessageType::kInitialize;
  msg.payload = std::move(payload);
  return msg;
}

ChipProvisionMessage ChipProvisionMessage::CreateDownloadFirmware() {
  ChipProvisionMessage msg;
  msg.type = ChipProvisionMessageType::kDownloadFirmware;
  msg.payload = DownloadFirmwarePayload{};
  return msg;
}

ChipProvisionMessage ChipProvisionMessage::CreateResetFirmware() {
  ChipProvisionMessage msg;
  msg.type = ChipProvisionMessageType::kResetFirmware;
  msg.payload = ResetFirmwarePayload{};
  return msg;
}

AsyncChipProvisioner::AsyncChipProvisioner()
    : worker_([this](ChipProvisionMessage msg) {
        this->ProcessMessage(std::move(msg));
      }) {}

AsyncChipProvisioner& AsyncChipProvisioner::GetProvisioner() {
  static AsyncChipProvisioner provisioner;
  return provisioner;
}

void AsyncChipProvisioner::PostInitialize(
    const std::function<void(HalState)> on_hal_state_update) {
  InitializePayload payload = {std::move(on_hal_state_update)};
  worker_.Post(ChipProvisionMessage::CreateInitialize(std::move(payload)));
}

void AsyncChipProvisioner::PostDownloadFirmware() {
  worker_.Post(ChipProvisionMessage::CreateDownloadFirmware());
}

void AsyncChipProvisioner::PostResetFirmware() {
  worker_.Post(ChipProvisionMessage::CreateResetFirmware());
}

void AsyncChipProvisioner::ProcessMessage(ChipProvisionMessage message) {
  LOG(DEBUG) << __func__
             << ": Message type: " << MessageTypeToString(message.type);

  switch (message.type) {
    case ChipProvisionMessageType::kInitialize:
      // Only Initialize has a payload now.
      if (auto* payload = std::get_if<InitializePayload>(&message.payload)) {
        HandleInitialize(*payload);
      } else {
        LOG(WARNING) << __func__ << ": Callback is null.";
      }
      break;
    case ChipProvisionMessageType::kDownloadFirmware:
      HandleDownloadFirmware();
      break;
    case ChipProvisionMessageType::kResetFirmware:
      HandleResetFirmware();
      break;
    default:
      break;
  }
}

void AsyncChipProvisioner::HandleInitialize(const InitializePayload& payload) {
  if (chip_provisioner_) {
    return;
  }
  chip_provisioner_ = ChipProvisionerInterface::Create();
  if (chip_provisioner_) {
    chip_provisioner_->Initialize(payload.on_hal_state_update);
  } else {
    LOG(ERROR) << __func__ << ": Failed to create ChipProvisioner instance.";
    // Consider how to report this failure, e.g., by invoking
    // on_hal_state_update with an error state.
  }
};

void AsyncChipProvisioner::HandleDownloadFirmware() {
  if (chip_provisioner_) {
    chip_provisioner_->DownloadFirmware();
  }
};

void AsyncChipProvisioner::HandleResetFirmware() {
  if (chip_provisioner_) {
    chip_provisioner_->ResetFirmware();
  }
};

}  // namespace chip
}  // namespace bluetooth_hal
