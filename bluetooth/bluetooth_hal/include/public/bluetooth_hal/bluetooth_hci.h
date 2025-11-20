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

#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "aidl/android/hardware/bluetooth/BnBluetoothHci.h"
#include "aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h"
#include "android/binder_auto_utils.h"
#include "android/binder_status.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {

class BluetoothHalDeathRecipient;

class BluetoothHci
    : public ::aidl::android::hardware::bluetooth::BnBluetoothHci {
 public:
  static BluetoothHci& GetHci();

  BluetoothHci();
  ::ndk::ScopedAStatus initialize(
      const std::shared_ptr<
          ::aidl::android::hardware::bluetooth::IBluetoothHciCallbacks>& cb)
      override;
  ::ndk::ScopedAStatus sendHciCommand(
      const std::vector<uint8_t>& packet) override;
  ::ndk::ScopedAStatus sendAclData(const std::vector<uint8_t>& data) override;
  ::ndk::ScopedAStatus sendScoData(const std::vector<uint8_t>& data) override;
  ::ndk::ScopedAStatus sendIsoData(const std::vector<uint8_t>& data) override;
  ::ndk::ScopedAStatus close() override;
  binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

 private:
  void DispatchPacketToStack(const ::bluetooth_hal::hci::HalPacket& packet);
  void HandleHalStateChanged(::bluetooth_hal::HalState new_state,
                             ::bluetooth_hal::HalState old_state);
  void SendDataToController(const ::bluetooth_hal::hci::HalPacket& packet);
  static void SigtermHandler(int signum);

  std::shared_ptr<::aidl::android::hardware::bluetooth::IBluetoothHciCallbacks>
      bluetooth_hci_callback_;
  std::shared_ptr<BluetoothHalDeathRecipient> death_recipient_;
  std::function<void(std::shared_ptr<BluetoothHalDeathRecipient>&)> unlink_cb_;
  bool is_initializing_;
  static std::atomic<bool> is_signal_handled_;
};

}  // namespace bluetooth_hal
