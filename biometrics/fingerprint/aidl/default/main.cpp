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

#include "Fingerprint.h"
#include "VirtualHal.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::biometrics::fingerprint::Fingerprint;
using aidl::android::hardware::biometrics::fingerprint::VirtualHal;

int main() {
    LOG(INFO) << "Fingerprint HAL started";
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Fingerprint> hal = ndk::SharedRefBase::make<Fingerprint>();
    auto binder = hal->asBinder();

    std::shared_ptr<VirtualHal> hal_ext = ndk::SharedRefBase::make<VirtualHal>(hal.get());
    auto binder_ext = hal_ext->asBinder();

    if (hal->connected()) {
        CHECK(STATUS_OK == AIBinder_setExtension(binder.get(), binder_ext.get()));
        const std::string instance = std::string(Fingerprint::descriptor) + "/virtual";
        binder_status_t status =
                AServiceManager_registerLazyService(binder.get(), instance.c_str());
        CHECK_EQ(status, STATUS_OK);
        AServiceManager_forceLazyServicesPersist(true);
    } else {
        LOG(ERROR) << "Fingerprint HAL is not connected";
    }

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
