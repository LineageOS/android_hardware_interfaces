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

#define LOG_TAG "soundtriggerhal"

#include <hwbinder/IInterface.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <hidl/IServiceManager.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>

using android::hardware::IPCThreadState;
using android::hardware::ProcessState;
using android::hardware::soundtrigger::V2_0::ISoundTriggerHw;

int main(int /* argc */, char* /* argv */ []) {
    android::sp<ISoundTriggerHw> service =
            ISoundTriggerHw::getService("sound_trigger.primary", true /* getStub */);

    service->registerAsService("sound_trigger.primary");

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
