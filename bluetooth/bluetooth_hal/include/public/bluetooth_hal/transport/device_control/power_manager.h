/*
 * Copyright 2024 The Android Open Source Project
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

#include "android-base/unique_fd.h"

namespace bluetooth_hal {
namespace transport {

/**
 * @brief The PowerManager class manages Bluetooth device power states and low
 * power mode (LPM).
 *
 * This class configures Bluetooth device activity related to power management.
 * Entering LPM puts the Bluetooth device into sleep mode to conserve power,
 * while exiting LPM wakes it up for normal operation.
 *
 * Additionally, it controls the power supply to the Bluetooth chip, enabling or
 * disabling the chip's power as needed.
 */
class PowerManager {
 public:
  virtual ~PowerManager() = default;

  /**
   * @brief Controls the Bluetooth chip's power state.
   *
   * Enables or disables power to the Bluetooth chip by writing to the rfkill
   * interface. This effectively powers the device on or off.
   *
   * @param is_enabled Set to true to power on the Bluetooth chip, false to
   * power it off.
   *
   * @return True if the operation succeeds, false otherwise.
   *
   */
  virtual bool PowerControl(bool is_enabled);

  /**
   * @brief Prepares the system to enter Low Power Mode (LPM).
   *
   * Configures the necessary environment so the Bluetooth device can enter LPM,
   * where it will enter a sleep state to save power.
   *
   * @return True if setup is successful, false otherwise.
   *
   */
  virtual bool SetupLowPowerMode();

  /**
   * @brief Cleans up after exiting Low Power Mode (LPM).
   *
   * Releases resources or resets configurations used during LPM.
   *
   */
  virtual void TeardownLowPowerMode();

  /**
   * @brief Wakes the Bluetooth device from Low Power Mode to active mode.
   *
   * Transitions the device from sleep back to full operation.
   *
   * @return True if resume succeeds, false otherwise.
   *
   */
  virtual bool ResumeFromLowPowerMode();

  /**
   * @brief Puts the Bluetooth device into Low Power Mode (LPM).
   *
   * Transitions the device to a low power sleep state.
   *
   * @return True if suspend succeeds, false otherwise.
   *
   */
  virtual bool SuspendToLowPowerMode();

  /**
   * @brief Checks if Low Power Mode setup has been completed.
   *
   * Determines if the device is properly configured to enter or exit LPM.
   *
   * @return True if LPM setup is complete, false otherwise.
   *
   */
  virtual bool IsLowPowerModeSetupCompleted() const;

  /**
   * @brief Sets the RX wakelock duration in the kernel.
   *
   * Configures how long the RX wakelock holds the device awake after receiving
   * data.
   *
   * @param duration Duration in milliseconds; must be positive.
   *
   * @return True if the duration is successfully configured, false otherwise.
   *
   */
  virtual bool ConfigRxWakelockTime(int duration);

 protected:
  ::android::base::unique_fd lpm_fd_{-1};
};
}  // namespace transport
}  // namespace bluetooth_hal
