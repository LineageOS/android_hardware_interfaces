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

#define LOG_TAG "android.hardware.biometrics.face@1.0-service"

#include <android/hardware/biometrics/face/1.0/types.h>
#include <android/hardware/biometrics/face/1.0/IBiometricsFace.h>
#include <android/log.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include "BiometricsFace.h"

using android::sp;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::biometrics::face::implementation::BiometricsFace;
using android::hardware::biometrics::face::V1_0::IBiometricsFace;

int main() {
    ALOGI("BiometricsFace HAL is being started.");

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    android::sp<IBiometricsFace> face = new BiometricsFace();
    const android::status_t status = face->registerAsService();

    if (status != android::OK) {
        ALOGE("Error starting the BiometricsFace HAL.");
        return 1;
    }

    ALOGI("BiometricsFace HAL has started successfully.");
    joinRpcThreadpool();

    ALOGI("BiometricsFace HAL is terminating.");
    return 1;  // should never get here
}
