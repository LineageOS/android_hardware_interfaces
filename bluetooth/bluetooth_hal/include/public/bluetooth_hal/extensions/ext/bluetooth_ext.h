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

#include <cstdint>
#include <vector>

#include "aidl/hardware/google/bluetooth/ext/BnBluetoothExt.h"
#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/ext/bluetooth_ext_handler.h"

namespace bluetooth_hal {
namespace extensions {
namespace ext {

struct BluetoothExt
    : public ::aidl::hardware::google::bluetooth::ext::BnBluetoothExt {
 public:
  ::ndk::ScopedAStatus setBluetoothCmdPacket(char16_t opcode,
                                             const std::vector<uint8_t>& params,
                                             bool* ret) override;

 private:
  BluetoothExtHandler handler_;
};

}  // namespace ext
}  // namespace extensions
}  // namespace bluetooth_hal
