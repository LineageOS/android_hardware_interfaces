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

#define LOG_TAG "bthal.extensions.ccc"

#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_handler.h"

#include <fcntl.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_handler_callback.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_timesync_command.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_timesync_event.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/util/system_call_wrapper.h"

namespace bluetooth_hal {
namespace extensions {
namespace ccc {
namespace {

using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciEventMonitor;
using ::bluetooth_hal::hci::HciMonitor;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::util::SystemCallWrapper;

constexpr std::string_view kTimesyncProcNode = "/proc/bluetooth/timesync";
constexpr std::chrono::seconds kPendingCallbackTimeout =
    std::chrono::seconds(3);

}  // namespace

BluetoothCccHandler::BluetoothCccHandler() {
  // Register monitor for CCC LMP events.
  HciEventMonitor ccc_lmp_event_monitor(
      static_cast<uint8_t>(EventCode::kVendorSpecific),
      static_cast<uint8_t>(TimesyncConstants::kSubEventCode),
      static_cast<int>(TimesyncEventOffset::kSubEventCode));
  RegisterMonitor(ccc_lmp_event_monitor, MonitorMode::kIntercept);
}

BluetoothCccHandler& BluetoothCccHandler::GetHandler() {
  static BluetoothCccHandler handler;
  return handler;
}

bool BluetoothCccHandler::RegisterForLmpEvents(
    const std::shared_ptr<BluetoothCccHandlerCallback>& callback) {
  std::unique_lock<std::mutex> lock(mutex_);
  const auto address = callback->GetAddress();
  const auto lmp_event_ids = callback->GetLmpEventIds();

  if (!IsBluetoothEnabled() || callback == nullptr || lmp_event_ids.empty()) {
    LOG(WARNING) << __func__ << ": Unable to register for LMP events";
    return false;
  }

  auto lmp_event_size = lmp_event_ids.size();
  if (lmp_event_size == 0 ||
      lmp_event_size > static_cast<size_t>(CccLmpEventId::kMax)) {
    LOG(WARNING) << __func__
                 << ": Incorrect size LMP events:" << lmp_event_size;
    return false;
  }
  LOG(INFO) << __func__ << ": address: " << address.ToString();

  // Push the callback to the pending queue to wait for the command complete
  // event.
  pending_callbacks_deque_.emplace_back(callback);
  auto command = BluetoothCccTimesyncCommand::CreateAddCommand(
      address, AddressType::kRandom, CccDirection::kTx, lmp_event_ids);
  return SendCommand(command);
}

bool BluetoothCccHandler::UnregisterLmpEvents(const BluetoothAddress& address) {
  std::unique_lock<std::mutex> lock(mutex_);
  LOG(INFO) << __func__ << ": address: " << address.ToString();

  if (!IsBluetoothEnabled()) {
    LOG(WARNING) << __func__
                 << ": Bluetooth is OFF, unable to unregister for LMP events";
    return false;
  }

  // If there is a pending callback waiting for command complete event, wait
  // for it.
  if (!pending_callbacks_cv_.wait_for(lock, kPendingCallbackTimeout, [&] {
        return pending_callbacks_deque_.empty();
      })) {
    LOG(FATAL) << "Timeout: pending_callbacks_deque_ was not empty!";
  }

  std::vector<BluetoothAddress> removed_addresses;
  auto it = std::remove_if(
      monitor_callbacks_.begin(), monitor_callbacks_.end(),
      [&address, &removed_addresses](
          const std::shared_ptr<BluetoothCccHandlerCallback>& callback) {
        if (callback->IsAddressEqual(address)) {
          removed_addresses.push_back(address);
          return true;
        }
        return false;
      });

  if (removed_addresses.empty()) {
    LOG(WARNING) << __func__
                 << ": was not registered for address: " << address.ToString();
    return false;
  }
  monitor_callbacks_.erase(it, monitor_callbacks_.end());

  bool all_success = true;
  for (const auto& removed_address : removed_addresses) {
    auto command = BluetoothCccTimesyncCommand::CreateRemoveCommand(
        removed_address, AddressType::kRandom);
    if (!SendCommand(command)) {
      LOG(WARNING) << __func__
                   << ": Failed to send REMOVE command for address: "
                   << removed_address.ToString();
      all_success = false;
    }
  }

  return all_success;
}

void BluetoothCccHandler::OnCommandCallback(const HalPacket& packet) {
  std::unique_lock<std::mutex> lock(mutex_);
  bool success = (packet.GetCommandCompleteEventResult() ==
                  static_cast<uint8_t>(EventResultCode::kSuccess));
  if (success) {
    LOG(INFO) << __func__ << ": event status: Success";
  } else {
    LOG(WARNING) << __func__ << ": event status: Failed!";
  }

  if (pending_callbacks_deque_.empty()) {
    // Pending queue is empty, which means the event is for a REMOVE or CLEAR
    // command.
    return;
  }

  // Command complete received, remove the callback from the pending queue.
  auto callback = std::move(pending_callbacks_deque_.front());
  pending_callbacks_deque_.pop_front();

  callback->OnRegistered(success);

  if (success) {
    // if the command complete event is success, keep the callback in
    // monitor_callbacks to look for DCK time sync events.
    monitor_callbacks_.push_back(std::move(callback));
  }
  pending_callbacks_cv_.notify_one();
}

void BluetoothCccHandler::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode, const HalPacket& packet) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (monitor_callbacks_.empty()) {
    return;
  }

