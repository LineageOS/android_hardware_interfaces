/*
 * Copyright 2024 The Android Open Source Project
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

#include "bluetooth_hal/transport/uart_h4/transport_uart_h4.h"

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/mock/mock_android_base_wrapper.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace transport {
namespace {

using ::testing::Test;

using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::util::MockAndroidBaseWrapper;

TEST(TransportUartH4Test, GetTransportReturnSameInstance) {
  MockAndroidBaseWrapper mock_android_base_wrapper;
  MockAndroidBaseWrapper::SetMockWrapper(&mock_android_base_wrapper);
  EXPECT_EQ(&TransportUartH4::GetTransport(), &TransportUartH4::GetTransport());
}

}  // namespace
}  // namespace transport
}  // namespace bluetooth_hal
