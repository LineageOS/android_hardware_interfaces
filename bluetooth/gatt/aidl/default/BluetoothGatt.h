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

#include <aidl/android/hardware/bluetooth/gatt/BnBluetoothGatt.h>

namespace aidl::android::hardware::bluetooth::gatt::impl {

class BluetoothGatt : public BnBluetoothGatt {
 public:
  BluetoothGatt();
  ~BluetoothGatt();

  ::ndk::ScopedAStatus init(
      const std::shared_ptr<
          ::aidl::android::hardware::bluetooth::gatt::IBluetoothGattCallback>&
          in_callback) override;
  ::ndk::ScopedAStatus getGattCapabilities(
      ::aidl::android::hardware::bluetooth::gatt::GattCapabilities*
          _aidl_return) override;
  ::ndk::ScopedAStatus registerService(
      int32_t in_sessionId, int32_t in_aclConnectionHandle, int32_t in_attMtu,
      ::aidl::android::hardware::bluetooth::gatt::IBluetoothGatt::Role in_role,
      const ::aidl::android::hardware::bluetooth::gatt::Uuid& in_serviceUuid,
      const std::vector<
          ::aidl::android::hardware::bluetooth::gatt::GattCharacteristic>&
          in_characteristics,
      const ::aidl::android::hardware::contexthub::EndpointId& in_endpointId)
      override;
  ::ndk::ScopedAStatus unregisterService(int32_t in_sessionId) override;
  ::ndk::ScopedAStatus clearServices(int32_t in_aclConnectionHandle) override;

 private:
  std::shared_ptr<IBluetoothGattCallback> callback_;
};

}  // namespace aidl::android::hardware::bluetooth::gatt::impl