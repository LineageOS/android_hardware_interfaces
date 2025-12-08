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

#include "bluetooth_hal/debug/debug_central.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace debug {

class MockDebugCentral : public DebugCentral {
 public:
  MOCK_METHOD(bool, RegisterDebugClient, (DebugClient * debug_client),
              (override));
  MOCK_METHOD(bool, UnregisterDebugClient, (DebugClient * debug_client),
              (override));
  MOCK_METHOD(void, Dump, (int fd), (override));
  MOCK_METHOD(void, SetBtUartDebugPort, (const std::string& uart_port),
              (override));
  MOCK_METHOD(void, AddLog, (AnchorType type, const std::string& log),
              (override));
  MOCK_METHOD(void, ReportBqrError,
              (::bluetooth_hal::bqr::BqrErrorCode error,
               std::string extra_info),
              (override));
  MOCK_METHOD(void, HandleRootInflammationEvent,
              (const ::bluetooth_hal::bqr::BqrRootInflammationEvent& event),
              (override));
  MOCK_METHOD(void, HandleDebugInfoEvent,
              (const ::bluetooth_hal::hci::HalPacket& packet), (override));
  MOCK_METHOD(void, HandleDebugInfoCommand, (), (override));
  MOCK_METHOD(void, GenerateVendorDumpFile,
              (const std::string& file_path, const std::vector<uint8_t>& data,
               uint8_t vendor_error_code),
              (override));
  MOCK_METHOD(void, GenerateCoredump,
              (CoredumpErrorCode error_code, uint8_t sub_error_code),
              (override));
  MOCK_METHOD(void, ResetCoredumpGenerator, (), (override));
  MOCK_METHOD(bool, IsCoredumpGenerated, (), (override));
  MOCK_METHOD(std::string&, GetCoredumpTimestampString, (), (override));

  static void SetMockDebugCentral(MockDebugCentral* mock_debug_central);

  static inline MockDebugCentral* mock_debug_central_{nullptr};
};

}  // namespace debug
}  // namespace bluetooth_hal
