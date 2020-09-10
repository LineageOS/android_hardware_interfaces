/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "android.hardware.automotive.sv@1.0-service"

#include <android/hardware/automotive/sv/1.0/ISurroundViewStream.h>
#include <android/hardware_buffer.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>
#include <thread>
#include <ui/GraphicBuffer.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <utils/SystemClock.h>

#include "SurroundViewService.h"

// libhidl:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

// implementation:
using android::hardware::automotive::sv::V1_0::implementation::SurroundViewService;

int main() {
    ALOGI("ISurroundViewService default implementation is starting");
    android::sp<ISurroundViewService> service = new SurroundViewService();

    configureRpcThreadpool(1, true /* callerWillJoin */);

    // Register our service -- if somebody is already registered by our name,
    // they will be killed (their thread pool will throw an exception).
    android::status_t status = service->registerAsService();

    LOG_ALWAYS_FATAL_IF(status != android::OK,
                        "Could not register default Surround View Service (%d)",
                        status);

    joinRpcThreadpool();

    // In normal operation, we don't expect the thread pool to exit
    ALOGE("Surround View Service is shutting down");
    return 1;
}
