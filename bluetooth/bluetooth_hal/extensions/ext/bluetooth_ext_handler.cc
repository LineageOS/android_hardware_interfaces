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

#define LOG_TAG "bluetooth_hal.extensions.ext"

#include "bluetooth_hal/extensions/ext/bluetooth_ext_handler.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <mutex>
#include <vector>

#include "android-base/logging.h"
#include "android-base/stringprintf.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace ext {

constexpr int kMaxCommandWaitTimeMs = 1000;

using ::android::base::StringPrintf;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciPacketType;

bool BluetoothExtHandler::SetBluetoothCmdPacket(
    char16_t opcode, const std::vector<uint8_t>& params, bool* ret) {
  std::lock_guard<std::mutex> lock(cmd_mutex_);

  // Reset promise and success state for the new command.
  command_promise_ = std::promise<void>();
  std::future<void> event_future = command_promise_.get_future();
  command_success_.store(false, std::memory_order_relaxed);

  uint8_t params_len = params.size();

  HalPacket hci_cmd;
  uint8_t cmd_length = HciConstants::kHciCommandPreambleSize + params_len;

  hci_cmd.resize(1 + cmd_length);

  hci_cmd[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  hci_cmd[1] = opcode & 0xff;
  hci_cmd[2] = (opcode >> 8u) & 0xff;
  hci_cmd[3] = params_len;

  if (params_len > 0) {
    // Parameters start after Type (1), Opcode (2), Length (1) = 4 bytes
    memcpy(hci_cmd.data() + 1 + HciConstants::kHciCommandPreambleSize,
           params.data(), params_len);
  }

  std::string submsg = StringPrintf("0x%04x LEN 0x%02x", opcode, params_len);
  LOG(INFO) << __func__ << ": Inject hci_cmd: " << submsg;

  SendCommand(hci_cmd);

  std::future_status future_status =
      event_future.wait_for(std::chrono::milliseconds(kMaxCommandWaitTimeMs));

  if (future_status == std::future_status::timeout) {
    LOG(WARNING) << __func__ << ": Wait for VSE 0x" << std::hex
                 << static_cast<uint16_t>(opcode) << " timed out after "
                 << kMaxCommandWaitTimeMs << "ms";
    *ret = false;
  } else if (future_status == std::future_status::deferred) {
    LOG(ERROR) << __func__ << ": Future was deferred for VSE 0x" << std::hex
               << static_cast<uint16_t>(opcode);
    *ret = false;
  } else {
    *ret = command_success_.load(std::memory_order_relaxed);
  }

  return true;
}

void BluetoothExtHandler::OnCommandCallback(
    const ::bluetooth_hal::hci::HalPacket& event) {
  bool success = (event.GetCommandCompleteEventResult() ==
                  static_cast<uint8_t>(EventResultCode::kSuccess));

  LOG(success ? INFO : WARNING)
      << __func__ << ": Recv VSE "
      << (command_success_ ? "[Success]" : "[Failed]");

  command_success_.store(success, std::memory_order_relaxed);
  command_promise_.set_value();
}

}  // namespace ext
}  // namespace extensions
}  // namespace bluetooth_hal
