/*
 * Copyright 2023, The Android Open Source Project
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

#include "MacsecPskPlugin.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

namespace android::hardware::macsec {

using namespace std::string_literals;
using ::aidl::android::hardware::macsec::MacsecPskPlugin;

extern "C" int main() {
    base::SetDefaultTag("MacsecPskPlugin");
    base::SetMinimumLogSeverity(base::VERBOSE);

    LOG(VERBOSE) << "Starting up...";
    auto service = ndk::SharedRefBase::make<MacsecPskPlugin>();
    const auto instance = MacsecPskPlugin::descriptor + "/default"s;
    const auto status = AServiceManager_addService(service->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK) << "Failed to add service " << instance;
    LOG(VERBOSE) << "Started successfully!";

    ABinderProcess_joinThreadPool();
    LOG(FATAL) << "MacsecPskPlugin exited unexpectedly!";
    return EXIT_FAILURE;
}
}  // namespace android::hardware::macsec
