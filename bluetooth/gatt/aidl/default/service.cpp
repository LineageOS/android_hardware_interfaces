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

#define LOG_TAG "aidl.android.hardware.bluetooth.gatt.service.default"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Log.h>

#include "BluetoothGatt.h"

using ::aidl::android::hardware::bluetooth::gatt::impl::BluetoothGatt;

int main(int /* argc */, char** /* argv */) {
  ALOGI("Starting IBluetoothGatt service");
  if (!ABinderProcess_setThreadPoolMaxThreadCount(0)) {
    ALOGE("Failed to set thread pool max thread count");
    return EXIT_FAILURE;
  }

  std::shared_ptr<BluetoothGatt> service =
      ndk::SharedRefBase::make<BluetoothGatt>();
  std::string instance = std::string() + BluetoothGatt::descriptor + "/default";
  auto result =
      AServiceManager_addService(service->asBinder().get(), instance.c_str());
  if (result != STATUS_OK) {
    ALOGE("Could not register as a service!");
    return EXIT_FAILURE;
  }
  ABinderProcess_joinThreadPool();
  return EXIT_SUCCESS;
}