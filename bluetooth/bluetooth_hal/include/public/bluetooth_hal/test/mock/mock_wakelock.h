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

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/power/wakelock.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace util {
namespace power {

class MockWakelock : public Wakelock {
 public:
  MOCK_METHOD(void, Acquire, (WakeSource source), (override));
  MOCK_METHOD(void, Release, (WakeSource source), (override));
  MOCK_METHOD(bool, IsAcquired, (), (override));
  MOCK_METHOD(bool, IsWakeSourceAcquired, (WakeSource source), (override));

  static void SetMockWakelock(MockWakelock* wakelock);
};

}  // namespace power
}  // namespace util
}  // namespace bluetooth_hal
