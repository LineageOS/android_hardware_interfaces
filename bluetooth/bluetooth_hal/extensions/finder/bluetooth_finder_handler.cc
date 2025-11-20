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

#define LOG_TAG "bthal.extensions.finder"

#include "bluetooth_hal/extensions/finder/bluetooth_finder_handler.h"

#include <chrono>
#include <cstdint>
#include <future>
#include <mutex>
#include <vector>

#include "aidl/android/hardware/bluetooth/finder/Eid.h"
#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/android_base_wrapper.h"

namespace bluetooth_hal {
namespace extensions {
namespace finder {

using ::aidl::android::hardware::bluetooth::finder::Eid;
using ::bluetooth_hal::Property;
using ::bluetooth_hal::hci::CommandOpCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::util::AndroidBaseWrapper;

constexpr int kMaxCommandWaitTime = 1000;

constexpr int kMaxKeyNumPerVsc = 12;
constexpr int kBytesPerKey = 20;

constexpr uint16_t kHciVscPofOpcode = 0xfd62;
constexpr uint8_t kHciVscStartPofSubOpCode = 0x02;
constexpr uint8_t kHciVscSetPrecomputedKeysPofSubOpCode = 0x01;

constexpr uint16_t kAdvertisingIntervalDefault = 0x640;     // 2000 ms
constexpr uint16_t kWaitTimeDefault = 0x5000;               // 20000 ms
constexpr uint16_t kPrecomputedKeyRotatedInterval = 0x400;  // 1024 s

BluetoothFinderHandler& BluetoothFinderHandler::GetHandler() {
  static BluetoothFinderHandler handler;
  return handler;
}

bool BluetoothFinderHandler::SendEids(const std::vector<Eid>& keys) {
  std::scoped_lock<std::mutex> lock(finder_mtx_);

  if (state_ != State::kIdle) {
    LOG(WARNING)
        << __func__
        << ": Could not send keys while entering powered off finder mode.";
    return false;
  }

  LOG(INFO) << __func__ << ": Send keys.";
  keys_.assign(keys.begin(), keys.end());
  // Reset key sending index if keys are updated while PoF is not started.
  current_key_index_ = 0;

  return true;
}

bool BluetoothFinderHandler::SetPoweredOffFinderMode(bool enable) {
  std::scoped_lock<std::mutex> lock(finder_mtx_);

  if (state_ != State::kIdle) {
    LOG(WARNING)
        << __func__
        << ": Could not set mode while entering powered off finder mode.";
    return false;
  }

  LOG(INFO) << __func__ << ": enable: " << enable << ".";
  is_pof_enabled_ = enable;

  if (!is_pof_enabled_) {
    keys_.clear();
    state_ = State::kIdle;
  }

  return true;
}

bool BluetoothFinderHandler::GetPoweredOffFinderMode(bool* return_value) {
  std::scoped_lock<std::mutex> lock(finder_mtx_);

  LOG(INFO) << __func__ << ": enable: " << is_pof_enabled_ << ".";
  *return_value = is_pof_enabled_;
  return true;
}

bool BluetoothFinderHandler::IsPoweredOffFinderEnabled() const {
  return is_pof_enabled_;
}

bool BluetoothFinderHandler::StartPoweredOffFinderMode() {
  std::scoped_lock<std::mutex> lock(finder_mtx_);

  if (!is_pof_enabled_) {
    LOG(WARNING) << __func__ << ": Powered off mode is not enabled.";
    return false;
  }

  std::string shutdown_action = AndroidBaseWrapper::GetWrapper().GetProperty(
      Property::kShutDownAction, "");
  if (shutdown_action.empty()) {
    LOG(WARNING) << __func__ << ": Device is not shutting down.";
    return false;
  }

  if (state_ != State::kIdle) {
    LOG(WARNING) << __func__ << ": Already entered powered off mode.";
    return false;
  }

  if (!IsBluetoothChipReady()) {
    LOG(WARNING) << __func__
                 << ": Unable to start powered off mode: bluetooth is off.";
    return false;
  } else if (!keys_.size()) {
    LOG(WARNING) << __func__ << ": Unable to start powered off mode: no key.";
    return false;
  }

  // Reset key index and start the state machine.
  current_key_index_ = 0;
  HandleNextStep(State::kReset);

  return state_ == State::kStarted;
}

void BluetoothFinderHandler::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode,
    [[maybe_unused]] const HalPacket& packet) {
  // Unused method.
}

void BluetoothFinderHandler::OnCommandCallback(const HalPacket& event) {
  command_success_ = (event.GetCommandCompleteEventResult() ==
                      static_cast<uint8_t>(EventResultCode::kSuccess));

  LOG(command_success_ ? INFO : WARNING)
      << __func__ << ": Recv VSE "
      << (command_success_ ? "[Success]" : "[Failed]");

  // Handle state transition based on the current state and command result.
  if (!command_success_) {
    LOG(ERROR) << __func__ << ": Command failed in state "
               << static_cast<int>(state_.load());
    // Reset state on failure.
    state_ = State::kIdle;
  }
  command_promise_.set_value();
}

void BluetoothFinderHandler::HandleNextStep(State next_state) {
  state_ = next_state;

  switch (state_) {
    case State::kReset: {
      HalPacket command = BuildFinderResetCommand();
      LOG(INFO) << __func__ << ": Sending Reset command.";

      if (!SendCommandAndWait(command)) {
        LOG(ERROR) << __func__ << ": Failed to send reset command.";
        state_ = State::kIdle;
        return;
      }

      HandleNextStep(State::kSendingKeys);

      break;
    }
    case State::kSendingKeys: {
      if (current_key_index_ >= keys_.size()) {
        LOG(ERROR) << __func__ << ": Invalid state: No more keys to send.";
        state_ = State::kIdle;
        return;
      }

      HalPacket command = BuildPrecomputedKeyCommand(keys_, current_key_index_);
      LOG(INFO) << __func__ << ": Sending keys starting from index "
                << current_key_index_ << ".";

      if (!SendCommandAndWait(command)) {
        LOG(ERROR) << __func__ << ": Failed to send key command.";
        state_ = State::kIdle;
        return;
      }

      if (current_key_index_ >= keys_.size()) {
        HandleNextStep(State::kStartingPof);
      } else {
        HandleNextStep(State::kSendingKeys);
      }

      break;
    }
    case State::kStartingPof: {
      // Assuming key index 0 for start.
      HalPacket command = BuildStartPoweredOffFinderModeCommand(0);
      LOG(INFO) << __func__ << ": Sending Start POF command.";

      if (!SendCommandAndWait(command)) {
        LOG(ERROR) << __func__ << ": Failed to send start POF command.";
        state_ = State::kIdle;
        return;
      }

      HandleNextStep(State::kStarted);

      break;
    }
    case State::kStarted:
      LOG(INFO) << __func__ << ": Start powered off finder successfully.";
      break;
    default:
      break;
  }
}

HalPacket BluetoothFinderHandler::BuildPrecomputedKeyCommand(
    const std::vector<Eid>& keys, uint_t cur_key_idx) {
  HalPacket command;
  uint8_t num_remaining_keys = keys.size() - cur_key_idx;
  uint8_t num_keys_to_send =
      (num_remaining_keys <= kMaxKeyNumPerVsc ? num_remaining_keys
                                              : kMaxKeyNumPerVsc);

  uint8_t param_length = 3 + num_keys_to_send * kBytesPerKey;
  uint8_t cmd_length = HciConstants::kHciCommandPreambleSize + param_length;

  command.resize(1 + cmd_length);

  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = kHciVscPofOpcode & 0xff;
  command[2] = (kHciVscPofOpcode >> 8u) & 0xff;
  command[3] = param_length;
  command[4] = kHciVscSetPrecomputedKeysPofSubOpCode;
  command[5] = cur_key_idx & 0xff;
  command[6] = num_keys_to_send;

  for (size_t i = 0; i < num_keys_to_send; ++i) {
    const auto& key = keys[cur_key_idx + i].bytes;
    std::copy_n(key.begin(), kBytesPerKey,
                command.begin() + 7 + i * kBytesPerKey);
  }

  // Update the index for the next batch *after* building the command.
  current_key_index_ += num_keys_to_send;

  return command;
}

HalPacket BluetoothFinderHandler::BuildFinderResetCommand() {
  HalPacket command;

  uint8_t param_length = 0;
  uint8_t cmd_length = HciConstants::kHciCommandPreambleSize + param_length;

  command.resize(1 + cmd_length);

  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = static_cast<uint16_t>(CommandOpCode::kHciReset) & 0xff;
  command[2] = (static_cast<uint16_t>(CommandOpCode::kHciReset) >> 8u) & 0xff;
  command[3] = param_length;

  return command;
}

HalPacket BluetoothFinderHandler::BuildStartPoweredOffFinderModeCommand(
    int32_t cur_key_idx) {
  HalPacket command;

  uint8_t param_length = 9;
  uint8_t cmd_length = HciConstants::kHciCommandPreambleSize + param_length;

  command.resize(1 + cmd_length);

  command[0] = static_cast<uint8_t>(HciPacketType::kCommand);
  command[1] = kHciVscPofOpcode & 0xff;
  command[2] = (kHciVscPofOpcode >> 8u) & 0xff;
  command[3] = param_length;
  command[4] = kHciVscStartPofSubOpCode;
  command[5] = kAdvertisingIntervalDefault & 0xff;
  command[6] = (kAdvertisingIntervalDefault >> 8u) & 0xff;
  command[7] = 0x0A;  // power level.
  command[8] = kPrecomputedKeyRotatedInterval & 0xff;
  command[9] = (kPrecomputedKeyRotatedInterval >> 8u) & 0xff;
  command[10] = cur_key_idx & 0xff;
  command[11] = kWaitTimeDefault & 0xff;
  command[12] = (kWaitTimeDefault >> 8u) & 0xff;

  return command;
}

bool BluetoothFinderHandler::SendCommandAndWait(const HalPacket& packet) {
  if (!SendCommand(packet)) {
    LOG(ERROR) << __func__ << "Failed to send command.";
    return false;
  }

  std::future_status status = command_promise_.get_future().wait_for(
      std::chrono::milliseconds(kMaxCommandWaitTime));
  if (status != std::future_status::ready) {
    LOG(ERROR) << __func__ << "Command timeout.";
    return false;
  }

  command_promise_ = std::promise<void>();
  return command_success_;  // Return the result set by OnCommandCallback.
}

}  // namespace finder
}  // namespace extensions
}  // namespace bluetooth_hal
