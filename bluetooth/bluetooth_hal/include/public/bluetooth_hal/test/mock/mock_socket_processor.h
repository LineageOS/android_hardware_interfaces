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

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bluetooth_hal/extensions/thread/socket_processor.h"
#include "bluetooth_hal/hal_packet.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace thread {

class MockSocketProcessor : public SocketProcessor {
 public:
  // Mocks the static Initialize and Cleanup methods of SocketProcessor.
  MOCK_METHOD(
      void, Initialize,
      (const std::string& socket_path,
       std::optional<::bluetooth_hal::hci::HalPacketCallback> hal_packet_cb));
  MOCK_METHOD(void, Cleanup, ());

  // Mocks the virtual socket operation methods, allowing their behavior to be
  // customized and verified in tests.
  MOCK_METHOD(bool, Send, (const std::vector<uint8_t>& data), (override));

  MOCK_METHOD(bool, Recv, (), (override));

  MOCK_METHOD(bool, OpenServer, (), (override));

  MOCK_METHOD(void, CloseServer, (), (override));

  MOCK_METHOD(void, CloseClient, (), (override));

  MOCK_METHOD(int, AcceptClient, (), (override));

  MOCK_METHOD(void, SetServerSocket, (int socket), (override));

  MOCK_METHOD(void, SetClientSocket, (int socket), (override));

  MOCK_METHOD(void, SetSocketMode, (SocketMode mode), (override));

  MOCK_METHOD(int, GetServerSocket, (), (const, override));

  MOCK_METHOD(int, GetClientSocket, (), (const, override));

  MOCK_METHOD(bool, IsSocketFileExisted, (), (const, override));

  MOCK_METHOD(int, OpenSocketFileMonitor, (), (override));

  MOCK_METHOD(void, CloseSocketFileMonitor, (), (override));

  MOCK_METHOD(int, GetSocketFileMonitor, (), (override));

  static void SetMockProcessor(MockSocketProcessor* processor);

  static inline MockSocketProcessor* mock_socket_processor_{nullptr};
};

}  // namespace thread
}  // namespace bluetooth_hal
