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

#include "bluetooth_hal/debug/vnd_snoop_logger.h"
#include "bluetooth_hal/hal_packet.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace debug {

class MockVndSnoopLogger : public VndSnoopLogger {
 public:
  MOCK_METHOD(void, StartNewRecording, (), (override));
  MOCK_METHOD(void, StopRecording, (), (override));
  MOCK_METHOD(void, Capture,
              (const ::bluetooth_hal::hci::HalPacket& packet,
               Direction direction),
              (override));

  static void SetMockVndSnoopLogger(MockVndSnoopLogger* logger);
};

}  // namespace debug
}  // namespace bluetooth_hal
