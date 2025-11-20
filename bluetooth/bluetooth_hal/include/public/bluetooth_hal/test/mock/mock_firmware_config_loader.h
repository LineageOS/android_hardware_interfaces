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
#include <optional>

#include "bluetooth_hal/config/firmware_config_loader.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace config {

class MockFirmwareConfigLoader;
static MockFirmwareConfigLoader* mock_firmware_config_loader = nullptr;

class MockFirmwareConfigLoader : public FirmwareConfigLoader {
 public:
  MOCK_METHOD(bool, LoadConfig, (), (override));

  MOCK_METHOD(bool, ResetFirmwareDataLoadingState, (), (override));

  MOCK_METHOD(std::optional<DataPacket>, GetNextFirmwareData, (), (override));

  MOCK_METHOD(std::optional<std::reference_wrapper<const SetupCommandPacket>>,
              GetSetupCommandPacket, (SetupCommandType), (const, override));

  MOCK_METHOD(int, GetLoadMiniDrvDelayMs, (), (const, override));

  MOCK_METHOD(int, GetLaunchRamDelayMs, (), (const, override));

  MOCK_METHOD(std::string, DumpConfigToString, (), (const, override));

  static FirmwareConfigLoader& GetLoader();

  static void ResetLoader();

  static void SetMockLoader(MockFirmwareConfigLoader* loader);
};

FirmwareConfigLoader& FirmwareConfigLoader::GetLoader() {
  return *mock_firmware_config_loader;
}

void MockFirmwareConfigLoader::SetMockLoader(MockFirmwareConfigLoader* loader) {
  mock_firmware_config_loader = loader;
}

void FirmwareConfigLoader::ResetLoader() {
  if (mock_firmware_config_loader != nullptr) {
    delete mock_firmware_config_loader;
    mock_firmware_config_loader = nullptr;
  }
}

}  // namespace config
}  // namespace bluetooth_hal
