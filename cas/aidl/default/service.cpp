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

#ifdef LAZY_SERVICE
const bool kLazyService = true;
#define LOG_TAG "android.hardware.cas-service.example-lazy"
#else
const bool kLazyService = false;
#define LOG_TAG "android.hardware.cas-service.example"
#endif

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "MediaCasService.h"

using aidl::android::hardware::cas::MediaCasService;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(8);
    ABinderProcess_startThreadPool();

    // Setup hwbinder service
    std::shared_ptr<MediaCasService> service = ::ndk::SharedRefBase::make<MediaCasService>();

    const std::string instance = std::string() + MediaCasService::descriptor + "/default";
    binder_status_t status;

    if (kLazyService) {
        status = AServiceManager_registerLazyService(service->asBinder().get(), instance.c_str());
    } else {
        status = AServiceManager_addService(service->asBinder().get(), instance.c_str());
    }
    LOG_ALWAYS_FATAL_IF(status != STATUS_OK, "Error while registering cas service: %d", status);

    ABinderProcess_joinThreadPool();
    return 0;
}
