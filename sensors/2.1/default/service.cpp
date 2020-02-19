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

#define LOG_TAG "android.hardware.sensors@2.1-service"

#include <android/hardware/sensors/2.1/ISensors.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>
#include <utils/StrongPointer.h>
#include "SensorsV2_1.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::sensors::V2_1::ISensors;
using android::hardware::sensors::V2_1::implementation::SensorsV2_1;

int main(int /* argc */, char** /* argv */) {
    configureRpcThreadpool(1, true);

    android::sp<ISensors> sensors = new SensorsV2_1();
    if (sensors->registerAsService() != ::android::OK) {
        ALOGE("Failed to register Sensors HAL instance");
        return -1;
    }

    joinRpcThreadpool();
    return 1;  // joinRpcThreadpool shouldn't exit
}
