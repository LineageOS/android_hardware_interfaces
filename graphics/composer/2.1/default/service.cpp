/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "HWComposerService"

#include <binder/ProcessState.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/StrongPointer.h>
#include "Hwc.h"

using android::sp;
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;
using android::hardware::graphics::composer::V2_1::IComposer;
using android::hardware::graphics::composer::V2_1::implementation::HIDL_FETCH_IComposer;

int main()
{
    const char instance[] = "hwcomposer";

    ALOGI("Service is starting.");

    sp<IComposer> service = HIDL_FETCH_IComposer(instance);
    if (service == nullptr) {
        ALOGI("getService returned NULL");
        return -1;
    }

    LOG_FATAL_IF(service->isRemote(), "Service is REMOTE!");

    service->registerAsService(instance);

    // the conventional HAL might start binder services
    android::ProcessState::self()->setThreadPoolMaxThreadCount(4);
    android::ProcessState::self()->startThreadPool();

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
