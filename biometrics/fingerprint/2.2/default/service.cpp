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

#define LOG_TAG "android.hardware.biometrics.fingerprint@2.2-service"

#include <android/log.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <android/hardware/biometrics/fingerprint/2.2/IBiometricsFingerprint.h>
#include <android/hardware/biometrics/fingerprint/2.2/types.h>
#include "BiometricsFingerprint.h"

using android::hardware::biometrics::fingerprint::V2_2::IBiometricsFingerprint;
using android::hardware::biometrics::fingerprint::V2_2::implementation::BiometricsFingerprint;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;

int main() {
    android::sp<IBiometricsFingerprint> bio = new BiometricsFingerprint();

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    if (::android::OK != bio->registerAsService()) {
        return 1;
    }

    joinRpcThreadpool();

    return 0; // should never get here
}
