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

#include <string>
#include <vector>

#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace config {

class MockHalConfigLoader;
static MockHalConfigLoader* mock_hal_config_loader = nullptr;

class MockHalConfigLoader : public HalConfigLoader {
 public:
  MOCK_METHOD(bool, LoadConfig, (), (override));

  MOCK_METHOD(bool, IsFastDownloadEnabled, (), (const, override));

  MOCK_METHOD(bool, IsSarBackoffHighResolutionEnabled, (), (const, override));

  MOCK_METHOD(int, GetBtRegOnDelayMs, (), (const, override));

  MOCK_METHOD(const std::string&, GetBtUartDevicePort, (), (const, override));

  MOCK_METHOD(const std::vector< ::bluetooth_hal::transport::TransportType>&,
              GetTransportTypePriority, (), (const, override));

  MOCK_METHOD(bool, IsAcceleratedBtOnSupported, (), (const, override));

  MOCK_METHOD(bool, IsThreadDispatcherEnabled, (), (const, override));

  MOCK_METHOD(bool, IsBtPowerControlledByLpp, (), (const, override));

  MOCK_METHOD(const std::vector<std::string>&,
              GetHwStagesWithoutLppControlBtPowerPin, (), (const, override));

  MOCK_METHOD(const std::vector<std::string>&, GetFwUnsupportedHwStages, (),
              (const, override));

  MOCK_METHOD(int, GetVendorTransportCrashIntervalSec, (), (const, override));

  MOCK_METHOD(bool, IsHpUartSkipSuspendSupported, (), (const, override));

  MOCK_METHOD(bool, IsEnergyControllerLoggingSupported, (), (const, override));

  MOCK_METHOD(bool, IsBtHalRestartRecoverySupported, (), (const, override));

  MOCK_METHOD(bool, IsBleNonConnectionSarEnabled, (), (const, override));

  MOCK_METHOD(int, GetKernelRxWakelockTimeMilliseconds, (), (const, override));

  MOCK_METHOD(bool, IsLowPowerModeSupported, (), (const, override));

  MOCK_METHOD(bool, IsTranportFallbackEnabled, (), (const, override));

  MOCK_METHOD(bool, IsBtSnoopLogFullModeOn, (), (const, override));

  MOCK_METHOD(::bluetooth_hal::uart::BaudRate, GetUartBaudRate,
              (::bluetooth_hal::transport::TransportType type),
              (const, override));

  MOCK_METHOD(bool, IsUserDebugOrEngBuild, (), (const, override));

  MOCK_METHOD(std::string, DumpConfigToString, (), (const, override));

  MOCK_METHOD(const std::string&, GetLpmEnableProcNode, (), (const, override));

  MOCK_METHOD(const std::string&, GetLpmWakingProcNode, (), (const, override));

  MOCK_METHOD(const std::string&, GetLpmWakelockCtrlProcNode, (),
              (const, override));

  MOCK_METHOD(const std::string&, GetRfkillFolderPrefix, (), (const, override));

  MOCK_METHOD(const std::string&, GetRfkillTypeBluetooth, (),
              (const, override));

  static void SetMockLoader(MockHalConfigLoader* loader);
};

HalConfigLoader& HalConfigLoader::GetLoader() {
  return *mock_hal_config_loader;
}

void MockHalConfigLoader::SetMockLoader(MockHalConfigLoader* loader) {
  mock_hal_config_loader = loader;
}

}  // namespace config
}  // namespace bluetooth_hal
