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

#ifdef LAZY_SERVICE
#define LOG_TAG "android.hardware.camera.provider@2.4-service-lazy"
#else
#define LOG_TAG "android.hardware.camera.provider@2.4-service"
#endif

#include <android/hardware/camera/provider/2.4/ICameraProvider.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>
#include <hidl/LegacySupport.h>
#include <malloc.h>

using android::status_t;
using android::hardware::defaultLazyPassthroughServiceImplementation;
using android::hardware::defaultPassthroughServiceImplementation;
using android::hardware::camera::provider::V2_4::ICameraProvider;

#ifdef LAZY_SERVICE
const bool kLazyService = true;
#else
const bool kLazyService = false;
#endif

int main()
{
    ALOGI("CameraProvider@2.4 legacy service is starting.");
    // The camera HAL may communicate to other vendor components via
    // /dev/vndbinder
    android::ProcessState::initWithDriver("/dev/vndbinder");

    // b/166675194
    if (property_get_bool("ro.vendor.camera.provider24.disable_mem_init", false)) {
        if (mallopt(M_BIONIC_ZERO_INIT, 0) == 0) {
            // Note - heap initialization is only present on devices with Scudo.
            // Devices with jemalloc don't have heap-init, and thus the mallopt
            // will fail. On these devices, you probably just want to remove the
            // property.
            ALOGE("Disabling heap initialization failed.");
        }
    }

    status_t status;
    if (kLazyService) {
        status = defaultLazyPassthroughServiceImplementation<ICameraProvider>("legacy/0",
                                                                              /*maxThreads*/ 6);
    } else {
        status = defaultPassthroughServiceImplementation<ICameraProvider>("legacy/0",
                                                                          /*maxThreads*/ 6);
    }
    return status;
}
