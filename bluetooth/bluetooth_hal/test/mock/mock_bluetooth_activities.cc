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

#include "bluetooth_hal/test/mock/mock_bluetooth_activities.h"

#include "android-base/logging.h"
#include "bluetooth_hal/debug/bluetooth_activities.h"

namespace bluetooth_hal {
namespace debug {

BluetoothActivities& BluetoothActivities::Get() {
  if (!MockBluetoothActivities::mock_bluetooth_activities_) {
    LOG(FATAL) << __func__
               << ": mock_bluetooth_activities_ is nullptr. Did you forget to "
                  "call SetMockBluetoothActivities in your test SetUp?";
  }
  return *MockBluetoothActivities::mock_bluetooth_activities_;
}

void MockBluetoothActivities::SetMockBluetoothActivities(
    MockBluetoothActivities* mock) {
  mock_bluetooth_activities_ = mock;
}

}  // namespace debug
}  // namespace bluetooth_hal
