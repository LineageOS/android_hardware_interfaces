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

#define LOG_TAG "GrallocService"

#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/StrongPointer.h>

using android::sp;
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;
using android::hardware::graphics::allocator::V2_0::IAllocator;

int main()
{
    const char instance[] = "gralloc";

    ALOGI("Service is starting.");

    sp<IAllocator> service = IAllocator::getService(instance,
            true /* getStub */);
    if (service == nullptr) {
        ALOGI("getService returned NULL");
        return -1;
    }

    LOG_FATAL_IF(service->isRemote(), "Service is REMOTE!");

    service->registerAsService(instance);

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
