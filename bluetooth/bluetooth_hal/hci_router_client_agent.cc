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

#define LOG_TAG "bthal.router_client_agent"

#include "bluetooth_hal/hci_router_client_agent.h"

#include <mutex>
#include <unordered_set>

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace hci {

class HciRouterClientAgentImpl : public HciRouterClientAgent {
 public:
  HciRouterClientAgentImpl()
      : current_state_(HalState::kShutdown),
        is_bluetooth_chip_ready_(false),
        is_bluetooth_enabled_(false) {};
  bool RegisterRouterClient(HciRouterClientCallback* callback) override;
  bool UnregisterRouterClient(HciRouterClientCallback* callback) override;
  MonitorMode DispatchPacketToClients(const HalPacket& packet) override;
  void NotifyHalStateChange(HalState new_state, HalState old_state) override;
  bool IsBluetoothEnabled() override;
  bool IsBluetoothChipReady() override;

 private:
  void HandleBluetoothEnable(const HalPacket& packet);
  void NotifyClientsBluetoothDisabled();
  void NotifyClientsBluetoothEnabled();
  void NotifyClientsBluetoothChipClosed();
  void NotifyClientsBluetoothChipReady();

  std::recursive_mutex mutex_;
  HalState current_state_;
  bool is_bluetooth_chip_ready_;
  bool is_bluetooth_enabled_;
  std::unordered_set<HciRouterClientCallback*> router_clients_;
};

HciRouterClientAgent& HciRouterClientAgent::GetAgent() {
  static HciRouterClientAgentImpl agent;
  return agent;
}

bool HciRouterClientAgentImpl::RegisterRouterClient(
    HciRouterClientCallback* client) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (router_clients_.count(client) > 0) {
    LOG(WARNING) << "callback already registered!";
    return false;
  }
  router_clients_.insert(client);

  if (IsBluetoothChipReady()) {
    client->OnBluetoothChipReady();
  }
  if (IsBluetoothEnabled()) {
    client->OnBluetoothEnabled();
  }
  return true;
}

bool HciRouterClientAgentImpl::UnregisterRouterClient(
    HciRouterClientCallback* callback) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (router_clients_.erase(callback) == 0) {
    LOG(WARNING) << "callback was not registered!";
    return false;
  }
  return true;
}

MonitorMode HciRouterClientAgentImpl::DispatchPacketToClients(
    const HalPacket& packet) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (!IsBluetoothEnabled()) {
    // Look for HCI_RESET complete event if Bluetooth is not enabled.
    HandleBluetoothEnable(packet);
  }

  MonitorMode result = MonitorMode::kNone;
  for (auto client : router_clients_) {
    if (client == nullptr) {
      LOG(WARNING) << "null router client callback in the registration list!";
      continue;
    }
    MonitorMode mode = client->OnPacketCallback(packet);
    result = (mode > result) ? mode : result;
  }
  return result;
}

void HciRouterClientAgentImpl::NotifyHalStateChange(HalState new_state,
                                                    HalState old_state) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);

#ifndef UNIT_TEST
  if (current_state_ > old_state) {
    LOG(FATAL) << __func__
               << " (old_state, current_state_in_client) is mismatched! "
                  "[ old_state("
               << static_cast<int>(old_state) << ") -> new_state("
               << static_cast<int>(new_state)
               << ") ], current_state_in_client: "
               << static_cast<int>(current_state_);
    return;
  }
#endif

  current_state_ = new_state;

  switch (new_state) {
    case HalState::kBtChipReady:
      if (!is_bluetooth_chip_ready_) {
        NotifyClientsBluetoothChipReady();
      }
      if (is_bluetooth_enabled_) {
        NotifyClientsBluetoothDisabled();
      }
      is_bluetooth_chip_ready_ = true;
      is_bluetooth_enabled_ = false;
      break;
    case HalState::kRunning:
      if (!is_bluetooth_chip_ready_) {
        NotifyClientsBluetoothChipReady();
      }
      // We do not handle is_bluetooth_enabled_ here because the clients have to
      // wait for a HCI_RESET before they can send packets to the chip.
      is_bluetooth_chip_ready_ = true;
      break;
    default:
      if (is_bluetooth_chip_ready_) {
        NotifyClientsBluetoothChipClosed();
      }
      if (is_bluetooth_enabled_) {
        NotifyClientsBluetoothDisabled();
      }
      is_bluetooth_chip_ready_ = false;
      is_bluetooth_enabled_ = false;
      break;
  }

  for (auto client : router_clients_) {
    if (client == nullptr) {
      LOG(WARNING) << __func__
                   << ": null router client callback in the registration list!";
      continue;
    }
    client->OnHalStateChanged(new_state, old_state);
  }
}

bool HciRouterClientAgentImpl::IsBluetoothEnabled() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  return is_bluetooth_enabled_;
}

bool HciRouterClientAgentImpl::IsBluetoothChipReady() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  return is_bluetooth_chip_ready_;
}

void HciRouterClientAgentImpl::HandleBluetoothEnable(const HalPacket& packet) {
  if (current_state_ == HalState::kRunning &&
      packet.GetCommandOpcodeFromGeneratedEvent() ==
          static_cast<uint16_t>(CommandOpCode::kHciReset) &&
      packet.GetCommandCompleteEventResult() ==
          static_cast<uint8_t>(EventResultCode::kSuccess)) {
    // Inform the client that Bluetooth has enabled after a HCI_RESET command is
    // sent in kRunning state.
    is_bluetooth_enabled_ = true;
    NotifyClientsBluetoothEnabled();
  }
}

void HciRouterClientAgentImpl::NotifyClientsBluetoothDisabled() {
  for (auto client : router_clients_) {
    if (client == nullptr) {
      LOG(WARNING) << __func__
                   << ": null router client callback in the registration list!";
      continue;
    }
    client->OnBluetoothDisabled();
  }
}

void HciRouterClientAgentImpl::NotifyClientsBluetoothEnabled() {
  for (auto client : router_clients_) {
    if (client == nullptr) {
      LOG(WARNING) << __func__
                   << ": null router client callback in the registration list!";
      continue;
    }
    client->OnBluetoothEnabled();
  }
}

void HciRouterClientAgentImpl::NotifyClientsBluetoothChipClosed() {
  for (auto client : router_clients_) {
    if (client == nullptr) {
      LOG(WARNING) << __func__
                   << ": null router client callback in the registration list!";
      continue;
    }
    client->OnBluetoothChipClosed();
  }
}

void HciRouterClientAgentImpl::NotifyClientsBluetoothChipReady() {
  for (auto client : router_clients_) {
    if (client == nullptr) {
      LOG(WARNING) << __func__
                   << ": null router client callback in the registration list!";
      continue;
    }
    client->OnBluetoothChipReady();
  }
}

}  // namespace hci
}  // namespace bluetooth_hal
