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

#include "bluetooth_hal/test/mock/mock_socket_processor.h"

#include <optional>
#include <string>

#include "android-base/logging.h"
#include "bluetooth_hal/extensions/thread/socket_processor.h"
#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace thread {

using ::bluetooth_hal::hci::HalPacketCallback;

void SocketProcessor::Initialize(
    const std::string& socket_path,
    std::optional<HalPacketCallback> hal_packet_cb) {
  if (MockSocketProcessor::mock_socket_processor_) {
    MockSocketProcessor::mock_socket_processor_->Initialize(socket_path,
                                                            hal_packet_cb);
  } else {
    LOG(ERROR) << __func__ << ": mock_socket_processor_ is nullptr.";
  }
}

void SocketProcessor::Cleanup() {
  if (MockSocketProcessor::mock_socket_processor_) {
    MockSocketProcessor::mock_socket_processor_->Cleanup();
  } else {
    LOG(ERROR) << __func__ << ": mock_socket_processor_ is nullptr.";
  }
}

SocketProcessor* SocketProcessor::GetProcessor() {
  if (!MockSocketProcessor::mock_socket_processor_) {
    LOG(FATAL) << __func__ << ": mock_socket_processor_ is nullptr.";
  }
  return MockSocketProcessor::mock_socket_processor_;
}

void MockSocketProcessor::SetMockProcessor(MockSocketProcessor* processor) {
  mock_socket_processor_ = processor;
}

}  // namespace thread
}  // namespace bluetooth_hal
