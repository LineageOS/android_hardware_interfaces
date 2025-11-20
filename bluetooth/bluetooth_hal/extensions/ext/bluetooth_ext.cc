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

#include "bluetooth_hal/extensions/ext/bluetooth_ext.h"

#include <cstdint>
#include <vector>

#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/ext/bluetooth_ext_handler.h"

namespace bluetooth_hal {
namespace extensions {
namespace ext {

using ::ndk::ScopedAStatus;

ScopedAStatus BluetoothExt::setBluetoothCmdPacket(
    char16_t opcode, const std::vector<uint8_t>& params, bool* ret) {
  bool status = handler_.SetBluetoothCmdPacket(opcode, params, ret);
  return status ? ScopedAStatus::ok()
                : ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
}

}  // namespace ext
}  // namespace extensions
}  // namespace bluetooth_hal
