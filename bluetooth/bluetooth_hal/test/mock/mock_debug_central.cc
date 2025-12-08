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

#include "bluetooth_hal/test/mock/mock_debug_central.h"

namespace bluetooth_hal {
namespace debug {

DebugCentral& DebugCentral::Get() {
  if (!MockDebugCentral::mock_debug_central_) {
    LOG(FATAL) << __func__
               << ": mock_debug_central_ is nullptr. Did you forget "
                  "to call SetMockLoader in your test SetUp?";
  }
  return *MockDebugCentral::mock_debug_central_;
}

void MockDebugCentral::SetMockDebugCentral(
    MockDebugCentral* mock_debug_central) {
  mock_debug_central_ = mock_debug_central;
}

}  // namespace debug
}  // namespace bluetooth_hal
