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

#define LOG_TAG "bluetooth_hal.hci"

#include "bluetooth_hal/bluetooth_hci.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>

#include "android-base/logging.h"
#include "bluetooth_hal/bluetooth_hci_callback.h"
#include "bluetooth_hal/debug/bluetooth_activities.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/extensions/finder/bluetooth_finder_handler.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router.h"
#include "bluetooth_hal/util/power/wakelock.h"

namespace bluetooth_hal {

using ::bluetooth_hal::BluetoothHciCallback;
using ::bluetooth_hal::HalState;
using ::bluetooth_hal::debug::BluetoothActivities;
using ::bluetooth_hal::debug::DebugCentral;
using ::bluetooth_hal::extensions::finder::BluetoothFinderHandler;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HalPacketCallback;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::HciRouter;
using ::bluetooth_hal::hci::HciRouterCallback;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::util::power::ScopedWakelock;
using ::bluetooth_hal::util::power::WakeSource;

using HalStateChangedCallback = std::function<void(HalState, HalState)>;

class HciCallback : public HciRouterCallback {
 public:
  HciCallback(const HalPacketCallback& dispatch_packet_to_stack,
              const HalStateChangedCallback& handle_hal_state_changed)
      : dispatch_packet_to_stack_(dispatch_packet_to_stack),
        handle_hal_state_changed_(handle_hal_state_changed) {}

  void OnCommandCallback(const HalPacket& packet) override {
    OnPacketCallback(packet);
  }

  MonitorMode OnPacketCallback(const HalPacket& packet) override {
    dispatch_packet_to_stack_(packet);
    return MonitorMode::kNone;
  }

  void OnHalStateChanged(const HalState new_state, const HalState old_state) {
    handle_hal_state_changed_(new_state, old_state);
  }

