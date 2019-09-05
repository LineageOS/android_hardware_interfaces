/*
 * Copyright 2017 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#ifdef LAZY_SERVICE
#define LOG_TAG "android.hardware.cas@1.1-service-lazy"
#else
#define LOG_TAG "android.hardware.cas@1.1-service"
#endif

#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>

#include "MediaCasService.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::LazyServiceRegistrar;
using android::hardware::cas::V1_1::IMediaCasService;
using android::hardware::cas::V1_1::implementation::MediaCasService;

#ifdef LAZY_SERVICE
const bool kLazyService = true;
#else
const bool kLazyService = false;
#endif

int main() {
    configureRpcThreadpool(8, true /* callerWillJoin */);

    // Setup hwbinder service
    android::sp<IMediaCasService> service = new MediaCasService();
    android::status_t status;
    if (kLazyService) {
        auto serviceRegistrar = LazyServiceRegistrar::getInstance();
        status = serviceRegistrar.registerService(service);
    } else {
        status = service->registerAsService();
    }
    LOG_ALWAYS_FATAL_IF(status != android::OK, "Error while registering cas service: %d", status);

    joinRpcThreadpool();
    return 0;
}
