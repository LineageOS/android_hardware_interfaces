/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.keymint1-service"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <AndroidKeyMint1Device.h>
#include <keymaster/soft_keymaster_logger.h>

using aidl::android::hardware::keymint::SecurityLevel;
using aidl::android::hardware::keymint::V1_0::AndroidKeyMint1Device;

int main() {
    // Zero threads seems like a useless pool, but below we'll join this thread to it, increasing
    // the pool size to 1.
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<AndroidKeyMint1Device> km5 =
            ndk::SharedRefBase::make<AndroidKeyMint1Device>(SecurityLevel::SOFTWARE);

    keymaster::SoftKeymasterLogger logger;
    const auto instanceName = std::string(AndroidKeyMint1Device::descriptor) + "/default";
    LOG(INFO) << "instance: " << instanceName;
    binder_status_t status =
            AServiceManager_addService(km5->asBinder().get(), instanceName.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
