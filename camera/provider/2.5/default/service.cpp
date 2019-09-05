/*
 * Copyright 2019 The Android Open Source Project
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
#define LOG_TAG "android.hardware.camera.provider@2.5-service-lazy"
#else
#define LOG_TAG "android.hardware.camera.provider@2.5-service"
#endif

#include <android/hardware/camera/provider/2.5/ICameraProvider.h>
#include <binder/ProcessState.h>
#include <hidl/HidlLazyUtils.h>
#include <hidl/HidlTransportSupport.h>

#include "CameraProvider_2_5.h"
#include "LegacyCameraProviderImpl_2_5.h"

using android::status_t;
using android::hardware::camera::provider::V2_5::ICameraProvider;

#ifdef LAZY_SERVICE
const bool kLazyService = true;
#else
const bool kLazyService = false;
#endif

int main()
{
    using namespace android::hardware::camera::provider::V2_5::implementation;

    ALOGI("CameraProvider@2.5 legacy service is starting.");

    ::android::hardware::configureRpcThreadpool(/*threads*/ HWBINDER_THREAD_COUNT, /*willJoin*/ true);

    ::android::sp<ICameraProvider> provider = new CameraProvider<LegacyCameraProviderImpl_2_5>();

    status_t status;
    if (kLazyService) {
        auto serviceRegistrar = ::android::hardware::LazyServiceRegistrar::getInstance();
        status = serviceRegistrar.registerService(provider, "legacy/0");
    } else {
        status = provider->registerAsService("legacy/0");
    }
    LOG_ALWAYS_FATAL_IF(status != android::OK, "Error while registering provider service: %d",
            status);

    ::android::hardware::joinRpcThreadpool();

    return 0;
}
