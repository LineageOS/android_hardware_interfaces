/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "EvsService"

#include <DefaultEvsEnumerator.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Log.h>

using ::aidl::android::hardware::automotive::evs::implementation::DefaultEvsEnumerator;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    std::shared_ptr<DefaultEvsEnumerator> vhal = ndk::SharedRefBase::make<DefaultEvsEnumerator>();

    ALOGI("Registering as service...");
    binder_exception_t err =
            AServiceManager_addService(vhal->asBinder().get(), "android.hardware.automotive.evs");
    if (err != EX_NONE) {
        ALOGE("failed to register android.hardware.automotive.evs service, exception: %d", err);
        return 1;
    }

    if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
        ALOGE("%s", "failed to set thread pool max thread count");
        return 1;
    }
    ABinderProcess_startThreadPool();

    ALOGI("Evs Service Ready");

    ABinderProcess_joinThreadPool();

    ALOGI("Evs Service Exiting");

    return 0;
}
