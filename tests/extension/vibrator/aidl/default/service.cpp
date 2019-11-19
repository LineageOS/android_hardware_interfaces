/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <vibrator-impl/Vibrator.h>
#include "CustomVibrator.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::tests::extension::vibrator::CustomVibrator;
using aidl::android::hardware::vibrator::Vibrator;

int main() {
    // these are threads in addition to the one we are joining below, so this
    // service will have a single thread
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    // making the core service
    std::shared_ptr<Vibrator> vib = ndk::SharedRefBase::make<Vibrator>();
    ndk::SpAIBinder vibBinder = vib->asBinder();

    // making the extension service
    std::shared_ptr<CustomVibrator> cvib = ndk::SharedRefBase::make<CustomVibrator>();

    // need to attach the extension to the same binder we will be registering
    CHECK(STATUS_OK == AIBinder_setExtension(vibBinder.get(), cvib->asBinder().get()));

    const std::string instance = std::string() + Vibrator::descriptor + "/default";
    CHECK(STATUS_OK == AServiceManager_addService(vibBinder.get(), instance.c_str()));

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
