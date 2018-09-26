/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "android.hardware.atrace@1.0-service"

#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>

#include "AtraceDevice.h"

using ::android::OK;
using ::android::sp;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::hardware::atrace::V1_0::IAtraceDevice;
using ::android::hardware::atrace::V1_0::Status;
using ::android::hardware::atrace::V1_0::TracingCategory;
using ::android::hardware::atrace::V1_0::implementation::AtraceDevice;

int main(int /* argc */, char* /* argv */ []) {
    sp<IAtraceDevice> atrace = new AtraceDevice;
    configureRpcThreadpool(1, true /* will join */);
    if (atrace->registerAsService() != OK) {
        ALOGE("Could not register service.");
        return 1;
    }
    joinRpcThreadpool();

    ALOGE("Service exited!");
    return 1;
}
