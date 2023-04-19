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

#define LOG_TAG "aidl.android.hardware.bluetooth.service.default"

#include <aidl/android/hardware/bluetooth/IBluetoothHci.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>

#include "BluetoothHci.h"

using ::aidl::android::hardware::bluetooth::impl::BluetoothHci;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;

int main(int /* argc */, char** /* argv */) {
  ALOGI("Bluetooth HAL starting");
  if (!ABinderProcess_setThreadPoolMaxThreadCount(0)) {
    ALOGI("failed to set thread pool max thread count");
    return 1;
  }

  std::shared_ptr<BluetoothHci> service =
      ndk::SharedRefBase::make<BluetoothHci>();
  std::string instance = std::string() + BluetoothHci::descriptor + "/default";
  auto result =
      AServiceManager_addService(service->asBinder().get(), instance.c_str());
  if (result == STATUS_OK) {
    ABinderProcess_joinThreadPool();
  } else {
    ALOGE("Could not register as a service!");
  }
  return 0;
}
