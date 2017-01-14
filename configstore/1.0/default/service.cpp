/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "android.hardware.configstore@1.0-service"

#include <android/hardware/configstore/1.0/ISurfaceFlingerConfigs.h>
#include <hidl/LegacySupport.h>

using android::hardware::configstore::V1_0::ISurfaceFlingerConfigs;
using android::hardware::configureRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;
using android::hardware::joinRpcThreadpool;

int main() {
    // TODO(b/34857894): tune the max thread count.
    configureRpcThreadpool(10, true);
    registerPassthroughServiceImplementation<ISurfaceFlingerConfigs>();
    // other interface registration comes here
    joinRpcThreadpool();
    return 0;
}
