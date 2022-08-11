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

#define LOG_TAG "RemoteAccessImpl"

#include "RemoteAccessService.h"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Log.h>

int main(int /* argc */, char* /* argv */[]) {
    ALOGI("Registering RemoteAccessService as service...");

    // TODO(b/241483300): Create GrpcClientStub here.
    auto service = ndk::SharedRefBase::make<
            android::hardware::automotive::remoteaccess::RemoteAccessService>(nullptr);

    binder_exception_t err = AServiceManager_addService(
            service->asBinder().get(), "android.hardware.automotive.remote.IRemoteAccess/default");
    if (err != EX_NONE) {
        ALOGE("failed to register android.hardware.automotive.remote.IRemoteAccess service, "
              "exception: %d",
              err);
        return 1;
    }

    if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
        ALOGE("%s", "failed to set thread pool max thread count");
        return 1;
    }
    ABinderProcess_startThreadPool();

    ALOGI("RemoteAccess service Ready");

    ABinderProcess_joinThreadPool();

    ALOGW("Should not reach here");

    return 0;
}
