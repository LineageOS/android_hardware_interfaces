/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BluetoothSocket.h"

using aidl::android::hardware::bluetooth::socket::Status;

namespace aidl::android::hardware::bluetooth::socket::impl {

BluetoothSocket::BluetoothSocket() {}
BluetoothSocket::~BluetoothSocket() {}

::ndk::ScopedAStatus BluetoothSocket::registerCallback(
    const std::shared_ptr<
        ::aidl::android::hardware::bluetooth::socket::IBluetoothSocketCallback>&
        in_callback) {
  if (in_callback == nullptr) {
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }
  callback_ = in_callback;
  return ::ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus BluetoothSocket::getSocketCapabilities(
    ::aidl::android::hardware::bluetooth::socket::SocketCapabilities*
        _aidl_return) {
  _aidl_return->leCocCapabilities.numberOfSupportedSockets = 0;
  _aidl_return->leCocCapabilities.mtu = 0;
  _aidl_return->rfcommCapabilities.numberOfSupportedSockets = 0;
  _aidl_return->rfcommCapabilities.maxFrameSize = 0;
  return ::ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus BluetoothSocket::opened(
    const ::aidl::android::hardware::bluetooth::socket::SocketContext&
    /* in_context */) {
  return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
::ndk::ScopedAStatus BluetoothSocket::closed(int64_t /*in_socketId*/) {
  return ::ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::bluetooth::socket::impl
