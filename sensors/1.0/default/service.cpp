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

#include <android-base/logging.h>
#include <android/hardware/sensors/1.0/ISensors.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>

int main() {
    using android::hardware::sensors::V1_0::ISensors;
    using android::sp;
    using android::OK;
    using namespace android::hardware;

    LOG(INFO) << "Service is starting.";
    sp<ISensors> sensors = ISensors::getService("sensors", true /* getStub */);

    if (sensors.get() == nullptr) {
        LOG(ERROR) << "ISensors::getService returned nullptr, exiting.";
        return 1;
    }

    LOG(INFO) << "Default implementation using sensors is "
              << (sensors->isRemote() ? "REMOTE" : "LOCAL");

    CHECK(!sensors->isRemote());

    LOG(INFO) << "Registering instance sensors.";
    sensors->registerAsService("sensors");
    LOG(INFO) << "Ready.";

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}

