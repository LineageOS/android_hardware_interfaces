/*
 * Copyright 2017 The Android Open Source Project
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

#define LOG_TAG "android.hardware.camera.provider@2.4-service"

#include <android/hardware/camera/provider/2.4/ICameraProvider.h>
#include <CameraProvider.h>

#include <hidl/HidlTransportSupport.h>
#include <utils/StrongPointer.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::hardware::camera::provider::V2_4::ICameraProvider;
using android::hardware::camera::provider::V2_4::implementation::HIDL_FETCH_ICameraProvider;

int main()
{
    const char instance[] = "legacy/0";

    ALOGI("Camera provider Service is starting.");

    configureRpcThreadpool(1, true /* callerWillJoin */);
    // TODO (b/34510650): check the passthrough/binderized dev key
    sp<ICameraProvider> service = HIDL_FETCH_ICameraProvider(instance);
    if (service == nullptr) {
        ALOGI("Camera provider getService returned NULL");
        return -1;
    }

    LOG_FATAL_IF(service->isRemote(), "Camera provider service is REMOTE!");

    service->registerAsService(instance);
    joinRpcThreadpool();

    return 0;
}