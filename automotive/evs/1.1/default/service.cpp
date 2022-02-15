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

#include "EvsDisplay.h"
#include "EvsEnumerator.h"
#include "ServiceNames.h"

#include <hidl/HidlTransportSupport.h>
#include <log/log.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

#include <unistd.h>

using android::frameworks::automotive::display::V1_0::IAutomotiveDisplayProxyService;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::automotive::evs::V1_0::DisplayState;
using android::hardware::automotive::evs::V1_1::IEvsEnumerator;
using android::hardware::automotive::evs::V1_1::implementation::EvsEnumerator;

int main() {
    ALOGI("EVS Hardware Enumerator service is starting");

    android::sp<IAutomotiveDisplayProxyService> carWindowService =
            IAutomotiveDisplayProxyService::getService("default");
    if (carWindowService == nullptr) {
        ALOGE("Cannot use AutomotiveDisplayProxyService.  Exiting.");
        return EXIT_FAILURE;
    }

    android::sp<IEvsEnumerator> service = new EvsEnumerator(carWindowService);

    configureRpcThreadpool(1, true /* callerWillJoin */);

    // Register our service -- if somebody is already registered by our name,
    // they will be killed (their thread pool will throw an exception).
    auto status = service->registerAsService(kEnumeratorServiceName);
    if (status == android::OK) {
        ALOGD("%s is ready.", kEnumeratorServiceName);
        joinRpcThreadpool();
    } else {
        ALOGE("Could not register service %s (%d).", kEnumeratorServiceName, status);
    }

    // In normal operation, we don't expect the thread pool to exit
    ALOGE("EVS Hardware Enumerator is shutting down");
    return EXIT_SUCCESS;
}
