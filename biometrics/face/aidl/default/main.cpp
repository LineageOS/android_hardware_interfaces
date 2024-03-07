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

#include "Face.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::biometrics::face::Face;

int main() {
    LOG(INFO) << "Face HAL started";
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Face> hal = ndk::SharedRefBase::make<Face>();

    const std::string instance = std::string(Face::descriptor) + "/virtual";
    binder_status_t status =
            AServiceManager_registerLazyService(hal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);
    AServiceManager_forceLazyServicesPersist(true);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
