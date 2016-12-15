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

#define LOG_TAG "android.hardware.evs@1.0-service"

#include <unistd.h>

#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <utils/Log.h>

#include "ServiceNames.h"
#include "EvsEnumerator.h"
#include "EvsDisplay.h"


// libhwbinder:
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

// Generated HIDL files
using android::hardware::evs::V1_0::IEvsEnumerator;
using android::hardware::evs::V1_0::IEvsDisplay;

// The namespace in which all our implementation code lives
using namespace android::hardware::evs::V1_0::implementation;
using namespace android;


int main() {
    ALOGI("EVS Hardware Enumerator service is starting");
    android::sp<IEvsEnumerator> service = new EvsEnumerator();

    // Register our service -- if somebody is already registered by our name,
    // they will be killed (their thread pool will throw an exception).
    status_t status = service->registerAsService(kEnumeratorServiceName);
    if (status == OK) {
        ALOGD("%s is ready.", kEnumeratorServiceName);

        // Set thread pool size to ensure the API is not called in parallel.
        // By setting the size to zero, the main thread will be the only one
        // serving requests once we "joinThreadPool".
        ProcessState::self()->setThreadPoolMaxThreadCount(0);

        // Note:  We don't start the thread pool because it'll add at least one (default)
        //        thread to it, which we don't want.  See b/31226656
        // ProcessState::self()->startThreadPool();

        // Send this main thread to become a permanent part of the thread pool.
        // This bumps up the thread count by 1 (from zero in this case).
        // This is not expected to return.
        IPCThreadState::self()->joinThreadPool();
    } else {
        ALOGE("Could not register service %s (%d).", kEnumeratorServiceName, status);
    }

    // In normal operation, we don't expect the thread pool to exit
    ALOGE("EVS Hardware Enumerator is shutting down");
    return 1;
}
