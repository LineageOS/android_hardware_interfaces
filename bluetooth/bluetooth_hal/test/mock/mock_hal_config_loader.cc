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

#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"

#include "android-base/logging.h"
#include "bluetooth_hal/config/hal_config_loader.h"

namespace bluetooth_hal {
namespace config {

HalConfigLoader& HalConfigLoader::GetLoader() {
  if (!MockHalConfigLoader::mock_hal_config_loader_) {
    LOG(FATAL) << __func__
               << ": mock_hal_config_loader_ is nullptr. Did you forget to "
                  "call SetMockLoader in your test SetUp?";
  }
  return *MockHalConfigLoader::mock_hal_config_loader_;
}

void MockHalConfigLoader::SetMockLoader(MockHalConfigLoader* loader) {
  mock_hal_config_loader_ = loader;
}

}  // namespace config
}  // namespace bluetooth_hal
