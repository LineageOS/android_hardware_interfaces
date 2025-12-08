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

#include <functional>
#include <memory>
#include <variant>

#include "bluetooth_hal/chip/chip_provisioner_interface.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/worker.h"

namespace bluetooth_hal {
namespace chip {

enum class ChipProvisionMessageType : int {
  kInitialize = 0,
  kDownloadFirmware,
  kResetFirmware,
};

struct InitializePayload {
  std::function<void(::bluetooth_hal::HalState)> on_hal_state_update;
};

struct DownloadFirmwarePayload {};

struct ResetFirmwarePayload {};

class ChipProvisionMessage {
 public:
  static ChipProvisionMessage CreateInitialize(InitializePayload payload);

  static ChipProvisionMessage CreateDownloadFirmware();

  static ChipProvisionMessage CreateResetFirmware();

  ChipProvisionMessageType type;
  std::variant<InitializePayload, DownloadFirmwarePayload, ResetFirmwarePayload>
      payload;

 private:
  ChipProvisionMessage() = default;
};

class AsyncChipProvisioner {
 public:
  AsyncChipProvisioner();

  static AsyncChipProvisioner& GetProvisioner();

  /**
   * @brief Posts an initialization request.
   *
   * @param on_hal_state_update Callback function for HAL state updates.
   *
   */
  void PostInitialize(
      const std::function<void(::bluetooth_hal::HalState)> on_hal_state_update);

  /**
   * @brief Posts a request to download firmware.
   *
   */
  void PostDownloadFirmware();

  /**
   * @brief Posts a request to reset firmware.
   *
   */
  void PostResetFirmware();

 private:
  void ProcessMessage(ChipProvisionMessage message);
  void HandleInitialize(const InitializePayload& payload);
  void HandleDownloadFirmware();
  void HandleResetFirmware();

  ::bluetooth_hal::util::Worker<ChipProvisionMessage> worker_;
  std::unique_ptr<ChipProvisionerInterface> chip_provisioner_;
};

}  // namespace chip
}  // namespace bluetooth_hal
