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

#define LOG_TAG "android.hardware.contexthub@1.1-service"

#include <android/hardware/contexthub/1.1/IContexthub.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>
#include <utils/StrongPointer.h>
#include "Contexthub.h"

using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::hardware::contexthub::V1_1::IContexthub;
using ::android::hardware::contexthub::V1_1::implementation::Contexthub;

int main() {
    configureRpcThreadpool(1, true /* callerWillJoin */);

    ::android::sp<IContexthub> contexthub = new Contexthub();
    if (contexthub->registerAsService() != ::android::OK) {
        ALOGE("Failed to register Contexthub HAL instance");
        return 1;
    }

    joinRpcThreadpool();
    ALOGE("Service exited");
    return 1;
}