 private:
  HalPacketCallback dispatch_packet_to_stack_;
  HalStateChangedCallback handle_hal_state_changed_;
};

BluetoothHci::BluetoothHci() : bluetooth_hci_callback_(nullptr) {
  // Lazily construct the static HciRouter instance.
  HciRouter::GetRouter();
  BluetoothActivities::Start();
}

void BluetoothHci::HandleSignal(int signum) {
  LOG(ERROR) << __func__ << ": Received signal: " << signum;

  if (is_sigterm_handled_.exchange(true)) {
    LOG(WARNING) << __func__ << ": Signal is already handled, Skip.";
    return;
  }

  if (!BluetoothFinderHandler::GetHandler().StartPoweredOffFinderMode()) {
    // Shutdown lower layer if finder is not enabled.
    Close();
  }

  kill(getpid(), SIGKILL);
}

void BluetoothHci::HandleServiceDied() {
  ANCHOR_LOG(AnchorType::kServiceDied) << __func__;
  {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (bluetooth_hci_callback_ == nullptr) {
      HAL_LOG(ERROR) << __func__ << ": called but callback is null";
      return;
    }
  }
  HAL_LOG(ERROR) << __func__ << ": Bluetooth service died!";
  if (DebugCentral::Get().IsCoredumpGenerated()) {
    LOG(ERROR) << __func__
               << ": Restart Bluetooth HAL after coredump is generated";
    kill(getpid(), SIGKILL);
  }
  Close();
}

bool BluetoothHci::Initialize(const std::shared_ptr<BluetoothHciCallback>& cb) {
  SCOPED_ANCHOR(AnchorType::kInitialize, __func__);
  ScopedWakelock wakelock(WakeSource::kInitialize);

  HAL_LOG(INFO) << "Initializing Bluetooth HAL, cb=" << cb;
  {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (bluetooth_hci_callback_ != nullptr) {
      HAL_LOG(WARNING) << "The HAL has already been initialized!";
      cb->InitializationComplete(BluetoothHciStatus::kHardwareInitializeError);
      return false;
    }

    is_initializing_ = true;
    bluetooth_hci_callback_ = cb;
  }

  auto callback = std::make_shared<HciCallback>(
      std::bind_front(&BluetoothHci::DispatchPacketToStack, this),
      std::bind_front(&BluetoothHci::HandleHalStateChanged, this));
  if (!HciRouter::GetRouter().Initialize(callback)) {
    HAL_LOG(ERROR) << "Failed to initialize HciRouter!";
    std::lock_guard<std::mutex> lock(callback_mutex_);
    is_initializing_ = false;
    bluetooth_hci_callback_ = nullptr;
  }
  return true;
}

bool BluetoothHci::SendHciCommand(const HalPacket& packet) {
  SCOPED_ANCHOR(
      AnchorType::kSendHciCommand,
      (std::stringstream() << __func__ << ": 0x" << std::hex << std::setw(4)
                           << std::setfill('0') << packet.GetCommandOpcode()
                           << " - " << std::dec << packet.size() << " bytes")
          .str());
  SendDataToController(packet);
  return true;
}

bool BluetoothHci::SendAclData(const HalPacket& packet) {
  SCOPED_ANCHOR(
      AnchorType::kSendAclData,
      (std::stringstream() << __func__ << ": " << packet.size() << " bytes")
          .str());
  SendDataToController(packet);
  return true;
}

bool BluetoothHci::SendScoData(const HalPacket& packet) {
  SCOPED_ANCHOR(
      AnchorType::kSendScoData,
      (std::stringstream() << __func__ << ": " << packet.size() << " bytes")
          .str());
  SendDataToController(packet);
  return true;
}

bool BluetoothHci::SendIsoData(const HalPacket& packet) {
  SCOPED_ANCHOR(
      AnchorType::kSendIsoData,
      (std::stringstream() << __func__ << ": " << packet.size() << " bytes")
          .str());
  SendDataToController(packet);
  return true;
}

bool BluetoothHci::Close() {
  {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    bluetooth_hci_callback_ = nullptr;
  }
  ANCHOR_LOG_INFO(AnchorType::kClose) << __func__;
  HAL_LOG(INFO) << __func__;
  ScopedWakelock wakelock(WakeSource::kClose);

  const bool is_sigterm = is_sigterm_handled_;
  if (is_sigterm) {
    // Shutdown the lower layer directly if the Close was from a SIGTERM.
    HciRouter::GetRouter().Cleanup();
  } else {
    HciRouter::GetRouter().Close();
  }
  return true;
}

bool BluetoothHci::Dump(int fd) {
  HAL_LOG(INFO) << __func__ << ": Dump debug log";
#ifndef UNIT_TEST
  DebugCentral::Get().Dump(fd);
#endif
  fsync(fd);
  return true;
}

void BluetoothHci::SendDataToController(const HalPacket& packet) {
  HciRouter::GetRouter().Send(packet);
}

void BluetoothHci::DispatchPacketToStack(const HalPacket& packet) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  if (bluetooth_hci_callback_ == nullptr) {
    LOG(ERROR) << "bluetooth_hci_callback is null! packet="
               << packet.ToString();
    return;
  }
  HciPacketType type = packet.GetType();
  switch (type) {
    case HciPacketType::kEvent: {
      SCOPED_ANCHOR(
          AnchorType::kCallbackHciEvent,
          (std::stringstream() << "BluetoothHciCallback->hciEventReceived: "
                               << packet.size() << " bytes")
              .str());
      bluetooth_hci_callback_->HciEventReceived(packet);
      break;
    }
    case HciPacketType::kAclData: {
      SCOPED_ANCHOR(
          AnchorType::kCallbackAclData,
          (std::stringstream() << "BluetoothHciCallback->aclDataReceived: "
                               << packet.size() << " bytes")
              .str());
      bluetooth_hci_callback_->AclDataReceived(packet);
      break;
    }
    case HciPacketType::kScoData: {
      SCOPED_ANCHOR(
          AnchorType::kCallbackScoData,
          (std::stringstream() << "BluetoothHciCallback->scoDataReceived: "
                               << packet.size() << " bytes")
              .str());
      bluetooth_hci_callback_->ScoDataReceived(packet);
      break;
    }
    case HciPacketType::kIsoData: {
      SCOPED_ANCHOR(
          AnchorType::kCallbackIsoData,
          (std::stringstream() << "BluetoothHciCallback->isoDataReceived: "
                               << packet.size() << " bytes")
              .str());
      bluetooth_hci_callback_->IsoDataReceived(packet);
      break;
    }
    default:
      LOG(ERROR) << "Unexpected packet type: " << packet.ToString();
      break;
  }
}

void BluetoothHci::HandleHalStateChanged(HalState new_state,
                                         [[maybe_unused]] HalState old_state) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  if (is_initializing_ && bluetooth_hci_callback_ != nullptr) {
    switch (new_state) {
      case HalState::kRunning:
        LOG(INFO) << "Initialization Complete!";
        is_initializing_ = false;
        bluetooth_hci_callback_->InitializationComplete(
            BluetoothHciStatus::kSuccess);
        break;
      case HalState::kShutdown:
        LOG(ERROR) << "Unexpected state change during initialization!";
        is_initializing_ = false;
        bluetooth_hci_callback_->InitializationComplete(
            BluetoothHciStatus::kHardwareInitializeError);
        break;
      default:
        break;
    }
  }
}

BluetoothHci& BluetoothHci::GetHci() {
  static BluetoothHci hci;
  return hci;
}

}  // namespace bluetooth_hal
