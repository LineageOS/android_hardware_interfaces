/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "android.hardware.radio@1.2-sap-service"

#include <android/hardware/radio/1.2/ISap.h>
#include <hidl/HidlTransportSupport.h>

#include "Sap.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::radio::V1_2::ISap;
using android::hardware::radio::V1_2::implementation::Sap;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    configureRpcThreadpool(1, true);

    sp<ISap> sap = new Sap;
    status_t status = sap->registerAsService();
    ALOGW_IF(status != OK, "Could not register ISap v1.2");
    ALOGD("Default service is ready.");

    joinRpcThreadpool();
    return 1;
}