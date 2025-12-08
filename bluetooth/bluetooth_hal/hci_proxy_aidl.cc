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

#define LOG_TAG "bluetooth_hal.hci_proxy_aidl"

#include "bluetooth_hal/hci_proxy_aidl.h"

#include <csignal>
#include <cstdint>
#include <memory>
#include <vector>

#include "aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h"
#include "aidl/android/hardware/bluetooth/Status.h"
#include "android-base/logging.h"
#include "android/binder_auto_utils.h"
#include "android/binder_status.h"
#include "bluetooth_hal/bluetooth_hci.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {

using ::aidl::android::hardware::bluetooth::IBluetoothHciCallbacks;
using ::aidl::android::hardware::bluetooth::Status;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;
using ::ndk::ScopedAStatus;

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
      // Do not crash here as the Bluetooth process could just got killed when
      // the device is shutting down.
      LOG(ERROR) << "Unable to unlink to death recipient";
    }
  }

  void ServiceDied() {
    if (bluetooth_hci_callback_ != nullptr &&
        !AIBinder_isAlive(bluetooth_hci_callback_->asBinder().get())) {
      LOG(ERROR)
          << "BluetoothHalDeathRecipient::serviceDied - Bluetooth service died";
      has_died_ = true;
    } else {
      LOG(ERROR) << "BluetoothHalDeathRecipient::serviceDied called but "
                    "service not dead";
    }
    BluetoothHci::GetHci().HandleServiceDied();
  }

  bool GetHasDied() const { return has_died_; }
  void SetHasDied(bool died) { has_died_ = died; }

 private:
  bool has_died_;
  std::shared_ptr<IBluetoothHciCallbacks> bluetooth_hci_callback_;
  AIBinder_DeathRecipient* client_death_recipient_;
};

class HciProxyCallback : public BluetoothHciCallback {
 public:
  HciProxyCallback(const std::shared_ptr<IBluetoothHciCallbacks>& cb)
      : bluetooth_hci_callback_(cb) {};

  void InitializationComplete(BluetoothHciStatus status) override {
    auto hci_status = BluetoothHciStatusToAidlStatus(status);
    bluetooth_hci_callback_->initializationComplete(hci_status);
  }

  void HciEventReceived(const HalPacket& packet) override {
    bluetooth_hci_callback_->hciEventReceived(packet.GetBody());
  }

  void AclDataReceived(const HalPacket& packet) override {
    bluetooth_hci_callback_->aclDataReceived(packet.GetBody());
  }

  void ScoDataReceived(const HalPacket& packet) override {
    bluetooth_hci_callback_->scoDataReceived(packet.GetBody());
  }

  void IsoDataReceived(const HalPacket& packet) override {
    bluetooth_hci_callback_->isoDataReceived(packet.GetBody());
  }

 private:
  Status BluetoothHciStatusToAidlStatus(BluetoothHciStatus status) {
    switch (status) {
      case BluetoothHciStatus::kSuccess:
        return Status::SUCCESS;
      case BluetoothHciStatus::kAlreadyInitialized:
        return Status::ALREADY_INITIALIZED;
      case BluetoothHciStatus::kHardwareInitializeError:
        return Status::HARDWARE_INITIALIZATION_ERROR;
      default:
        break;
    }
    return Status::UNKNOWN;
  }

  std::shared_ptr<IBluetoothHciCallbacks> bluetooth_hci_callback_;
};

HciProxyAidl::HciProxyAidl()
    : death_recipient_(std::make_shared<BluetoothHalDeathRecipient>()) {
  ANCHOR_LOG_INFO(AnchorType::kStartHci)
      << __func__ << ": Starting BluetoothHci with aidl proxy.";
  BluetoothHci::StartHci();
  std::signal(SIGTERM, SigtermHandler);
}

void HciProxyAidl::SigtermHandler(int signum) {
  BluetoothHci::GetHci().HandleSignal(signum);
}

ScopedAStatus HciProxyAidl::initialize(
    const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
  bool status =
      BluetoothHci::GetHci().Initialize(std::make_shared<HciProxyCallback>(cb));

  if (!status) {
    return ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }

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

  return ScopedAStatus::ok();
}

ScopedAStatus HciProxyAidl::sendHciCommand(
    const std::vector<uint8_t>& command) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kCommand), command);
  BluetoothHci::GetHci().SendHciCommand(packet);
  return ScopedAStatus::ok();
}

ScopedAStatus HciProxyAidl::sendAclData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kAclData), data);
  BluetoothHci::GetHci().SendAclData(packet);
  return ScopedAStatus::ok();
}

ScopedAStatus HciProxyAidl::sendScoData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kScoData), data);
  BluetoothHci::GetHci().SendScoData(packet);
  return ScopedAStatus::ok();
}

ScopedAStatus HciProxyAidl::sendIsoData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kIsoData), data);
  BluetoothHci::GetHci().SendIsoData(packet);
  return ScopedAStatus::ok();
}

ScopedAStatus HciProxyAidl::close() {
  unlink_cb_(death_recipient_);
  BluetoothHci::GetHci().Close();
  return ScopedAStatus::ok();
}

binder_status_t HciProxyAidl::dump(int fd, const char**, uint32_t) {
  BluetoothHci::GetHci().Dump(fd);
  return STATUS_OK;
}

}  // namespace bluetooth_hal
