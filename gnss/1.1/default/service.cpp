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
#define LOG_TAG "android.hardware.gnss@1.1-service"

#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include "Gnss.h"

using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::gnss::V1_1::implementation::Gnss;
using ::android::hardware::gnss::V1_1::IGnss;
using ::android::hardware::joinRpcThreadpool;
using ::android::OK;
using ::android::sp;

int main(int /* argc */, char* /* argv */ []) {
    sp<IGnss> gnss = new Gnss();
    configureRpcThreadpool(1, true /* will join */);
    if (gnss->registerAsService() != OK) {
        ALOGE("Could not register gnss 1.1 service.");
        return 1;
    }
    joinRpcThreadpool();

    ALOGE("Service exited!");
    return 1;
}
