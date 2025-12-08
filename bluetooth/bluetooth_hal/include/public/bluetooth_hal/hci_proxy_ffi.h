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

#include <memory>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "hal/ffi.h"

namespace bluetooth_hal {

/**
 * @brief A Foreign Function Interface(FFI) proxy to handle binder tasks
 * from libbluetooth_offload_hal.
 *
 * This class acts as an FFI proxy to manage binder tasks originating from
 * libbluetooth_offload_hal, a Rust-based static library integrated within
 * the Android Bluetooth stack.
 */
class HciProxyFfi
    : public ::aidl::android::hardware::bluetooth::hal::IBluetoothHci {
 public:
  HciProxyFfi();

  void initialize(
      const std::shared_ptr<
          ::aidl::android::hardware::bluetooth::hal::IBluetoothHciCallbacks>&
          cb) override;
  void sendHciCommand(const std::vector<uint8_t>& packet) override;
  void sendAclData(const std::vector<uint8_t>& packet) override;
  void sendScoData(const std::vector<uint8_t>& packet) override;
  void sendIsoData(const std::vector<uint8_t>& packet) override;
  void close() override;
  void clientDied() override;
  void dump(int fd);

 private:
  static void SigtermHandler(int signum);
};

}  // namespace bluetooth_hal