  BluetoothCccTimesyncEvent time_sync_event(packet);
  if (!time_sync_event.IsValid()) {
    LOG(WARNING) << __func__ << ": Invalid time sync event!";
    return;
  }

  uint8_t toggle_count = time_sync_event.GetToggleCount();
  uint16_t timesync_offset = time_sync_event.GetTimesyncOffset();
  uint64_t system_time = GetSystemTime(toggle_count, timesync_offset);
  if (system_time == 0) {
    LOG(WARNING) << __func__ << ": Invalid system time, drop the report.";
    return;
  }
  CccTimestamp timestamp(static_cast<long>(system_time),
                         static_cast<long>(time_sync_event.GetTimestamp()));

  auto address = time_sync_event.GetAddress();
  auto direction = time_sync_event.GetDirection();
  auto lmp_event_id = time_sync_event.GetEventId();
  uint8_t event_counter = time_sync_event.GetEventCount();

  LOG(INFO) << "Recv address: " << address.ToString()
            << ", direction: " << static_cast<int>(direction)
            << ", lmp_event_id: " << static_cast<int>(lmp_event_id)
            << ", event_counter: " << static_cast<int>(event_counter)
            << ", toggle_count: " << static_cast<int>(toggle_count)
            << ", timesync_offset: " << timesync_offset
            << ", bluetooth_time: " << timestamp.bluetooth_time
            << ", system_time: " << timestamp.system_time;

  for (const auto& callback : monitor_callbacks_) {
    if (callback->IsAddressEqual(address) &&
        callback->ContainsEventId(lmp_event_id)) {
      callback->OnEventGenerated(timestamp, address, direction, lmp_event_id,
                                 event_counter);
    }
  }
}

void BluetoothCccHandler::OnBluetoothEnabled() {
  std::unique_lock<std::mutex> lock(mutex_);
  // Clear all pending time sync data from the timesync fd.
  for (uint64_t time = -1; time != 0;) {
    time = GetSystemTime(1, 0);
  }
  previous_toggle_count_ = 0;
}

void BluetoothCccHandler::OnBluetoothDisabled() {
  std::unique_lock<std::mutex> lock(mutex_);
  monitor_callbacks_.clear();
  previous_toggle_count_ = 0;
  while (!pending_callbacks_deque_.empty()) {
    pending_callbacks_deque_.front()->OnRegistered(false);
    pending_callbacks_deque_.pop_front();
  }
}

uint64_t BluetoothCccHandler::GetSystemTime(uint8_t current_toggle_count,
                                            uint16_t offset) {
  int fd = 0;
  if ((fd = SystemCallWrapper::GetWrapper().Open(kTimesyncProcNode.data(),
                                                 O_RDONLY)) < 0) {
    LOG(WARNING) << __func__ << ": Unable to open timesync node";
    return 0;
  }

  // Calculate the difference using modular arithmetic to handle overflow
  // This correctly calculates the number of increments, even if
  // current_toggle_count wrapped around.
  uint8_t toggle_count = current_toggle_count - previous_toggle_count_;
  previous_toggle_count_ = current_toggle_count;

  char buff[TimesyncConstants::kUint64MaxDigitInDec];
  for (uint8_t i = 0; i < toggle_count; i++) {
    // The LMP commands can be sent multiple times to the air and we only care
    // about one that success, which is the one that toggle count points to.
    SystemCallWrapper::GetWrapper().Read(
        fd, buff, TimesyncConstants::kUint64MaxDigitInDec);
  }
  SystemCallWrapper::GetWrapper().Close(fd);

  uint64_t system_time = static_cast<uint64_t>(strtoul(buff, nullptr, 0));
  if (system_time > offset) {
    system_time -= offset;
  }

  return system_time;
}

}  // namespace ccc
}  // namespace extensions
}  // namespace bluetooth_hal
