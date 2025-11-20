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

#include <mutex>
#include <string>
#include <vector>

#include "bluetooth_hal/config/config_loader.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace config {

class HalConfigLoader : public ConfigLoader {
 public:
  virtual ~HalConfigLoader() = default;

  virtual bool LoadConfig() override = 0;
  virtual std::string DumpConfigToString() const override = 0;

  /**
   * @brief Checks if fast download is enabled.
   *
   * @return true if fast download is enabled, false otherwise.
   */
  virtual bool IsFastDownloadEnabled() const = 0;

  /**
   * @brief Checks if SAR backoff is enabled for high-resolution timing.
   *
   * @return true if high-resolution SAR backoff is enabled, false otherwise.
   */
  virtual bool IsSarBackoffHighResolutionEnabled() const = 0;

  /**
   * @brief Gets the Bluetooth regulator on delay in Milliseconds.
   *
   * @return The delay in Milliseconds for Bluetooth regulator on.
   */
  virtual int GetBtRegOnDelayMs() const = 0;

  /**
   * @brief Retrieves the Bluetooth UART device port.
   *
   * @return A constant reference to the string representing the Bluetooth UART
   * device port.
   */
  virtual const std::string& GetBtUartDevicePort() const = 0;

  /**
   * @brief Gets the priority of transport types for Bluetooth communication.
   *
   * @return A constant reference to a vector containing transport type
   * priorities.
   */
  virtual const std::vector< ::bluetooth_hal::transport::TransportType>&
  GetTransportTypePriority() const = 0;

  /**
   * @brief Checks if accelerated Bluetooth on is supported.
   *
   * @return true if accelerated Bluetooth on is supported, false otherwise.
   */
  virtual bool IsAcceleratedBtOnSupported() const = 0;

  /**
   * @brief Checks if thread dispatcher is enabled.
   *
   * @return true if the thread dispatcher is enabled, false otherwise.
   */
  virtual bool IsThreadDispatcherEnabled() const = 0;

  /**
   * @brief Checks if the Bluetooth power pin is controlled by the Low Power
   * Processor (LPP).
   *
   * @return True if LPP controls the Bluetooth power pin, false otherwise.
   */
  virtual bool IsBtPowerControlledByLpp() const = 0;

  /**
   * @brief Retrieves a list of hardware stages where the Bluetooth power pin
   * is not controlled by the Low Power Processor (LPP).
   *
   * @return A constant reference to a vector of strings, each representing a
   * hardware stage name where LPP does not control the Bluetooth power pin.
   */
  virtual const std::vector<std::string>&
  GetHwStagesWithoutLppControlBtPowerPin() const = 0;

  /**
   * @brief Gets a list of hwardware stages that firmware does not support.
   *
   * @return A constant reference to a vector of strings containing unsupported
   * hardware stages.
   */
  virtual const std::vector<std::string>& GetFwUnsupportedHwStages() const = 0;

  /**
   * @brief Gets the minimum interval between two vendor transport crashes in
   * seconds.
   *
   * This value represents the minimum allowable time between two consecutive
   * vendor transport crashes. It can be used to filter out repeated crash
   * events that occur too closely together to be considered independent
   * failures.
   *
   * @return The minimum interval between two vendor transport crashes, in
   * seconds.
   */
  virtual int GetVendorTransportCrashIntervalSec() const = 0;

  /**
   * @brief Checks if skipping suspend for host processor (hp) UART is
   * supported.
   *
   * @return true if hp UART suspend skip is supported, false otherwise.
   */
  virtual bool IsHpUartSkipSuspendSupported() const = 0;

  /**
   * @brief Checks if energy controller logging is supported.
   *
   * @return true if energy controller logging is supported, false otherwise.
   */
  virtual bool IsEnergyControllerLoggingSupported() const = 0;

  /**
   * @brief Checks if bt hal restarts for recovery is supported.
   *
   * @return true if bt hal restarts for recovery is supported, false
   * otherwise.
   */
  virtual bool IsBtHalRestartRecoverySupported() const = 0;

  /**
   * @brief Checks whether SAR is enabled for BLE non-connection mode.
   *
   * @return true if BLE non-connection SAR is enabled; false otherwise.
   */
  virtual bool IsBleNonConnectionSarEnabled() const = 0;

  /**
   * @brief Gets the kernel rx wakelock time in milliseconds.
   *
   * @return The rx wakelock time in milliseconds for kernel.
   */
  virtual int GetKernelRxWakelockTimeMilliseconds() const = 0;

  /**
   * @brief Checks whether Low Power Mode is supported for Bluetooth chip.
   *
   * @return true if Low Power Mode is supported; false otherwise.
   */
  virtual bool IsLowPowerModeSupported() const = 0;

  /**
   * @brief Checks whether transport fallback is enabled.
   *
   * @return true if fallback is enabled; false otherwise.
   */
  virtual bool IsTranportFallbackEnabled() const = 0;

  /**
   * @brief Checks whether full Bluetooth snoop log mode is enabled.
   *
   * @return true if full mode is on; false otherwise.
   */
  virtual bool IsBtSnoopLogFullModeOn() const = 0;

  /**
   * @brief Gets the UART baud rate for the given transport type.
   *
   * @param type The transport type.
   *
   * @return The UART baud rate.
   */
  virtual ::bluetooth_hal::uart::BaudRate GetUartBaudRate(
      ::bluetooth_hal::transport::TransportType type) const = 0;

  /**
   * @brief Checks whether the current build is userdebug or eng.
   *
   * @return true if build is userdebug or eng; false otherwise.
   */
  virtual bool IsUserDebugOrEngBuild() const = 0;

  /**
   * @brief Gets the proc node path for enabling Low Power Mode (LPM).
   *
   * @return A constant reference to the string representing the LPM enable
   * proc node path.
   */
  virtual const std::string& GetLpmEnableProcNode() const = 0;

  /**
   * @brief Gets the proc node path for waking from Low Power Mode (LPM).
   *
   * @return A constant reference to the string representing the LPM waking
   * proc node path.
   */
  virtual const std::string& GetLpmWakingProcNode() const = 0;

  /**
   * @brief Gets the proc node path for controlling LPM wakelock.
   *
   * @return A constant reference to the string representing the LPM wakelock
   * control proc node path.
   */
  virtual const std::string& GetLpmWakelockCtrlProcNode() const = 0;

  /**
   * @brief Gets the rfkill folder prefix.
   *
   * @return A constant reference to the string representing the rfkill folder
   * prefix.
   */
  virtual const std::string& GetRfkillFolderPrefix() const = 0;

  /**
   * @brief Gets the rfkill type for Bluetooth.
   * @return A constant reference to the string representing the rfkill type for
   * Bluetooth.
   */
  virtual const std::string& GetRfkillTypeBluetooth() const = 0;

  static HalConfigLoader& GetLoader();

  static void ResetLoader();

 private:
  static HalConfigLoader* loader_;
  static std::mutex loader_mutex_;
};

}  // namespace config
}  // namespace bluetooth_hal
