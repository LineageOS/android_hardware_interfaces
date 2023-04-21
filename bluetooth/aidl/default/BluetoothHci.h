/*
 * Copyright 2022 The Android Open Source Project
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

#include <aidl/android/hardware/bluetooth/BnBluetoothHci.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHciCallbacks.h>

#include <future>
#include <string>

#include "async_fd_watcher.h"
#include "h4_protocol.h"
#include "net_bluetooth_mgmt.h"

namespace aidl::android::hardware::bluetooth::impl {

class BluetoothDeathRecipient;

// This Bluetooth HAL implementation connects with a serial port at dev_path_.
class BluetoothHci : public BnBluetoothHci {
 public:
  BluetoothHci(const std::string& dev_path = "/dev/hvc5");

  ndk::ScopedAStatus initialize(
      const std::shared_ptr<IBluetoothHciCallbacks>& cb) override;

  ndk::ScopedAStatus sendHciCommand(
      const std::vector<uint8_t>& packet) override;

  ndk::ScopedAStatus sendAclData(const std::vector<uint8_t>& packet) override;

  ndk::ScopedAStatus sendScoData(const std::vector<uint8_t>& packet) override;

  ndk::ScopedAStatus sendIsoData(const std::vector<uint8_t>& packet) override;

  ndk::ScopedAStatus close() override;

  static void OnPacketReady();

  static BluetoothHci* get();

 private:
  int mFd{-1};
  std::shared_ptr<IBluetoothHciCallbacks> mCb = nullptr;

  std::shared_ptr<::android::hardware::bluetooth::hci::H4Protocol> mH4;

  std::shared_ptr<BluetoothDeathRecipient> mDeathRecipient;

  std::string mDevPath;

  ::android::hardware::bluetooth::async::AsyncFdWatcher mFdWatcher;

  int getFdFromDevPath();
  [[nodiscard]] ndk::ScopedAStatus send(
      ::android::hardware::bluetooth::hci::PacketType type,
      const std::vector<uint8_t>& packet);
  std::unique_ptr<NetBluetoothMgmt> management_{};

  // Send a reset command and discard all packets until a reset is received.
  void reset();

  // Don't close twice or open before close is complete
  std::mutex mStateMutex;
  enum class HalState {
    READY,
    INITIALIZING,
    ONE_CLIENT,
    CLOSING,
  } mState{HalState::READY};
};

}  // namespace aidl::android::hardware::bluetooth::impl
