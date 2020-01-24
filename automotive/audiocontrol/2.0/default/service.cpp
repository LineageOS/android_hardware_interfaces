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
#include <unistd.h>

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

#include "AudioControl.h"

// libhidl:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

// Generated HIDL files
using android::hardware::automotive::audiocontrol::V2_0::IAudioControl;

// The namespace in which all our implementation code lives
using namespace android::hardware::automotive::audiocontrol::V2_0::implementation;
using namespace android;

// Main service entry point
int main() {
    // Create an instance of our service class
    android::sp<IAudioControl> service = new AudioControl();
    configureRpcThreadpool(1, true /*callerWillJoin*/);

    if (service->registerAsService() != OK) {
        LOG(ERROR) << "registerAsService failed";
        return 1;
    }

    // Join (forever) the thread pool we created for the service above
    joinRpcThreadpool();

    // We don't ever actually expect to return, so return an error if we do get here
    return 2;
}