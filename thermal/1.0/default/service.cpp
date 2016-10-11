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

#include <iostream>
#include <unistd.h>

#include <android/hardware/thermal/1.0/IThermal.h>

#include <hidl/IServiceManager.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>

#define LOG_TAG "android.hardware.thermal@1.0-service"
#include <utils/Log.h>
#include <utils/StrongPointer.h>

using android::sp;

// libhwbinder:
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

// Generated HIDL files
using android::hardware::thermal::V1_0::IThermal;

int main() {
    const char instance[] = "thermal";
    sp<IThermal> service = IThermal::getService(instance, true /* getStub */);
    if (service.get() == nullptr) {
        ALOGE("IThermal::getService returned NULL, exiting");
        return EXIT_FAILURE;
    }
    LOG_FATAL_IF(service->isRemote(), "Implementation is REMOTE!");
    service->registerAsService(instance);

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
