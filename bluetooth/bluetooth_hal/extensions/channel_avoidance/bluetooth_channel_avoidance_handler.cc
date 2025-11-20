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

#define LOG_TAG "bthal.extensions.channel_avoidance"

#include "bluetooth_hal/extensions/channel_avoidance/bluetooth_channel_avoidance_handler.h"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace extensions {
namespace channel_avoidance {
namespace {

using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::MonitorMode;

constexpr uint16_t kHciChannelAvoidanceOpcode = 0x0c3f;
constexpr uint8_t kHciChannelAvoidanceMapSize = 10;

constexpr int kMaxCommandWaitTimeMs = 1000;

template <typename TContainer>
std::string BytesToHexString(const TContainer& container) {
  std::ostringstream oss;
  bool first = true;
  for (uint8_t byte : container) {
    if (!first) {
      oss << " ";
    }
    oss << "0x" << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(byte);
    first = false;
  }
  return oss.str();
}

}  // namespace

HalPacket BluetoothChannelAvoidanceHandler::BuildSetChannelAvoidanceCommand(
    const std::array<uint8_t, 10>& channel_map) {
  HalPacket command;

  uint8_t cmd_length =
      HciConstants::kHciCommandPreambleSize + kHciChannelAvoidanceMapSize;

  command.reserve(1 + cmd_length);

  command.push_back(static_cast<uint8_t>(HciPacketType::kCommand));

  command.push_back(kHciChannelAvoidanceOpcode & 0xff);
  command.push_back((kHciChannelAvoidanceOpcode >> 8) & 0xff);

  // Param length.
  command.push_back(kHciChannelAvoidanceMapSize);

  command.insert(command.end(), channel_map.begin(), channel_map.end());

  return command;
}

bool BluetoothChannelAvoidanceHandler::SetBluetoothChannelStatus(
    const std::array<uint8_t, 10>& channel_map) {
  std::scoped_lock<std::mutex> lock(command_mtx_);

  if (!IsBluetoothEnabled()) {
    LOG(WARNING) << __func__ << ": BT off, unable to set channel map <"
                 << BytesToHexString(channel_map) << ">.";
    return false;
  }

  LOG(INFO) << __func__ << ": Setting Channel Map <"
            << BytesToHexString(channel_map) << ">.";

  HalPacket command_packet = BuildSetChannelAvoidanceCommand(channel_map);

  command_promise_ = std::promise<void>();
  if (!SendCommand(command_packet)) {
    LOG(ERROR) << __func__ << ": Failed to send HCI command.";
    return false;
  }

  std::future<void> future = command_promise_.get_future();
  if (future.wait_for(std::chrono::milliseconds(kMaxCommandWaitTimeMs)) !=
      std::future_status::ready) {
    LOG(ERROR) << __func__ << ": Command timed out.";
    command_success_ = false;
    return false;
  }

  return command_success_.load();
}

void BluetoothChannelAvoidanceHandler::OnCommandCallback(
    const HalPacket& event_packet) {
  bool success = (event_packet.GetCommandCompleteEventResult() ==
                  static_cast<uint8_t>(EventResultCode::kSuccess));
  command_success_ = success;

  LOG(success ? INFO : WARNING)
      << __func__ << ": Set Channel Avoidance VSE "
      << (success ? "succeeded" : "failed") << ". Status: 0x" << std::hex
      << static_cast<int>(event_packet.GetCommandCompleteEventResult());

  command_promise_.set_value();
}

void BluetoothChannelAvoidanceHandler::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode,
    [[maybe_unused]] const HalPacket& packet) {}

}  // namespace channel_avoidance
}  // namespace extensions
}  // namespace bluetooth_hal
