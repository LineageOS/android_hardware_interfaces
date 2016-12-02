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

#define LOG_TAG "android.hardware.vehicle@2.0-service"
#include <android/log.h>

#include <iostream>

#include <hwbinder/IPCThreadState.h>

#include <vehicle_hal_manager/VehicleHalManager.h>
#include <impl/DefaultVehicleHal.h>

using namespace android;
using namespace android::hardware;
using namespace android::hardware::vehicle::V2_0;

int main(int /* argc */, char* /* argv */ []) {
    auto hal = std::make_unique<impl::DefaultVehicleHal>();
    auto service = std::make_unique<VehicleHalManager>(hal.get());

    ALOGI("Registering as service...");
    service->registerAsService("Vehicle");

    ALOGI("Ready");
    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
