/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "android.hardware.input.classifier@1.0"

#include <inttypes.h>

#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

#include "InputClassifier.h"
#include "android/hardware/input/classifier/1.0/IInputClassifier.h"

using android::sp;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::input::classifier::V1_0::IInputClassifier;
using android::hardware::input::classifier::V1_0::implementation::InputClassifier;

int main() {
    sp<IInputClassifier> classifier = new InputClassifier();

    configureRpcThreadpool(1, true);
    const status_t status = classifier->registerAsService();

    if (status != android::OK) {
        ALOGE("Could not register InputClassifier HAL!");
        return EXIT_FAILURE;  // or handle error
    }

    joinRpcThreadpool();
    LOG_ALWAYS_FATAL("Under normal operation, joinRpcThreadpool should never return");
    return EXIT_FAILURE;
}
