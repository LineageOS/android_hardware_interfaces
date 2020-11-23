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

#define LOG_TAG "android.hardware.audio@7.0-service.example"
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

#include "DevicesFactory.h"
#include "EffectsFactory.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using namespace android;

status_t registerDevicesFactoryService() {
    sp<::android::hardware::audio::V7_0::IDevicesFactory> devicesFactory =
            new ::android::hardware::audio::V7_0::implementation::DevicesFactory();
    status_t status = devicesFactory->registerAsService("example");
    ALOGE_IF(status != OK, "Error registering devices factory as service: %d", status);
    return status;
}

status_t registerEffectsFactoryService() {
    sp<::android::hardware::audio::effect::V7_0::IEffectsFactory> devicesFactory =
            new ::android::hardware::audio::effect::V7_0::implementation::EffectsFactory();
    status_t status = devicesFactory->registerAsService("example");
    ALOGE_IF(status != OK, "Error registering effects factory as service: %d", status);
    return status;
}

int main() {
    configureRpcThreadpool(1, true);
    status_t status = registerDevicesFactoryService();
    if (status != OK) {
        return status;
    }
    status = registerEffectsFactoryService();
    if (status != OK) {
        return status;
    }
    joinRpcThreadpool();

    return 1;
}
