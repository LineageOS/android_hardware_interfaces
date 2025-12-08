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

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/provider_factory.h"

namespace bluetooth_hal {
namespace chip {

class ChipProvisioner;

class ChipProvisionerInterface
    : public ::bluetooth_hal::util::ProviderFactory<ChipProvisionerInterface,
                                                    ChipProvisioner> {
 public:
  /**
   * @brief Registers a vendor-specific factory for creating
   * ChipProvisionerInterface instances.
   *
   * @param factory The factory function to register.
   */
  static void RegisterVendorChipProvisioner(FactoryFn factory) {
    RegisterProviderFactory(std::move(factory));
  }

  /**
   * @brief Unregisters the vendor-specific factory.
   *
   * This is primarily intended for use in test environments.
   */
  static void UnregisterVendorChipProvisioner() { UnregisterProviderFactory(); }

  virtual ~ChipProvisionerInterface() = default;

  /**
   * @brief Initializes the chip provisioner.
   *
   * @param on_hal_state_update A callback function invoked when the HAL state
   * changes.
   */
  virtual void Initialize(const std::function<void(::bluetooth_hal::HalState)>
                              on_hal_state_update) = 0;

  /**
   * @brief Downloads firmware to the Bluetooth chip.
   *
   * @return True if firmware download is successful, false otherwise.
   */
  virtual bool DownloadFirmware() = 0;

  /**
   * @brief Resets the firmware on the Bluetooth chip.
   *
   * @return True if firmware reset is successful, false otherwise.
   */
  virtual bool ResetFirmware() = 0;
};

}  // namespace chip
}  // namespace bluetooth_hal
