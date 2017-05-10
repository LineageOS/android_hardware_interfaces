/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "android.hardware.power@1.1-service"

#include <android/log.h>
#include <hidl/HidlTransportSupport.h>
#include <android/hardware/power/1.1/IPower.h>
#include <hardware/power.h>
#include "Power.h"

using android::sp;
using android::status_t;
using android::OK;

// libhwbinder:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

// Generated HIDL files
using android::hardware::power::V1_1::IPower;
using android::hardware::power::V1_1::implementation::Power;

int main() {

    status_t status;
    android::sp<IPower> service = nullptr;
    const hw_module_t* hw_module = nullptr;
    power_module_t* power_module = nullptr;
    int err;

    ALOGI("Power HAL Service 1.1 (Default) is starting.");

    err = hw_get_module(POWER_HARDWARE_MODULE_ID, &hw_module);
    if (err) {
        ALOGE("hw_get_module %s failed: %d", POWER_HARDWARE_MODULE_ID, err);
        goto shutdown;
    }

    if (!hw_module->methods || !hw_module->methods->open) {
        power_module = reinterpret_cast<power_module_t*>(
            const_cast<hw_module_t*>(hw_module));
    } else {
        err = hw_module->methods->open(hw_module, POWER_HARDWARE_MODULE_ID,
                                           reinterpret_cast<hw_device_t**>(&power_module));
        if (err) {
            ALOGE("Passthrough failed to load legacy HAL.");
            goto shutdown;
        }
    }

    service = new Power(power_module);
    if (service == nullptr) {
        ALOGE("Can not create an instance of Power HAL Iface, exiting.");

        goto shutdown;
    }

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    status = service->registerAsService();
    if (status != OK) {
        ALOGE("Could not register service for Power HAL Iface (%d).", status);
        goto shutdown;
    }

    ALOGI("Power Service is ready");
    joinRpcThreadpool();
    //Should not pass this line

shutdown:
    // In normal operation, we don't expect the thread pool to exit

    ALOGE("Power Service is shutting down");
    return 1;
}
