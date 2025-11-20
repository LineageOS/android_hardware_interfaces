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

#define LOG_TAG "bthal.hci"

#include "bluetooth_hal/bluetooth_hci.h"

#include <atomic>
#include <csignal>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

#include "aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h"
#include "aidl/android/hardware/bluetooth/Status.h"
#include "android-base/logging.h"
#include "android/binder_auto_utils.h"
#include "android/binder_status.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/extensions/finder/bluetooth_finder_handler.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router.h"
#include "bluetooth_hal/util/power/wakelock.h"

namespace bluetooth_hal {

using ::aidl::android::hardware::bluetooth::IBluetoothHciCallbacks;
using ::aidl::android::hardware::bluetooth::Status;
using ::bluetooth_hal::HalState;
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

using ::ndk::ScopedAStatus;

using HalStateChangedCallback = std::function<void(HalState, HalState)>;

std::atomic<bool> BluetoothHci::is_signal_handled_{false};

class BluetoothHalDeathRecipient {
 public:
  void LinkToDeath(const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
    bluetooth_hci_callback_ = cb;

    auto on_link_died = [](void* cookie) {
      auto* death_recipient = static_cast<BluetoothHalDeathRecipient*>(cookie);
      death_recipient->ServiceDied();
    };

    client_death_recipient_ = AIBinder_DeathRecipient_new(on_link_died);

    binder_status_t link_to_death_return_status =
        AIBinder_linkToDeath(bluetooth_hci_callback_->asBinder().get(),
                             client_death_recipient_, this /* cookie */);

    if (link_to_death_return_status != STATUS_OK) {
      LOG(FATAL) << "Unable to link to death recipient";
    }
  }

  void UnlinkToDeath(const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
    if (cb != bluetooth_hci_callback_) {
      LOG(FATAL) << "Unable to unlink mismatched pointers";
    }

    binder_status_t unlink_to_death_return_status =
        AIBinder_unlinkToDeath(bluetooth_hci_callback_->asBinder().get(),
                               client_death_recipient_, this);

    if (unlink_to_death_return_status != STATUS_OK) {
      LOG(FATAL) << "Unable to unlink to death recipient";
    }
  }

  void ServiceDied() {
    if (bluetooth_hci_callback_ != nullptr &&
        !AIBinder_isAlive(bluetooth_hci_callback_->asBinder().get())) {
      LOG(ERROR)
          << "BluetoothHalDeathRecipient::serviceDied - Bluetooth service died";
    } else {
      LOG(ERROR) << "BluetoothHalDeathRecipient::serviceDied called but "
                    "service not dead";
      return;
    }
    has_died_ = true;
    ANCHOR_LOG(AnchorType::SERVICE_DIED) << __func__;

    // TODO: b/414524533 - remove FATAL message once the bug is fixed.
    LOG(FATAL) << "Bluetooth stack ServiceDied!";

    BluetoothHci::GetHci().close();
  }
  bool GetHasDied() const { return has_died_; }
  void SetHasDied(bool died) { has_died_ = died; }

 private:
  bool has_died_;
  std::shared_ptr<IBluetoothHciCallbacks> bluetooth_hci_callback_;
  AIBinder_DeathRecipient* client_death_recipient_;
};

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

BluetoothHci::BluetoothHci()
    : bluetooth_hci_callback_(nullptr),
      death_recipient_(std::make_shared<BluetoothHalDeathRecipient>()) {
  // Get configs.

  std::signal(SIGTERM, SigtermHandler);

  // Lazily construct the static HciRouter instance.
  HciRouter::GetRouter();
}

void BluetoothHci::SigtermHandler(int signum) {
  LOG(ERROR) << __func__ << ": Received signal: " << signum;

  if (is_signal_handled_.exchange(true)) {
    LOG(WARNING) << __func__ << ": Signal is already handled, Skip.";
    return;
  }

  if (BluetoothFinderHandler::GetHandler().StartPoweredOffFinderMode()) {
    return;
  }

  BluetoothHci::GetHci().close();
  kill(getpid(), SIGKILL);
}

ScopedAStatus BluetoothHci::initialize(
    const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
  DURATION_TRACKER(AnchorType::BTHAL_INIT, __func__);
  ScopedWakelock wakelock(WakeSource::kInitialize);

  LOG(INFO) << "Initializing Bluetooth HAL.";
  if (bluetooth_hci_callback_ != nullptr) {
    LOG(WARNING) << "The HAL has already been initialized!";
    cb->initializationComplete(Status::ALREADY_INITIALIZED);
    return ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }

  is_initializing_ = true;
  bluetooth_hci_callback_ = cb;

  death_recipient_->SetHasDied(false);
  death_recipient_->LinkToDeath(cb);
  unlink_cb_ =
      [cb](std::shared_ptr<BluetoothHalDeathRecipient>& death_recipient) {
        if (death_recipient->GetHasDied()) {
          LOG(INFO) << "Skipping unlink call, service died.";
        } else {
          death_recipient->UnlinkToDeath(cb);
        }
      };

  auto callback = std::make_shared<HciCallback>(
      std::bind_front(&BluetoothHci::DispatchPacketToStack, this),
      std::bind_front(&BluetoothHci::HandleHalStateChanged, this));
  bool status = HciRouter::GetRouter().Initialize(callback);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

ScopedAStatus BluetoothHci::sendHciCommand(
    const std::vector<uint8_t>& command) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kCommand), command);
  DURATION_TRACKER(
      AnchorType::SEND_HCI_CMD,
      (std::stringstream() << __func__ << ": 0x" << std::hex << std::setw(4)
                           << std::setfill('0') << packet.GetCommandOpcode()
                           << " - " << std::dec << packet.size() << " bytes")
          .str());
  SendDataToController(packet);
  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothHci::sendAclData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kAclData), data);
  DURATION_TRACKER(
      AnchorType::SEND_ACL_DAT,
      (std::stringstream() << __func__ << ": " << packet.size() << " bytes")
          .str());
  SendDataToController(packet);
  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothHci::sendScoData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kScoData), data);
  DURATION_TRACKER(
      AnchorType::SEND_SCO_DAT,
      (std::stringstream() << __func__ << ": " << packet.size() << " bytes")
          .str());
  SendDataToController(packet);
  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothHci::sendIsoData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kIsoData), data);
  DURATION_TRACKER(
      AnchorType::SEND_ISO_DAT,
      (std::stringstream() << __func__ << ": " << packet.size() << " bytes")
          .str());
  SendDataToController(packet);
  return ScopedAStatus::ok();
}

