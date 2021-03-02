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

#define LOG_TAG "automotive.vehicle@2.0-service"
#include <android/log.h>
#include <hidl/HidlTransportSupport.h>

#include <iostream>

#include <android/binder_process.h>
#include <utils/Looper.h>
#include <vhal_v2_0/EmulatedVehicleConnector.h>
#include <vhal_v2_0/EmulatedVehicleHal.h>
#include <vhal_v2_0/VehicleHalManager.h>
#include <vhal_v2_0/WatchdogClient.h>

using namespace android;
using namespace android::hardware;
using namespace android::hardware::automotive::vehicle::V2_0;

int main(int /* argc */, char* /* argv */ []) {
    auto store = std::make_unique<VehiclePropertyStore>();
    auto connector = std::make_unique<impl::EmulatedVehicleConnector>();
    auto hal = std::make_unique<impl::EmulatedVehicleHal>(store.get(), connector.get());
    auto emulator = std::make_unique<impl::VehicleEmulator>(hal.get());
    auto service = std::make_unique<VehicleHalManager>(hal.get());
    connector->setValuePool(hal->getValuePool());

    configureRpcThreadpool(4, false /* callerWillJoin */);

    ALOGI("Registering as service...");
    status_t status = service->registerAsService();

    if (status != OK) {
        ALOGE("Unable to register vehicle service (%d)", status);
        return 1;
    }

    // Setup a binder thread pool to be a car watchdog client.
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    sp<Looper> looper(Looper::prepare(0 /* opts */));
    std::shared_ptr<WatchdogClient> watchdogClient =
            ndk::SharedRefBase::make<WatchdogClient>(looper, service.get());
    // The current health check is done in the main thread, so it falls short of capturing the real
    // situation. Checking through HAL binder thread should be considered.
    if (!watchdogClient->initialize()) {
        ALOGE("Failed to initialize car watchdog client");
        return 1;
    }
    ALOGI("Ready");
    while (true) {
        looper->pollAll(-1 /* timeoutMillis */);
    }

    return 1;
}
