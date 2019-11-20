//
// Copyright 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#define LOG_TAG "android.hardware.bluetooth@1.1-service"

#include <android/hardware/bluetooth/1.1/IBluetoothHci.h>
#include <hidl/HidlTransportSupport.h>

#include "bluetooth_hci.h"

// Generated HIDL files
using android::hardware::bluetooth::V1_1::IBluetoothHci;
using android::hardware::bluetooth::V1_1::implementation::BluetoothHci;

using android::sp;
using android::status_t;

int main() {
  ::android::hardware::configureRpcThreadpool(1 /*threads*/, true /*willJoin*/);

  sp bluetoothHci = new BluetoothHci();
  const status_t status = bluetoothHci->registerAsService();
  if (status != ::android::OK) {
    ALOGE("Cannot register Bluetooth HAL service");
    return 1;  // or handle error
  }

  ::android::hardware::joinRpcThreadpool();
  return 1;  // joinRpcThreadpool should never return
}
