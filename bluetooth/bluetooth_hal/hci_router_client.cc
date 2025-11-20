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

#define LOG_TAG "bthal.router_client"

#include "bluetooth_hal/hci_router_client.h"

#include <algorithm>
#include <functional>
#include <map>
#include <mutex>

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router.h"
#include "bluetooth_hal/hci_router_client_agent.h"

namespace bluetooth_hal {
namespace hci {

using ::bluetooth_hal::HalState;

HciRouterClient::HciRouterClient() {
  HciRouterClientAgent::GetAgent().RegisterRouterClient(this);
}

HciRouterClient::~HciRouterClient() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  monitors_.clear();
  HciRouterClientAgent::GetAgent().UnregisterRouterClient(this);
}

MonitorMode HciRouterClient::OnPacketCallback(const HalPacket& packet) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  // Find the mode with the highest priority.
  MonitorMode mode = MonitorMode::kNone;
  for (const auto& it : monitors_) {
    if (it.first == packet) {
      mode = (it.second > mode) ? it.second : mode;
    }
  }

  if (mode != MonitorMode::kNone) {
    OnMonitorPacketCallback(mode, packet);
  }
  return mode;
}

bool HciRouterClient::IsBluetoothChipReady() {
  return HciRouterClientAgent::GetAgent().IsBluetoothChipReady();
}

bool HciRouterClient::IsBluetoothEnabled() {
  return HciRouterClientAgent::GetAgent().IsBluetoothEnabled();
}

bool HciRouterClient::RegisterMonitor(const HciMonitor& monitor,
                                      MonitorMode mode) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (mode == MonitorMode::kNone) {
    LOG(ERROR) << __func__ << ": Monitor mode cannot be kNone!";
    return false;
  }
  auto it = std::find_if(
      monitors_.begin(), monitors_.end(),
      [&monitor](const auto& entry) { return entry.first == monitor; });
  if (it != monitors_.end()) {
    LOG(ERROR) << __func__ << ": The same monitor already exist!";
    return false;
  }
  monitors_.insert({monitor, mode});
  return true;
}

bool HciRouterClient::UnregisterMonitor(const HciMonitor& monitor) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  auto it = std::find_if(
      monitors_.begin(), monitors_.end(),
      [&monitor](const auto& entry) { return entry.first == monitor; });
  if (it == monitors_.end()) {
    LOG(ERROR) << __func__ << ": Monitor not registered!";
    return false;
  }
  monitors_.erase(it);
  return true;
}

bool HciRouterClient::SendCommand(const HalPacket& packet) {
  if (packet.GetType() != HciPacketType::kCommand) {
    return false;
  }
  return HciRouter::GetRouter().SendCommand(
      packet, std::bind_front(&HciRouterClient::OnCommandCallback, this));
}

bool HciRouterClient::SendData(const HalPacket& packet) {
  if (packet.GetType() == HciPacketType::kCommand) {
    return false;
  }
  return HciRouter::GetRouter().Send(packet);
}

}  // namespace hci
}  // namespace bluetooth_hal
