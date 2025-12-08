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

#define LOG_TAG "bluetooth_hal.bt_activities"

#include "bluetooth_hal/debug/bluetooth_activities.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "android-base/logging.h"
#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/debug/command_error_code.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_client.h"
#include "bluetooth_hal/util/logging.h"

namespace bluetooth_hal {
namespace debug {
namespace {

using ::bluetooth_hal::hci::BleMetaEventSubCode;
using ::bluetooth_hal::hci::BluetoothAddress;
using ::bluetooth_hal::hci::EventCode;
using ::bluetooth_hal::hci::EventResultCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciBleMetaEventMonitor;
using ::bluetooth_hal::hci::HciEventMonitor;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::util::Logger;

constexpr uint16_t kBtMaxConnectHistoryRecord = 64;
constexpr size_t kBleConnectionEventStatusOffset = 4;
constexpr size_t kBleConnectionHandleOffset = 5;
constexpr size_t kBleConnectionBdAddressOffset = 9;
constexpr size_t kConnectionEventStatusOffset = 3;
constexpr size_t kConnectionHandleOffset = 4;
constexpr size_t kConnectionBdAddressOffset = 6;
constexpr size_t kDisconnectionEventStatusOffset = 3;
constexpr size_t kDisconnectionHandleOffset = 4;
constexpr int kUint8HexStringDigit = 2;
constexpr int kUint16HexStringDigit = 4;

std::string ToHexString(uint16_t value, int num_of_digits) {
  std::stringstream ss;
  ss << std::hex << std::setw(num_of_digits) << std::setfill('0') << value;
  return "0x" + ss.str();
}

}  // namespace

class BluetoothActivitiesImpl : public BluetoothActivities,
                                public ::bluetooth_hal::hci::HciRouterClient {
 public:
  BluetoothActivitiesImpl();

  bool HasConnectedDevice() const;
  bool IsConnected(uint16_t connection_handle) const;
  size_t GetConnectionHandleCount() const;
  void HandleBleMetaEvent(const ::bluetooth_hal::hci::HalPacket& event);
  void HandleConnectCompleteEvent(const ::bluetooth_hal::hci::HalPacket& event);
  void HandleDisconnectCompleteEvent(
      const ::bluetooth_hal::hci::HalPacket& event);

  void OnCommandCallback(
      [[maybe_unused]] const ::bluetooth_hal::hci::HalPacket& packet) override {
  };
  void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const ::bluetooth_hal::hci::HalPacket& packet) override;
  void OnBluetoothChipReady() override {};
  void OnBluetoothChipClosed() override;
  void OnBluetoothEnabled() override {};
  void OnBluetoothDisabled() override {};

 private:
  struct ConnectionActivity {
    uint16_t connection_handle;
    BluetoothAddress bd_address;
    std::string event;
    std::string status;
    std::string timestamp;
  };

  void UpdateConnectionHistory(const ConnectionActivity& device);

  HciBleMetaEventMonitor ble_connection_complete_event_monitor_;
  HciBleMetaEventMonitor ble_enhanced_connection_complete_v1_event_monitor_;
  HciBleMetaEventMonitor ble_enhanced_connection_complete_v2_event_monitor_;
  HciEventMonitor connection_complete_event_monitor_;
  HciEventMonitor disconnection_complete_event_monitor_;

  std::list<ConnectionActivity> connection_history_;
  std::unordered_map<uint16_t, BluetoothAddress> connected_device_address_;
};

BluetoothActivitiesImpl::BluetoothActivitiesImpl()
    : ble_connection_complete_event_monitor_(HciBleMetaEventMonitor(
          static_cast<uint8_t>(BleMetaEventSubCode::kConnectionComplete))),
      ble_enhanced_connection_complete_v1_event_monitor_(
          HciBleMetaEventMonitor(static_cast<uint8_t>(
              BleMetaEventSubCode::kEnhancedConnectionCompleteV1))),
      ble_enhanced_connection_complete_v2_event_monitor_(
          HciBleMetaEventMonitor(static_cast<uint8_t>(
              BleMetaEventSubCode::kEnhancedConnectionCompleteV2))),
      connection_complete_event_monitor_(HciEventMonitor(
          static_cast<uint8_t>(EventCode::kConnectionComplete))),
      disconnection_complete_event_monitor_(HciEventMonitor(
          static_cast<uint8_t>(EventCode::kDisconnectionComplete))) {
  RegisterMonitor(ble_connection_complete_event_monitor_,
                  MonitorMode::kMonitor);
  RegisterMonitor(connection_complete_event_monitor_, MonitorMode::kMonitor);
  RegisterMonitor(disconnection_complete_event_monitor_, MonitorMode::kMonitor);
}

void BluetoothActivities::Start() { BluetoothActivities::Get(); }

BluetoothActivities& BluetoothActivities::Get() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!instance_) {
    instance_ = std::make_unique<BluetoothActivitiesImpl>();
  }
  return *instance_;
}

