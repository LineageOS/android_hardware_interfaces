/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "IvnAndroidDeviceImpl"

#include "IvnAndroidDeviceService.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <stdlib.h>

constexpr char SERVICE_NAME[] = "android.hardware.automotive.ivn.IIvnAndroidDevice/default";
constexpr char DEFAULT_CONFIG_DIR[] = "/vendor/etc/automotive/IvnConfig/DefaultConfig.json";

int main(int /* argc */, char* /* argv */[]) {
    LOG(INFO) << "Registering IvnAndroidDeviceService as service...";
    auto service =
            ndk::SharedRefBase::make<android::hardware::automotive::ivn::IvnAndroidDeviceService>(
                    DEFAULT_CONFIG_DIR);
    if (!service->init()) {
        LOG(ERROR) << "Failed to init IvnAndroidDeviceService";
        exit(1);
    }

    binder_exception_t err = AServiceManager_addService(service->asBinder().get(), SERVICE_NAME);
    if (err != EX_NONE) {
        LOG(ERROR) << "Failed to register IvnAndroidDeviceService service, exception: " << err;
        exit(1);
    }

    if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
        LOG(ERROR) << "Failed to set thread pool max thread count";
        exit(1);
    }
    ABinderProcess_startThreadPool();

    LOG(INFO) << "IvnAndroidDeviceService Ready";

    ABinderProcess_joinThreadPool();

    LOG(ERROR) << "IvnAndroidDeviceService init failed! Should not reach here";

    return 0;
}
