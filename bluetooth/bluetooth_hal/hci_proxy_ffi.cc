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

#define LOG_TAG "bluetooth_hal.hci_proxy_ffi"

#include "bluetooth_hal/hci_proxy_ffi.h"

#include <csignal>
#include <cstdint>
#include <memory>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/bluetooth_hci.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {

using ::aidl::android::hardware::bluetooth::hal::IBluetoothHciCallbacks;
using ::aidl::android::hardware::bluetooth::hal::Status;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;

class HciProxyFfiCallback : public BluetoothHciCallback {
 public:
  HciProxyFfiCallback(const std::shared_ptr<IBluetoothHciCallbacks>& cb)
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

HciProxyFfi::HciProxyFfi() {
  ANCHOR_LOG_INFO(AnchorType::kStartHci)
      << __func__ << ": Starting BluetoothHci with ffi proxy.";
  BluetoothHci::StartHci();
  std::signal(SIGTERM, SigtermHandler);
}

void HciProxyFfi::SigtermHandler(int signum) {
  BluetoothHci::GetHci().HandleSignal(signum);
}

void HciProxyFfi::initialize(
    const std::shared_ptr<IBluetoothHciCallbacks>& cb) {
  BluetoothHci::GetHci().Initialize(std::make_shared<HciProxyFfiCallback>(cb));
}

void HciProxyFfi::sendHciCommand(const std::vector<uint8_t>& command) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kCommand), command);
  BluetoothHci::GetHci().SendHciCommand(packet);
}

void HciProxyFfi::sendAclData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kAclData), data);
  BluetoothHci::GetHci().SendAclData(packet);
}

void HciProxyFfi::sendScoData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kScoData), data);
  BluetoothHci::GetHci().SendScoData(packet);
}

void HciProxyFfi::sendIsoData(const std::vector<uint8_t>& data) {
  HalPacket packet(static_cast<uint8_t>(HciPacketType::kIsoData), data);
  BluetoothHci::GetHci().SendIsoData(packet);
}

void HciProxyFfi::clientDied() { BluetoothHci::GetHci().HandleServiceDied(); }

void HciProxyFfi::close() { BluetoothHci::GetHci().Close(); }

void HciProxyFfi::dump(int fd) { BluetoothHci::GetHci().Dump(fd); }

}  // namespace bluetooth_hal
