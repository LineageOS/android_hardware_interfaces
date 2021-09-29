/*
 * Copyright 2021 The Android Open Source Project
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
#define LOG_TAG "android.hardware.camera.provider@2.7-external-service-lazy"
#else
#define LOG_TAG "android.hardware.camera.provider@2.7-external-service"
#endif

#include <android/hardware/camera/provider/2.7/ICameraProvider.h>
#include <binder/ProcessState.h>
#include <hidl/HidlLazyUtils.h>
#include <hidl/HidlTransportSupport.h>

#include "CameraProvider_2_7.h"
#include "ExternalCameraProviderImpl_2_7.h"

using android::status_t;
using android::hardware::camera::provider::V2_7::ICameraProvider;
using android::hidl::base::V1_0::IBase;

#ifdef LAZY_SERVICE
const bool kLazyService = true;
#else
const bool kLazyService = false;
#endif

int main() {
    using namespace android::hardware::camera::provider::V2_7::implementation;

    ALOGI("CameraProvider@2.7 external webcam service is starting.");

    ::android::hardware::configureRpcThreadpool(/*threads*/ HWBINDER_THREAD_COUNT,
                                                /*willJoin*/ true);

    ::android::sp<CameraProvider<ExternalCameraProviderImpl_2_7>> provider =
            new CameraProvider<ExternalCameraProviderImpl_2_7>();

    status_t status;
    if (kLazyService) {
        auto serviceRegistrar = ::android::hardware::LazyServiceRegistrar::getInstance();
        status = serviceRegistrar.registerService(provider, "external/0");
    } else {
        status = provider->registerAsService("external/0");
    }

    LOG_ALWAYS_FATAL_IF(status != android::OK, "Error while registering provider service: %d",
                        status);

    ::android::hardware::joinRpcThreadpool();

    return 0;
}