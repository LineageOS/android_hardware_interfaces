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

#include <string>
#include <vector>

#include "bluetooth_hal/config/cs_config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace config {

class MockCsConfigLoader : public CsConfigLoader {
 public:
  MOCK_METHOD(bool, LoadConfig, (), (override));

  MOCK_METHOD(const std::vector<::bluetooth_hal::hci::HalPacket>&,
              GetCsCalibrationCommands, (), (const, override));
  MOCK_METHOD(std::string, DumpConfigToString, (), (const, override));

  static CsConfigLoader& GetLoader();
  static void SetMockLoader(MockCsConfigLoader* loader);

  static inline MockCsConfigLoader* mock_cs_config_loader_{nullptr};
};

}  // namespace config
}  // namespace bluetooth_hal