ScopedAStatus BluetoothHci::close() {
  bluetooth_hci_callback_ = nullptr;
  ANCHOR_LOG_INFO(AnchorType::BTHAL_CLOSE) << __func__;
  ScopedWakelock wakelock(WakeSource::kClose);
  HciRouter::GetRouter().Cleanup();
  unlink_cb_(death_recipient_);
  return ScopedAStatus::ok();
}

binder_status_t BluetoothHci::dump(int fd, const char**, uint32_t) {
  LOG(INFO) << __func__ << ": Dump debug log";
#ifndef UNIT_TEST
  DebugCentral::Get()->Dump(fd);
#endif
  fsync(fd);
  return STATUS_OK;
}

void BluetoothHci::SendDataToController(const HalPacket& packet) {
  HciRouter::GetRouter().Send(packet);
}

void BluetoothHci::DispatchPacketToStack(const HalPacket& packet) {
  if (bluetooth_hci_callback_ == nullptr) {
    LOG(ERROR) << "bluetooth_hci_callback is null!";
    return;
  }
  HciPacketType type = packet.GetType();
  switch (type) {
    case HciPacketType::kEvent: {
      DURATION_TRACKER(AnchorType::CALLBACK_HCI_EVT,
                       (std::stringstream() << "cb->hciEventReceived: "
                                            << packet.size() << " bytes")
                           .str());
      bluetooth_hci_callback_->hciEventReceived(packet.GetBody());
      break;
    }
    case HciPacketType::kAclData: {
      DURATION_TRACKER(AnchorType::CALLBACK_HCI_ACL,
                       (std::stringstream()
                        << "cb->aclDataReceived: " << packet.size() << " bytes")
                           .str());
      bluetooth_hci_callback_->aclDataReceived(packet.GetBody());
      break;
    }
    case HciPacketType::kScoData: {
      DURATION_TRACKER(AnchorType::CALLBACK_HCI_SCO,
                       (std::stringstream()
                        << "cb->scoDataReceived: " << packet.size() << " bytes")
                           .str());
      bluetooth_hci_callback_->scoDataReceived(packet.GetBody());
      break;
    }
    case HciPacketType::kIsoData: {
      DURATION_TRACKER(AnchorType::CALLBACK_HCI_ISO,
                       (std::stringstream()
                        << "cb->isoDataReceived: " << packet.size() << " bytes")
                           .str());
      bluetooth_hci_callback_->isoDataReceived(packet.GetBody());
      break;
    }
    default:
      LOG(ERROR) << "Unexpected packet type: " << packet.ToString();
      break;
  }
}

void BluetoothHci::HandleHalStateChanged(HalState new_state,
                                         [[maybe_unused]] HalState old_state) {
  if (is_initializing_ && bluetooth_hci_callback_ != nullptr) {
    switch (new_state) {
      case HalState::kRunning:
        LOG(INFO) << "Initialization Complete!";
        is_initializing_ = false;
        bluetooth_hci_callback_->initializationComplete(Status::SUCCESS);
        break;
      case HalState::kShutdown:
        LOG(ERROR) << "Unexpected state change during initialization!";
        is_initializing_ = false;
        bluetooth_hci_callback_->initializationComplete(
            Status::HARDWARE_INITIALIZATION_ERROR);
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