void BluetoothActivities::Stop() {
  if (!instance_) {
    return;
  }
  instance_.reset();
}

bool BluetoothActivitiesImpl::HasConnectedDevice() const {
  return connected_device_address_.size() > 0;
}

bool BluetoothActivitiesImpl::IsConnected(uint16_t connection_handle) const {
  return connected_device_address_.find(connection_handle) !=
         connected_device_address_.end();
}

size_t BluetoothActivitiesImpl::GetConnectionHandleCount() const {
  return connected_device_address_.size();
}

void BluetoothActivitiesImpl::OnMonitorPacketCallback(
    [[maybe_unused]] MonitorMode mode, const HalPacket& packet) {
  switch (packet.GetEventCode()) {
    case static_cast<uint8_t>(EventCode::kBleMeta):
      HandleBleMetaEvent(packet);
      break;
    case static_cast<uint8_t>(EventCode::kConnectionComplete):
      HandleConnectCompleteEvent(packet);
      break;
    case static_cast<uint8_t>(EventCode::kDisconnectionComplete):
      HandleDisconnectCompleteEvent(packet);
      break;
  }
}

void BluetoothActivitiesImpl::OnBluetoothChipClosed() {
  connected_device_address_.clear();
}

void BluetoothActivitiesImpl::HandleBleMetaEvent(const HalPacket& event) {
  uint8_t event_status = event.At(kBleConnectionEventStatusOffset);
  ConnectionActivity activity{
      .connection_handle =
          event.AtUint16LittleEndian(kBleConnectionHandleOffset),
      .bd_address = event.GetBluetoothAddressAt(kBleConnectionBdAddressOffset),
      .event = "LE Connection Complete " +
               ToHexString(event.GetBleSubEventCode(), kUint8HexStringDigit),
      .status = std::string(GetResultString(event_status)),
      .timestamp = Logger::GetLogFormatTimestamp(),
  };
  UpdateConnectionHistory(activity);

  if (event_status == static_cast<uint8_t>(EventResultCode::kSuccess)) {
    connected_device_address_[activity.connection_handle] = activity.bd_address;
    LOG(INFO) << __func__ << ": " << activity.event << ", connection handle: "
              << ToHexString(activity.connection_handle, kUint16HexStringDigit)
              << ", BD address: " << activity.bd_address.ToString() << ".";
  }
}

void BluetoothActivitiesImpl::HandleConnectCompleteEvent(
    const HalPacket& event) {
  uint8_t event_status = event.At(kConnectionEventStatusOffset);
  ConnectionActivity activity{
      .connection_handle = event.AtUint16LittleEndian(kConnectionHandleOffset),
      .bd_address = event.GetBluetoothAddressAt(kConnectionBdAddressOffset),
      .event = "Connect Complete " +
               ToHexString(event.GetEventCode(), kUint8HexStringDigit),
      .status = std::string(GetResultString(event_status)),
      .timestamp = Logger::GetLogFormatTimestamp(),
  };
  UpdateConnectionHistory(activity);

  if (event_status == static_cast<uint8_t>(EventResultCode::kSuccess)) {
    connected_device_address_[activity.connection_handle] = activity.bd_address;
    LOG(INFO) << __func__ << ": " << activity.event << ", connection handle: "
              << ToHexString(activity.connection_handle, kUint16HexStringDigit)
              << ", BD address: " << activity.bd_address.ToString() << ".";
  }
}

void BluetoothActivitiesImpl::HandleDisconnectCompleteEvent(
    const HalPacket& event) {
  uint8_t event_status = event.At(kDisconnectionEventStatusOffset);
  ConnectionActivity activity{
      .connection_handle =
          event.AtUint16LittleEndian(kDisconnectionHandleOffset),
      .bd_address = connected_device_address_[activity.connection_handle],
      .event = "Disconnect Complete " +
               ToHexString(event.GetEventCode(), kUint8HexStringDigit),
      .status = std::string(GetResultString(event_status)),
      .timestamp = Logger::GetLogFormatTimestamp(),
  };
  UpdateConnectionHistory(activity);

  if (event_status == static_cast<uint8_t>(EventResultCode::kSuccess)) {
    connected_device_address_.erase(activity.connection_handle);
    LOG(INFO) << __func__ << ": " << activity.event << ", connection handle: "
              << ToHexString(activity.connection_handle, kUint16HexStringDigit)
              << ", BD address: " << activity.bd_address.ToString() << ".";
  }
}

void BluetoothActivitiesImpl::UpdateConnectionHistory(
    const ConnectionActivity& device) {
  if (connection_history_.size() >= kBtMaxConnectHistoryRecord) {
    connection_history_.pop_front();
  }
  connection_history_.emplace_back(device);
}

}  // namespace debug
}  // namespace bluetooth_hal
