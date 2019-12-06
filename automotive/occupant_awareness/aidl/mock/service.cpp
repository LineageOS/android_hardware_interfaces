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

#define LOG_TAG "android.hardware.automotive.occupant_awareness@1.0-service_mock"

#include <unistd.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "OccupantAwareness.h"

using ::aidl::android::hardware::automotive::occupant_awareness::IOccupantAwareness;
using ::android::hardware::automotive::occupant_awareness::V1_0::implementation::OccupantAwareness;
using ::ndk::ScopedAStatus;
using ::ndk::SharedRefBase;

const static char kOccupantAwarenessServiceName[] = "default";

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    LOG(INFO) << "Occupant Awareness service is starting";
    std::shared_ptr<OccupantAwareness> occupantAwareness = SharedRefBase::make<OccupantAwareness>();

    const std::string instance =
            std::string() + IOccupantAwareness::descriptor + "/" + kOccupantAwarenessServiceName;

    binder_status_t status =
            AServiceManager_addService(occupantAwareness->asBinder().get(), instance.c_str());
    if (status == STATUS_OK) {
        LOG(INFO) << "Service " << kOccupantAwarenessServiceName << " is ready";
        ABinderProcess_joinThreadPool();
    } else {
        LOG(ERROR) << "Could not register service " << kOccupantAwarenessServiceName
                   << ", status: " << status;
    }

    // In normal operation, we don't expect the thread pool to exit.
    LOG(ERROR) << "Occupant Awareness service is shutting down";
    return 1;
}
