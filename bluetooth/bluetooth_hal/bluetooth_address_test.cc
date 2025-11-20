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

#include "bluetooth_hal/bluetooth_address.h"

#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace hci {
namespace {

TEST(BluetoothAddressTest, HandleToString) {
  BluetoothAddress address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  ASSERT_EQ(address.ToFullString(), "11:22:33:44:55:66");
  ASSERT_EQ(address.ToString(), "XX:XX:XX:XX:55:66");
}

}  // namespace
}  // namespace hci
}  // namespace bluetooth_hal
