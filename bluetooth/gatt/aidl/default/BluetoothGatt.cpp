/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include "BluetoothGatt.h"

namespace aidl::android::hardware::bluetooth::gatt::impl {

BluetoothGatt::BluetoothGatt() {}
BluetoothGatt::~BluetoothGatt() {}

::ndk::ScopedAStatus BluetoothGatt::init(
    const std::shared_ptr<IBluetoothGattCallback>& in_callback) {
  if (in_callback == nullptr) {
    return ndk::ScopedAStatus::fromServiceSpecificError(STATUS_BAD_VALUE);
  }
  callback_ = in_callback;
  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BluetoothGatt::getGattCapabilities(
    GattCapabilities* _aidl_return) {
  _aidl_return->supportedGattClientProperties = 0;
  _aidl_return->supportedGattServerProperties = 0;
  return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus BluetoothGatt::registerService(
    int32_t /* in_sessionId */, int32_t /* in_aclConnectionHandle */,
    int32_t /* in_attMtu */, IBluetoothGatt::Role /* in_role */,
    const Uuid& /* in_serviceUuid */,
    const std::vector<GattCharacteristic>& /* in_characteristics */,
    const ::aidl::android::hardware::contexthub::
        EndpointId& /* in_endpointId */) {
  return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

::ndk::ScopedAStatus BluetoothGatt::unregisterService(
    int32_t /* in_sessionId */) {
  return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

::ndk::ScopedAStatus BluetoothGatt::clearServices(
    int32_t /* in_aclConnectionHandle */) {
  return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace aidl::android::hardware::bluetooth::gatt::impl